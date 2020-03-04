/*************************************************************************
                    5BARz International Incorporated.

                 (C) Copyright 2017. All Rights Reserved.

Filename: fs_file.c

*************************************************************************/

/*==================================================================================================
                            INCLUDE FILES
==================================================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "m2m_type.h"
#include "m2m_fs_api.h"
#include "m2m_hw_api.h"
#include "m2m_os_api.h"
#include "m2m_cloud_defines.h"
#include "cmd_pkt.h"
#include "fs_file.h"
#include "log.h"

/*==================================================================================================
                            LOCAL CONSTANT DEFINITION
==================================================================================================*/
// Name of the config file
char configfilename[30]   = "/MOD/5bconfigfile.txt\0"; // Note: filename is case sensitive, don't change it.
char configfilenameBK[30] = "/MOD/5bbackupfile.txt\0"; // Back-up config file
char logFileName[17]      = "/MOD/logfile.txt\0";

/*==================================================================================================
                            LOCAL TYPES DEFINITION
==================================================================================================*/
#define RX_LOG_BUF_SIZE   1000
#define MAX_LOG_FILE_SIZE 1000000
#define MIN_CFG_FILESIZE  110

/*==================================================================================================
                            LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
response_t WriteLogFile( UINT8 *buf, UINT16 len );
response_t verify_config_file( char *filename );
bool       compare_config_files( void );
/*==================================================================================================
                            GLOBALS 
==================================================================================================*/
M2M_CLOUD_GLOBALS  globals;

/*==================================================================================================
                            LOCAL VARIABLES
==================================================================================================*/
M2M_T_FS_HANDLE Fhandle;
M2M_T_FS_HANDLE LogFhandle = NULL;

UINT8  logbuffer[RX_LOG_BUF_SIZE];
UINT16 logIndex = 0;

M2M_T_OS_LOCK   config_file_lock;


/////////////////////////////////////////////////////////////////////////////
// Function: NewLogEntry()
//
// Parameters: none
//
// Return: response_t -> response enum type
//
// Description: This function appends the BTL log message to an internal
// memory buffer.  When the buffer is full, the data is flushed to the
// logfile.
// 
///////////////////////////////////////////////////////////////////////////// 
void NewLogEntry( UINT8 *buf, UINT8 len )
{
  bool flush = false;
  bool copy  = false;
  
  if( len == 0 || buf == NULL || LogFhandle == NULL ) return;
  
  if( (len + logIndex) > (RX_LOG_BUF_SIZE - 2) )
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "BTL buffer overrun, flushing to file");
    flush = true;
  }
  else if( (len + logIndex) < (RX_LOG_BUF_SIZE - CMD_PKT_MAX_DATA_PAYLOAD - 2) )
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Adding line to BTL log buffer, buf=%d len=%d", logIndex, len );
    copy = true;
  }
  else if( (len + logIndex) < (RX_LOG_BUF_SIZE - 2) ) // flush
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "BTL log buffer full, flush to log file=%d", logIndex );
    copy = true;
    flush = true;
  }
  
  if( copy )
  {
    memcpy( &logbuffer[logIndex], buf, len );
    logIndex += len;
    logbuffer[logIndex++] = CARR_RET;  
    logbuffer[logIndex++] = LINEFEED;
  }
  
  if( flush )
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "BTL buffer full, flush to LogFile, bytes=%d\n", logIndex );
    CloseLogFile( true ); // Flush, Close, and re-Open to force
    OpenLogFile();        // data to be saved to the file.
  }
}


/////////////////////////////////////////////////////////////////////////////
// Function: CloseLogFile()
//
// Parameters: bool -> flush the BTL memeory buffer first
//
// Return: response_t -> response enum type
//
// Description: This function closes the BTL log file with the option to
// flush the BTL memory buffer first. The logfile must be opened with a 
// valid local file handle.
// 
///////////////////////////////////////////////////////////////////////////// 
void CloseLogFile( bool flush )
{
  if( LogFhandle == NULL ) return;
  
  if( flush && logIndex > 0 )
  {
    FlushLogFile();
  }
  
  if( m2m_fs_close( LogFhandle ) == M2M_API_RESULT_SUCCESS )
  {
    LogFhandle = NULL;
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "LogFile Closed\n" );
  }
  else
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "LogFile failed to close, error=%d\n", m2m_fs_last_error() );
}


/////////////////////////////////////////////////////////////////////////////
// Function: FlushLogFile()
//
// Parameters: none
//
// Return: none
//
// Description: This function will flush all log data in the local memory
// buffer and append it to the LogFile.  Only writes to the LogFile if
// is smaller than the maxiumu size allowed.
// 
///////////////////////////////////////////////////////////////////////////// 
void FlushLogFile( void )
{
  if( LogFhandle == NULL ) return;
  
  if( logIndex > 0 )
  {
    if( LogFileSize() < MAX_LOG_FILE_SIZE ) WriteLogFile( logbuffer, logIndex );
    logIndex = 0;
  }  
}


/////////////////////////////////////////////////////////////////////////////
// Function: WriteLogFile()
//
// Parameters: none
//
// Return: response_t -> response enum type
//
// Description: This function appends the BTL log file with log data
// from the local log memory buffer.  The logfile must be opened with
// a valid local file handle.
// 
///////////////////////////////////////////////////////////////////////////// 
response_t WriteLogFile( UINT8 *buf, UINT16 len )
{
  UINT32 bytes;
  
  if( len == 0 || buf == NULL || LogFhandle == NULL )
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Cannot write to LogFile, invalid buffer or handle");
    return( RSP_FS_CANNOT_WRITE_FILE );
  }
  
  bytes = m2m_fs_write( LogFhandle, buf, len );
  if( bytes == M2M_FS_ERROR || bytes < len )
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Write to LogFile failed, error=%d", m2m_fs_last_error() );
    return( RSP_FS_CANNOT_WRITE_FILE );
  }

  return( RSP_SUCCESS );
}


/////////////////////////////////////////////////////////////////////////////
// Function: OpenLogFile()
//
// Parameters: none
//
// Return: response_t -> response enum type
//
// Description: This function the opens the BTL log file for appending,
// assigns a file handle to it.  If the file does not exist, it creates 
// it first.
// 
///////////////////////////////////////////////////////////////////////////// 
response_t OpenLogFile( void )
{
  M2M_T_FS_ERROR_TYPE err;
  char buf[M2M_FS_MAX_FILENAME_LEN];
  
  // Already exist? Create it if not:
  memset( buf, 0, M2M_FS_MAX_FILENAME_LEN );
  if( m2m_fs_find_first( buf, logFilePathName ) == M2M_API_RESULT_FAIL )
  {
    if( m2m_fs_create( logFilePathName ) != M2M_API_RESULT_SUCCESS )
    {
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Cannot create LogFile, error=%d", m2m_fs_last_error() );
      return( RSP_FS_CANNOT_OPEN_FILE );
    }    
  }
  
  // Open to Append:
  LogFhandle = m2m_fs_open( logFilePathName, M2M_FS_OPEN_APPEND );
  if( LogFhandle == NULL )
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Cannot open LogFile, error=%d", m2m_fs_last_error() );
    return( RSP_FS_CANNOT_OPEN_FILE );
  }

  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "LogFile opened" );
  return( RSP_SUCCESS );
}


/////////////////////////////////////////////////////////////////////////////
// Function: DeleteLogFile()
//
// Parameters: none
//
// Return: response_t -> response enum type
//
// Description: This function the deletes the BTL log file.  If the file
// is open, it will close it first.
// 
///////////////////////////////////////////////////////////////////////////// 
response_t DeleteLogFile( void )
{
  if( LogFhandle != NULL )
  {
    if( m2m_fs_close( LogFhandle ) == M2M_API_RESULT_SUCCESS )
    {
      LogFhandle = NULL;
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "LogFile Closed\n" );
    }
  }

  if( m2m_fs_delete( logFilePathName ) == M2M_API_RESULT_SUCCESS )
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "LogFile Deleted\n" );
    return( RSP_SUCCESS );
  }
  else
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "LogFile Deletion failed\n" );
    return( RSP_FS_CANNOT_DELETE_FILE );
  }
}


/////////////////////////////////////////////////////////////////////////////
// Function: append_time_stamp_logfile()
//
// Parameters: stamp_t -> enum indicating the event type for time stamp
//
// Return: none
//
// Description: This function the appends a time/date stamp to the log
// file.  This happens at the beginning, end, or resumption of a log
// after a power failure
// 
///////////////////////////////////////////////////////////////////////////// 
void append_time_stamp_logfile( stamp_t event )
{
  char           buf[80];
  M2M_T_RTC_DATE date;
  M2M_T_RTC_TIME time;
  
  if( LogFhandle != NULL )
  {
    memset( buf, 0, sizeof( buf ));  
    m2m_rtc_get_date( &date );
    m2m_rtc_get_time( &time );
    
    sprintf( buf, "Time,%d/%d/%d,%d:%d:%d,", 
             date.day, date.month, date.year, time.hour, time.minute, time.second );
    if( event == STAMP_OPEN )          strcat( buf, "START\r\n" );
    else if( event == STAMP_RESUME )   strcat( buf, "RESUME\r\n" );
    else if( event == STAMP_FINISH )   strcat( buf, "COMPLETE\r\n" );
    else if( event == STAMP_PWR_FAIL ) strcat( buf, "POWER_FAILURE\r\n" );
    m2m_fs_write( LogFhandle, buf, strlen( buf ) );
  }
}


/////////////////////////////////////////////////////////////////////////////
// Function: LogFileExist()
//
// Parameters: none
//
// Return: response_t -> response enum type
//
// Description: This function determines if the Log File exists or not.
// 
///////////////////////////////////////////////////////////////////////////// 
response_t LogFileExist( void )
{
  char buf[M2M_FS_MAX_FILENAME_LEN];
  
  // Log File exist?
  memset( buf, 0, M2M_FS_MAX_FILENAME_LEN );
  if( m2m_fs_find_first( buf, logFilePathName ) == M2M_API_RESULT_FAIL )
  {
    return( RSP_FS_CANNOT_FIND_FILE );
  }
  else
  {
    return( RSP_SUCCESS );
  }
}


/////////////////////////////////////////////////////////////////////////////
// Function: LogFileSize()
//
// Parameters: none
//
// Return: UINT32 - Log File size
//
// Description: This function returns the Log File size.
// 
///////////////////////////////////////////////////////////////////////////// 
UINT32 LogFileSize( void )
{
  return( m2m_fs_get_size( logFilePathName ));
}


/////////////////////////////////////////////////////////////////////////////
// Function: read_config_file_item()
//
// Parameters: cfg_file_item_t -> The item to be read from config file
//             char* data    -> pointer to buffer to place data read, 
//                              min size = 100
//             char* comment -> pointer to buffer to place the comments,
//                              may be NULL is not desired. min size = 100
//
// Return: response_t -> response enum type
//
// Description: This function the opens and reads one item out of the
// 5bconfigfile.txt file in the :\MOD directory and returns the data
// and comments.
// 
///////////////////////////////////////////////////////////////////////////// 
response_t read_config_file_item( cfg_file_item_t item, char *data, char *comment )
{
  char buf[CFG_FILE_ITEM_LEN];
  int  filesize = 0;
  int  i, k, len;
  int  fptr = 0;
  bool os = true;
  char test;
  char params[CFG_FILE_ITEM_MAX][CFG_FILE_ITEM_LEN];
  char comm[100];
  
  if( item >= CFG_FILE_ITEM_MAX ) return( RSP_PARAMETER_OUT_OF_RANGE );
  
  m2m_os_lock_wait( config_file_lock, 10000 );
  Fhandle = m2m_fs_open( configfilename, M2M_FS_OPEN_READ );
  if( Fhandle != NULL )
  {
    filesize = m2m_fs_get_size( configfilename );
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"Config file size = %d\n", filesize);
    if( filesize > (CFG_FILE_ITEM_MAX * CFG_FILE_ITEM_LEN) )
    {
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Config File too big");
      m2m_fs_close( Fhandle ); // Close the file
      m2m_os_lock_unlock( config_file_lock );
      return( RSP_CFG_FILE_FORMAT_ERROR );
    }
    
    for( i = 0; i < CFG_FILE_ITEM_MAX; i++ )
    {  
      m2m_fs_seek( Fhandle, fptr );
      while( 1 ) // skip over any extra CR & LF that may exist
      {  
        test = m2m_fs_getc( Fhandle );
        if( test != CARR_RET && test != LINEFEED ) break;
          else fptr++;
      }
      m2m_fs_seek( Fhandle, fptr );
      memset( buf, 0, sizeof(buf));
      m2m_fs_gets( buf, (filesize - fptr), Fhandle );
      fptr += strlen( buf );
      if( i == item )
      {
        memset( comm, 0, sizeof(comm));
        strcpy( data, strtok( buf, "," ));   // Parse the data before comma
        strcpy( comm, strtok( NULL, "\0" )); // Parse the comment
        break;
      }      
    }
    m2m_fs_close( Fhandle ); // Close the file
    
    // Clean up the comment of spaces, Tabs, LF, CR:
    if( comment )
    {
      k = 0;
      len = strlen( comm );
      for( i = 0; i < len; i++ )
      {
        if( comm[i] != TAB &&
            comm[i] != LINEFEED &&
            comm[i] != CARR_RET )
        {       
          if( os ) // remove opening spaces only
          {  
            if( comm[i] != SPACE )
            {
              comment[k++] = comm[i];
              os = false;
            }
          }
          else comment[k++] = comm[i];
        }
      }
      comment[k] = 0; // null term
    }

    m2m_os_lock_unlock( config_file_lock );
    return( RSP_SUCCESS );
  }
  else
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"ERROR: Unable to open Config file\n");
    m2m_os_lock_unlock( config_file_lock );
    return( RSP_CFG_FILE_NOT_PRESENT );
  }
}


/////////////////////////////////////////////////////////////////////////////
// Function: read_config_file()
//
// Parameters: none
//
// Return: response_t -> response enum type
//
// Description: This function the opens and reads the 5bconfigfile.txt
// in the :\MOD directory and initializes all the applicable global variables.
// This function is intended to be called once, during initialization.
// 
///////////////////////////////////////////////////////////////////////////// 
response_t read_config_file( void )
{
  char buf[CFG_FILE_ITEM_LEN];
  int  filesize = 0;
  int  i;
  int  fptr = 0;
  char test;
  char params[CFG_FILE_ITEM_MAX][CFG_FILE_ITEM_LEN];

  Fhandle = m2m_fs_open( configfilename, M2M_FS_OPEN_READ );
  if( Fhandle != NULL )
  {
    filesize = m2m_fs_get_size( configfilename );
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"Config file size = %d\n", filesize);
    if( filesize > (CFG_FILE_ITEM_MAX * CFG_FILE_ITEM_LEN) )
    {
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Config File too big");
      m2m_fs_close( Fhandle ); // Close the file
      return( RSP_CFG_FILE_FORMAT_ERROR );
    }
    
    for( i = 0; i < CFG_FILE_ITEM_MAX; i++ )
    {  
      m2m_fs_seek( Fhandle, fptr );
      while( 1 ) // skip over any extra CR & LF that may exist
      {  
        test = m2m_fs_getc( Fhandle );
        if( test != CARR_RET && test != LINEFEED ) break;
          else fptr++;
      }
      m2m_fs_seek( Fhandle, fptr );
      memset( buf, 0, sizeof(buf));
      m2m_fs_gets( buf, (filesize - fptr), Fhandle );
      fptr += strlen( buf );
      strcpy( params[i], strtok( buf,"," ));
    }
    m2m_fs_close( Fhandle ); // Close the file

    // Copy the extracted data to corresponding global variables
    memset( globals.module.RMS_Activate,   0, sizeof(globals.module.RMS_Activate) );
    memset( globals.module.apn,            0, sizeof(globals.module.apn) );
    memset( globals.module.sms_addr,       0, sizeof(globals.module.sms_addr) );
    memset( globals.module.mqttPWD,        0, sizeof(globals.module.mqttPWD) );
    memset( globals.module.mqttIP,         0, sizeof(globals.module.mqttIP) );
    memset( globals.module.HB_intvl,       0, sizeof(globals.module.HB_intvl) );
    memset( globals.module.perf_intvl,     0, sizeof(globals.module.perf_intvl) );
    memset( globals.module.Loc_intvl,      0, sizeof(globals.module.Loc_intvl) );
    memset( globals.module.TCA_ver_cfg,    0, sizeof(globals.module.TCA_ver_cfg) );
    memset( globals.module.ftp_state_str,  0, sizeof(globals.module.ftp_state_str) );
    memset( globals.module.log_ftp_ip,     0, sizeof(globals.module.log_ftp_ip) );
    memset( globals.module.log_ftp_path,   0, sizeof(globals.module.log_ftp_path) );
    memset( globals.module.log_ftp_userID, 0, sizeof(globals.module.log_ftp_userID) );
    memset( globals.module.log_ftp_passWD, 0, sizeof(globals.module.log_ftp_passWD) );
    memset( globals.module.log_start_time, 0, sizeof(globals.module.log_start_time) );
    memset( globals.module.log_stop_time,  0, sizeof(globals.module.log_stop_time) );
    memset( globals.module.sw_ftp_ip,      0, sizeof(globals.module.sw_ftp_ip) );
    memset( globals.module.sw_ftp_path,    0, sizeof(globals.module.sw_ftp_path) );
    memset( globals.module.sw_ftp_userID,  0, sizeof(globals.module.sw_ftp_userID) );
    memset( globals.module.sw_ftp_passWD,  0, sizeof(globals.module.sw_ftp_passWD) );

    strncpy( globals.module.RMS_Activate, params[CFG_FILE_ITEM_RMS_ACTIVATE], RMS_ACT_LEN );
    strncpy( globals.module.apn, params[CFG_FILE_ITEM_APN], APN_STR_LEN );
    strncpy( globals.module.sms_addr, params[CFG_FILE_ITEM_SMS_SHORT_CODE], SMS_ADDR_LEN );
    strncpy( globals.module.mqttPWD, params[CFG_FILE_ITEM_MQTT_PASSWORD], MQTTPWD_STR_LEN );
    strncpy( globals.module.mqttIP, params[CFG_FILE_ITEM_MQTT_URL], URL_STR_LEN );
    strncpy( globals.module.HB_intvl, params[CFG_FILE_ITEM_MQTT_HEARTBEAT], HB_INTVL_LEN );
    strncpy( globals.module.perf_intvl, params[CFG_FILE_ITEM_PERF_INTERVAL], PERF_INTVL_LEN);
    strncpy( globals.module.Loc_intvl, params[CFG_FILE_ITEM_LOC_INTERVAL], PERF_INTVL_LEN );
    strncpy( globals.module.TCA_ver_cfg, params[CFG_FILE_ITEM_TCA_VERSION], FIRMWARE_STR_LEN );
    strncpy( globals.module.ftp_state_str, params[CFG_FILE_ITEM_FTP_STATE], FTP_STATE_LEN );
    strncpy( globals.module.log_ftp_ip, params[CFG_FILE_ITEM_LOG_FTP_URL], CFG_FILE_ITEM_LEN );
    strncpy( globals.module.log_ftp_path, params[CFG_FILE_ITEM_LOG_PATH], CFG_FILE_ITEM_LEN );
    strncpy( globals.module.log_ftp_userID, params[CFG_FILE_ITEM_LOG_FTP_USERID], USERID_PASSWD_LEN );
    strncpy( globals.module.log_ftp_passWD, params[CFG_FILE_ITEM_LOG_FTP_PASSWD], USERID_PASSWD_LEN );
    strncpy( globals.module.log_start_time, params[CFG_FILE_ITEM_LOG_START], LOG_TIME_MSG_LEN );
    strncpy( globals.module.log_stop_time, params[CFG_FILE_ITEM_LOG_STOP], LOG_TIME_MSG_LEN );
    strncpy( globals.module.sw_ftp_ip, params[CFG_FILE_ITEM_SW_FTP_URL], CFG_FILE_ITEM_LEN );
    strncpy( globals.module.sw_ftp_path, params[CFG_FILE_ITEM_SW_PATH], CFG_FILE_ITEM_LEN );
    strncpy( globals.module.sw_ftp_userID, params[CFG_FILE_ITEM_SW_FTP_USERID], USERID_PASSWD_LEN );
    strncpy( globals.module.sw_ftp_passWD, params[CFG_FILE_ITEM_SW_FTP_PASSWD], USERID_PASSWD_LEN );
    
    memset( buf, 0, sizeof( buf ));
    strcpy( buf, params[CFG_FILE_ITEM_CONN_METHOD] );
    globals.module.conn_method = atoi( buf );
    if( globals.module.conn_method < 0 || globals.module.conn_method >= CONN_METHOD_MAX_VALUE )
      globals.module.conn_method = CONN_METHOD_SMS_ONLY;
    
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"1->%s",params[CFG_FILE_CARRIER_NAME]);
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"2->%s",globals.module.RMS_Activate);
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"3->%d",globals.module.conn_method);
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"4->%s",globals.module.apn);
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"5->%s",globals.module.sms_addr);
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"6->%s",globals.module.mqttPWD);
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"7->%s",globals.module.mqttIP);
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"8->%s",globals.module.HB_intvl);
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"9->%s",globals.module.perf_intvl);
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"10->%s",globals.module.Loc_intvl);
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"11->%s",globals.module.TCA_ver_cfg);
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"12->%s",globals.module.ftp_state_str);
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"13->%s",globals.module.log_ftp_ip);
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"14->%s",globals.module.log_ftp_path);
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"15->%s",globals.module.log_ftp_userID);
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"16->%s",globals.module.log_ftp_passWD);
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"17->%s",globals.module.log_start_time);
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"18->%s",globals.module.log_stop_time);
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"19->%s",globals.module.sw_ftp_ip);
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"20->%s",globals.module.sw_ftp_path);
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"21->%s",globals.module.sw_ftp_userID);
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"22->%s",globals.module.sw_ftp_passWD);
    
    return( RSP_SUCCESS );
  }
  else
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"ERROR: Unable to open Config file\n");
    return( RSP_CFG_FILE_NOT_PRESENT );
  }
}
 

/////////////////////////////////////////////////////////////////////////////
// Function: update_config_file()
//
// Parameters: cfg_file_item_t -> item to be updated
//             char *buf -> pointer to new item string (null terminated)
//
// Return: response_t -> response enum type
//
// Description: This function modifies a single item within the Config File.
// It preserves the origenal comments on the line.
// 
///////////////////////////////////////////////////////////////////////////// 
response_t update_config_file( cfg_file_item_t item, char *str )
{
  int  i;
  int  fptr = 0;
  int  filesize = 0;
  char line[CFG_FILE_ITEM_MAX][CFG_FILE_ITEM_LEN];
  char comment[100];
  char test;
  char *param;
  
  memset( line, 0, sizeof( line ));
  memset( comment, 0, sizeof( comment ));
  
  m2m_os_lock_wait( config_file_lock, 10000 ); // place a lock
  Fhandle = m2m_fs_open( configfilename, M2M_FS_OPEN_READ );
  if( Fhandle != NULL )
  {
    filesize = m2m_fs_get_size( configfilename );
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"Config file size = %d\n", filesize);
    if( filesize > (CFG_FILE_ITEM_MAX * CFG_FILE_ITEM_LEN) )
    {
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Config File too big");
      m2m_fs_close( Fhandle ); // Close the file
      m2m_os_lock_unlock( config_file_lock );
      return( RSP_CFG_FILE_FORMAT_ERROR );
    }
    
    for( i = 0; i < CFG_FILE_ITEM_MAX; i++ )
    {  
      m2m_fs_seek( Fhandle, fptr );
      while( 1 ) // skip over any extra CR & LF that may exist
      {  
        test = m2m_fs_getc( Fhandle );
        if( test != CARR_RET && test != LINEFEED ) break;
        else 
        {  
          fptr++;
          if( i > 0 ) strncat( line[i-1], &test, 1 );
          dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"%d:new len=%d\r",i-1,strlen( line[i-1] ));
        }
      }
      m2m_fs_seek( Fhandle, fptr );
      m2m_fs_gets( line[i], (filesize - fptr), Fhandle );
      fptr += strlen( line[i] );
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"%d,%d:%s\r",i,strlen( line[i] ), line[i]);
    }
    m2m_fs_close( Fhandle ); // Close the file for reading
    
    // Parse our new string into the line:
    param = strtok( line[item], "," );            // Parse the data before comma
    strcpy( comment, strtok( NULL, "\0" ));       // Parse the comment
    memset( line[item], 0, sizeof( line[item] )); // Blank the line
    sprintf( line[item], "%s,%s", str, comment ); // re-construct the line with new data

    // Let's write it back in:
    Fhandle = m2m_fs_open( configfilename,  M2M_FS_OPEN_CREATE );
    if( Fhandle != NULL )
    {    
      for( i = 0; i < CFG_FILE_ITEM_MAX; i++ ) 
      {  
        m2m_fs_write( Fhandle, line[i], strlen( line[i] ) );
      }
      m2m_fs_close( Fhandle ); // Close the file
           
      // Update the Global variable too:
      if( item == CFG_FILE_ITEM_RMS_ACTIVATE ) strncpy( globals.module.RMS_Activate, str, RMS_ACT_LEN );
      if( item == CFG_FILE_ITEM_CONN_METHOD )
      {
        globals.module.conn_method = atoi( str );
        if( globals.module.conn_method < 0 || globals.module.conn_method >= CONN_METHOD_MAX_VALUE )
          globals.module.conn_method = CONN_METHOD_SMS_ONLY;
      }
      if( item == CFG_FILE_ITEM_APN ) strncpy( globals.module.apn, str, APN_STR_LEN );
      if( item == CFG_FILE_ITEM_SMS_SHORT_CODE ) strncpy( globals.module.sms_addr, str, SMS_ADDR_LEN );
      if( item == CFG_FILE_ITEM_MQTT_PASSWORD ) strncpy( globals.module.mqttPWD, str, MQTTPWD_STR_LEN );
      if( item == CFG_FILE_ITEM_MQTT_URL ) strncpy( globals.module.mqttIP, str, URL_STR_LEN );
      if( item == CFG_FILE_ITEM_MQTT_HEARTBEAT ) strncpy( globals.module.HB_intvl, str, HB_INTVL_LEN );
      if( item == CFG_FILE_ITEM_PERF_INTERVAL ) strncpy( globals.module.perf_intvl, str, PERF_INTVL_LEN);
      if( item == CFG_FILE_ITEM_LOC_INTERVAL ) strncpy( globals.module.Loc_intvl, str, PERF_INTVL_LEN );
      if( item == CFG_FILE_ITEM_TCA_VERSION ) strncpy( globals.module.TCA_ver_cfg, str, FIRMWARE_STR_LEN );
      if( item == CFG_FILE_ITEM_FTP_STATE ) strncpy( globals.module.ftp_state_str, str, FTP_STATE_LEN );
      if( item == CFG_FILE_ITEM_LOG_FTP_URL ) strncpy( globals.module.log_ftp_ip, str, CFG_FILE_ITEM_LEN );
      if( item == CFG_FILE_ITEM_LOG_PATH ) strncpy( globals.module.log_ftp_path, str, CFG_FILE_ITEM_LEN );
      if( item == CFG_FILE_ITEM_LOG_FTP_USERID ) strncpy( globals.module.log_ftp_userID, str, USERID_PASSWD_LEN );
      if( item == CFG_FILE_ITEM_LOG_FTP_PASSWD ) strncpy( globals.module.log_ftp_passWD, str, USERID_PASSWD_LEN );
      if( item == CFG_FILE_ITEM_LOG_START ) strncpy( globals.module.log_start_time, str, LOG_TIME_MSG_LEN );
      if( item == CFG_FILE_ITEM_LOG_STOP ) strncpy( globals.module.log_stop_time, str, LOG_TIME_MSG_LEN );
      if( item == CFG_FILE_ITEM_SW_FTP_URL ) strncpy( globals.module.sw_ftp_ip, str, CFG_FILE_ITEM_LEN );
      if( item == CFG_FILE_ITEM_SW_PATH ) strncpy( globals.module.sw_ftp_path, str, CFG_FILE_ITEM_LEN );
      if( item == CFG_FILE_ITEM_SW_FTP_USERID ) strncpy( globals.module.sw_ftp_userID, str, USERID_PASSWD_LEN );
      if( item == CFG_FILE_ITEM_SW_FTP_PASSWD ) strncpy( globals.module.sw_ftp_passWD, str, USERID_PASSWD_LEN );
      
      m2m_os_lock_unlock( config_file_lock );
      return( RSP_SUCCESS ); 
    }
    else
    {
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"ERROR: Unable to open Config file for writing\n");
      m2m_os_lock_unlock( config_file_lock );      
      return( RSP_CFG_FILE_NOT_PRESENT );
    }
  }
  else
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"ERROR: Unable to open Config file\n");
    m2m_os_lock_unlock( config_file_lock );
    return( RSP_CFG_FILE_NOT_PRESENT );
  }
}


/////////////////////////////////////////////////////////////////////////////
// Function: init_config_file_lock()
//
// Parameters: void
//
// Return: none
//
// Description: This function initializes an Critical Section Lock for config
// file reads and writes.   This prevents 2 tasks colliding over config
// file operations.
// 
///////////////////////////////////////////////////////////////////////////// 
void init_config_file_lock( void )
{
  config_file_lock = m2m_os_lock_init( M2M_OS_LOCK_CS );
}


/////////////////////////////////////////////////////////////////////////////
// Function: config_file_maintenance()
//
// Parameters: void
//
// Return: bool
//
// Description: This function inspects the config file.  If there is a 
// problem, it restores it from the back-up file.  Also, if the back-up
// file is missing or out of date, it restores it too.   This function
// should be called at boot-up initialization, and periodically to
// keep the back-up up to date.  It should NOT be called every time
// the config file is updated!
// 
///////////////////////////////////////////////////////////////////////////// 
void config_file_maintenance( void )
{
  // Verify the Config File, make back-up if needed:
  if( verify_config_file( configfilename ) == RSP_SUCCESS )
  {
    // See if we need to update the back-up
    if( verify_config_file( configfilenameBK ) != RSP_SUCCESS ||   
        !compare_config_files())
    {      
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Config Maintenance: Restore Back-up");
      m2m_fs_delete( configfilenameBK );
      m2m_fs_copy( configfilename, configfilenameBK ); // original -> backup
    }
  }
  else // We got a problem, verify the backup, and restore original:
  {
    if( verify_config_file( configfilenameBK ) == RSP_SUCCESS )
    {
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Config Maintenance: Restore Original");
      m2m_fs_delete( configfilename );
      m2m_fs_copy( configfilenameBK, configfilename ); // backup -> original
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
// Function: verify_config_file()
//
// Parameters: char * -> ptr to the string with the full path to the
//                       config file or config file backup.
//
// Return: response_t
//
// Description: This function verifies the existance and proper format
// of the config file or the config file backup.
// 
///////////////////////////////////////////////////////////////////////////// 
response_t verify_config_file( char *filename )
{
  int        i, filesize, comma = 0;
  bool       valid = true;
  char       *buf;
  char       name[M2M_FS_MAX_FILENAME_LEN];
  response_t rsp = RSP_SUCCESS;
  
  // Config file already exist?
  memset( name, 0, M2M_FS_MAX_FILENAME_LEN );
  if( m2m_fs_find_first( name, filename ) == M2M_API_RESULT_FAIL )
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Verify: Config file does not exist" );
    return( RSP_CFG_FILE_NOT_PRESENT );
  }
  
  m2m_os_lock_wait( config_file_lock, 10000 ); // place a lock
  
  Fhandle = m2m_fs_open( filename, M2M_FS_OPEN_READ );
  if( Fhandle != NULL )
  {  
    // check file size
    filesize = m2m_fs_get_size_with_handle( Fhandle );
    if( filesize < MIN_CFG_FILESIZE || 
        filesize > (CFG_FILE_ITEM_MAX * CFG_FILE_ITEM_LEN)) rsp = RSP_CFG_FILE_FORMAT_ERROR;

    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Verify: filesize = %d", filesize );
    
    // Add up # of commas
    buf = m2m_os_mem_alloc( filesize );
    if( buf != 0 )
    {
      memset( buf, 0, sizeof( buf ) );
      m2m_fs_read( Fhandle, buf, filesize );
      
      for( i = 0; i < filesize; i++ ) if( buf[i] == COMMA ) comma++;
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Verify: Number of Commas: %d", comma );
      if( comma < CFG_FILE_ITEM_MAX ) rsp = RSP_CFG_FILE_FORMAT_ERROR;
    }
    m2m_os_mem_free( buf );
    m2m_fs_close( Fhandle ); // Close the file
  }
  else rsp = RSP_FS_CANNOT_OPEN_FILE;

  m2m_os_lock_unlock( config_file_lock );
  return( rsp );
}


/////////////////////////////////////////////////////////////////////////////
// Function: compare_config_file()
//
// Parameters: void
//
// Return: bool -> true if both config files are exactly the same
//
// Description: This function compares the config file to the config file
// backup, and return true if they are identical.
// 
///////////////////////////////////////////////////////////////////////////// 
bool compare_config_files( void )
{
  int  i, filesize1, filesize2;
  char *buf1, *buf2;
  char name[M2M_FS_MAX_FILENAME_LEN];
  bool rsp = true;
  
  memset( name, 0, M2M_FS_MAX_FILENAME_LEN );
  if( m2m_fs_find_first( name, configfilename ) == M2M_API_RESULT_FAIL )
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Compare: Config file does not exist" );
    return( false );
  }

  memset( name, 0, M2M_FS_MAX_FILENAME_LEN );
  if( m2m_fs_find_first( name, configfilenameBK ) == M2M_API_RESULT_FAIL )
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Compare: Config file backup does not exist" );
    return( false );
  }

  m2m_os_lock_wait( config_file_lock, 10000 ); // place a lock  
  
  Fhandle = m2m_fs_open( configfilename, M2M_FS_OPEN_READ );
  if( Fhandle != NULL )
  {  
    filesize1 = m2m_fs_get_size_with_handle( Fhandle );
    buf1 = m2m_os_mem_alloc( filesize1 );
    if( buf1 != 0 )
    {
      memset( buf1, 0, sizeof( buf1 ) );
      m2m_fs_read( Fhandle, buf1, filesize1 );
      m2m_fs_close( Fhandle );
    }
    else rsp = false;
  }
  else rsp = false;
  
  
  Fhandle = m2m_fs_open( configfilenameBK, M2M_FS_OPEN_READ );
  if( Fhandle != NULL )
  {  
    filesize2 = m2m_fs_get_size_with_handle( Fhandle );
    buf2 = m2m_os_mem_alloc( filesize2 );
    if( buf2 != 0 )
    {
      memset( buf2, 0, sizeof( buf2 ) );
      m2m_fs_read( Fhandle, buf2, filesize2 );
      m2m_fs_close( Fhandle );
    }
    else rsp = false;
  }
  else rsp = false;
  
  m2m_os_lock_unlock( config_file_lock );  

  // Check file sizes:
  if( filesize1 != filesize2 ) rsp = false;

  // Check each byte:
  if( rsp == true )
  {
    for( i = 0; i < filesize1; i++ )
    {
      if( buf1[i] != buf2[i]  )
      {
        rsp = false;
        break;
      }
    }
  }

  if( buf1 ) m2m_os_mem_free( buf1 );
  if( buf2 ) m2m_os_mem_free( buf2 );
  
  return( rsp );
}

