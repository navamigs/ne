/*************************************************************************
                    5BARz International Incorporated. 

                 (C) Copyright 2016. All Rights Reserved.

Filename: utilities.c

Description: This file contains several utility functions to process 
various connection states, monitor activities, and control various
state machines.

*************************************************************************/
#include <stdio.h>
#include <string.h>

#include "m2m_type.h"
#include "m2m_os_api.h"
#include "m2m_cloud_api.h"
#include "m2m_cloud_defines.h"
#include "m2m_timer_api.h"
#include "utilities.h"
#include "cmd_pkt.h"
#include "cloud_user_method.h"
#include "sms.h"
#include "ftp.h"
#include "fs_file.h"
#include "log.h"
#include "paho_task.h"


#define SAMPLE_PERIOD_POWER  5       // 5 sec sample period for Power monitor
#define SAMPLE_PERIOD_ALARM  275     // ~5 sec sample period for Alarms
#define SAMPLE_DAILY         4752000 // ~24 hours sample period for Config file maintenance
#define NO_SERVICE_TIMEOUT   3600000 // 1 Hour in msec

// Global Variables:
M2M_CLOUD_GLOBALS  globals;

// Local Variables:
M2M_T_TIMER_HANDLE timer_sec_hdl;
M2M_T_TIMER_HANDLE timer_shutdown_hdl;
M2M_T_TIMER_HANDLE timer_lost_service_hdl;
M2M_T_OS_LOCK      mqtt_lock;


// Local Function Prototypes:
void Timer_Seconds_handler( void *arg );
void Timer_Perf_data_handler( void *arg );
void Timer_Loc_data_handler( void *arg );
void Timer_shutdown_handler( void *arg );
void Timer_lost_service_handler( void *arg );


/////////////////////////////////////////////////////////////////////////////
// Function: Registration_Manager()
//
// Parameters: none
//
// Return: none
//
// Description: This function tracks the current state of network
// registration, roaming or not.
//
/////////////////////////////////////////////////////////////////////////////
void Registration_Manager( void )
{
  msg_queue_t msg;
  int         i = 0;
  char        *token, *array[3];
  static bool once = true;
  static bool first_service = false;
  static M2M_T_NETWORK_REG_STATUS_INFO RegStatus1;
  M2M_T_NETWORK_REG_STATUS_INFO        RegStatus2;
  M2M_T_NETWORK_CELL_INFORMATION       cell_info;
  
  if( once ) // Init once
  {
    RegStatus1.status = 0xFFFF;
    once = false;
  }
  
  m2m_network_get_reg_status( &RegStatus2 );
  
  if( RegStatus1.status == RegStatus2.status ) return;

  // Registration Change, process and translate:
  if( RegStatus2.status > 5 || RegStatus2.status == 4 )
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Reg Status Error:%d\n",RegStatus2.status);
    return;
  }

  RegStatus1.status = RegStatus2.status;
  if( RegStatus1.status == 0 || RegStatus1.status == 2 )globals.Reg_state = REG_STATE_SEARCHING;
  if( RegStatus1.status == 1 ) globals.Reg_state = REG_STATE_CAMPING_HOME;
  if( RegStatus1.status == 3 ) globals.Reg_state = REG_STATE_DENIED;
  if( RegStatus1.status == 5 ) globals.Reg_state = REG_STATE_CAMPING_ROAM;
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Reg Status Change:%d\n",RegStatus1.status);
  NE_debug_var("Registration:",RegStatus1.status);
  
  
  ////////////////////////////////////////////////////////////////////
  //              First Cell Service Processing                     //
  //  This code runs whenever the module first acquires cell phone  //
  //  service (Home Network only) after boot-up.                    //
  ////////////////////////////////////////////////////////////////////
  if( (RegStatus1.status == 1 ) && first_service == false )
  {
    first_service = true;
    
    // Aquire GSM Channel:
    if( m2m_network_get_cell_information( &cell_info ) )
    {  
      globals.module.GSM_channel = cell_info.neighbors[0].nARFCN;
    }
    
    // Stop the Lost Service timer:
    m2m_timer_stop( timer_lost_service_hdl );
    
    ///////////////////////////////////////////////////
    //       Grab the OTA Phone Number if needed     //
    ///////////////////////////////////////////////////
#ifdef INDIA_VODAFONE
    Req_OTA_phone_num();
#endif
#ifdef INDIA_AIRTEL
    Req_OTA_phone_num();
#endif
    
  
    //////////////////////////////////////////////////////////
    //      Notify Logging we have a Cell Signal:           //
    //////////////////////////////////////////////////////////
    Logging_Manager( LOG_EVENT_CELL_SIGNAL );
    
       
    //////////////////////////////////////////////////////////
    //     Notify MQTT client we have a Cell Signal:        //
    //////////////////////////////////////////////////////////
    if( globals.module.conn_method == CONN_METHOD_MQTT_ONLY )
      MQTT_cell_service_change( true );
    
    
    ///////////////////////////////////////////////////
    //         Process TCA Version if needed         //
    ///////////////////////////////////////////////////
    if( globals.module.conn_method == CONN_METHOD_SMS_ONLY ) // Only SMS supported
    {                                                        
      if( globals.module.ftp_state_str[0] != 0 ) // Checking Ftp_msgid from config file 
      {
        token = strtok( globals.module.ftp_state_str, ":" );
        while( token != NULL )       // Msg id of  previous Ftp request before bootup is stored in file .
        { 
          if( i < 3 ) array[i++] = token;
          token = strtok( NULL, ":" );
        }
        dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_TRUE, "Ftp_status:%s",array[1]);
        dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_TRUE, "Msg_id:%s",array[2]);
        if( !strcmp( array[1], "0") ) // (FTP:0:Msgid) means download aborted
        {
          dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_TRUE, "Error in File upgrade");
          FTP_Error_Resp(array[2]) ;         // Download Error Msg sent 
          ftp_update_config_Msgid("X","FF"); // Back to default state 
        }
        if( !strcmp( array[1], "1") ) // (FTP:1:Msgid) means Sucessful download.
        {
          dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_TRUE, "In file value is 1");
          if( strcmp( software_version, globals.module.TCA_ver_cfg) )
          {
            dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_TRUE, "Comparing software version");
            FTP_version( globals.module.TCA_ver_cfg ,array[2]);   // Msg sent with msgID read from file
            ftp_version_write();                                  // Writing New version to file.
            NE_debug("Sending_ver");
            dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_TRUE, "Sending_Ver");
            ftp_update_config_Msgid("X","FF");   // Default state after success
			msg.cmd = MSQ_QUEUE_CMD_BASIC_MSG2;
			PushToMsgQueue( msg );
			msg.cmd = MSQ_QUEUE_CMD_BASIC_MSG1;
			PushToMsgQueue( msg );
			Req_location_info();
			dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "latitude response at FTP:%s\n", globals.module.location.latitude);
			dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "longitude response at FTP: %s\n",globals.module.location.longitude);
			dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "accuracy response at FTP: %d\n",globals.module.location.accuracy);
			msg.cmd = MSQ_QUEUE_CMD_BASIC_MSG3;
			PushToMsgQueue( msg );
			msg.cmd = MSQ_QUEUE_CMD_PERF_MSG;  // Sending performance data upon bootup
			PushToMsgQueue( msg );
          }
        }
      }    
    }
    ///////////////////////////////////////////////////
    //            Process RMS Activition             //
    ///////////////////////////////////////////////////
    if( !strcmp( globals.module.RMS_Activate, "Y" ) ||
        !strcmp( globals.module.RMS_Activate, "y" ) )  // Yes, RMS Activated

    {
		send_MO_MSG( NE_POWERED_UP, NULL, REPLY_ID_UNSOLCITED,0,NULL, globals.module.conn_method );
		if( atoi( globals.module.perf_intvl ) > 0 ) Start_Perf_Data_timer();
		if( atoi( globals.module.Loc_intvl ) > 0 )  Start_Loc_Data_timer();
    }
    else // We're not RMS Activated yet
    {
	  if( globals.module.cloud_enabled ) Cloud_Disable_NE(); // Sorry, NE not allowed to run yet!
      Req_location_info();
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Latitude :%s\n", globals.module.location.latitude);
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Longitude: %s\n",globals.module.location.longitude);
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Accuracy : %d\n",globals.module.location.accuracy);
      msg.cmd = MSQ_QUEUE_CMD_BASIC_MSG3;
	  PushToMsgQueue( msg );
    }
    return;
  }
  
  // Notify the NE of gain/loss of GSM service during normal (non-bootup) operation:
  if( RegStatus1.status == 1 ) // Home Service (non-roam)
  {
    if( globals.module.conn_method == CONN_METHOD_MQTT_ONLY ) MQTT_cell_service_change( true );
    m2m_timer_stop( timer_lost_service_hdl );
    WriteCmdPkt16( CMD_PKT_NOTIFY_CELL_SERVICE, 1 );
    dwPrintDebug( M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Regained GSM Service, notifying NE");
    if( m2m_network_get_cell_information( &cell_info ) )
    {  
      globals.module.GSM_channel = cell_info.neighbors[0].nARFCN;
    }
  }
  else if( first_service == true ) // Lost GSM service, after initial acquisition
  {
    if( globals.module.conn_method == CONN_METHOD_MQTT_ONLY ) MQTT_cell_service_change( false );
    Start_lost_service_timer();
    globals.module.GSM_channel = 0;
  }
}


/////////////////////////////////////////////////////////////////////////////
// Function: CellService()
//
// Parameters: none
//
// Return: bool -> true if module has cell service, wither home or roam
//
// Description: This function returns whether the module has cell service
// or not.
//
/////////////////////////////////////////////////////////////////////////////
bool CellService( void )
{
  if( globals.Reg_state == REG_STATE_CAMPING_HOME ) return( true );
    else return( false );
}


/////////////////////////////////////////////////////////////////////////////
// Function: Power_Monitor()
//
// Parameters: none
//
// Return: none
//
// Description: This function monitors the 12v line by sending an AT command 
// to read the ADC.  It waits about 30 seconds after boot-up (to give time
// to charge the super-caps, then queries every 5 seconds.
// 
/////////////////////////////////////////////////////////////////////////////
void Power_Monitor( void )
{
  static UINT32 sec;
  
  if( globals.module.sec_running < 30 ) return;
  
  if( globals.power_state == PWR_STATE_12V_OFF_SHUTDOWN ) return;
  
  if( sec != globals.module.sec_running )
  {
    sec = globals.module.sec_running;
    if( sec % SAMPLE_PERIOD_POWER != 0 ) return; // Every 5 seconds  
    dwSendATCommand(at_cmd_adc, M2M_TRUE ); // Query 12v voltage via ADC 
  } 
}


/////////////////////////////////////////////////////////////////////////////
// Function: Init_lost_service_timer()
//
// Parameters: none
//
// Return: none
//
// Description: This function initializes a timer that tracks when GSM 
// service is lost.
// 
/////////////////////////////////////////////////////////////////////////////
void Init_lost_service_timer( void )
{
  timer_lost_service_hdl = m2m_timer_create( Timer_lost_service_handler, NULL );
}


/////////////////////////////////////////////////////////////////////////////
// Function: Start_lost_service_timer()
//
// Parameters: none
//
// Return: none
//
// Description: This function starts a timer when GSM cell service is lost
// or never acquired after boot-up.
// 
/////////////////////////////////////////////////////////////////////////////
void Start_lost_service_timer( void )
{
  if( timer_lost_service_hdl )
  {
    m2m_timer_start( timer_lost_service_hdl, NO_SERVICE_TIMEOUT );
    dwPrintDebug( M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Starting Lost GSM Service Timer");
  }
}


/////////////////////////////////////////////////////////////////////////////
// Function: Timer_lost_service_handler()
//
// Parameters: none
//
// Return: none
//
// Description: This function handler notifies the NE that GSM service has
// been lost
// 
/////////////////////////////////////////////////////////////////////////////
void Timer_lost_service_handler( void *arg )
{
  msg_queue_t msg;

  dwPrintDebug( M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Lost GSM Service Timer Expired");
  msg.cmd = MSG_QUEUE_CMD_LOST_SERVICE;
  PushToMsgQueue( msg );
}


/////////////////////////////////////////////////////////////////////////////
// Function: Start_shutdown_timer()
//
// Parameters: none
//
// Return: none
//
// Description: This function starts a timer that will shutdown the
// whole module (12V power loss).
// 
/////////////////////////////////////////////////////////////////////////////
void Start_shutdown_timer( void )
{
  timer_shutdown_hdl = m2m_timer_create( Timer_shutdown_handler, NULL );
  if( timer_shutdown_hdl )
  {
    m2m_timer_start( timer_shutdown_hdl, (TWENTY_SECONDS+globals.module.start_stop_delay) );
    dwPrintDebug( M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Starting Shutdown Timer");
  }
}


/////////////////////////////////////////////////////////////////////////////
// Function: Timer_shutdown_handler()
//
// Parameters: none
//
// Return: none
//
// Description: This function handler shuts down the entire module.
// 
/////////////////////////////////////////////////////////////////////////////
void Timer_shutdown_handler( void *arg )
{
  m2m_hw_power_down();
}


/////////////////////////////////////////////////////////////////////////////
// Function: Init_Seconds_timer()
//
// Parameters: none
//
// Return: bool -> true if sucessfull
//
// Description: This function the Initializes the System Timer 
// 
/////////////////////////////////////////////////////////////////////////////
bool Init_Seconds_timer( void )
{
  globals.module.sec_running = 0;
  timer_sec_hdl = m2m_timer_create( Timer_Seconds_handler, NULL );
  if( timer_sec_hdl )
  {
    m2m_timer_start( timer_sec_hdl, ONE_SECOND );
    dwPrintDebug( M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Starting System Timer");
    return( true );
  }
  else
  {  
    dwPrintDebug( M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Failed to start System Timer");
    return( false );
  }
}


/////////////////////////////////////////////////////////////////////////////
// Function: Timer_Seconds_handler()
//
// Parameters: none
//
// Return: none
//
// Description: This function the handler that is called when the 
// system timer expires after 1 second.  This function is a call back
// and runs within the low level modem tasks.
// 
/////////////////////////////////////////////////////////////////////////////
void Timer_Seconds_handler( void *arg )
{
  globals.module.sec_running++;
  m2m_timer_start( timer_sec_hdl, ONE_SECOND );
}


/////////////////////////////////////////////////////////////////////////////
// Function: Start_Perf_Data_timer()
//
// Parameters: none
//
// Return: bool -> true if sucessfull
//
// Description: This function the Starts the Periodic Performance
// Data Timer, if the value of Performance interval > 0.
//
/////////////////////////////////////////////////////////////////////////////
bool Start_Perf_Data_timer( void )
{
  int perf_intvl;
  
  perf_intvl = atoi(globals.module.perf_intvl);
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "perf_intvl:%s",globals.module.perf_intvl);
  if( perf_intvl == 0 ) return( true ); // Do nothing

  if( timer_perf_data_hdl )
  {
    m2m_timer_start( timer_perf_data_hdl, (ONE_MINUTE) * perf_intvl );
    dwPrintDebug( M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Starting Data Timer");
    return( true );
  }
  else
  {
    dwPrintDebug( M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Failed to start System Timer");
    return( false );
  }
}


/////////////////////////////////////////////////////////////////////////////
// Function: Timer_Perf_data_handler()
//
// Parameters: none
//
// Return: none
//
// Description: This function the Timer handler that is called to trigger
// a periodic performance data message to be send to the RMS.  This function 
// is a call back and runs within the low level modem tasks.
// 
/////////////////////////////////////////////////////////////////////////////
void Timer_Perf_data_handler( void *arg ) 
{
  int         perf_intvl;
  msg_queue_t msg;

  // Must have Home Cell Service and 12v power:
  if( CellService() && globals.module.lost_12_volts == false ) 
  {
    msg.cmd = MSQ_QUEUE_CMD_PERF_MSG;
    PushToMsgQueue( msg );
  }
  
  perf_intvl = atoi(globals.module.perf_intvl);
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "perf_intvl:%d",perf_intvl);
  if( perf_intvl > 0 )
  {  
    m2m_timer_start( timer_perf_data_hdl, (perf_intvl*ONE_MINUTE) );
  }
}


/////////////////////////////////////////////////////////////////////////////
// Function: Start_Loc_Data_timer()
//
// Parameters: none
//
// Return: bool -> true if sucessfull
//
// Description: This function the Starts the Periodic location
// Data Timer.
//
/////////////////////////////////////////////////////////////////////////////
bool Start_Loc_Data_timer( void )
{
  int loc_intvl;

  loc_intvl = atoi( globals.module.Loc_intvl );
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "LOC_intvl:%s",globals.module.Loc_intvl);
  if( loc_intvl == 0 ) return( true ); // Do nothing

  if( timer_loc_data_hdl )
  {
    m2m_timer_start( timer_loc_data_hdl, (ONE_MINUTE) * loc_intvl );
    dwPrintDebug( M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Starting Location Timer");
    return( true );
  }
  else
  {
    dwPrintDebug( M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Failed to start System Timer");
    return( false );
  }
}


/////////////////////////////////////////////////////////////////////////////
// Function: Timer_Loc_data_handler()
//
// Parameters: none
//
// Return: none
//
// Description: This function the Timer handler that is called to trigger
// a periodic location message to be send to the RMS.
//
/////////////////////////////////////////////////////////////////////////////
void Timer_Loc_data_handler( void *arg )
{
  int         loc_intvl;
  msg_queue_t msg;

  // Must have Home Cell Service and 12V power:
  if( CellService() && globals.module.lost_12_volts == false ) 
  {
    msg.cmd = MSQ_QUEUE_CMD_LOC_MSG;
    PushToMsgQueue( msg );
  }

  loc_intvl = atoi( globals.module.Loc_intvl );
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "location_intvl at utilities:%d",loc_intvl);
  if( loc_intvl > 0 )
  {
    m2m_timer_start( timer_loc_data_hdl, (loc_intvl*ONE_MINUTE) );
  }
}


/////////////////////////////////////////////////////////////////////////////
// Function: restart_handler()
//
// Parameters: none
//
// Return: none
//
// Description: This Timer handler that is called to reset the module
//
/////////////////////////////////////////////////////////////////////////////
void restart_handler( void *arg )
{
  dwPrintDebug( M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Reset Module" );
  NE_debug( "Reset Module" );
  WriteCmdPkt0( CMD_PKT_RESET_MODULE );
}


/////////////////////////////////////////////////////////////////////////////
// Function: NE_debug()
//
// Parameters: *char -> debug string
//
// Return: response_t -> response enum type
//
// Description: This function tunnels a short (21 char total) debug string
// to the NE, and the NE displays it on its USB.  The NE's USB port must
// be unlocked and C34=191 or C34=0 to display this log message.
// 
// Note: the max string length is 50 chars, any chars after that will
// be truncated.
//
/////////////////////////////////////////////////////////////////////////////
response_t NE_debug( const char *str )
{
  char cmd[CMD_PKT_MAX_DATA_PAYLOAD+1];
  
  if( !globals.module.NE_debug_msg ) return( false );
  
  memset( cmd, 0, sizeof( cmd ));
  strncpy( cmd, str, CMD_PKT_MAX_DATA_PAYLOAD );
  return( WriteCmdPacket( CMD_PKT_SEND_DEBUG_MSG, cmd, strlen(cmd)) );  
}


/////////////////////////////////////////////////////////////////////////////
// Function: NE_debug_var()
//
// Parameters: *char -> debug string
//              int  -> debug data
//
// Return: response_t -> response enum type
//
// Description: This function tunnels a short (21 char total) debug string
// to the NE, and the NE displays it on its USB.  The NE's USB port must
// be unlocked and C34=191 or C34=0 to display this log message.
// 
// Note: the max string length (str + data) is 21 chars.  Plan your
// str length and data length according.  Max str length is 20 chars.
//
/////////////////////////////////////////////////////////////////////////////
response_t NE_debug_var( const char *str, int data )
{
  char cmd[100];
  
  if( !globals.module.NE_debug_msg ) return( false );
    
  if( strlen( str ) >= CMD_PKT_MAX_DATA_PAYLOAD )
  {
    return( WriteCmdPacket( CMD_PKT_SEND_DEBUG_MSG, "String too long", 15 ));
  }
  
  memset( cmd, 0, sizeof( cmd ));
  sprintf( cmd, "%s%d", str, data );
  cmd[CMD_PKT_MAX_DATA_PAYLOAD] = 0; // Truncate to 50 chars if needed
  
  return( WriteCmdPacket( CMD_PKT_SEND_DEBUG_MSG, cmd, strlen(cmd)) );  
}


/////////////////////////////////////////////////////////////////////////////
// Function: Req_OTA_phone_num()
//
// Parameters: none
//
// Return: void
//
// Description: This function requests the phone OTA from the network
// via special GSM USSD codes.   This is needed for new SIM cards in
// India because some are programmed OTA.
//
// Note: do not call this function until Cell Service is established.
//
/////////////////////////////////////////////////////////////////////////////
void Req_OTA_phone_num( void )
{
  int res;
  M2M_T_NETWORK_CURRENT_NETWORK curr_net;
  char atCmd[150];
  
  res = m2m_network_get_currently_selected_operator( &curr_net );
  dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE, "res:%d", res );
  dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE, "curr_net.longAlphanumeric:%s.\n",curr_net.longAlphanumeric);
  strcpy(globals.module.Operator,strupr(curr_net.longAlphanumeric));
  dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE, "curr_net.longAlphanumeric:%s.\n",globals.module.Operator);
  // Converted to uppercase and comparing:
  if( strncmp( globals.module.Operator, "VODAFONE",8 ) == 0 )
  {
    sprintf(atCmd,"AT+CUSD=1,%s,15\r", globals.module.USSD_voda);
    dwSendATCommand( atCmd, M2M_TRUE );
  }
  else if( strncmp( globals.module.Operator, "AIRTEL", 6 ) == 0 )
  {
    sprintf(atCmd,"AT+CUSD=1,%s,15\r", globals.module.USSD_air);
    dwSendATCommand( atCmd, M2M_TRUE );
  }
}

/////////////////////////////////////////////////////////////////////////////
// Function: xstrtok()
//
// Parameters: string, delimiter and count
//
// Return: character pointer of the parameter string given
//
// Description: This function parses the location response from the telit.
//
/////////////////////////////////////////////////////////////////////////////
char *xstrtok(char *line, char *delims, int *count)
{
static char *saveline = NULL;
char *p;
int n;

if(line != NULL)
   saveline = line;
/*
*see if we have reached the end of the line
*/
if(saveline == NULL || *saveline == '\0')
   return(NULL);
/*
*return the number of characters that aren't delims
*/
n = strcspn(saveline, delims);
*count=n;
p = saveline; /*save start of this token*/

saveline += n; /*bump past the delim*/

if(*saveline != '\0') /*trash the delim if necessary*/
   *saveline++ = '\0';

return(p);
}

/////////////////////////////////////////////////////////////////////////////
// Function: Req_location_info()
//
// Parameters: none
//
// Return: response_t -> response enum type
//
// Description: This function starts a PPP session and contacts
// Telit's location services for a location fix.
//
// Note: do not call this function until Cell Service is established.
//
/////////////////////////////////////////////////////////////////////////////
response_t Req_location_info( void )
{
  int res;

  res = m2m_cloud_pdp_context_activate( globals.module.apn, NULL, NULL );

  if( res == M2M_CLOUD_SUCCESS )
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "PPP is Connected");
    NE_debug("PPP Connected");
    dwSendATCommand(at_cmd_loc, M2M_TRUE);
    m2m_os_sleep_ms(1000);
  }
  else return( RSP_PDP_SESSION_FAILURE );

  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Disconnecting PPP..\n");
  NE_debug("PPP Disconnecting...");
  res = m2m_cloud_pdp_context_deactivate();
  if( res == M2M_CLOUD_SUCCESS )
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "PPP is Disconnected");
    NE_debug("PPP Disconnected");
  }
  return( RSP_SUCCESS );
}


/////////////////////////////////////////////////////////////////////////////
// Function: Alarm_Monitor()
//
// Parameters: none
//
// Return: none
//
// Description: This function monitors the NE for any Alarms once every
// ~5 seconds, and notifies the RMS server accordingly.  It also looks
// for any extended internal TCA Alarms.
//
// Note: This function must always run off the main control loop, it
// queries the NE every ~5 seconds, and this pets the NE's watchdog
// timer that monitors the Telit health.
//
/////////////////////////////////////////////////////////////////////////////
void Alarm_Monitor( void )
{
  UINT8         ne_code = NO_ALARM;
  response_t    rsp;
  char          alarm_str[25];
  UINT8         dLen;  
  static UINT8  old_ne_code = NO_ALARM;
  static UINT8  old_ext_code = NO_ALARM;
  static UINT32 cycles = 0;
  
  cycles++;
  
  // Once a day, maintain the config file & backup:
  if( cycles % SAMPLE_DAILY == 0 ) config_file_maintenance();
  
  if( cycles % SAMPLE_PERIOD_ALARM == 0 ) // ~5 seconds @ 18msec per cycle
  {  
    // Poll the NE for any Alarms, this action also resets the watchdog
    // code on the NE that monitors if the TCA is still alive.
    if( globals.module.ext_alarm_code == NO_ALARM )
    {
      memset( alarm_str, 0, sizeof( alarm_str ));
      rsp = Read_Alarms( &ne_code, alarm_str );   // Read possible Alarm
      dLen = strlen( alarm_str );                 // Add time stamp
      alarm_str[dLen++] = COMMA;
      epoch_time( &alarm_str[dLen]);
      dLen = strlen( alarm_str );

      if( rsp != RSP_SUCCESS ) 
      {
        dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Alarm Read Failed, rsp=%d",rsp);
        if( rsp == RSP_NE_COMM_ERROR )
        {
          ne_code = NO_ALARM;
          globals.module.ext_alarm_code = EXT_ALARM_CODE_NO_COMM;
        }  
        else
        {
          if( globals.module.ext_alarm_code == NO_ALARM ) return; // do nothing
        }  
      }
    }
    else
    {
      // At a minimum, ping the NE to keep the Watchdog happy
      WriteCmdPkt0( CMD_PKT_PING );
    }
    
    // New Extended Alarm?
    if( globals.module.ext_alarm_code != old_ext_code )
    {
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "New Ext Alarm=%d",globals.module.ext_alarm_code);
      memset( alarm_str, 0, sizeof( alarm_str ));
      sprintf( alarm_str, "%d\0", globals.module.ext_alarm_code );
      dLen = strlen( alarm_str );      // Add time stamp
      alarm_str[dLen++] = COMMA;
      epoch_time( &alarm_str[dLen]);
      dLen = strlen( alarm_str );

      // Can we report it?
      if( CellService() )
      {
        // call Alarm Unsol MQTT or SMS here
        // We must be RMS Activated to send any Alarms:
        if( !strcmp( globals.module.RMS_Activate, "Y" ) || 
            !strcmp( globals.module.RMS_Activate, "y" ) )
        {  
          rsp = send_MO_MSG( NE_ALARM_STR, NULL, REPLY_ID_UNSOLCITED, dLen, alarm_str, globals.module.conn_method );
          dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "SMS Ext Alarm=%d", globals.module.ext_alarm_code );
        }
        old_ext_code = globals.module.ext_alarm_code;
      }
      // Set old_code to 0xFF if !CellService(), this allows the
      // alarm to be sent when service is acquired in the future.
      else
      {
        old_ext_code = 0xFF;
        dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "No Cell, Alarm=%d",globals.module.ext_alarm_code);
      }
    }

    // New NE Alarm?
    if( ne_code != old_ne_code )
    {
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "New NE Alarm=%d",ne_code);

      // Can we report it?
      if( CellService() )
      {
        // call Alarm Unsol MQTT or SMS here
        // We must be RMS Activated to send any Alarms:
        if( !strcmp( globals.module.RMS_Activate, "Y" ) || 
            !strcmp( globals.module.RMS_Activate, "y" ) )
        {  
          rsp = send_MO_MSG( NE_ALARM_STR, NULL, REPLY_ID_UNSOLCITED, dLen, alarm_str, globals.module.conn_method );
          dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "SMS NE Alarm=%d, %d", ne_code, rsp);
        }
        old_ne_code = ne_code;
      }
      // Set old_code to 0xFF if !CellService(), this allows the
      // alarm to be sent when service is acquired in the future.
      else
      {
        old_ne_code = 0xFF;
        dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "No Cell, Alarm=%d",ne_code);
      }
    }
  } // every 5 seconds
}


