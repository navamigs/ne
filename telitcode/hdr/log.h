/*************************************************************************
                    5BARz International Incorporated. 

                 (C) Copyright 2017. All Rights Reserved.

Filename: log.h

Description: This header file contains interfaces to several OTA
BTL log functions.

*************************************************************************/
#ifndef HDR_LOG_H_
#define HDR_LOG_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include "m2m_cloud_defines.h"


typedef enum {
  LOG_EVENT_INIT = 0,    // 0: Initialize
  LOG_EVENT_SEC_TICK,    // 1: Periodic tick from control loop
  LOG_EVENT_CELL_SIGNAL, // 2: New Cell Service
  LOG_EVENT_NO_POWER,    // 3: Lost 12v power
  LOG_EVENT_FTP_SUCCESS, // 4: FTP upload sucessful
  LOG_EVENT_FTP_FAIL,    // 5: FTP upload failed
  LOG_EVENT_START_FTP,   // 6: Start a FTP upload
  LOG_EVENT_STOP_LOGGING // 7: Stop any current logging
} log_event_t;

 
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
void Logging_Manager( log_event_t event );

 
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
bool Log_check_ftp_config( void );


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
bool check_log_time_validity( UINT32 logstart, UINT32 logstop );


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
response_t Log_new_start_stop( char *data );


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
bool LoggingNow( void );


#ifdef __cplusplus
}
#endif   
   
#endif /* HDR_LOG_H_ */
