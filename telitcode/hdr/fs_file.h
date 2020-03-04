/*************************************************************************
                    5BARz International Incorporated.

                 (C) Copyright 2017. All Rights Reserved.

Filename: fs_file.h

*************************************************************************/
#ifndef HDR_FS_FILE_H_
#define HDR_FS_FILE_H_

#ifdef __cplusplus
 extern "C" {
#endif


typedef enum {
  CFG_FILE_CARRIER_NAME         = 0,  // line 1
  CFG_FILE_ITEM_RMS_ACTIVATE    = 1,  // line 2
  CFG_FILE_ITEM_CONN_METHOD     = 2,  // line 3
  CFG_FILE_ITEM_APN             = 3,  // line 4
  CFG_FILE_ITEM_SMS_SHORT_CODE  = 4,  // line 5
  CFG_FILE_ITEM_MQTT_PASSWORD   = 5,  // line 6
  CFG_FILE_ITEM_MQTT_URL        = 6,  // line 7
  CFG_FILE_ITEM_MQTT_HEARTBEAT  = 7,  // line 8
  CFG_FILE_ITEM_PERF_INTERVAL   = 8,  // line 9
  CFG_FILE_ITEM_LOC_INTERVAL    = 9,  // line 10
  CFG_FILE_ITEM_TCA_VERSION     = 10, // line 11
  CFG_FILE_ITEM_FTP_STATE       = 11, // line 12
  CFG_FILE_ITEM_LOG_FTP_URL     = 12, // line 13
  CFG_FILE_ITEM_LOG_PATH        = 13, // line 14
  CFG_FILE_ITEM_LOG_FTP_USERID  = 14, // line 15
  CFG_FILE_ITEM_LOG_FTP_PASSWD  = 15, // line 16
  CFG_FILE_ITEM_LOG_START       = 16, // line 17
  CFG_FILE_ITEM_LOG_STOP        = 17, // line 18
  CFG_FILE_ITEM_SW_FTP_URL      = 18, // line 19
  CFG_FILE_ITEM_SW_PATH         = 19, // line 20
  CFG_FILE_ITEM_SW_FTP_USERID   = 20, // line 21
  CFG_FILE_ITEM_SW_FTP_PASSWD   = 21, // line 22
  CFG_FILE_ITEM_UNUSED_1        = 22,
  CFG_FILE_ITEM_UNUSED_2        = 23,
  CFG_FILE_ITEM_UNUSED_3        = 24,
  CFG_FILE_ITEM_UNUSED_4        = 25,
  CFG_FILE_ITEM_UNUSED_5        = 26,
  CFG_FILE_ITEM_UNUSED_6        = 27,
  CFG_FILE_ITEM_UNUSED_7        = 28,
  CFG_FILE_ITEM_UNUSED_8        = 29,
  CFG_FILE_ITEM_UNUSED_9        = 30,
  CFG_FILE_ITEM_UNUSED_10       = 31,
  CFG_FILE_ITEM_MAX             = 32,
} cfg_file_item_t;


typedef enum {
  STAMP_OPEN,
  STAMP_RESUME,
  STAMP_FINISH,
  STAMP_PWR_FAIL
} stamp_t;


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
response_t read_config_file_item( cfg_file_item_t item, char *data, char *comment );


/////////////////////////////////////////////////////////////////////////////
// Function: read_config_file()
//
// Parameters: none
//
// Return: response_t -> response enum type
//
// Description: This function the opens and reads the 5bconfigfile.txt
// in the :\MOD directory and initializes all the applicable global variables.
// 
///////////////////////////////////////////////////////////////////////////// 
response_t read_config_file( void );


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
response_t update_config_file( cfg_file_item_t item, char *str );


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
response_t OpenLogFile( void );


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
void NewLogEntry( UINT8 *buf, UINT8 len );


/////////////////////////////////////////////////////////////////////////////
// Function: WriteLogFile()
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
void CloseLogFile( bool flush );


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
response_t DeleteLogFile( void );


/////////////////////////////////////////////////////////////////////////////
// Function: append_time_stamp_logfile()
//
// Parameters: bool -> true if a power failure has occurred
//
// Return: none
//
// Description: This function the appends a time/date stamp to the log
// file.  This happens at the beginning, end, or resumption of a log
// after a power failure.
// 
///////////////////////////////////////////////////////////////////////////// 
void append_time_stamp_logfile( stamp_t event );


/////////////////////////////////////////////////////////////////////////////
// Function: FlushLogFile()
//
// Parameters: none
//
// Return: none
//
// Description: This function will flush all log data in the local memory
// buffer and append it to the LogFile.
// 
///////////////////////////////////////////////////////////////////////////// 
void FlushLogFile( void );


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
response_t LogFileExist( void );


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
UINT32 LogFileSize( void );


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
void init_config_file_lock( void );


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
void config_file_maintenance( void );


#ifdef __cplusplus
}
#endif


#endif /* HDR_FS_FILE_H_ */
