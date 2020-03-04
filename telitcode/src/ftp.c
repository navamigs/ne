/*************************************************************************
                    5BARz International Incorporated. 

                 (C) Copyright 2016. All Rights Reserved.

Filename: ftp.c

Description: This file contains functions to handle all FTP and FOTA
sessions.

*************************************************************************/
#include <stdlib.h>
#include "cmd_pkt.h"
#include "fs_file.h"
#include "cloud_user_method.h"
#include "ftp.h"
#include "m2m_ftp.h"
#include "utilities.h"
#include "sms.h"
#include "md5.h"

#define LOCAL_ORIG_FILE     "5bclientapp.bin"      // this is the original running file on the module.
#define LOCAL_TEMP_FILE     "m2mapz_tmp.bin"       // the original file will be renamed temporarily to this.
#define TCA_FILE_PATH       "/MOD/5bclientapp.bin" // Full path for orignal file
#define DLOAD_FILE_PATH     "/MOD/paulTest.bin"    // this will downloaded into /MOD/ local folder   
#define TEMP_FILE_PATH      "/MOD/m2mapz_tmp.bin"  // Temp filepath used for switching filenames
#define TCA_PATH            "/test/"
#define MD5_BUF_SIZE        1024
#define NUM_ITEMS_TCA_MSG   5                      // Number of string items within TCA dload Message, separated by colons
#define FTP_PORT            21
#define REMOTE_PATH_LEN     150
#define MAX_LOG_FILE_SIZE   1000000
#define MAX_TCA_FILE_SIZE   1000000

M2M_CLOUD_GLOBALS globals;

// Local Functions Prototypes:
void Start_ftp_tca( char buffer[SMS_MT_MAX_DATA_LEN], INT32 msgID );
void Start_ftp_upload( INT32 msgID );
void ftp_send_response( response_t rsp, INT32 msgID );
void ftp_session_failed( response_t rsp, INT32 msgID, bool End_PDP );
void main_MD5_file( UINT32 bytes, char filename[DW_FIELD_LEN], char *md5_str );



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
response_t ftp_start( ftp_session_t session, char buffer[SMS_MT_MAX_DATA_LEN], char *msgID )
{
  char* mem = 0;
  INT32 HmsgID;
  
  if( session == FTP_DOWNLOAD_TCA )
  {
    //debug: need some sort of IPC lock here perhaps?
    mem = (char *)m2m_os_mem_alloc( SMS_MT_MAX_DATA_LEN );
    if( mem )
    {
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"Free Space ftp1: %d\n", m2m_os_get_mem_info((UINT32 *)NULL));
      memset( mem, 0, sizeof(mem));
      strncpy( mem, buffer, SMS_MT_MAX_DATA_LEN );
      
      if( strlen( msgID ) == SMS_HDR_MSG_ID_LEN )
      {  
        HmsgID = a_h_n(msgID[0]) * 0x10;
        HmsgID += a_h_n(msgID[1]);
        m2m_os_send_message_to_task( globals.module.ftp_taskID, (INT32)session, (long)mem, HmsgID );
        return( RSP_SUCCESS );
      }
      else return( RSP_PARAMETER_OUT_OF_RANGE );
    }
    else return( RSP_GENERAL_ERROR );
  }
  else if( session == FTP_UPLOAD_FILE ) // Upload OTA Log file
  {
    if( msgID == NULL )
    {
      // msgID = 0x100 triggers unsolicited with self-generated msgID
      m2m_os_send_message_to_task( globals.module.ftp_taskID, (INT32)session, 0, 0x100 );
      return( RSP_SUCCESS );
    }
    else if( strlen( msgID ) == SMS_HDR_MSG_ID_LEN )
    {  
      HmsgID = a_h_n(msgID[0]) * 0x10;
      HmsgID += a_h_n(msgID[1]);
      m2m_os_send_message_to_task( globals.module.ftp_taskID, (INT32)session, 0, HmsgID );
      return( RSP_SUCCESS );
    }
    else return( RSP_PARAMETER_OUT_OF_RANGE );
  }
  else
  {
    return( RSP_NOT_SUPPORTED );
  }
}


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
INT32 FTP_InitProc( INT32 type, INT32 param1, INT32 param2 )
{
  msg_queue_t msg;
  
  if( type == FTP_DOWNLOAD_TCA )
  {
    globals.module.ftp_in_progress = true;
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "FTP download TCA, msgID=%d", param2 );
    Start_ftp_tca( (char *)param1, param2 );
  }
  else if( type == FTP_UPLOAD_FILE )
  {
    globals.module.ftp_in_progress = true;
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "FTP upload LogFile, msgID=%d", param2 );
    Start_ftp_upload( param2 );
  }
  else
  {
    if( param1 ) m2m_os_mem_free( (void *)param1 );
    globals.module.ftp_in_progress = false;
    msg.cmd = MSQ_QUEUE_CMD_FTP_SMS;
    msg.payload1 = param2; // msgID
    msg.payload2 = RSP_NOT_SUPPORTED;
    PushToMsgQueue( msg );
  }

  return( 0 );
}


/////////////////////////////////////////////////////////////////////////////
// Function: Start_ftp_upload()
//
// Parameters: msgID  -> The original SMS msgID for return status SMS
//
// Return: none
//
// Description: This function initializes an FTP session to upload
// the OTA Log File.
// 
/////////////////////////////////////////////////////////////////////////////
void Start_ftp_upload( INT32 msgID )
{
  char  remoteFilePath[REMOTE_PATH_LEN];
  int   i;
  bool  PDP_Active;
  INT32 res;
  
  FTP_CLIENT_ERROR      rsp;
  M2M_SOCKET_BSD_SOCKET socket;
  msg_queue_t           msg;


  // Init:
  PDP_Active = false;
  memset( remoteFilePath, 0, strlen( remoteFilePath ));
  msg.cmd      = MSQ_QUEUE_CMD_FTP_UPLOAD;
  msg.payload1 = 0; // pre-load with failed session indicator
      
  // Prep the Remote path & filename:
  strcpy( remoteFilePath, globals.module.sw_ftp_path );
  i = strlen( remoteFilePath );
  
  if( i == 0 || i > (REMOTE_PATH_LEN - 29) )
  {
    ftp_session_failed( RSP_PARAMETER_OUT_OF_RANGE, msgID, PDP_Active );
    return;
  }  

  if( remoteFilePath[i-1] != FSLASH ) strcat( remoteFilePath, "/" ); // Add a "/" if needed
  read_config_file_item( CFG_FILE_ITEM_LOG_START, globals.module.log_start_time, NULL ); // update
  strcat( remoteFilePath, globals.module.imei );           //  <path>/IMEI
  strcat( remoteFilePath, "_" );                           //  <path>/IMEI_
  strcat( remoteFilePath, globals.module.log_start_time ); //  <path>/IMEI_MMDDHH
  strcat( remoteFilePath, ".txt" );                        //  <path>/IMEI_MMDDHH.txt
  
  // Fire off the PDP:
  NE_debug( "FTP:Start PDP" );
  dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_TRUE, "PDP Activation\n");
  res = m2m_cloud_pdp_context_activate( globals.module.apn, NULL, NULL ); 
  if( res != M2M_CLOUD_SUCCESS )
  {
    NE_debug( "FTP:PDP Failed" );
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
    dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_FALSE, "FTP: PDP Failed" );  
    ftp_session_failed( RSP_PDP_SESSION_FAILURE, msgID, PDP_Active );
    PushToMsgQueue( msg ); // Inform Logging Manager
    return;
  }
  else
  {
    dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_FALSE, "FTP:PDP Success" );    
  }
  
  // Establish the connection with the FTP server:
  if( M2M_FTP_SUCCESS != m2m_ftp_connection_create( &socket, globals.module.sw_ftp_ip, FTP_PORT )) 
  {
    dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_FALSE,"FTP Connection to %s:%d failed.\n", globals.module.sw_ftp_ip, FTP_PORT);
    NE_debug( "FTP:No Conn Server" ); 
    ftp_session_failed( RSP_FTP_SERVER_CONNECT_FAIL, msgID, PDP_Active );
    PushToMsgQueue( msg ); // Inform Logging Manager
    return;
  }
  else
  {  
    dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_FALSE, "FTP:Conn Srv Good" );    
  }


  // Log in to FTP Server:
  if (M2M_FTP_SUCCESS != m2m_ftp_session_create( socket, globals.module.sw_ftp_userID, globals.module.sw_ftp_passWD )) 
  {
    dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_FALSE, "FTP client: Session failed.\n");
    m2m_ftp_connection_destroy( &socket );
    ftp_session_failed( RSP_FTP_SOCKET_CONNECT_FAIL, msgID, PDP_Active );
    PushToMsgQueue( msg ); // Inform Logging Manager
    return;
  }
  else
  {  
    dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_FALSE, "FTP:Login Good" );  
  }

  if( m2m_ftp_set_ASCII_type( socket ) != M2M_FTP_SUCCESS )
    dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"FTP client: ASCII type failed" );
 
  rsp = m2m_ftp_upload_local_file( socket, logFilePathName, remoteFilePath );
  if( rsp != M2M_FTP_SUCCESS )
  {
    dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE, "FTP client: upload file failed, rsp=%d",rsp);
    ftp_session_failed( RSP_FTP_FILE_UPLOAD_FAILED, msgID, false );
  }  
  else
  {
    dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_FALSE, "FTP:file upload worked" );
    msg.payload1 = 1; // worked    
    ftp_send_response( RSP_FTP_FILE_UPLOAD_SUCCESS, msgID );
  }

  PushToMsgQueue( msg ); // Inform Logging Manager
  
  // Clean up the PDP & FTP sessions:
  if( M2M_FTP_SUCCESS == m2m_ftp_session_close(socket) ) 
    dwPrintDebug( M2M_CLOUD_LOG_LOG, M2M_FALSE, "FTP Session successfully closed.\n" );
  m2m_ftp_connection_destroy( &socket );
  if( m2m_cloud_pdp_context_deactivate() == M2M_CLOUD_SUCCESS )
  {
	  PDP_Active = false;
	  m2m_pdp_deactive();
  }
  NE_debug("FTP:PDP ended" );
}


/////////////////////////////////////////////////////////////////////////////
// Function: Start_ftp_tca()
//
// Parameters: buffer -> A string containing the FTP addr,port,username,
//                       romote_file, and filesize separated by colons.
//                       Must be a pre-allocated buffer of minimum size
//                       SMS_MT_MAX_DATA_LEN.
//             msgID  -> The original SMS msgID for return status SMS
//
// Return: none
//
// Description: This function initializes an FTP session to download
// the TCA bin file.
// 
/////////////////////////////////////////////////////////////////////////////
void Start_ftp_tca( char buffer[SMS_MT_MAX_DATA_LEN], INT32 msgID )
{
  char   remote_path[REMOTE_PATH_LEN];
  char   remote_file[50], fileSize_str[10], Version_str[20];
  char   localFilePath[128], md5_calsum[35], md5_checksum[35];
  char   *token, *array[NUM_ITEMS_TCA_MSG], ftp_cfg_data[FTP_MSGID_LEN];
  int    iFTP_port, i, pdp;
  INT32  res;
  UINT32 filesize, ret_fs, dload_fs;
  bool   PDP_Active = false;
  
  M2M_SOCKET_BSD_SOCKET socket;
  msg_queue_t           msg;
  
  // Init:
  i = 0;
  pdp = 0;
  ret_fs = 0;
  memset( remote_file,   0, strlen( remote_file ));
  memset( fileSize_str,  0, strlen( fileSize_str ));
  memset( Version_str,   0, strlen( Version_str ));
  memset( md5_checksum,  0, strlen( md5_checksum ));
  memset( md5_calsum,    0, strlen( md5_calsum ));
  memset( localFilePath, 0, strlen( localFilePath ));
  memset( remote_path,   0, strlen( remote_path ));
  
  // Parse Input string:
  token = strtok( buffer, "," );
  while( token != NULL )
  {
    if( i < NUM_ITEMS_TCA_MSG ) array[i++] = token;
    token = strtok( NULL, "," );
  }
  
  // Check for the number of items spearated by a comma: 
  if( i != NUM_ITEMS_TCA_MSG )
  {
    ftp_session_failed( RSP_SMS_DATA_PAYLOAD_ERROR, msgID, PDP_Active );
    return;  
  }  
  
  //3517320506628920D0B143F/RMS,5bclientapp.bin,244220,TCA2.5B,24E8623B311E294E582F8546FEBB04837D21
  sprintf( localFilePath, "%s", DLOAD_FILE_PATH );
  sprintf( remote_path,   "%s", array[0] );
  sprintf( remote_file,   "%s", array[1] );
  sprintf( fileSize_str,  "%s", array[2] );
  sprintf( Version_str,   "%s", array[3] );
  sprintf( md5_checksum,  "%s", array[4] );  
  
  filesize = atoi( fileSize_str );
  
  if( filesize > MAX_TCA_FILE_SIZE ) // If filesize mentioned in msg is higher don't download file.
  {
    ftp_session_failed( RSP_FTP_FILE_SIZE_UNMATCHED, msgID, PDP_Active );
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"Downloadable filesize limit exceeded\n");
    return;
  }

  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"Software version requested:%s",Version_str);
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"Current software version  :%s",software_version);

  // Compare Current SW version to requested version:
  if( !(strncmp(Version_str, software_version, strlen(software_version)))) 
  {
    dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_FALSE,"FTP new & curr ver same!");
    NE_debug("FTP new & current version same");
    ftp_session_failed( RSP_FTP_NEW_CURR_VER_SAME, msgID, PDP_Active );
    return;
  }
  
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"Remote_file:%s",remote_file);
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"Filesize:%d",filesize);
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"MD5_checksum:%s",md5_checksum);
  
  // Free the memory buffer:
  if( buffer ) m2m_os_mem_free( (void *)buffer );
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"Free Space ftp2: %d\n", m2m_os_get_mem_info((UINT32 *)NULL));
  
  // Must have Cell Service:
  if ( !CellService() )
  {
    dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_FALSE,"FTP: Failure, No Cell Signal\n");
    ftp_session_failed( RSP_NO_CELL_COVERAGE, msgID, PDP_Active );
    return;
  }
  
  // Check for proper power to continue:
  if( globals.module.lost_12_volts == true ||
      globals.power_state != PWR_STATE_12V_ON )
  {
    ftp_session_failed( RSP_NO_12_VOLTS, msgID, PDP_Active );
    return;  
  }

  //////////////////////////////////////////////////////////////////////  

  NE_debug( "FTP:Start PDP" );
  dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"PDP Activation\n");
  
  for( pdp = 0; pdp < 3; pdp++ )  //Temporary retrail procedure to ocnnect to Ftp
  {  
    res = m2m_cloud_pdp_context_activate( globals.module.apn, NULL, NULL ); 
    if( res != M2M_CLOUD_SUCCESS )
    {
      NE_debug( "FTP:PDP Failed" );  
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
  }
  
  if( PDP_Active == false )
  {
    NE_debug( "FTP:PDP Failed" );  
    ftp_session_failed( RSP_PDP_SESSION_FAILURE, msgID, PDP_Active );
    return;
  }
  else
  {
    NE_debug( "FTP:PDP Success" );
    ftp_send_response( RSP_PDP_SESSION_SUCCESS, msgID );
  }
    
  // Check again for proper power to continue:
  if( globals.module.lost_12_volts == true ||
      globals.power_state != PWR_STATE_12V_ON )
  {
    ftp_session_failed( RSP_NO_12_VOLTS, msgID, PDP_Active );
    return;  
  }
  
  // Establish the connection with the FTP server:
  if( M2M_FTP_SUCCESS != m2m_ftp_connection_create( &socket, globals.module.sw_ftp_ip, FTP_PORT ))
  {
    dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_FALSE,"FTP Connection to %s:%d failed.\n", globals.module.sw_ftp_ip, FTP_PORT);
    NE_debug( "FTP:No Conn Server" ); 
    ftp_session_failed( RSP_FTP_SERVER_CONNECT_FAIL, msgID, PDP_Active );
    return;
  }
  else
  {  
    NE_debug( "FTP:Conn Srv Good" );
    dwPrintDebug( M2M_CLOUD_LOG_LOG, M2M_FALSE,"FTP:Conn Srv Good.");
    ftp_send_response( RSP_FTP_SOCKET_SUCCESS, msgID );
  }
      
  // Log in to FTP Server:
  if (M2M_FTP_SUCCESS != m2m_ftp_session_create( socket, globals.module.sw_ftp_userID, globals.module.sw_ftp_passWD ))
  {
    dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_FALSE,"FTP client: Session failed.\n");
    NE_debug("FTP:Socket error" );
    m2m_ftp_connection_destroy( &socket );
    ftp_session_failed( RSP_FTP_SOCKET_CONNECT_FAIL, msgID, PDP_Active );
    return;
  }
  else
  {  
    NE_debug( "FTP:Login Good" );  
    ftp_send_response( RSP_FTP_LOGIN_SUCCESS, msgID );
  }
  
  dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Remote Path: %s", remote_path );
  i = strlen( remote_path );
  if( i == 0 || i > (REMOTE_PATH_LEN - 29) )
  {
    ftp_session_failed( RSP_PARAMETER_OUT_OF_RANGE, msgID, PDP_Active );
    return;
  }

  if( remote_path[i-1] != FSLASH ) strcat( remote_path, "/" ); // Add a "/" if needed
  strcat( remote_path, remote_file );                          //  <path>/5bclientapp.bin
  dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"FTP Session successfully established.");
  dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Local file_path name: %s", localFilePath);
  dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Remote full path: %s", remote_path);
  
  // Set Binary mode:
  if (M2M_FTP_SUCCESS != m2m_ftp_set_Image_type( socket ))
  {
    NE_debug("Binary mode fail");
  }
  else
  {
    NE_debug("Binary mode success");
  }

  // Start File download:
  NE_debug("FTP:TCA dload start" );
  if( M2M_FTP_SUCCESS != m2m_ftp_download_remote_file( socket, localFilePath, 
		  remote_path, &ret_fs, filesize ))
  {
    dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"FTP client: download file failed.\n");
    NE_debug("FTP:file dload error" );

    // localTmpPath contains the renamed original file path
    dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Deleting corrupted downloaded app..\r\n");
    if( M2M_API_RESULT_SUCCESS != m2m_fs_delete(localFilePath) ) 
      dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"cannot delete %s! error code: %d", localFilePath, m2m_fs_last_error());
    else
       dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"file delete done.\r\n");

    m2m_ftp_connection_destroy( &socket );
    ftp_session_failed( RSP_FTP_FILE_DOWNLOAD_FAIL, msgID, PDP_Active );
    return;
  }
  else
  {
    NE_debug("FTP:TCA dload good" );
    dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"FTP:TCA dload good.");
    ftp_send_response( RSP_FTP_FILE_DLOAD_SUCCESS, msgID );  
  }
  
  // Clean up the PDP & FTP sessions:
  if( M2M_FTP_SUCCESS == m2m_ftp_session_close(socket) ) 
    dwPrintDebug( M2M_CLOUD_LOG_LOG, M2M_FALSE, "FTP Session successfully closed.\n" );
  m2m_ftp_connection_destroy( &socket );
  if( m2m_cloud_pdp_context_deactivate() == M2M_CLOUD_SUCCESS ) PDP_Active = false;
  NE_debug("FTP:PDP ended" );
  
  // Check for proper power to continue again:
  if( globals.module.lost_12_volts == true )
  {
    ftp_session_failed( RSP_NO_12_VOLTS, msgID, PDP_Active );
    return;  
  }

  
  ////////////////////////////////////////////////////////
  //           Process Downloaded TCA file              //
  ////////////////////////////////////////////////////////
  dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"File size %d .\n",filesize);
  dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"File size_Check %d .,%d\n",ret_fs,filesize);
  dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"File download completed.\n");

  // Check File Size
  dload_fs = m2m_fs_get_size( localFilePath );
  dwPrintDebug( M2M_CLOUD_LOG_LOG, M2M_FALSE,"Check downloaded TCA filesize=%d\r\n", dload_fs );
  if( dload_fs != filesize ) 
  {
    NE_debug("FTP:file size bad" );
    dwPrintDebug( M2M_CLOUD_LOG_LOG, M2M_FALSE, "File size DOESN'T match!\r\n" );
    dwPrintDebug( M2M_CLOUD_LOG_LOG, M2M_FALSE, "Deleting corrupted downloaded app..\r\n" );
    if( M2M_API_RESULT_SUCCESS != m2m_fs_delete(localFilePath) ) 
      dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"cannot delete %s! error code: %d", localFilePath, m2m_fs_last_error());
    else
       dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"file delete done.\r\n");
    
    ftp_session_failed( RSP_FTP_FILE_SIZE_UNMATCHED, msgID, PDP_Active );
    return;
  }
  else
  {
    dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"File sizes match!\r\n");
    NE_debug("FTP:file sizes match");
  }
  
  // Check MD5 checksum:
  main_MD5_file( dload_fs, localFilePath, md5_calsum );
  dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE, "check_sum:%s\n", md5_calsum );

  if( strcmp( md5_calsum, md5_checksum ) )    
  {  
    NE_debug("FTP:file MD5 bad" );
    dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"MD5 Checksum DOESN'T match!\r\n");
    if( M2M_API_RESULT_SUCCESS != m2m_fs_delete(localFilePath) ) 
      dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"cannot delete %s! error code: %d", localFilePath, m2m_fs_last_error());
    else
       dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"file delete done.\r\n");
    
    ftp_session_failed( RSP_FTP_FILE_MD5_UNMATCHED, msgID, PDP_Active );
    return;
  }
  else
  {
    dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Checksums matched\n");
    NE_debug("FTP:file MD5 good" );
  }
  
  // Swap Filenames and Exec Permissions:  
  dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Rename original app and set the run permission (failsafe)\r\n");
  if( M2M_API_RESULT_SUCCESS != m2m_fs_rename( TCA_FILE_PATH, LOCAL_TEMP_FILE ) ) 
    dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"cannot rename %s! error code: %d", TCA_FILE_PATH, m2m_fs_last_error());
  else
    dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Done.\r\n");

  dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Set renamed original app permission to run\r\n");
  m2m_fs_set_exec_permission( (CHAR *)LOCAL_TEMP_FILE );
  m2m_fs_set_run_permission( M2M_F_RUN_PERM_MODE_SET_RESET_OTHERS, (CHAR *)LOCAL_TEMP_FILE );
  
  // localFilePath contains the downloaded binary file path
  dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Renaming the downloaded file %s into %s\r\n", localFilePath, LOCAL_ORIG_FILE);
  if (M2M_API_RESULT_SUCCESS != m2m_fs_rename( DLOAD_FILE_PATH, LOCAL_ORIG_FILE )) 
  {
    dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"cannot rename %s! error code: %d", localFilePath, m2m_fs_last_error());
    NE_debug("Cannot rename the orig file!");
  }
  else
  {
    dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Done.\r\n");
    NE_debug("FTP:Rename Orig file");
  }
            
  dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Set renamed downloaded app permission to run\r\n");
  m2m_fs_set_exec_permission( (CHAR *)LOCAL_ORIG_FILE );
  m2m_fs_set_run_permission( M2M_F_RUN_PERM_MODE_SET_RESET_OTHERS, (CHAR *)LOCAL_ORIG_FILE );
            
  // localTmpPath contains the renamed original file path
  dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Deleting original (renamed) app..\r\n");
  if (M2M_API_RESULT_SUCCESS != m2m_fs_delete( TEMP_FILE_PATH )) 
  {
    dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"cannot delete %s! error code: %d", localFilePath, m2m_fs_last_error());
  }
  else
  {
    dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Deleted Temp file\r\n");
  }
  
  dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"FTP Success\r\n");
  NE_debug("FTP:file swap good" );
  ftp_send_response( RSP_FTP_REBOOT_AND_FINISH, msgID );
  
  sprintf(ftp_cfg_data,"%02X",msgID);  // Store Msgid & status  after sucessful download
  ftp_update_config_Msgid("1",ftp_cfg_data);

  // All good, re-boot the module: 
  NE_debug("FTP:Re-booting..." );
  m2m_os_sleep_ms( 10000 );  // give time for all MO SMS/MQTT to clear, config file too!
  msg.cmd = MSQ_QUEUE_CMD_REBOOT_MODULE;
  PushToMsgQueue( msg );
}


/////////////////////////////////////////////////////////////////////////////
// Function: ftp_send_response()
//
// Parameters: response_t -> response enum type
//             INT32      -> the original Message ID
//
// Return: none
//
// Description: This function sends an SMS to the RMS with FTP response
// code to indicate the state of the current FTP session
// 
/////////////////////////////////////////////////////////////////////////////
void ftp_send_response( response_t rsp, INT32 msgID )
{
  msg_queue_t msg; 
  
  msg.cmd      = MSQ_QUEUE_CMD_FTP_SMS;
  msg.payload1 = msgID;
  msg.payload2 = rsp;
  msg.payload3 = REPLY_ID_UNSOLCITED;
  PushToMsgQueue( msg );
}


/////////////////////////////////////////////////////////////////////////////
// Function: ftp_session_failed()
//
// Parameters: response_t -> response enum type
//             INT32      -> the original Message ID
//             bool       -> flag to end the PDP session
//
// Return: none
//
// Description: This function handkes a failed FTP session by sending an 
// SMS to the RMS with FTP response code and ending the PDP session if
// requested.
//
/////////////////////////////////////////////////////////////////////////////
void ftp_session_failed( response_t rsp, INT32 msgID, bool End_PDP )
{
  msg_queue_t msg; 
  
  // For Both MQTT and SMS.
  msg.cmd      = MSQ_QUEUE_CMD_FTP_SMS;
  msg.payload1 = msgID;
  msg.payload2 = rsp;
  msg.payload3 = REPLY_ID_FAILED;
  PushToMsgQueue( msg );

  // Tear down PDP if needed:
  if( End_PDP == true )
  {
    NE_debug("FTP:Tearing Down PDP");
    dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_FALSE,"FTP: Tearing Down PDP session.");
    m2m_cloud_pdp_context_deactivate();
    m2m_pdp_deactive();
  }
  globals.module.ftp_in_progress = false;
  ftp_update_config_Msgid("X","FF"); // Back to default state after sending failure msg
}


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
response_t ftp_version_write( void )
{
  char data[FIRMWARE_STR_LEN];

  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Writing TCA SW Version to Config file\n");
  memset( data, 0, sizeof( data ));
  strcpy( data, software_version );
  return( update_config_file( CFG_FILE_ITEM_TCA_VERSION, data ) );
}


/////////////////////////////////////////////////////////////////////////////
// Function: ftp_update_config_Msgid()
//
// Parameters: None
//
// Return: response_t -> response enum type
//
// Description: This function writes the status of FTP & Msgid to the
// config file
//
/////////////////////////////////////////////////////////////////////////////
response_t ftp_update_config_Msgid( CHAR status[3], CHAR msgID[SMS_HDR_MSG_ID_LEN+1] )
{
  char ftp_cfg_id[20];

  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Writing Ftp_msg_id to Config file\n");
  memset( ftp_cfg_id, 0, sizeof( ftp_cfg_id  ));
  sprintf( ftp_cfg_id, "FTP:%s:%s", status,msgID );
  NE_debug( ftp_cfg_id );
  return(update_config_file( CFG_FILE_ITEM_FTP_STATE, ftp_cfg_id ) );
}


/////////////////////////////////////////////////////////////////////////////
// Function: main_MD5_file()
//
// Parameters: UINT32 -> filesize in bytes
//             char[] -> filename to open
//             char*  -> return string of MD5 calculation
//
// Return: response_t -> response enum type
//
// Description: This function processes an MD5 checksum on a file.
// 
/////////////////////////////////////////////////////////////////////////////
void main_MD5_file( UINT32 bytes, char filename[DW_FIELD_LEN], char *md5_str )
{
  unsigned char   c[MD5_BLOCK_SIZE];
  M2M_T_FS_HANDLE Fcrc;
  MD5_CTX         mdContext;
  unsigned char   data[MD5_BUF_SIZE],buf[35];
  int             i;

  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Able to go to MD5 fun\n");
  memset(buf,0,strlen(buf));
  memset(md5_str,0,strlen(md5_str));
  Fcrc = m2m_fs_open( filename, M2M_FS_OPEN_READ );
  if( Fcrc != NULL )
  {
    md5_init(&mdContext);
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Started reading from file\n");
    while ((bytes = m2m_fs_read (Fcrc, data, MD5_BUF_SIZE )) != 0)
    {
      md5_update(&mdContext, data, bytes);
    }

    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Started checking MD5\n");
    md5_final(&mdContext,c);
    for( i = 0; i < MD5_BLOCK_SIZE; i++ )
    {
      sprintf(buf,"%02X",c[i]);
      if( md5_str != NULL ) strcat(md5_str,buf);
    }
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE," Total_str:%s\n", md5_str);
    m2m_fs_close (Fcrc);
  }
  else
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"ERROR: Unable to open Config file\n");
  }
}
