/*************************************************************************
                    5Barz International Incorporated. 

                 (C) Copyright 2016. All Rights Reserved.

Filename: ftp.h

Description: This header file contains interfaces to functions that
handling all the FTP/FOTA processing for the Client App.

*************************************************************************/
#ifndef _FTP_H_
#define _FTP_H_

#ifdef __cplusplus
 extern "C" {
#endif 


#define FTP_TASK_PRIORTIY  5
   
typedef enum
{
  FTP_DOWNLOAD_TCA  = 1,
  FTP_DOWNLOAD_FOTA = 2,
  FTP_UPLOAD_FILE   = 3,
} ftp_session_t;
   
   
extern M2M_T_OS_LOCK     PDPLock;


/////////////////////////////////////////////////////////////////////////////
// Function: ftp_start()
//
// Parameters: ftp_session_t -> type of ftp session
//             char buffer   -> SMS data payload containing session paramters
//             char*         -> string containing msgID for SMS unsol status
//
// Return: response_t -> response enum type
//
// Description: This function initializes an FTP session by calling
// the FTP task with instructions.  This function runs within Task 1.
// 
/////////////////////////////////////////////////////////////////////////////
response_t ftp_start( ftp_session_t session, char buffer[SMS_MT_MAX_DATA_LEN], char *msgID );


/////////////////////////////////////////////////////////////////////////////
// Function: FTP_InitProc()
//
// Parameters: INT32 type, param1, param2:  Three INTs from another
// tasks that is calling.
//
// Return: int return value 
//
// Description: This is the entry point of FTP task that handles
// all FTP/FOTA sessions.
// 
/////////////////////////////////////////////////////////////////////////////
INT32 FTP_InitProc( INT32 type, INT32 param1, INT32 param2 );


/////////////////////////////////////////////////////////////////////////////
// Function: ftp_version_write()
//
// Parameters: None
//
// Return: response_t -> response enum type
//
// Description: This function writes the version of the current 
// Telit TCA software to file.
// 
/////////////////////////////////////////////////////////////////////////////
response_t ftp_version_write( void );


/////////////////////////////////////////////////////////////////////////////
// Function: ftp_update_config_Msgid()
//
// Parameters: char - status of ftp process for a request
//             char  - msgid of the ftp request 
//
// Return: response_t -> response enum type
//
// Description: This function updates/writes the status of ftp &  Msgid to file
//
//
/////////////////////////////////////////////////////////////////////////////
response_t ftp_update_config_Msgid( CHAR status[3] , CHAR msgID[SMS_HDR_MSG_ID_LEN+1] );

#ifdef __cplusplus
}
#endif

#endif /* _FTP_H_ */

