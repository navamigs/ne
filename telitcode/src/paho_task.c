/*************************************************************************
                    5BARz International Incorporated. 

                 (C) Copyright 2017. All Rights Reserved.

Filename: paho_task.c

Description: This file contains functions to initialize and maintain a
MQTT task and client.

*************************************************************************/
/*=======================================================================
                            INCLUDE FILES
=========================================================================*/
#include <stdint.h>
#include "MQTTConnect.h"
#include "paho_task.h"
#include "stddef.h"
#include "ftp.h"
#include "client.h"
#include "fs_file.h"
#include "m2m_cloud_defines.h"


/*=======================================================================
                            LOCAL VARIABLES
=======================================================================*/
M2M_CLOUD_GLOBALS globals;
// Buffers
unsigned char buf[CLIENT_BUFF_SIZE];
unsigned char readbuf[CLIENT_BUFF_SIZE];
char          clientID[CLIENT_BUF_SIZE];
char          client_disc[CLIENT_BUFF_SIZE];
char          subscribe_topic[CLIENT_BUF_SIZE];


/////////////////////////////////////////////////////////////////////////////
// Function: MQTT_InitProc()
//
// Parameters:    type:	event id
//                param1: addition info
//                param2: addition info
//
// Returns:       INT32 (response_t enum)
//
// Description: This function is the start of MQTT connection and its functions.
//
/////////////////////////////////////////////////////////////////////////////
INT32 MQTT_InitProc( INT32 type, INT32 param1, INT32 param2 )
{
  bool PDP_Active=false;
  response_t rsp;
  int res,i;

  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "MQTT_TaskInitProc:%d",type);
  NE_debug("MQTT STARTED");

  switch( type )
  {
    case PPP_CONNECT:
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "PPP_CONNECT");
      res = m2m_cloud_pdp_context_activate( globals.module.apn, NULL, NULL );
      if( res != M2M_CLOUD_SUCCESS )
      {
        NE_debug( "PDP Failed" );
      }
      // Polling Loop to detect a proper PDP session from modem layers:
      for( i = 0; i < 5; i++ )
      {
        if( m2m_pdp_get_status() != M2M_PDP_STATE_ACTIVE )
        {
          dwPrintDebug( M2M_CLOUD_LOG_ERRORS, M2M_FALSE, "Retrying PDP %d", i );
          m2m_pdp_activate( globals.module.apn, NULL, NULL );
          m2m_os_lock_wait( PDPLock, 10000 );
        }
        else
        {
          PDP_Active = true;
          break;
        }
      }
      if( PDP_Active == false )
      {
        dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "PDP not active");
        if( globals.module.ppp_mqtt_retries < 3 )
        {
          globals.PPP_state = PPP_CONNECTING;
          globals.module.ppp_mqtt_retries += 1;
        }
        else
        {
          globals.PPP_state = PPP_CONNECT_FAIL;
          globals.module.ppp_mqtt_retries = 0;
        }        
      }
      else
      {
        NE_debug("PPP Connected");
        globals.PPP_state = PPP_CONNECTED;
        dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "PDP is active");
      }
      break;

    case MQTT_CONNECT:
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "MQTT_SER_CONNECT");
      rsp = MQTT_client_connect( param1, true );
      if( RSP_SUCCESS != rsp )
      {
        if( globals.module.ppp_mqtt_retries < 2 )
        {
          dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Restarted MQTT");
          globals.MQTT_state = MQTT_CONNECTING;
          globals.module.ppp_mqtt_retries += 1;
        }
        else
        {
          globals.MQTT_state = MQTT_IDLE;
          globals.module.ppp_mqtt_retries = 0;
          mqtt_session_failed( RSP_MQTT_SUBSCRIBE_FAIL, param1 );
        }
      }
      else globals.MQTT_state = MQTT_CONNECT_SUCC;
      break;

    case PPP_DISCONNECT :
      res = m2m_cloud_pdp_context_deactivate();
      if( res == M2M_CLOUD_SUCCESS )
    	{
        dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "PPP is Disconnected");
        NE_debug("PPP Disconnected");
        globals.PPP_state = PPP_DISCONNECTED;
      }
      else
      {
        dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"PPP Failed to Disconnect");
        NE_debug("PPP Disconnect Failed");
        globals.PPP_state = PPP_DISCONN_FAIL;
      }
      break;

    case MQTT_DISCONNECT:
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Disconnecting MQTT..");
      rsp = Disconnect_RMS();
      if( rsp == RSP_SUCCESS ) globals.MQTT_state = MQTT_DISCONNECTED;
    	 else globals.MQTT_state = MQTT_DISCONN_FAIL;
		 break;

    default:
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "MQTT_InitProc:%d not matched",type);
      break;
  }
  return( rsp );
}


/////////////////////////////////////////////////////////////////////////////
// Function: MQTT_client_connect()
//
// Parameters:    INT32: The original SMS msgID for return status SMS 
//                int:   Status of PDP connection
//                param2: addition info
//
// Returns:       response_t enum
//
// Description: Initializes the MQTT Paho Client, set up subscriptions and 
// initializes the timers and tasks, and yield mqtt client to check 
// subscriptions.
//
/////////////////////////////////////////////////////////////////////////////
response_t MQTT_client_connect( INT32 msgID, bool PDP_Active )
{
   /* Initialize Variables */
  MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
  char mqtt_broker[30], port[PORT_LEN], conn_mthd[CON_MODE_LEN], url[URL_LEN];
  char *token, *array[2], *token1, *ftp_array[3];
  int  ret, i = 0, j = 0, mqtt_port;
  response_t  rsp = RSP_MQTT_FAILURE;
  msg_queue_t msg;

  memset( url, 0, sizeof( url ));
  memset( mqtt_broker, 0, sizeof( mqtt_broker));
  memset( port, 0, sizeof( port ));
  memset( clientID, 0, sizeof( clientID ));
  memset( client_disc, 0, sizeof( client_disc ));
  memset( subscribe_topic, 0, sizeof(subscribe_topic));
  memset( conn_mthd, 0, sizeof(conn_mthd));

  MQTT_ping_failed(client_disc);
  sprintf(subscribe_topic,"%s",globals.module.imei);
  strcpy( url, globals.module.mqttIP ); // MQTTServerURL
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "URL_PORT:%s",url);
    // Parse Input string:
  token = strtok( url, ":" );
  while( token != NULL )
  {
    if( i < 2 ) array[i++] = token;
    token = strtok( NULL, ":" );
  }
  sprintf( mqtt_broker,"%s", array[0] );
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "MQTT_BROKER_NAME:%s",mqtt_broker);
  sprintf( port,"%s", array[1] );
  mqtt_port = atoi( port );
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "MQTT_PORT:%d",mqtt_port);

  ret = ConnectNetwork(&network,mqtt_broker,mqtt_port);
  if( ret != SOCK_CONNECTED )
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Mqtt not connected, please try again %d",ret);
    return( RSP_MQTT_CONNECT_FAIL ); // send msg
  }

  MQTTClient( &client, &network, 10000, buf, CLIENT_BUFF_SIZE, readbuf, CLIENT_BUFF_SIZE );

  /* Configure the MQTT Connection Data */
  sprintf( clientID,"%s_%s",globals.module.imei,MQTT_CLIENT_ID );
  data.clientID.cstring = clientID;
  data.cleansession = 1; // discard any previous Session
  data.keepAliveInterval = 100;
  data.willFlag = 1;
  data.will.retained = 1;//the Server MUST publish the Will Message as a non-retained message
  data.will.qos = 0;
  data.will.topicName.cstring = TOPIC_PUBLISH;
  data.will.message.cstring = client_disc;
  
  /* Connect to the MQTT Broker */
  rsp = MQTT_Connect( &client, &data );
  if(rsp == RSP_MQTT_FAILURE)
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Unable to connect to server");
    return( RSP_MQTT_CONNECT_FAIL );// send msg  
  }
  else
  {
	  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "done connecting network %d",rsp);
  }

 	/* Subscribe to the Topics (set callback functions) */
  rsp = MQTT_Subscribe( &client, subscribe_topic, QOS0 );
  if( rsp == RSP_MQTT_FAILURE )
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Unable to subscribe to the server");
    m2m_timer_stop( client.png_timer );
    return( RSP_MQTT_SUBSCRIBE_FAIL ); // send msg
  }
  else
  {
    globals.module.MQTT_start_time = globals.module.sec_running;
    NE_debug("MQTT Connected");
    rsp = RSP_SUCCESS;
    if(globals.module.mqtt_bootup == true) // true on boot-up only and sending bootup messages after MQTT connected.
    {
      if( !strcmp( globals.module.RMS_Activate, "Y" ) ||
          !strcmp( globals.module.RMS_Activate, "y" ) )  // Yes, RMS Activated
      {
        msg.cmd = MSQ_QUEUE_CMD_BASIC_MSG2;
        PushToMsgQueue( msg );
        msg.cmd = MSQ_QUEUE_CMD_BASIC_MSG1;
        PushToMsgQueue( msg );
        msg.cmd = MSQ_QUEUE_CMD_PERF_MSG;  // Sending performance data upon bootup
        PushToMsgQueue( msg );
      }
      else // We're not RMS Activated yet
      {
        if( globals.module.cloud_enabled ) Cloud_Disable_NE(); // Sorry, NE not allowed to run yet!
      }
      if( globals.module.ftp_state_str[0] != 0 ) // Checking Ftp_msgid from config file
      {
        token1 = strtok( globals.module.ftp_state_str, ":" );
        while( token1 != NULL )     // Msg id of  previous Ftp request before bootup is stored in file .
        {
          if( j < 3 ) ftp_array[j++] = token1;
          token1 = strtok( NULL, ":" );
        }
        dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_TRUE, "Ftp_status:%s",ftp_array[1]);
        dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_TRUE, "Msg_id:%s",ftp_array[2]);
        if( !strcmp( ftp_array[1], "0") ) // (FTP:0:Msgid) means download aborted
        {
          dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_TRUE, "Error in File upgrade");
          FTP_Error_Resp(ftp_array[2]) ;        // Download Error Msg sent
          ftp_update_config_Msgid("X","FF"); // Back to default state
        }
        if( !strcmp( ftp_array[1], "1") ) // (FTP:1:Msgid) means Sucessful download.
        {
          dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_TRUE, "In file value is 1");
          if( strcmp( software_version, globals.module.TCA_ver_cfg) )
          {
            dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_TRUE, "Comparing software version");
            FTP_version( globals.module.TCA_ver_cfg ,ftp_array[2]);   // Msg sent with msgID read from file
            ftp_version_write();                                  // Writing New version to file.
            NE_debug("Sending_ver");
            ftp_update_config_Msgid("X","FF");   // Default state after success
          }
        }
      }
      globals.module.mqtt_bootup =false;
    }
    m2m_timer_start(client.cycle_timer,2000);
  }
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Out of Client_init_function");
  return rsp;
}


/////////////////////////////////////////////////////////////////////////////
// Function: MQTT_ping_failed()
//
// Parameters:    char*: pointer to a char buffer to store the failed return  
//                       message response for  ping during MQTT connection.
//
// Returns:       response_t enum
//
// Description: This function creates the Ping packet to be sent for the 
// will message.
//
/////////////////////////////////////////////////////////////////////////////
response_t MQTT_ping_failed( char *msg )
{
  char       str[CMD_PKT_MAX_DATA_PAYLOAD];
  char       data[6], buf[SMS_MO_MAX_DATA_LEN];
  UINT8      i, res, dLen;
  crc        crc_cal;
  response_t rsp;

  if( msg != NULL )
  {
    // Add IMEI:
    strncpy( msg, globals.module.imei, SMS_HDR_IMEI_LEN );
    i = SMS_HDR_IMEI_LEN;

    // Command ID:
    strcpy( &msg[i], NE_MQTT_STR );
    i += SMS_HDR_CMD_LEN;

    // Message ID:
    msg[i++] = NUM0;
    msg[i++] = NUM0;
    // Reply Id:
    msg[i++] = REPLY_ID_FAILED;

    memset( data, 0, sizeof( data ));      //Error value
    memset( buf, 0, sizeof( buf ));

    sprintf( data, "%d", (int)RSP_MQTT_BROKER_PING_FAIL ); // check once this error.
    NE_debug(data);

    strcpy( buf, data );
    dLen = strlen( buf );
    buf[dLen++] = COMMA;               // Along with Error Epoch time is sent based on replyid
    //epoch time
    rsp = epoch_time( &buf[dLen]);          // EpochTime(current time)
    if( rsp != RSP_SUCCESS ) return( rsp );
    dLen = strlen( buf );

    // Data Length
    if( dLen == 0 )
    {
      msg[i++] = NUM0;
      msg[i++] = NUM0;
    }
    else
    {
      msg[i++] = n_h_a( dLen >> 4 );   // MSN
      msg[i++] = n_h_a( dLen & 0x0F ); // LSN
      if( dLen > SMS_MO_MAX_DATA_LEN || dLen > strlen( buf )) return( RSP_SMS_FORMAT_ERROR );
      strncpy( &msg[i], buf, dLen );
      i += dLen;
    }

    // CRC
    crc_cal = crcFast( msg, i );
    msg[i++] = n_h_a((UINT8)(crc_cal >> 12));
    msg[i++] = n_h_a((UINT8)((crc_cal & 0x0F00) >> 8));
    msg[i++] = n_h_a((UINT8)((crc_cal & 0x00F0) >> 4));
    msg[i++] = n_h_a((UINT8)(crc_cal & 0x000F));
  }
  return (rsp);
}


/////////////////////////////////////////////////////////////////////////////
// Function: MQTT_start()
//
// Parameters:   none
//
// Return:       response_t -> response enum type
//
// Description: This function initializes MQTT connection session by calling
// the MQTT task with instructions.
//
/////////////////////////////////////////////////////////////////////////////
response_t MQTT_start( void )
{
  response_t rsp;

  if(!(globals.MQTT_state == MQTT_CONNECTED)) // checking if MQTT already connected
  {
    // checking if MQTT is in process
    if(!(globals.MQTT_state == MQTT_CONNECTING))
    {
      globals.MQTT_state = MQTT_CONNECTING;
      rsp = RSP_SUCCESS;
    }
    else
    {
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "MQTT connection in progress..");
      rsp = RSP_MQTT_IN_PROGRESS;
    }
  }
  else
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "MQTT is already connected");
    rsp = RSP_NE_ALREADY_DONE;
  }
  return(rsp);
}


/////////////////////////////////////////////////////////////////////////////
// Function: MQTT_stop()
//
// Parameters:none
//
// Return:       response_t -> response enum type
//
// Description: This function stops the MQTT connection.
//
/////////////////////////////////////////////////////////////////////////////
response_t MQTT_stop(void)
{
  response_t rsp;

  if (globals.MQTT_state == MQTT_CONNECTED) // check if MQTT stop request has been already sent
  {
    globals.MQTT_state= MQTT_DISCONNECTING;
    rsp = RSP_SUCCESS;
  }
  else
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "MQTT is already stopped");
    rsp = RSP_NE_ALREADY_DONE;
  }
  return(rsp);
}


/////////////////////////////////////////////////////////////////////////////
// Function: Disconnect_RMS()
//
// Parameters:  NONE.
//
// Return:      response_t -> response enum type
//
// Description: Disconnect from the MQTT Client.
//
/////////////////////////////////////////////////////////////////////////////
response_t Disconnect_RMS( void )
{
  response_t rsp = RSP_MQTT_FAILURE;

  rsp = MQTTDisconnect(&client);
  if( rsp == RSP_SUCCESS )
  {
    m2m_timer_stop( client.png_timer );
    m2m_timer_stop( client.cycle_timer );
  }
  return( rsp );
}


/////////////////////////////////////////////////////////////////////////////
// Function: mqtt_send_response()
//
// Parameters: response_t -> response enum type
//             INT32      -> the original Message ID
//
// Return: none
//
// Description: This function Publish to the RMS with MQTT response
// code to indicate the state of the current MQTT connection.
//
/////////////////////////////////////////////////////////////////////////////
void mqtt_send_response( response_t rsp, INT32 msgID )
{
  msg_queue_t msg;

  msg.cmd      = MSQ_QUEUE_CMD_MQTT;
  msg.payload1 = msgID;
  msg.payload2 = rsp;
  msg.payload3 = REPLY_ID_UNSOLCITED; // Unsolicated id  added in format to send in 0E04 .
  PushToMsgQueue( msg );
}


/////////////////////////////////////////////////////////////////////////////
// Function: mqtt_session_failed()
//
// Parameters: response_t -> response enum type
//             INT32      -> the original Message ID
//
// Return: none
//
// Description: This function handles a failed MQTT session by Publish of
// MQTT response  and ending the PDP session .
//
/////////////////////////////////////////////////////////////////////////////
void mqtt_session_failed( response_t rsp, INT32 msgID )
{
  msg_queue_t msg;

  msg.cmd      = MSQ_QUEUE_CMD_MQTT;
  msg.payload1 = msgID;
  msg.payload2 = rsp;
  msg.payload3 = REPLY_ID_FAILED;  // Replyid added in format to send in 0E07 .
  PushToMsgQueue( msg );
}


/////////////////////////////////////////////////////////////////////////////
// Function: Initialize_MQTT()
//
// Parameters: none
//
// Return: init_result_t:
//  INIT_SUCCESS,       Initialization success
//  INIT_FAIL_SOFTWARE, Initernal Software failure
//  INIT_FAIL_MQTT    ,MQTT task creation failed
//
// Description: This function initializes all necessary software
// for proper MQTT operation.
//
/////////////////////////////////////////////////////////////////////////////
init_result_t Initialize_MQTT( void )
{
  response_t rsp;
  globals.PPP_state  = PPP_IDLE;
  globals.MQTT_state = MQTT_IDLE;

  // Init Globals:
  globals.module.ppp_mqtt_retries = 0;
  
  // Init MQTT Task items:
  globals.module.mqtt_taskID = m2m_os_create_task( M2M_OS_TASK_STACK_L,
                                                   MQTT_TASK_PRIORTIY,
                                                   M2M_OS_TASK_MBOX_S,
                                                   MQTT_InitProc );
  if( globals.module.mqtt_taskID < 1 )
  {
    NE_debug("Failed MQTT task init");
    return (INIT_FAIL_SOFTWARE);
  }
  else rsp = INIT_SUCCESS;

	// Timer creation in initialization:
  client.png_timer = m2m_timer_create(user_timeout_handler, NULL);
  client.waiting = m2m_timer_create(wait_timeout_handler, NULL);
  client.cycle_timer = m2m_timer_create(cycle_timeout_handler, NULL);
  if( client.png_timer == 0 || client.waiting == 0 || client.cycle_timer == 0 )
	{
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Failed to create MQTT timers....");
    NE_debug("MQTT timer Failed");
    rsp = INIT_FAIL_SOFTWARE;
  }
	return rsp;
}


/////////////////////////////////////////////////////////////////////////////
// Function: MQTT_cell_service_change()
//
// Parameters: bool
//
// Return: none
//
// Description: This function informs the change in reg.status when device 
// is in MQTT mode and connects to PPP if cell signal is arrived.
//
/////////////////////////////////////////////////////////////////////////////
void MQTT_cell_service_change( bool cell_service )
{
  bool res;
  static bool once = false; //temporary
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "mqttState:%d", cell_service );

  if( cell_service )
  {
    if( once == false ) // once for the first time
    {
      once = true;
      globals.module.mqtt_bootup = true;   // Signal boot-up
      MQTT_start();
    }
    else dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "cell service is available");
  }
  else // it will wait till the
  {
    globals.PPP_state = PPP_CELL_SERVICE;
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Unable to detect cell service");
    NE_debug("cell_service lost");
  }
}


/////////////////////////////////////////////////////////////////////////////
// Function: Is_MQTT_Connected()
//
// Parameters: none
//
// Return: bool -> true if connected
//
// Description: This function returns true if connected to the RMS
//
/////////////////////////////////////////////////////////////////////////////
bool Is_MQTT_Connected( void )
{
  if( globals.MQTT_state == MQTT_CONNECTED ) return( true );
    else return( false );
}


/////////////////////////////////////////////////////////////////////////////
// Function: MQTT_task_manager()
//
// Parameters: none
//
// Return: none
//
// Description: This function controls and monitors the current PPP
// data link and MQTT data links
//
/////////////////////////////////////////////////////////////////////////////
void MQTT_task_manager( INT32 param1 )
{
  char conn_mthd[CON_MODE_LEN];
  int rsp;
  
  switch( globals.PPP_state )
  {
    case PPP_CELL_SERVICE:
      if( globals.Reg_state == REG_STATE_CAMPING_HOME ) // Only home service allowed
      {
        globals.MQTT_state = MQTT_CONNECTING;
        NE_debug("Got cell_signal");
      }
      break;

    case PPP_IDLE:
      // Do nothing for now
      break;

    case PPP_CONNECTING:
      m2m_os_send_message_to_task( globals.module.mqtt_taskID, PPP_CONNECT, (INT32)NULL, (INT32)NULL);
      globals.PPP_state = PPP_WAITING;
      break;

    case PPP_CONNECTED:
      // Do nothing for now
      break;

    case PPP_DISCONNECTING:
     	m2m_os_send_message_to_task( globals.module.mqtt_taskID, PPP_DISCONNECT, (INT32)NULL, (INT32)NULL);
     	globals.PPP_state =PPP_WAITING;
      break;

    case PPP_DISCONNECTED:
    	// Do nothing for now
      break;

    case PPP_WAITING:
      // Do nothing for now
      break;

    case PPP_CONNECT_FAIL:
      if( globals.module.conn_method == CONN_METHOD_MQTT_ONLY )  /* updating the connection method*/
      {
        memset( conn_mthd, 0, sizeof(conn_mthd) );
        sprintf( conn_mthd, "%d", CONN_METHOD_SMS_ONLY);
        rsp = update_config_file( CFG_FILE_ITEM_CONN_METHOD, conn_mthd );
        if( rsp == RSP_SUCCESS )
        {
          mqtt_session_failed( RSP_PDP_SESSION_FAILURE, param1 );
        }
      }
      else mqtt_session_failed( RSP_PDP_SESSION_FAILURE, param1 );
      globals.MQTT_state = MQTT_IDLE;
      globals.PPP_state  = PPP_IDLE;
      break;

    case PPP_DISCONN_FAIL:
      globals.PPP_state =PPP_DISCONNECTING;
      break;
  }

  switch( globals.MQTT_state )
  {
    case MQTT_IDLE:
    // Do nothing for now
    break;

    case MQTT_CONNECTING:  // If PPP is already connected connect to MQTT else connect PPP and try MQTT
      if(globals.PPP_state == PPP_CONNECTED )
      {
        dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Started connecting MQTT");
        m2m_os_send_message_to_task( globals.module.mqtt_taskID,MQTT_CONNECT,(INT32)NULL, (INT32)NULL);
        globals.MQTT_state = MQTT_WAITING;
      }
      else
      {
        if(globals.PPP_state != PPP_WAITING) globals.PPP_state = PPP_CONNECTING;
      }
      break;

    case MQTT_CONNECTED:
      // Do nothing right now
      break;

    case MQTT_DISCONNECTING:
      m2m_os_send_message_to_task( globals.module.mqtt_taskID,MQTT_DISCONNECT,(INT32)NULL, (INT32)NULL);
     	globals.MQTT_state = MQTT_WAITING;
      break;

    case MQTT_DISCONNECTED:
      globals.MQTT_state = MQTT_IDLE;
      globals.PPP_state  = PPP_DISCONNECTING;
      break;

    case MQTT_WAITING:
      // Do nothing right now
      break;

    case MQTT_CONNECT_FAIL:
      if( m2m_pdp_get_status() == M2M_PDP_STATE_ACTIVE ) globals.MQTT_state = MQTT_CONNECTING;
    	 else globals.PPP_state  = PPP_CONNECTING;
      break;

    case MQTT_DISCONN_FAIL:
      globals.MQTT_state = MQTT_DISCONNECTING;
      break;
  }
}


/////////////////////////////////////////////////////////////////////////////
// Function: wait_timeout_handler()
//
// Parameters: none
//
// Return: none
//
// Description: This Timer handler that is called to trigger a wait
// for receive message.
//
/////////////////////////////////////////////////////////////////////////////
void wait_timeout_handler( void *arg )
{
  waitTimeout = 1;
}


/////////////////////////////////////////////////////////////////////////////
// Function: cycle_timeout_handler()
//
// Parameters: none
//
// Return: none
//
// Description: This Timer handler that is called to trigger a MQTT
// receive message.
//
/////////////////////////////////////////////////////////////////////////////
void cycle_timeout_handler( void *arg )
{
  response_t rsp;
  rsp = cycle(&client);
  if (rsp == RSP_SUCCESS) NE_debug("GOT_MQTT_PACKET");
  m2m_timer_start( client.cycle_timer, 3000 );
}


/////////////////////////////////////////////////////////////////////////////
// Function: user_timeout_handler()
//
// Parameters: none
//
// Return: none
//
// Description: This Timer handler that is used to ping the MQTT
// broker periodically.
//
/////////////////////////////////////////////////////////////////////////////
void user_timeout_handler( void *arg )
{
  response_t rsp = RSP_MQTT_FAILURE, PDP_Active = true;

  if( client.keepAliveInterval == 0 )
  {
    rsp = RSP_SUCCESS;
  }

  //NE_debug("PING");

  rsp = MQTT_ping( &client );

  // Check for proper power to continue:
  if( rsp == RSP_MQTT_FAILURE)
  {
    m2m_timer_stop( client.png_timer );
    m2m_timer_stop( client.cycle_timer );
    globals.MQTT_state = MQTT_DISCONNECTED;
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Broker disconnected..check the broker and activate MQTT again");
    mqtt_session_failed( RSP_MQTT_BROKER_PING_FAIL, 0 );
    return;
  }
  else
  {
 	  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Started pinging the broker");
 	  m2m_timer_start( client.png_timer, 30000 );
  }
}

