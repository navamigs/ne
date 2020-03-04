/*==================================================================================================
                            INCLUDE FILES
==================================================================================================*/

#include <stdio.h>
#include <string.h>

#include "m2m_type.h"
#include "m2m_clock_api.h"
#include "m2m_fs_api.h"
#include "m2m_hw_api.h"
#include "m2m_os_api.h"
#include "m2m_os_lock_api.h"
#include "m2m_socket_api.h"
#include "m2m_timer_api.h"
#include "m2m_sms_api.h"
#include "m2m_network_api.h"

#include "m2m_cloud_api.h"
#include "m2m_cloud_methods_api.h"
#include "cmd_pkt.h"
#include "sms.h"
#include "utilities.h"
#include "fs_file.h"
#include "ftp.h"
#include "log.h"
#include "paho_task.h"

/*==================================================================================================
                            LOCAL CONSTANT DEFINITION
==================================================================================================*/

/*=======================================
 = If SSL is required:
 = - Initialize the certificate file path defining CERT_PATH (see below)
 = - IMPORTANT! Remember to set the security variable to M2M_CLOUD_SSL_ON in the m2m_cloud_config() function

	On GE910 use .crt certificate files only.
	On 3G modules, both .crt or .der files are supported.

 = if SSL is not required:
 = Comment the #define line below.
 =========================================*/

//#define CERT_PATH "/rootCA.crt"
//#define CERT_PATH "/rootCA.der"


/*==================================================================================================
                            LOCAL TYPES DEFINITION
==================================================================================================*/

/*==================================================================================================
                            LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
#ifdef FEATURE_5BARZ

init_result_t initialize_system( void );
void          CheckMsgQueue( void );
void          fatal_error( void );
void          blink_led( void );

/*==================================================================================================
                            GLOBAL VARIABLES PROTOTYPES
==================================================================================================*/
M2M_CLOUD_GLOBALS globals;
M2M_T_OS_LOCK     msg_queue_lock;
M2M_T_OS_LOCK     PDPLock;

#endif // FEATURE_5BARZ 
/*==================================================================================================
                            LOCAL MACROS
==================================================================================================*/

/*==================================================================================================
                            LOCAL VARIABLES
==================================================================================================*/

/*==================================================================================================
                            GLOBAL VARIABLES
==================================================================================================*/

/*==================================================================================================
                            LOCAL FUNCTIONS IMPLEMENTATION
==================================================================================================*/


/*==================================================================================================
                            GLOBAL FUNCTIONS IMPLEMENTATION
==================================================================================================*/

/* =================================================================================================
 *
 * DESCRIPTION:     Handles events sent to process 1
 *
 * PARAMETERS:      type:	event id
 *                  param1: addition info
 *                  param2: addition info
 *
 * RETURNS:         None.
 *
 * PRE-CONDITIONS:  None.
 *
 * POST-CONDITIONS: None.
 *
 * IMPORTANT NOTES: This process has the highest priority of all 10 user tasks.
 * 					This function runs on the user task #1 (on it's main loop).
 *					Running complex code here will block other events coming to this task.
 * ============================================================================================== */
INT32 M2M_msgProc1(INT32 type, INT32 param1, INT32 param2)
{
  bool   test = false;
  bool   finish = false;
  int    res;
  UINT16 enabled,version;
  UINT32 seconds = 0;
  init_result_t init_rsp;

  
  // Start Initialization:
  init_rsp = initialize_system();
  if( init_rsp == INIT_SUCCESS )
  {
    NE_debug("Init Passed");
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Sucessful Initialization of Module.");
    NE_debug(software_version);
  }
  else
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Initialization of Module Failed:%d",init_rsp);
    NE_debug_var("Init Failed:",init_rsp);

    if( init_rsp == INIT_FAIL_SIM )
    {
      WriteCmdPkt0( CMD_PKT_NO_SIM_CARD_PRESENT );
      fatal_error();
    }
    else if( init_rsp == INIT_FAIL_CONFIG || init_rsp == INIT_FAIL_SOFTWARE )  
    {
      WriteCmdPkt0( CMD_PKT_MODULE_SOFTWARE_FAILURE );
      fatal_error();
    }
    else if( init_rsp == INIT_FAIL_NE )
    {
      globals.module.ext_alarm_code = EXT_ALARM_CODE_NO_COMM;
    }
  }
   
 
  ////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////
  //                                                        //
  //            M a i n   C o n t r o l   L o o p           //
  //                                                        //
  ////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////  
  while(1)
  {
    CheckMsgQueue();
    Registration_Manager();
    Power_Monitor();
    Alarm_Monitor();
    MQTT_task_manager( param1 );
    
    // Notify Logging:
    if( globals.module.sec_running % 3 == 0 && seconds != globals.module.sec_running )
    {
      seconds = globals.module.sec_running;
      Logging_Manager( LOG_EVENT_SEC_TICK );
    }

    if( globals.module.rev_cmd_pkt == true ) ProcessRevCmdPacket();
    
    blink_led();
    
    // We need a small delay here to allow other tasks to run if needed.
    m2m_os_sleep_ms(1); 
  }
}

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Function: CheckMsgQueue()
//
// Parameters: none
//
// Return: none
//
// Description: This function checks the Task1 Message Queue for any new
// Message and processes them.  Only one message at a time is processed.
//
/////////////////////////////////////////////////////////////////////////////
void CheckMsgQueue( void )
{
  static int last = 0;
  static char msg[SMS_MAX_LENGTH];
  
  if( last == globals.msg_queue.last ) return;
  last = ++last % 20;
  if( globals.msg_queue.queue[last].cmd == MSG_QUEUE_CMD_NONE ) return;

  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Msg Queue: New Message Command:%d Payload:%d local_last:%d", \
               globals.msg_queue.queue[last].cmd, globals.msg_queue.queue[last].payload1, last );
    
  /////////////////////////////////////////////////
  //          Disconnect from RMS Server         //
  /////////////////////////////////////////////////
  if( globals.msg_queue.queue[last].cmd == MSG_QUEUE_CMD_DISCONNECT ) // Disconnect MQTT
  {
    MQTT_stop();
  }
  /////////////////////////////////////////////////
  //                  New MT SMS                 //
  /////////////////////////////////////////////////
  else if( globals.msg_queue.queue[last].cmd == MSG_QUEUE_CMD_MT_SMS ) // New MT SMS Arrived
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "New MT SMS, index=%d", globals.msg_queue.queue[last].payload1 );
    read_mt_sms( globals.msg_queue.queue[last].payload1 );
  }
  /////////////////////////////////////////////////
  //        Periodic Performance Message         //
  /////////////////////////////////////////////////
  else if( globals.msg_queue.queue[last].cmd == MSQ_QUEUE_CMD_PERF_MSG )
  {
    PeriodicPerformanceData( NULL );
  }
  /////////////////////////////////////////////////
  //        Boot-up Performance Message 1        //
  /////////////////////////////////////////////////
  else if( globals.msg_queue.queue[last].cmd == MSQ_QUEUE_CMD_BASIC_MSG1 )
  {
    Bootup_perf_basic1();
  }
  /////////////////////////////////////////////////
  //        Boot-up Performance Message 2        //
  /////////////////////////////////////////////////
  else if( globals.msg_queue.queue[last].cmd == MSQ_QUEUE_CMD_BASIC_MSG2 )
  {
    Bootup_perf_basic2();
  }
  /////////////////////////////////////////////////
  //        Boot-up Performance Message 3        //
  /////////////////////////////////////////////////
  else if( globals.msg_queue.queue[last].cmd == MSQ_QUEUE_CMD_BASIC_MSG3 )
  {
    Bootup_perf_basic3();
  }
  /////////////////////////////////////////////////
  //             Lost the 12v Power              //
  /////////////////////////////////////////////////
  else if( globals.msg_queue.queue[last].cmd == MSQ_QUEUE_CMD_12V_LOST )
  {
    globals.module.lost_12_volts = true;
    globals.module.ext_alarm_code = EXT_ALARM_CODE_NO_12V;
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "No 12v, Start the shut down timer");
    Logging_Manager( LOG_EVENT_NO_POWER );
    m2m_timer_stop( client.cycle_timer ); // Once the timers are stopped, automatic
    m2m_timer_free( client.cycle_timer ); // MQTT disconnect notification will be 
    m2m_timer_stop( client.png_timer );   // to RMS by broker.
    m2m_timer_free( client.png_timer );
    Start_shutdown_timer();
  }
  /////////////////////////////////////////////////
  //         Periodic Location Message           //
  /////////////////////////////////////////////////
  else if( globals.msg_queue.queue[last].cmd == MSQ_QUEUE_CMD_LOC_MSG )
  {
    PeriodicLocationData();
  }
  /////////////////////////////////////////////////
  //        Unsolicited FTP SMS Message          //
  /////////////////////////////////////////////////
  else if( globals.msg_queue.queue[last].cmd == MSQ_QUEUE_CMD_FTP_SMS )
  {  
    send_unsol_rsp_msg( NE_FTP_RSP, globals.msg_queue.queue[last].payload1, 
                        globals.msg_queue.queue[last].payload2, 
                        globals.msg_queue.queue[last].payload3 );
  }
  /////////////////////////////////////////////////
  //              Re-Boot Module                 //
  /////////////////////////////////////////////////
  else if( globals.msg_queue.queue[last].cmd == MSQ_QUEUE_CMD_REBOOT_MODULE )
  {  
    Reset_Module( 0 );
  }
  /////////////////////////////////////////////////
  //      FTP Upload of Log file finished        //
  /////////////////////////////////////////////////
  else if( globals.msg_queue.queue[last].cmd == MSQ_QUEUE_CMD_FTP_UPLOAD )
  {  
    if( globals.msg_queue.queue[last].payload1 == 1 ) Logging_Manager( LOG_EVENT_FTP_SUCCESS );
      else Logging_Manager( LOG_EVENT_FTP_FAIL );
  }
  /////////////////////////////////////////////////
  //      MQTT MODE: unsolicited MQTT CMD        //
  /////////////////////////////////////////////////
  else if( globals.msg_queue.queue[last].cmd == MSQ_QUEUE_CMD_MQTT )
  {
    send_unsol_rsp_msg( NE_MQTT_STR, 
                        globals.msg_queue.queue[last].payload1,
                        globals.msg_queue.queue[last].payload2,
                        globals.msg_queue.queue[last].payload3 );
  }
  ////////////////////////////////////////////////////
  //  MQTT MESSAGE: MQTT msg to send to SMS parsing //
  ////////////////////////////////////////////////////
  else if( globals.msg_queue.queue[last].cmd == MSQ_QUEUE_CMD_MQTT_MSG )
  {  
    memset( msg, 0, sizeof(msg) );
    if( globals.msg_queue.queue[last].payload1 <= SMS_MAX_LENGTH ) // Msg len >120 for 0D0B(FTP) so used
    {                                                              // SMS_MAX_LENGTH(Max Length preferred)
      strncpy( msg, globals.msg_queue.queue[last].msg, globals.msg_queue.queue[last].payload1 );
      parse_mt_sms( msg, globals.msg_queue.queue[last].payload1, CONN_METHOD_MQTT_ONLY );
    }
    m2m_os_mem_free( globals.msg_queue.queue[last].msg );
  }
  ////////////////////////////////////////////////////
  //        Notify the NE GSM service is lost       //
  ////////////////////////////////////////////////////
  else if( globals.msg_queue.queue[last].cmd == MSG_QUEUE_CMD_LOST_SERVICE )
  {
    WriteCmdPkt16( CMD_PKT_NOTIFY_CELL_SERVICE, 0 );
  }
  
  // Clean up:
  globals.msg_queue.queue[last].cmd = MSG_QUEUE_CMD_NONE;
  globals.msg_queue.queue[last].payload1 = 0;
  globals.msg_queue.queue[last].payload2 = 0;
}


/////////////////////////////////////////////////////////////////////////////
// Function: initialize_system()
//
// Parameters: none
//
// Return: init_result_t:
//  INIT_SUCCESS,       Initialization success
//  INIT_FAIL_NE,       No serial communication or unsupported NE
//  INIT_FAIL_SIM,      No SIM card detected
//  INIT_FAIL_CONFIG,   Config file failed to load
//  INIT_FAIL_SOFTWARE, Initernal Software failure
//
// Description: This function initializes all necessary hardware and software
// systems for proper module operation.
//
/////////////////////////////////////////////////////////////////////////////
init_result_t initialize_system( void )
{
  int   res, loc;
  char  *pc, buf[CMD_PKT_MAX_DATA_PAYLOAD+1];
  UINT8 len;
  response_t    rsp;
  init_result_t init_rsp;
 
  // Init Radio, data, mqtt, power:
  globals.Reg_state   = REG_STATE_SEARCHING;
  globals.power_state = PWR_STATE_12V_ON;

  // Init LEDs:
  m2m_hw_gpio_conf(1,1);  // Configure LED1 gpio as output
  m2m_hw_gpio_conf(2,1);  // Configure LED2 gpio as output
  LED1_ON;
  LED2_OFF;

  Init_uart();

  // Init Globals:
  globals.module.NE_debug_msg      = true;  // Turn ON NE debug messages for development, off for commericial
  globals.module.SIM_present       = false; // Default to false, will test for SIM downstream with AT#QSS
  globals.module.lost_12_volts     = false; // Default is false
  globals.module.ftp_in_progress   = false; // Default is false
  globals.module.mqtt_bootup       = false; // Default is false
  globals.module.MQTT_start_time   = 0;
  globals.module.location.accuracy = 0;
  globals.module.GSM_channel       = 0;
  globals.module.ext_alarm_code    = NO_ALARM;
  
  memset( globals.module.location.latitude,  0, sizeof(globals.module.location.latitude) );
  memset( globals.module.location.longitude, 0, sizeof(globals.module.location.longitude) );
  memset( globals.module.imei,         0, sizeof(globals.module.imei) );
  memset( globals.module.imei_quote,   0, sizeof(globals.module.imei_quote) );
  memset( globals.module.model,        0, sizeof(globals.module.model) );
  memset( globals.module.firmware,     0, sizeof(globals.module.firmware) );
  memset( globals.module.phone_num,    0, sizeof(globals.module.phone_num) );
  memset( globals.module.ne_version,   0, sizeof(globals.module.ne_version) );

  // Defaults for location & phone number:  
  strcpy(globals.module.location.latitude,"0.0"); 
  strcpy(globals.module.location.longitude,"0.0");
  strcpy(globals.module.phone_num,"0");
  strcpy(globals.module.USSD_voda,"*111*2#");
  strcpy(globals.module.USSD_air,"*282#");
  
  
  //debug: Choose one:
  //AZ2 connected to AT2, Small HEAP size (64k), Log Level to debug, logs printed on USB0 channel (HighSpeed Modem). Use USB_CH_NONE to print to UART
  //res = m2m_cloud_init(M2M_AZ2, AT2, M2M_CLOUD_HEAP_S, M2M_CLOUD_LOG_NONE, USB_CH_DEFAULT); // turns off all debug msg on USB, no need for USB cable
  res = m2m_cloud_init(M2M_AZ2, AT2, M2M_CLOUD_HEAP_S, M2M_CLOUD_LOG_DEBUG, USB_CH_DEFAULT); // turns on debug msg on USB, must have USB cable present
  //res = m2m_cloud_init(M2M_AZ2, AT2, M2M_CLOUD_HEAP_S, M2M_CLOUD_LOG_DEBUG, USB_CH2);       // turns on debug msg on USB, must have USB cable present
  if (res != M2M_CLOUD_SUCCESS)
  {
    dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_TRUE, "Error Init: At_cmd iface, Heap, USB. Return value: %d", res);
    NE_debug("Cloud Init Failed");
    return( INIT_FAIL_SOFTWARE );
  }
  else dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Internal Interface Init Successful");
  
  
  
  
  // Read the Cloud Enabled status from the NE:
  rsp = Read_NE_Cloud_Enabled( &globals.module.cloud_enabled, NULL );
  if( rsp == RSP_SUCCESS )
  {  
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Cloud Enabled:%d",globals.module.cloud_enabled);
  }
  else
  {  
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Failed to Read Cloud Enabled, rsp=%d",rsp);
    // Read again:
    m2m_os_sleep_ms(51); // wait for RX buffer to timeout
    rsp = Read_NE_Cloud_Enabled( &globals.module.cloud_enabled, NULL );
    if( rsp == RSP_SUCCESS )
    {
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Cloud Enabled:%d",globals.module.cloud_enabled);
    }   
    else
    {
      return( INIT_FAIL_NE ); // Declare Init failure
    }
  }  
  
  // Read the NE's Software Version:
  memset( buf, 0, sizeof( buf ) );
  rsp = ReadCmdPacket( CMD_PKT_READ_SW_VERSION, buf, &len );
  if( rsp == RSP_SUCCESS )
  {
    strncpy( globals.module.ne_version, buf, len );  
  }
  else
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Failed to read SW version, rsp=%d", rsp);
  }

  //Init DW remoteAT feature
  res = m2m_cloud_remoteAT(M2M_CLOUD_DW_ENABLED);
  if (res != M2M_CLOUD_SUCCESS)
  {
    dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_TRUE, "Error Init: DW_Cloud Services:%d", res );
    NE_debug("Remote AT cmd Failed");
    return( INIT_FAIL_SOFTWARE );
  }
  else dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "DW_Cloud Service Init Successful");

  // Read in the 5Barz Config file:
  config_file_maintenance();
  if( read_config_file() != RSP_SUCCESS )
  {
    NE_debug("Config file error");
    return( INIT_FAIL_CONFIG );
  }
  else init_config_file_lock(); // Init FS Lock
    
  // Call after m2m_cloud_remoteAT():
  dwSendATCommand(at_cmd_CGMM, M2M_TRUE ); // Retrieve Model Designation
  dwSendATCommand(at_cmd_CGMR, M2M_TRUE ); // Retrieve Firmware Rev
  dwSendATCommand(at_cmd_CGSN, M2M_TRUE ); // Retrieve IMEI
  
  // Test for IMEI:  
  if( strlen( globals.module.imei ) == 0 ) 
  {
    dwSendATCommand(at_cmd_CGSN, M2M_TRUE ); // Retrieve IMEI again
    if( strlen( globals.module.imei ) == 0 )
    {
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Cannot read IMEI");
      NE_debug("Cannot read IMEI\0");
      return( INIT_FAIL_SOFTWARE );
    }
  }
  
  dwSendATCommand(at_cmd_qss,  M2M_TRUE ); // Query SIM status

  if( globals.module.SIM_present == false )
  {
    // Try AT#QSS? again:
    dwSendATCommand(at_cmd_qss, M2M_TRUE ); // Query SIM status again
    if( globals.module.SIM_present == false )
    {
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "No SIM card detected");
      NE_debug("No SIM card");
      return( INIT_FAIL_SIM );
    }
  }

#ifdef USA_TMOBILE  
  dwSendATCommand(at_cmd_cnum, M2M_TRUE ); // Retrieve Phone number from SIM for T-Mobile only
  // Test for Phone Number 
  if( strlen( globals.module.phone_num ) == 0 ) 
  {
    dwSendATCommand(at_cmd_cnum, M2M_TRUE ); // Retrieve IMEI again
    if( strlen( globals.module.phone_num ) == 0 )
    {
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Cannot read Phone Number");
      NE_debug("Cannot read Phone Number\0");
    }
  }  
#endif // TMOBILE  


  // SMS Initialization:  
  if( !init_sms() ) 
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "SMS Init Failed");
    NE_debug("SMS Init Failed");
    return( INIT_FAIL_SOFTWARE );
  }
  delete_all_SMS();
  crcInit();
  
  // Timer creation in initialization:
  Init_lost_service_timer();
  timer_perf_data_hdl = m2m_timer_create( Timer_Perf_data_handler, NULL );
  timer_loc_data_hdl  = m2m_timer_create( Timer_Loc_data_handler, NULL );
  timer_restart       = m2m_timer_create( restart_handler, NULL );
  if( timer_perf_data_hdl == 0 || timer_loc_data_hdl == 0 || timer_restart == 0 )
  {
    NE_debug("Init timer Failed");
    return( INIT_FAIL_SOFTWARE );
  }    
     
  if( !Init_Seconds_timer() )
  {
    NE_debug("Init Sec timer Failed");
    return( INIT_FAIL_SOFTWARE );
  }
  
  // Init the Task1 Message Queue & Lock:
  msg_queue_lock = m2m_os_lock_init(M2M_OS_LOCK_CS);
  memset( &globals.msg_queue, 0, sizeof( globals.msg_queue ) );
  globals.msg_queue.last = 0;
  
  // Init FTP Task items:
  globals.module.ftp_taskID = m2m_os_create_task( M2M_OS_TASK_STACK_L, 
                                                  FTP_TASK_PRIORTIY, 
                                                  M2M_OS_TASK_MBOX_S, 
                                                  FTP_InitProc );
  if( globals.module.ftp_taskID < 1 )
  {
    NE_debug("Failed FTP task init");
    return( INIT_FAIL_SOFTWARE );
  }

  // Start Initialization of MQTT:
  init_rsp = Initialize_MQTT();
  if( init_rsp == INIT_SUCCESS )
  {
	  NE_debug("MQTT Init Passed");
	  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Successful Initialization of MQTT");
  }
  else
  {
	  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Initialization of Module MQTT failed:%d",init_rsp);
	  NE_debug_var("MQTT Init Failed:",init_rsp);
    return( init_rsp );
  }    

  // Init the Logging Manager:
  Logging_Manager( LOG_EVENT_INIT );
  
  // Determine witch NE we are connected to:
  if( ReadCmdPkt16( CMD_PKT_READ_BOARD_ID, &globals.module.NE_boardID ) == RSP_SUCCESS )
  {
    if( globals.module.NE_boardID != ATLAS_BAND1 && 
        globals.module.NE_boardID != TITAN_BAND1_BAND8 )
    {
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Unknown NE board type");
      return( INIT_FAIL_NE );
    }
  }
  else return( INIT_FAIL_NE );
  
  // Calculate Random start-up delay hashed from the IMEI:
  globals.module.start_stop_delay = (globals.module.imei[13] % 5) * 10;
  globals.module.start_stop_delay += globals.module.imei[14] % 10;
  globals.module.start_stop_delay *= 100; // 0.0 to 4.9 seconds or 0 - 4900 msec
  m2m_os_sleep_ms( globals.module.start_stop_delay ); 
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Start-up delay: %d", globals.module.start_stop_delay);
  
  Start_lost_service_timer(); // this should be last
  LED1_OFF;
  return( INIT_SUCCESS );
}


/////////////////////////////////////////////////////////////////////////////
// Function: PushToMsgQueue()
//
// Parameters: msg_queue_t
//
// Return: none
//
// Description: This function allows any other tasks or callback to 
// send a message to Task1 safety.
//
/////////////////////////////////////////////////////////////////////////////
void PushToMsgQueue( msg_queue_t msg )
{
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Msg Queue: Push Called, CMD:%d", msg.cmd );
  
  m2m_os_lock_wait(msg_queue_lock, 10000); // Lock if needed
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Msg Queue: Push Locked");
    
  globals.msg_queue.last = ++globals.msg_queue.last % 20;
  globals.msg_queue.queue[globals.msg_queue.last] = msg;
  m2m_os_lock_unlock( msg_queue_lock );  

  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Msg Queue: Push Unlocked, Last=%d", globals.msg_queue.last );
}


/////////////////////////////////////////////////////////////////////////////
// Function: fatal_error()
//
// Parameters: none
//
// Return: none
//
// Description: This function never returns.  It runs an infinite loop
// that pings the NE every 4 seconds to keep the NE's watchdog timer
// happy.  This function is called only if some fatal error has occured
// that prevents the TCA from running properly or communicating with
// the RMS.  In this rare case, we want to keep the AppZone running
// but perform no real tasks without the NE's watchdog from resetting
// the module over and over again.
//
/////////////////////////////////////////////////////////////////////////////
void fatal_error( void )
{
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Fatal Error, cannot continue.");
  while( 1 )
  {
    LED2_ON;
    m2m_os_sleep_ms(2000);
    LED2_OFF;
    m2m_os_sleep_ms(2000);
    
    WriteCmdPkt0( CMD_PKT_PING );
  }
}


/////////////////////////////////////////////////////////////////////////////
// Function: blink_led()
//
// Parameters: none
//
// Return: none
//
// Description: This function blinks LED2 on/off every second.
//
/////////////////////////////////////////////////////////////////////////////
void blink_led( void )
{
  static UINT32 sec;
  
  if( sec != globals.module.sec_running )
  {
    sec = globals.module.sec_running;
    if( sec % 2 == 0 ) LED2_ON;
      else LED2_OFF;
  }
}


void M2M_msgProcCompl( INT8 procId, INT32 type, INT32 result )
{
  /* write code fulfilling the requirements of the M2M project */
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Task Done: %d, %d, %d",procId,type,result);
}
  
