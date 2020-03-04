/*************************************************************************
                    5BARz International Incorporated. 

                 (C) Copyright 2017. All Rights Reserved.

Filename: log.c

Description: This file contains several OTA BTL log functions including
a state machine to control all logging and various support functions.

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


#define MIN_LOG_TIME     10100    // Jan 1, 00 hours 
#define MAX_LOG_TIME    123123    // Dec 31, 23 hours

// Global Variables:
M2M_CLOUD_GLOBALS  globals;


// Local Function Prototypes:
bool Log_init_start_stop_config( void );
bool check_log_dates( UINT32 logtime );
void Log_update_current_time( void );

typedef enum{
  LOG_IDLE = 0,         // 0: Do nothing state
  LOG_INIT,             // 1: Initialization after power-up
  LOG_INIT_TIME,        // 2: Wait for GSM service (if necessary), then init start or end time/date
  LOG_WATIING_TO_START, // 3: We have valid start time in the future, waiting to start
  LOG_START_LOGGING,    // 4: Start the logging process
  LOG_LOGGING,          // 5: Currently Logging
  LOG_STOP_LOGGING,     // 6: Done logging, save file 
  LOG_START_FTP,        // 7: Start FTP file upload
  LOG_WAITING_FTP,      // 8: FTP upload taking place, waiting for result
} log_state_t;

static log_state_t logState = LOG_IDLE;

UINT32 log_start = 0;
UINT32 log_stop  = 0;
UINT32 current   = 0;


/////////////////////////////////////////////////////////////////////////////
// Function: Logging_Manager()
//
// Parameters: none
//
// Return: none
//
// Description: This function manages all OTA logging activities.  It will
// start and stop logging, upload the logfile via FTP, and delete off the
// logfile when done.  It can handle a loss of power too.
//
/////////////////////////////////////////////////////////////////////////////
void Logging_Manager( log_event_t event )
{
  UINT16             data16;
  response_t         rsp;
  M2M_T_RTC_DATE     date;
  M2M_T_RTC_TIME     time;
  
  if( event == LOG_EVENT_INIT ) logState = LOG_INIT;
  
  
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Log State:%d  Event:%d", logState, event );
  
  switch( logState )
  {
    case LOG_IDLE: // State 0
      // Do nothing because logging is done or invalid config data
      globals.module.logging = false;
      Log_update_current_time();
      if( event == LOG_EVENT_START_FTP ) logState = LOG_START_FTP;
      break;
    
    case LOG_INIT: // State 1
      globals.module.logging = false;
      if( Log_check_ftp_config() && 
          Log_init_start_stop_config() ) logState = LOG_INIT_TIME;
      else logState = LOG_IDLE;
      break;
            
    case LOG_INIT_TIME: // State 2
      // Do we have valid times:
      if( !log_start || !log_stop )
      {
        logState = LOG_IDLE;
        break;
      }
      
      // Get the time/date and determine where we stand:
      Log_update_current_time();
      if( current == 0 ) break; // Time not valid, wait...
        
      if( current > log_stop )         logState = LOG_IDLE;             // Past event completed
      else if( current == log_stop )   logState = LOG_STOP_LOGGING;     // Done, wrap things up
      else if( current <  log_start )  logState = LOG_WATIING_TO_START; // Logging in future
      else if( current >= log_start &&                                  // Starting now or in the middle
               current <  log_stop )   logState = LOG_START_LOGGING;
      else logState = LOG_IDLE;                                         // default, shouldn't happen
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Log: Init time, going to state:%d", logState );
      break;
  
    case LOG_WATIING_TO_START: // State 3  
      if( event == LOG_EVENT_START_FTP ) 
      {
        logState = LOG_START_FTP;
        break;
      }
      Log_update_current_time();
      if( current == 0 ) break; // Time not valid, wait...
      if( current >= log_start && current < log_stop ) logState = LOG_START_LOGGING;
      if( current >= log_stop ) logState = LOG_IDLE;
      break;
      
    case LOG_START_LOGGING: // State 4
      if( OpenLogFile() != RSP_SUCCESS )
      {
        dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Could not open Logfile");
        logState = LOG_IDLE;
      }
      if( current == log_start ) append_time_stamp_logfile( STAMP_OPEN );
        else if( current > log_start ) append_time_stamp_logfile( STAMP_RESUME );
      globals.module.logging = true;
      // Turn On Logging:
      if( WriteCmdPkt0( CMD_PKT_START_OTA_LOGGING ) != RSP_SUCCESS )
      {
        dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Could not turn ON OTA Logging");
        logState = LOG_IDLE;
      }
      logState = LOG_LOGGING;
      break;
      
    case LOG_LOGGING: // State 5
      Log_update_current_time();
      
      if( event == LOG_EVENT_NO_POWER ) // Monitor for lost 12v
      {
        globals.module.logging = false;
        FlushLogFile();
        append_time_stamp_logfile( STAMP_PWR_FAIL );
        CloseLogFile( false );
        logState = LOG_IDLE;
        break;
      }
      if( current >= log_stop || current == 0 || event == LOG_EVENT_STOP_LOGGING ) // Monitor if we are done
      {
        globals.module.logging = false;
        logState = LOG_STOP_LOGGING;
      }
      break;
      
    case LOG_STOP_LOGGING: // State 6
      globals.module.logging = false;
      FlushLogFile();
      append_time_stamp_logfile( STAMP_FINISH );
      WriteCmdPkt0( CMD_PKT_STOP_OTA_LOGGING ); // Turn Off OTA BTL Logging:
      
      CloseLogFile( false ); // Close the logFile
      log_start = 0;
      log_stop = 0;
      logState = LOG_START_FTP;            
      update_config_file( CFG_FILE_ITEM_LOG_STOP, "0" );
      // Note: don't blank the start time yet, used in ftp filename
      break;
      
    case LOG_START_FTP: // State 7
      if( Log_check_ftp_config() )
      {
        if( LogFileExist() == RSP_SUCCESS )
        {
          if( ftp_start( FTP_UPLOAD_FILE, NULL, NULL ) == RSP_SUCCESS ) 
          {
            logState = LOG_WAITING_FTP;
            dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "LOG: FTP started, going to FTP waiting state");
            break;
          } 
          else dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "LOG: FTP not started, error");
        }
        else dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "LOG: FTP not started, No Log File present");
      }
      else dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "LOG: FTP not started, Config invalid");
      logState = LOG_IDLE;
      break;
      
    case LOG_WAITING_FTP: // State 8
      if( event == LOG_EVENT_SEC_TICK ) break; // still waiting for ftp to finish
      
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "LOG: FTP Done, event:%d", event);
      
      logState = LOG_IDLE;
      if( event == LOG_EVENT_FTP_SUCCESS )
      {
        dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "LOG: ftp worked, deleting log file");
        DeleteLogFile();
        update_config_file( CFG_FILE_ITEM_LOG_START, "0" );
      }
      if( event == LOG_EVENT_FTP_FAIL )
      {
        dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "LOG: ftp failed, saving log file");
      }

      // Special case if we starting FTP while waiting for a new session:
      if( log_start != 0 && log_stop != 0 )
      {  
        Log_update_current_time();
        if( current != 0 )
        {
          if( current < log_start && current < log_stop ) logState = LOG_INIT_TIME;
        }
      }
      break;
      
    default:
      logState = LOG_IDLE;
      break;
  }
}


/////////////////////////////////////////////////////////////////////////////
// Function: Log_check_ftp_config()
//
// Parameters: none
//
// Return: bool -> true if the configuration data is valid
//
// Description: This function reviews the logging information read in from
// the config file.  It verifies the FTP server data is valid.
//
/////////////////////////////////////////////////////////////////////////////
bool Log_check_ftp_config( void )
{
  bool res = true;
  
  // Check the FTP Server configuration:
  if( strlen( globals.module.log_ftp_ip ) == 1     && globals.module.log_ftp_ip[0] == NUM0 ) res = false;
  if( strlen( globals.module.log_ftp_path ) == 1   && globals.module.log_ftp_path[0] == NUM0 ) res = false;
  if( strlen( globals.module.log_ftp_userID ) == 1 && globals.module.log_ftp_userID[0] == NUM0 ) res = false;
  if( strlen( globals.module.log_ftp_passWD ) == 1 && globals.module.log_ftp_passWD[0] == NUM0 ) res = false;
  if( globals.module.log_ftp_path[0] != FSLASH ) res = false;
  
  if( res )
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Log FTP Server Configuration data is good" );
  }
  else
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Invalid Log FTP Server Configuration data");
  }
  
  return( res );
}


/////////////////////////////////////////////////////////////////////////////
// Function: Log_init_start_stop_config()
//
// Parameters: none
//
// Return: bool -> true if the start/stop configuration data is valid
//
// Description: This function reviews the logging information read in from
// the config file.  It verifies the proper log start and stop time/dates 
// are valid.
//
/////////////////////////////////////////////////////////////////////////////
bool Log_init_start_stop_config( void )
{
  bool res = true;
  
  memset( globals.module.log_start_time, 0, sizeof(globals.module.log_start_time) );
  memset( globals.module.log_start_time, 0, sizeof(globals.module.log_stop_time) );
  
  read_config_file_item( CFG_FILE_ITEM_LOG_START, globals.module.log_start_time, NULL );
  read_config_file_item( CFG_FILE_ITEM_LOG_STOP, globals.module.log_stop_time, NULL );
  
  // Check the Start & Stop time & dates:
  if( strlen( globals.module.log_start_time ) != 6 ) res = false;
  if( strlen( globals.module.log_stop_time ) != 6 )  res = false;
  
  if( res )
  {
    log_start = atoi( globals.module.log_start_time );
    log_stop = atoi( globals.module.log_stop_time );
    res = check_log_time_validity( log_start, log_stop );
  }
    
  if( res )
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Log Start: %s", globals.module.log_start_time );
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Log Stop:  %s", globals.module.log_stop_time );
  }
  else
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Invalid Log Start or Stop Times");
    log_start = 0;
    log_stop = 0;
  }
  
  return( res );
}


/////////////////////////////////////////////////////////////////////////////
// Function: Log_new_start_stop()
//
// Parameters: char* data -> string containing the start,stop date/time.
//
// note: the string must be comma spearated valid log times: MMDDHH,MMDDHH 
// or MDDHH,MDDHH (1 or 2 chars for the month are allowed).
//
// Return: response_t
//
// Description: This function parses a string with new start and stop 
// logging times.  It verifies the proper log start and stop time/dates 
// are valid, writes them to the config file, and notifies the Log
// Manager of the new times. 
//
/////////////////////////////////////////////////////////////////////////////
response_t Log_new_start_stop( char *data )
{
  int    i;
  char   start[14];
  char   stop[14];
  char   tmp[14];
  UINT32 start32;
  UINT32 stop32;

  // Check if we are logging or FTP'ing a log file:
  if( logState == LOG_START_LOGGING ||
      logState == LOG_LOGGING       ||
      logState == LOG_STOP_LOGGING  ||
      logState == LOG_START_FTP     ||
      logState == LOG_WAITING_FTP )  return( RSP_LOGGING_IN_PROCESS );
  
  i = strlen( data );
  if( i < 11 || i > 13 ) return( RSP_PARAMETER_OUT_OF_RANGE );
  if( strchr( data, COMMA ) == 0 ) return( RSP_PARAMETER_OUT_OF_RANGE );
  
  memset( start, 0, sizeof( start ));
  memset( tmp, 0, sizeof( tmp ));
  strcpy( tmp, strtok( data, "," ) );
  i = strlen( tmp );
  if( i < 5 || i > 6 ) return( RSP_PARAMETER_OUT_OF_RANGE );
  if( i == 5 ) // Pad a leading "0" for months 1-9 if the RMS didn't send it.
  {
    start[0] = NUM0;      
    strcat( start, tmp );
  }
  else
  {
    strcpy( start, tmp );
  }
  
  memset( stop, 0, sizeof( stop ));
  memset( tmp, 0, sizeof( tmp ));
  strcpy( tmp, strtok( NULL, "\0" ));
  i = strlen( tmp );
  if( i < 5 || i > 6 ) return( RSP_PARAMETER_OUT_OF_RANGE );
  if( i == 5 ) // Pad a leading "0" for months 1-9 if the RMS didn't send it.
  {
    stop[0] = NUM0;
    strcat( stop, tmp );
  }
  else
  {
    strcpy( stop, tmp );
  }  
  
  start32 = atoi( start );
  stop32 = atoi( stop );
  
  if( !check_log_time_validity( start32, stop32 ) )
  {  
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Log: New log dates/times are invalid");
    return( RSP_PARAMETER_OUT_OF_RANGE );
  }

  // Update the Config file entries, locals, and re-init the times:
  update_config_file( CFG_FILE_ITEM_LOG_START, start );
  update_config_file( CFG_FILE_ITEM_LOG_STOP, stop );
  strcpy( globals.module.log_start_time, start );
  strcpy( globals.module.log_stop_time, stop );
  log_start = start32;
  log_stop  = stop32;
  logState  = LOG_INIT_TIME;
  
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "New Valid Log Start=%s, Stop=%s,", start, stop );
  return( RSP_SUCCESS );  
}



/////////////////////////////////////////////////////////////////////////////
// Function: check_log_dates()
//
// Parameters: UNIT32 -> logtime MMDDHH or MDDHH
//
// Return: bool -> true if the date/time is valid
//
// Description: This function reviews the date/time information and verifies
// if the dates match real calender dates, and time is valid too.
//
/////////////////////////////////////////////////////////////////////////////
bool check_log_dates( UINT32 logtime )
{
  bool result = true;
  UINT8 month, day, hour;  

  if( logtime < MIN_LOG_TIME || logtime > MAX_LOG_TIME ) result = false;  

  month = logtime / 10000;
  day   = logtime % 10000 / 100;
  hour  = logtime % 100;
  
  
  if( month < 1 || month > 12 ||
      day   < 1 || day > 31   ||
      hour  > 23 )
  {
    result = false;
  }  
  
  if( ( month == 4 ||    // April
        month == 6 ||    // June
        month == 9 ||    // September
        month == 11 ) && // November
        day > 30 )
  {
    result = false;
  }

  if( month == 2 && day > 29 ) result = false; // February
  
  if( !result ) dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Invalid Log Date: %d,%d,%d", month, day, hour);
  return( result );  
}


/////////////////////////////////////////////////////////////////////////////
// Function: check_log_time_validity()
//
// Parameters: UNIT32 -> logstart (MMDDHH or MDDHH)
//             UNIT32 -> logstop  (MMDDHH or MDDHH) 
//
// Return: bool -> true if start and stop time are valid & within 5 days
//
// Description: This function reviews the start & stop date/time information 
// and verifies the validity and if the difference is less than 5 days. 
// (maximum allowed log time).
//
/////////////////////////////////////////////////////////////////////////////
bool check_log_time_validity( UINT32 logstart, UINT32 logstop )
{
  UINT8 month1, month2;
  UINT8 day1, day2;
  
  if( !check_log_dates( logstart ) ) return( false );      
  if( !check_log_dates( logstop ) ) return( false );      
  if( logstop <= logstart ) return( false );
  
  month1 = logstart / 10000;
  month2 = logstop / 10000;
  day1 = logstart % 10000 / 100;
  day2 = logstop % 10000 / 100;
     
  if( month1 == month2 )
  {
    if( (day2 - day1) <= 5 ) return( true );
  }

  if( (month1 + 1) == month2 )
  {
    day1 = 31 - day1;
    if( month1 == 4 ||  // April
        month1 == 6 ||  // June
        month1 == 9 ||  // September
        month1 == 11 )  // November
    {
      if( day1 > 0 ) day1--;
    }  
    if( month1 == 2 && day1 > 2 ) day1 -= 2; // February
     
    if( (day1 + day2) <= 5 ) return( true );  
  }
  return( false );
}


/////////////////////////////////////////////////////////////////////////////
// Function: Log_update_current_time()
//
// Parameters: none
//
// Return: none
//
// Description: This function updates the current time in a local variable
// for log time/date calculations.
//
/////////////////////////////////////////////////////////////////////////////
void Log_update_current_time( void )
{
  M2M_T_RTC_DATE     date;
  M2M_T_RTC_TIME     time;
  
  m2m_rtc_get_date( &date );
  m2m_rtc_get_time( &time );
    
  if( date.year != 0 )
  {  
    current = date.month * 10000;
    current += date.day * 100;
    current += time.hour;
  }
  else
  {
    current = 0;
  }
}


/////////////////////////////////////////////////////////////////////////////
// Function: LoggingNow()
//
// Parameters: none
//
// Return: bool -> currently logging
//
// Description: This function returns if the log manager is currently
// logging data or FTP'ing a log file.   It is called by external
// functions to allow house cleaning of the log file.
//
/////////////////////////////////////////////////////////////////////////////
bool LoggingNow( void )
{
  if( logState == LOG_START_LOGGING ||
      logState == LOG_LOGGING       ||
      logState == LOG_STOP_LOGGING  ||
      logState == LOG_START_FTP     ||
      logState == LOG_WAITING_FTP )  
  {
    return( true );
  }
  else return( false );
}

