/*************************************************************************
                    5BARz International Incorporated. 

                 (C) Copyright 2017. All Rights Reserved.

Filename: sms.c

Description: This file contains functions to process all SMS related
activities betweem the Client App and the Cloud.

*************************************************************************/


#include <stdio.h>
#include <string.h>

#include "m2m_type.h"
#include "m2m_os_api.h"
#include "m2m_os_lock_api.h"
#include "m2m_sms_api.h"
#include "m2m_network_api.h"
#include "m2m_cloud_api.h"
#include "ftp.h"
#include "sms.h"
#include "fs_file.h"
#include "utilities.h"
#include "sms_utils.h"
#include "log.h"
#include "paho_task.h"

// Global Variables:
M2M_CLOUD_GLOBALS  globals;

typedef unsigned short  crc;

#define CRC_NAME             "CRC-16"
#define POLYNOMIAL           0x8005
#define INITIAL_REMAINDER    0x0000
#define FINAL_XOR_VALUE      0x0000
#define CHECK_VALUE          0xBB3D
#define WIDTH                (8 * sizeof(crc))
#define TOPBIT               (1 << (WIDTH - 1))
#define REFLECT_DATA(X)      ((unsigned char) reflect((X), 8))
#define REFLECT_REMAINDER(X) ((crc) reflect((X), WIDTH))

#define RESET_DELAY			     5000 // 5 seconds
#define MAX_SMS_DATA_LEN     161


// Function Prototypes:
response_t parse_mt_sms( char *buf, int len, conn_meth_t conn_mode );
crc        crcFast( unsigned char const message[], int nBytes );
static unsigned long reflect(unsigned long data, unsigned char nBits);

// Locals:
crc  crcTable[256];  


/////////////////////////////////////////////////////////////////////////////
// Function: init_sms()
//
// Parameters: none
//
// Return: bool -> true if sucessfull
//
// Description: This function initializes all SMS functions.
// 
/////////////////////////////////////////////////////////////////////////////
bool init_sms( void )
{
  
  if( m2m_sms_enable_new_message_indication())
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "SMS NMI Init Worked\n");
  }
  else
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "SMS NMI Init Failed\n");
    return( false );
  }
  
  if( m2m_sms_set_text_mode_format()) 
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "SMS Text Format Init Worked\n");
  }
  else
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "SMS Text Format Init Failed\n");
    //return( false );
  }
  
/*
  //This code is not working in USA yet.
  if( m2m_sms_set_preferred_message_storage("SM","SM","SM") )
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "SMS Message storage Init Worked\n");
    //NE_debug("SMS Storage Worked");
  }
  else
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "SMS Message storage Init Failed\n");
    //NE_debug("SMS Storage Failed");
    //return( false );
  }  
*/
  
  return( true );
}


/////////////////////////////////////////////////////////////////////////////
// Function: delete_all_SMS()
//
// Parameters: none
//
// Return: none
//
// Description: This function is called on boot-up and queries the SIM 
// if there are any undeleted SMS caused by SPAM messages.  Any SMS
// found are deleted.
//
/////////////////////////////////////////////////////////////////////////////
void delete_all_SMS( void )
{
  INT32 i;
  M2M_T_SMS_MEM_STORAGE mem[3];
  
  if( m2m_sms_get_preferred_message_storage( mem ) )
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "SMS_memr:%s,%d,%d", mem[0].mem, mem[0].nUsed, mem[0].nTotal );
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "SMS_memw:%s,%d,%d", mem[1].mem, mem[1].nUsed, mem[1].nTotal );
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "SMS_mems:%s,%d,%d", mem[2].mem, mem[2].nUsed, mem[2].nTotal );
    
    if( mem[0].nUsed > 0 )
    {
      for( i = 1; i <= mem[0].nUsed; i++ ) delete_sms( i );
    }
  }
  else
  {  
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "SMS: Failed to get SMS MEM storage");
    delete_sms( 1 ); // At a minimum, delete off any potential SMS at index 1, existing or not.
  }
}


/////////////////////////////////////////////////////////////////////////////
// Function: mt_sms_callback()
//
// Parameters: char* mem,  UINT32 message_index
//
// Return: none
//
// Description: This function is called from the M2M_onMsgIndEvent() callback
// when a new MT SMS arrvies.   Because it runs in a lower level modem task,
// do not perfrom any major function other placing a message on the Task_1
// message queue.
//
/////////////////////////////////////////////////////////////////////////////
void mt_sms_callback( char *mem, UINT32 index )
{
  msg_queue_t     msg;

  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"MT_SMS index:%d", index);
  NE_debug_var("SMS MT Index=", (int)index );
  
  msg.cmd = MSG_QUEUE_CMD_MT_SMS;
  msg.payload1 = (INT32)index;
  PushToMsgQueue( msg );
}


/////////////////////////////////////////////////////////////////////////////
// Function: read_mt_sms()
//
// Parameters: INT32 message_index
//
// Return: none
//
// Description: This function reads a MT SMS out of memory and extracts
// the message portion, calls a parsing function, then deletes the SMS
// from memory.
//
/////////////////////////////////////////////////////////////////////////////
void read_mt_sms( INT32 index )
{
  M2M_T_SMS_INFO        pSMS;
  M2M_T_SMS_MEM_STORAGE sms_mem;
  M2M_T_SMS_DATA_INFO   *data_info;
  CODING_SCHEME         scheme;
  static char           ascii_msg[MAX_SMS_DATA_LEN];

  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"MT_SMS index:%d", index);
  
  if( m2m_sms_get_text_message( (INT32)index, &pSMS)) 
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "SMS: Success MT SMS Index:%d",index);
  else 
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "SMS: Failed to get MT SMS Index:%d",index);

  // New Telit Translation code for Special Characters:  
  data_info = (M2M_T_SMS_DATA_INFO*) &(pSMS.data);
  memset( ascii_msg, 0, sizeof( ascii_msg ));
  FromRAWToASCII( ascii_msg, data_info->data, data_info->udl );
  
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "SMS:%s",ascii_msg );
  parse_mt_sms( ascii_msg, strlen(ascii_msg), CONN_METHOD_SMS_ONLY );
  delete_sms( index );
}


/////////////////////////////////////////////////////////////////////////////
// Function: parse_mt_sms()
//
// Parameters: char *buf -> string ptr to the message to parse
//             int len   -> Length of the message sting
//             conn_mode -> SMS or MQTT parsing and response
//
// Return: response_t -> response enum type
//
// Description: This function parses an incoming MT SMS or MQTT message
// for the proper message format, and executes the proper action.
// 
/////////////////////////////////////////////////////////////////////////////
response_t parse_mt_sms( char *buf, int len, conn_meth_t conn_mode )
{
  char        *puf; // local pointer
  char        imei[SMS_HDR_IMEI_LEN+1];
  char        cmd[SMS_HDR_CMD_LEN+1];
  char        msgID[SMS_HDR_MSG_ID_LEN+1];
  char        d_len[SMS_HDR_DATA_LEN_LEN+1];
  char        data[SMS_MT_MAX_DATA_LEN+1];
  char        rdata[SMS_MO_MAX_DATA_LEN+1];
  char        crc_str[SMS_CRC_LEN+1];
  char        Alarm[25], msg_str[8];  
  char        comment[100];
  char        *token, *array[4];
  crc         crc_cal, crc_msg;
  int         num, num2, i;
  UINT8       dLen;
  UINT16      command, data16, data2_16;
  response_t  mo_rsp, rsp = RSP_SUCCESS;
  replyID_t   replyID;
  msg_queue_t msg;
  
  // Init:
  puf = buf;
  crc_cal = crcFast( buf, (len-SMS_CRC_LEN) );
  memset( imei,    0, sizeof(imei));
  memset( cmd,     0, sizeof(cmd));
  memset( msgID,   0, sizeof(msgID));
  memset( d_len,   0, sizeof(d_len));
  memset( data,    0, sizeof(data));
  memset( rdata,   0, sizeof(rdata));
  memset( crc_str, 0, sizeof(crc_msg));
  replyID = REPLY_ID_NONE;
  
  // Check Length:
  if( len < SMS_MT_MIN_LEN || len > SMS_MAX_LENGTH ) 
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Invalid SMS Length=%d", len );
    NE_debug("Rx Spam SMS");
    return( RSP_SMS_FORMAT_ERROR ); // Throw away spam SMS
  }
  
  // Parse the Header:
  strncpy( imei, puf, SMS_HDR_IMEI_LEN );
  puf += SMS_HDR_IMEI_LEN;
  strncpy( cmd, puf, SMS_HDR_CMD_LEN );
  puf += SMS_HDR_CMD_LEN;
  strncpy( msgID, puf, SMS_HDR_MSG_ID_LEN );
  puf += SMS_HDR_MSG_ID_LEN;
  strncpy( d_len, puf, SMS_HDR_DATA_LEN_LEN );
  puf += SMS_HDR_DATA_LEN_LEN;

  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"SMS Length=%d", len );
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"IMEI:%s", imei );
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"CMD:%s", cmd );
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"msgID:%s", msgID );
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"dLength:%s", d_len );
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"CRC=%X", crc_cal );
  
  // Compare the IMEI, if there is no match, we stop all processing
  // and drop the message assuming it is a spam SMS:
  if( strncmp( imei, globals.module.imei, SMS_HDR_IMEI_LEN ) != 0 ) 
  {
    NE_debug("Rx Spam SMS");
    return( RSP_SMS_WRONG_IMEI ); // Throw away spam SMS
  }
   
  // Parse Data Length and compare:
  if( !isHex(d_len[0]) || !isHex(d_len[1])) rsp = RSP_SMS_FORMAT_ERROR;
    dLen = a_h_n(d_len[0]) * 16 + a_h_n(d_len[1]);
  if( dLen > SMS_MT_MAX_DATA_LEN || dLen != (len - SMS_MT_HDR_LEN - SMS_CRC_LEN) ) 
    rsp = RSP_SMS_FORMAT_ERROR;

  if( dLen > 0 ) // Capture data, if any
  {
    strncpy( data, puf, dLen );
    puf += dLen;
  }
    
  // Process CRC
  strncpy( crc_str, puf, SMS_CRC_LEN );
  if( isHex( crc_str[0] )) crc_msg = a_h_n(crc_str[0]) * 0x1000;
    else rsp = RSP_SMS_FORMAT_ERROR;
  if( isHex( crc_str[1] )) crc_msg += a_h_n(crc_str[1]) * 0x100;
    else rsp = RSP_SMS_FORMAT_ERROR;
  if( isHex( crc_str[2] )) crc_msg += a_h_n(crc_str[2]) * 0x10;
    else rsp = RSP_SMS_FORMAT_ERROR;
  if( isHex( crc_str[3] )) crc_msg += a_h_n(crc_str[3]);
    else rsp = RSP_SMS_FORMAT_ERROR;
  crc_cal = crcFast( buf, (len-SMS_CRC_LEN) );
  if( crc_msg != crc_cal ) rsp = RSP_SMS_CRC_FAIL; //debug: remove for testing if needed.
    
  if( isHex( cmd[0] )) command = a_h_n(cmd[0]) * 0x1000;
    else rsp = RSP_SMS_FORMAT_ERROR;    
  if( isHex( cmd[1] )) command += a_h_n(cmd[1]) * 0x100;
    else rsp = RSP_SMS_FORMAT_ERROR;
  if( isHex( cmd[2] )) command += a_h_n(cmd[2]) * 0x10;
    else rsp = RSP_SMS_FORMAT_ERROR;
  if( isHex( cmd[3] )) command += a_h_n(cmd[3]);
    else rsp = RSP_SMS_FORMAT_ERROR;

  if( rsp != RSP_SUCCESS ) replyID = REPLY_ID_FAILED;
  
  if( rsp == RSP_SUCCESS )
  {
    memset( comment, 0, sizeof( comment ));
    if( command >= 0x1000 ) sprintf( comment, "MT SMS cmd:%X\0", command );
      else sprintf( comment, "MT SMS cmd:0%X\0", command );
    NE_debug( comment );
    
    switch( command )
    {
      /////////////////////////////////////////////////////////
      //                C o m m a n d s                      //
      /////////////////////////////////////////////////////////
      case CD_DISABLE_NE: // 0x0C00 - Disable NE
        rsp = Cloud_Disable_NE();
        if( rsp == RSP_SUCCESS ) replyID = REPLY_ID_SUCCESS;
          else replyID = REPLY_ID_FAILED;
        break;

      case CD_ENABLE_NE: // 0x0C01 - Enable NE
        rsp = Cloud_Enable_NE();
        if( rsp == RSP_SUCCESS ) replyID = REPLY_ID_SUCCESS;
          else replyID = REPLY_ID_FAILED;
        break;
        
      case CD_START_MQTT:  // 0x0C06 - MQTT_start
        rsp = MQTT_start();
        if( rsp == RSP_SUCCESS ) replyID = REPLY_ID_SUCCESS;
          else replyID = REPLY_ID_FAILED;
        break;
        
      case CD_STOP_MQTT:  // 0x0C07 - Connection method
        rsp = MQTT_stop();
        if( rsp == RSP_SUCCESS ) replyID = REPLY_ID_SUCCESS;
          else replyID = REPLY_ID_FAILED;
        break;
        
      case CD_ACTIVATE_NE: // 0x0C08 - Activate NE
        rsp = Activate_NE();
        if( rsp == RSP_SUCCESS ) replyID = REPLY_ID_SUCCESS;
          else replyID = REPLY_ID_FAILED;
        break;
        
      case CD_RESET_NE: // 0x0C09 - Reset NE
        rsp = Reset_NE();
        if( rsp == RSP_SUCCESS ) replyID = REPLY_ID_SUCCESS;
          else replyID = REPLY_ID_FAILED;
        break;
        
      case CD_DELETE_LOG_FILE: // 0x0C0A - Delete Log File
        if( !LoggingNow() )
        {
          rsp = LogFileExist();
          if( rsp == RSP_SUCCESS )
          {
            rsp = DeleteLogFile();
            if( rsp == RSP_SUCCESS ) 
            {
              replyID = REPLY_ID_SUCCESS;
              break;
            }  
          }
        }
        else rsp = RSP_LOGGING_IN_PROCESS;
        replyID = REPLY_ID_FAILED;
        break;
        
      case CD_FTP_LOG_FILE: // 0x0C0B - Start new FTP upload session for Log file
        if( !LoggingNow() )
        {
          rsp = LogFileExist();
          if( rsp == RSP_SUCCESS )
          {
            Logging_Manager( LOG_EVENT_START_FTP );
            replyID = REPLY_ID_SUCCESS;
            break;
          }
        }
        else rsp = RSP_LOGGING_IN_PROCESS;
        replyID = REPLY_ID_FAILED;
        break;
        
      case CD_STOP_LOGGING: // 0x0C0C - Stop Logging now.
        if( LoggingNow() )
        {
          Logging_Manager( LOG_EVENT_STOP_LOGGING );
          replyID = REPLY_ID_SUCCESS;
          break;
        }
        else rsp = RSP_WRONG_STATE;
        replyID = REPLY_ID_FAILED;
        break;
      
      case CD_DEACTIVATE_NE:  // 0x0C0D - Deactivate NE
        if( !strcmp( globals.module.RMS_Activate, "Y" ) ||
            !strcmp( globals.module.RMS_Activate, "y" ) )  // Yes, RMS Activated
        {
          rsp = update_config_file( CFG_FILE_ITEM_RMS_ACTIVATE, "N\0" );
          if( rsp == RSP_SUCCESS )
          {
            replyID = REPLY_ID_SUCCESS;
            m2m_timer_stop(timer_loc_data_hdl);
            m2m_timer_stop(timer_perf_data_hdl);
            Cloud_Disable_NE();
            if( globals.module.conn_method == CONN_METHOD_MQTT_ONLY )
            {
              MQTT_stop();
            }
          }
          else replyID = REPLY_ID_FAILED;
        }
        else 
        {
          rsp = RSP_NE_ALREADY_DONE;
          replyID = REPLY_ID_FAILED;
        }   
        break;

      case CD_PING_DEV: // 0x0C0E - Command Ping
        replyID = REPLY_ID_SUCCESS;
        break;
        
      case CD_ENABLE_A_BAND: // 0x0C0F
        rsp = Cloud_Enable_NE_A_only();
        if( rsp == RSP_SUCCESS ) replyID = REPLY_ID_SUCCESS;
          else replyID = REPLY_ID_FAILED;
        break;
        
      case CD_ENABLE_B_BAND: // 0x0C10
        rsp = Cloud_Enable_NE_B_only();
        if( rsp == RSP_SUCCESS ) replyID = REPLY_ID_SUCCESS;
          else replyID = REPLY_ID_FAILED;
        break;
        
        
      /////////////////////////////////////////////////////////
      //                   W r i t e s                       //
      /////////////////////////////////////////////////////////
      case WR_RESET_MODULE: // 0x0D00 - Reset Module
        if( dLen > 0 ) num = atoi(data);
          else num = 0;
        rsp = Reset_Module( num ); 
        if( rsp == RSP_SUCCESS ) replyID = REPLY_ID_SUCCESS;
          else replyID = REPLY_ID_FAILED;
        break;
        
      case WR_UL_CHANNEL_A: // 0x0D02 - Write Uplink Channel, Band A
        if( dLen == 4 ) rsp = Set_UL_Channel_A( data );
          else rsp = RSP_SMS_DATA_PAYLOAD_ERROR;
        if( rsp == RSP_SUCCESS ) replyID = REPLY_ID_SUCCESS;
          else replyID = REPLY_ID_FAILED;
        break;
        
      case WR_PERF_INTERVAL: // 0x0D03 - Write Performance Interval     
        if( dLen > 0 && dLen < PERF_INTVL_LEN )
        {
          num = atoi( data );
          if( num < 0 || num > 0xFFFF )
          {
            rsp = RSP_PARAMETER_OUT_OF_RANGE;
            replyID = REPLY_ID_FAILED;
          }
          else
          {
            rsp = update_config_file( CFG_FILE_ITEM_PERF_INTERVAL, data );
            if( rsp == RSP_SUCCESS )
            {
              // Added to stop and start timer based on new timer value
              m2m_timer_stop( timer_perf_data_hdl );
              dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Performance_Interval: %d", num);
              if( num > 0 )
              {
                m2m_timer_start(timer_perf_data_hdl, (num*ONE_MINUTE) );
              }
              replyID = REPLY_ID_SUCCESS;
            }
            else replyID = REPLY_ID_FAILED;
          }
        }
        else
        {
          rsp = RSP_PARAMETER_OUT_OF_RANGE;
          replyID = REPLY_ID_FAILED;
        }                
        break;
        
      case WR_SMS_SHORT_CODE: // 0x0D04 - Write SMS Short Code     
        if( dLen > 0 && dLen < SMS_ADDR_LEN )
        {
          rsp = update_config_file( CFG_FILE_ITEM_SMS_SHORT_CODE, data );
          if( rsp == RSP_SUCCESS )
          {
            msg.cmd = MSQ_QUEUE_CMD_REBOOT_MODULE;  // Changed inorder to get update to previous server  
            PushToMsgQueue( msg );                  // also.(Updates,Sends success msg then reset)
            replyID = REPLY_ID_SUCCESS;
          }
          else replyID = REPLY_ID_FAILED;
        }
        else
        {
          rsp = RSP_PARAMETER_OUT_OF_RANGE;
          replyID = REPLY_ID_FAILED;
        }                
        break;
        
      case WR_APN: // 0x0D06 - Write APN
        if( dLen > 0 && dLen < APN_STR_LEN )
        {
          rsp = update_config_file( CFG_FILE_ITEM_APN, data );
          if( rsp == RSP_SUCCESS ) replyID = REPLY_ID_SUCCESS;
            else replyID = REPLY_ID_FAILED;
        }
        else
        {
          rsp = RSP_PARAMETER_OUT_OF_RANGE;
          replyID = REPLY_ID_FAILED;
        }                
        break;
        
      case WR_MQTT_URL: // 0x0D07 - Write MQTT URL
        if( dLen > 0 && dLen < URL_STR_LEN )
        {
          rsp = update_config_file( CFG_FILE_ITEM_MQTT_URL, data );
          if( rsp == RSP_SUCCESS ) replyID = REPLY_ID_SUCCESS;
          else replyID = REPLY_ID_FAILED;
        }
        else
        {
          rsp = RSP_PARAMETER_OUT_OF_RANGE;
          replyID = REPLY_ID_FAILED;
        }                
        break;        
               
      case WR_FTP_TCA_FILE: // 0x0D0B - FTP download of the TCA file
        if( globals.module.ftp_in_progress == false && // Process only if no FTP session is in progress
            globals.module.lost_12_volts == false )    // Cannot be on back-up power, must have 12v
        {
          rsp = ftp_start( FTP_DOWNLOAD_TCA, data, msgID );
          if( rsp == RSP_SUCCESS ) 
          {
            ftp_update_config_Msgid( "0", msgID ); //Write msgID to config file
            replyID = REPLY_ID_SUCCESS;
          }
          else replyID = REPLY_ID_FAILED;
        }
        else //send an error code to RMS if already another FTP session in progress
        {
          if( globals.module.ftp_in_progress == true ) rsp = RSP_FTP_DOWNLOAD_IN_PROGRESS;
          if( globals.module.lost_12_volts == true )   rsp = RSP_NO_12_VOLTS;
          replyID = REPLY_ID_FAILED;
        }
        break;
        
      case WR_CFG_FILE_ITEM: // 0x0D0C - Write Configfile Item
        if( dLen > 0 )
        {  
          num = atoi( strtok( data, "," ) );
          if( num <= 0 || num > CFG_FILE_ITEM_MAX  )
          {
            rsp = RSP_PARAMETER_OUT_OF_RANGE;
            replyID = REPLY_ID_FAILED;
          }
          else
          {
            num--; // subtract 1 so line #1 translates to 0 for internal enum
            memset( comment, 0, sizeof( comment ));
            strcpy( comment, strtok( NULL, "\0" ));
            rsp = update_config_file( (cfg_file_item_t)num, comment );
            if( rsp == RSP_SUCCESS ) replyID = REPLY_ID_SUCCESS;
              else replyID = REPLY_ID_FAILED;
          }
        }
        else
        {
          rsp = RSP_PARAMETER_OUT_OF_RANGE;
          replyID = REPLY_ID_FAILED;
        }        
        break;
        
      case WR_LOC_INTERVAL: // 0x0D0D - Write Location Interval
        if( dLen > 0 && dLen < PERF_INTVL_LEN )
        {
          num = atoi( data );
          if( num < 0 || num > 0xFFFF )
          {
            rsp = RSP_PARAMETER_OUT_OF_RANGE;
            replyID = REPLY_ID_FAILED;
          }
          else
          {
            rsp = update_config_file( CFG_FILE_ITEM_LOC_INTERVAL, data );
            if( rsp == RSP_SUCCESS )
            {
              m2m_timer_stop(  timer_loc_data_hdl );
              dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "LOCATION_Interval: %d", num);
              if( num > 0 )
              {
                m2m_timer_start( timer_loc_data_hdl, (num*ONE_MINUTE) );
              }
              replyID = REPLY_ID_SUCCESS;
            }
            else replyID = REPLY_ID_FAILED;
          }
        }
        else
        {
          rsp = RSP_PARAMETER_OUT_OF_RANGE;
          replyID = REPLY_ID_FAILED;
        }
        break;
        
      case WR_NE_CAL_ITEM: // 0x0D0E - Write NE Cal item
        if( dLen > 0 )
        {  
          num = atoi( strtok( data, "," ) );
          if( num <= 0 || num > ATLAS_MAX_CAL_ITEM  )
          {
            rsp = RSP_PARAMETER_OUT_OF_RANGE;
            replyID = REPLY_ID_FAILED;
          }
          else
          {
            memset( comment, 0, sizeof( comment ));
            strcpy( comment, strtok( NULL, "\0" ));
            num2 = atoi( comment );
            if( num2 < 0 || num2 > 0xFFFF )
            {
              rsp = RSP_PARAMETER_OUT_OF_RANGE;
              replyID = REPLY_ID_FAILED;
              break;
            }
            rsp = Write_NE_Cal_item( (UINT16)num, (UINT16)num2 );
            if( rsp == RSP_SUCCESS ) replyID = REPLY_ID_SUCCESS;
              else replyID = REPLY_ID_FAILED;
          }
        }
        else
        {
          rsp = RSP_PARAMETER_OUT_OF_RANGE;
          replyID = REPLY_ID_FAILED;
        }        
        break;
       
      case WR_LOG_START_STOP: // 0x0D10 - Write Log Start & Stop times
        rsp = Log_new_start_stop( data );
        if( rsp == RSP_SUCCESS ) replyID = REPLY_ID_SUCCESS;
          else replyID = REPLY_ID_FAILED;
        break;
        
      case WR_UL_CHANNEL_B: // 0x0D11 - Write Uplink Channel Band B
        if( dLen == 4 ) rsp = Set_UL_Channel_B( data );
          else rsp = RSP_SMS_DATA_PAYLOAD_ERROR;
        if( rsp == RSP_SUCCESS ) replyID = REPLY_ID_SUCCESS;
          else replyID = REPLY_ID_FAILED;
        break;
       
      case  WR_FTP_CONN_INFO: // 0x0D12 - Update FTP SW server detail. url,username,password,download_path
        token = strtok( data, "," );
        while( token != NULL )
        {
          if( i < 4 ) array[i++] = token;
          token = strtok( NULL, "," );
        }
        if( i != 4 )
        {
          rsp = RSP_SMS_DATA_PAYLOAD_ERROR;
          replyID = REPLY_ID_FAILED;
        }
        else
        {
          rsp = update_config_file( CFG_FILE_ITEM_SW_FTP_URL, array[0] );
          if( rsp == RSP_SUCCESS )
          {
            replyID = REPLY_ID_SUCCESS;
            rsp = update_config_file( CFG_FILE_ITEM_SW_FTP_USERID, array[1] );
            if( rsp == RSP_SUCCESS )
            {
              replyID = REPLY_ID_SUCCESS;
              rsp = update_config_file( CFG_FILE_ITEM_SW_FTP_PASSWD, array[2] );
              if( rsp == RSP_SUCCESS )
              {
                replyID = REPLY_ID_SUCCESS;
                rsp = update_config_file( CFG_FILE_ITEM_SW_PATH, array[3] );
                if( rsp == RSP_SUCCESS ) replyID = REPLY_ID_SUCCESS;
                  else replyID = REPLY_ID_FAILED;
              }
              else replyID = REPLY_ID_FAILED;
            }
            else replyID = REPLY_ID_FAILED;
          }
          else replyID = REPLY_ID_FAILED;
        }
        break;
        
        
      /////////////////////////////////////////////////////////
      //                   R e a d s                         //
      /////////////////////////////////////////////////////////
      case RD_STATUS: // 0x0B00 - Read NE Status 
        rsp = Read_NE_Cloud_Enabled( NULL, rdata );
        if( rsp == RSP_SUCCESS ) replyID = REPLY_ID_SUCCESS; 
          else replyID = REPLY_ID_FAILED;
        break;

      case RD_UL_DL_CHANNEL_A: // 0x0B01 - Read UL,DL Channel Numbers, Band A
        rsp = Read_UL_Channel_A( &data16, NULL, NULL );
        if( rsp == RSP_SUCCESS ) 
        {
          rsp = Read_DL_Channel_A( &data2_16, NULL, NULL );
          if( rsp == RSP_SUCCESS ) 
          {
            sprintf( rdata, "%d,%d", data16, data2_16 );
            replyID = REPLY_ID_SUCCESS; 
          }      
          else replyID = REPLY_ID_FAILED;
        }
        else replyID = REPLY_ID_FAILED;
        break;

      case RD_UL_DL_GAIN_A: // 0x0B02 - Read UL, DL Gain, Band A
        rsp = Read_UL_Gain_A( rdata );
        if( rsp == RSP_SUCCESS ) 
        {
          dLen = strlen( rdata );
          rdata[dLen++] = COMMA;
          rsp = Read_DL_Gain_A( &rdata[dLen] );
          if( rsp == RSP_SUCCESS ) 
          {
            replyID = REPLY_ID_SUCCESS; 
          }      
          else replyID = REPLY_ID_FAILED;
        }
        else replyID = REPLY_ID_FAILED;
        break;

      case RD_UL_DL_RSSI_A: // 0x0B03 - Read UL, DL RSSI, Band A
        rsp = Read_UL_RSSI_A( NULL, rdata );
        if( rsp == RSP_SUCCESS ) 
        {
          dLen = strlen( rdata );
          rdata[dLen++] = COMMA;
          rsp = Read_DL_RSSI_A( NULL, &rdata[dLen] );
          if( rsp == RSP_SUCCESS ) 
          {
            replyID = REPLY_ID_SUCCESS; 
          }      
          else replyID = REPLY_ID_FAILED;
        }
        else replyID = REPLY_ID_FAILED;
        break;

      case RD_TEMPERATURE: // 0x0B04 - Read Temperature  
        rsp = Read_Temperature( rdata );
        if( rsp == RSP_SUCCESS ) replyID = REPLY_ID_SUCCESS; 
          else replyID = REPLY_ID_FAILED;
        break;

      case RD_ALARM_STATE: // 0x0B05 - Read Alarm State
        rsp = Read_Alarms( NULL, rdata );
        if( rsp == RSP_SUCCESS ) replyID = REPLY_ID_SUCCESS; 
          else replyID = REPLY_ID_FAILED;
        break;

      case RD_ACTIVITY_DETECT_A: // 0x0B06 - Read Activity Detection, Band A
        rsp = Read_UL_Activity_State_A( NULL, rdata );
        if( rsp == RSP_SUCCESS ) replyID = REPLY_ID_SUCCESS; 
          else replyID = REPLY_ID_FAILED;
        break;
        
      case RD_LED_STATUS: // 0x0B07 - Read LED Status
        rsp = Read_LED_Status( NULL, rdata );
        if( rsp == RSP_SUCCESS ) replyID = REPLY_ID_SUCCESS; 
          else replyID = REPLY_ID_FAILED;
        break;
        
      case RD_MODULE_INFO: // 0x0B0A - Read Module Info
        // "Model Designation,FirmwareVersion,ClientSoftwareVersion,channel"
        if( globals.module.model[0] != 0 && globals.module.firmware[0] != 0 )
        {
          strcpy( rdata, globals.module.model );
          dLen = strlen( rdata );
          rdata[dLen++] = COMMA;
          strcpy( &rdata[dLen], globals.module.firmware );
          dLen = strlen( rdata );
          rdata[dLen++] = COMMA;
          strcpy( &rdata[dLen], software_version );
          rdata[dLen++] = COMMA;
          sprintf( &rdata[dLen], "%d", globals.module.GSM_channel );
          rsp = RSP_SUCCESS; 
          replyID = REPLY_ID_SUCCESS;
        }
        else
        {
          rsp = RSP_NE_READ_FAILED;
          replyID = REPLY_ID_FAILED;
        }
        break;
        
      case RD_PHONE_NUMBER: // 0x0B0D - Read Phone Number 
        if( globals.module.phone_num[0] != 0 )
        {
          strcpy( rdata, globals.module.phone_num );
          rsp = RSP_SUCCESS;
          replyID = REPLY_ID_SUCCESS;
        }
        else
        {
          rsp = RSP_NE_READ_FAILED;
          replyID = REPLY_ID_FAILED;
        }
        break;
        
      case RD_LOCATION: // 0x0B0E - Read Location
        rsp = Req_location_info();
        if( rsp == RSP_SUCCESS )
        { 
          if( globals.module.location.latitude[0] != 0 )
          {
            strcpy( rdata, globals.module.location.latitude );
            dLen = strlen( rdata );
            rdata[dLen++] = COMMA;
            strcpy( &rdata[dLen], globals.module.location.longitude );
            dLen = strlen( rdata );
            rdata[dLen++] = COMMA;
            sprintf( &rdata[dLen], "%d", globals.module.location.accuracy );
            replyID = REPLY_ID_SUCCESS; 
          }
          else 
          {  
            rsp = RSP_LOCATION_FIX_FAILURE;
            replyID = REPLY_ID_FAILED;
          } 
        }  
        else replyID = REPLY_ID_FAILED;
        break;
        
      case RD_CFG_FILE_ITEM: // 0x0B0F - Read Config File item
        if( dLen > 0 )
        {  
          num = atoi( data );
          if( num <= 0 || num > CFG_FILE_ITEM_MAX  )
          {
            rsp = RSP_PARAMETER_OUT_OF_RANGE;
            replyID = REPLY_ID_FAILED;
          }
          else
          {
            num--; // subtract 1 so line #1 translates to 0 for internal enum
            memset( comment, 0, sizeof( comment ));
            rsp = read_config_file_item( (cfg_file_item_t)num, rdata, NULL );
            if( rsp == RSP_SUCCESS )
            {
              if( (strlen( rdata ) + strlen( comment ) + 1) > SMS_MO_MAX_DATA_LEN || 
                   rdata[0] == 0 )
              {
                rsp = RSP_GENERAL_ERROR;
                replyID = REPLY_ID_FAILED;
              }
              else // everything good:
              {
                replyID = REPLY_ID_SUCCESS;
                dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"data:->%s<-", rdata);
              }
            }
            else replyID = REPLY_ID_FAILED;
          }
        }
        else
        {
          rsp = RSP_PARAMETER_OUT_OF_RANGE;
          replyID = REPLY_ID_FAILED;
        }
        break;
        
      case RD_NE_CAL_ITEM: // 0x0B10 - Read NE Cal item
        if( dLen > 0 )
        {  
          num = atoi( data );
          if( num <= 0 || num > ATLAS_MAX_CAL_ITEM  )
          {
            rsp = RSP_PARAMETER_OUT_OF_RANGE;
            replyID = REPLY_ID_FAILED;
          }
          else
          {
            rsp = Read_NE_Cal_item( (UINT16)num, NULL, rdata );
            if( rsp == RSP_SUCCESS ) replyID = REPLY_ID_SUCCESS; 
              else replyID = REPLY_ID_FAILED;
          }
        }
        break;
        
      case RD_UL_DL_TX_POWER_A: // 0x0B11 - Read UL,DL TX Power in dBm, Band A
        rsp = Read_UL_TX_Power_A( rdata );
        if( rsp == RSP_SUCCESS ) 
        {
          dLen = strlen( rdata );
          rdata[dLen++] = COMMA;
          rsp = Read_DL_TX_Power_A( &rdata[dLen] );
          if( rsp == RSP_SUCCESS ) 
          {
            replyID = REPLY_ID_SUCCESS; 
          }      
          else replyID = REPLY_ID_FAILED;
        }
        else replyID = REPLY_ID_FAILED;
        break;
        
      case RD_LOG_FILE_SIZE: // 0x0B12 - Log file size?
        rsp = LogFileExist();
        if( rsp == RSP_SUCCESS ) 
        {
          num = (int)LogFileSize();
          sprintf( rdata, "%d", num );
          replyID = REPLY_ID_SUCCESS; 
        }
        else replyID = REPLY_ID_FAILED;
        break;  
        
      case RD_LOG_START_STOP: // 0x0B13
        rsp = read_config_file_item( CFG_FILE_ITEM_LOG_START, rdata, NULL );
        dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Made it 1");
        if( rsp == RSP_SUCCESS )
        {  
          num = strlen( rdata );
          if( num <= 6 )
          {
            rdata[num++] = COMMA;
            rsp = read_config_file_item( CFG_FILE_ITEM_LOG_STOP, &rdata[num], NULL );
            dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Made it 2:%s",rdata);
            if( rsp == RSP_SUCCESS )
            {  
              dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Made it 3");
              //num += strlen( rdata );
              if( strlen( rdata ) <= 13 ) replyID = REPLY_ID_SUCCESS;
              break;
            }    
          }
          else rsp = RSP_CFG_FILE_FORMAT_ERROR;
        }
        dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Made it 4");
        replyID = REPLY_ID_FAILED;
        break;
        
      case RD_PERF_DATA: // 0x0B14 - Reads   Performance data,Led status,channel,location
        rsp = PeriodicPerformanceData( rdata );
        if( rsp == RSP_SUCCESS ) replyID = REPLY_ID_SUCCESS;
          else replyID = REPLY_ID_FAILED;
        break;
 
      case RD_UL_DL_CHANNEL_B: // 0x0B15 - Read UL,DL Channel_B Numbers
        rsp = Read_UL_Channel_B( &data16, NULL, NULL );
        if( rsp == RSP_SUCCESS ) 
        {
          rsp = Read_DL_Channel_B( &data2_16, NULL, NULL );
          if( rsp == RSP_SUCCESS ) 
          {
            sprintf( rdata, "%d,%d", data16, data2_16 );
            replyID = REPLY_ID_SUCCESS; 
          }      
          else replyID = REPLY_ID_FAILED;
        }
        else replyID = REPLY_ID_FAILED;
        break;

      case RD_UL_DL_GAIN_B: // 0x0B16 - Read UL,DL Gain, Band B
        rsp = Read_UL_Gain_B( rdata );
        if( rsp == RSP_SUCCESS ) 
        {
          dLen = strlen( rdata );
          rdata[dLen++] = COMMA;
          rsp = Read_DL_Gain_B( &rdata[dLen] );
          if( rsp == RSP_SUCCESS ) 
          {
            replyID = REPLY_ID_SUCCESS; 
          }      
          else replyID = REPLY_ID_FAILED;
        }
        else replyID = REPLY_ID_FAILED;
        break;
        
      case RD_UL_DL_RSSI_B: // 0x0B17 - Read UL,DL RSSI, Band B
        rsp = Read_UL_RSSI_B( NULL, rdata );
        if( rsp == RSP_SUCCESS ) 
        {
          dLen = strlen( rdata );
          rdata[dLen++] = COMMA;
          rsp = Read_DL_RSSI_B( NULL, &rdata[dLen] );
          if( rsp == RSP_SUCCESS ) 
          {
            replyID = REPLY_ID_SUCCESS; 
          }      
          else replyID = REPLY_ID_FAILED;
        }
        else replyID = REPLY_ID_FAILED;
        break;
        
      case RD_ACTIVITY_DETECT_B: // 0x0B18 - Read Activity Detection, Band B
        rsp = Read_UL_Activity_State_B( NULL, rdata );
        if( rsp == RSP_SUCCESS ) replyID = REPLY_ID_SUCCESS; 
          else replyID = REPLY_ID_FAILED;
        break;
  
      case RD_UL_DL_TX_POWER_B: // 0x0B19 - Read UL,DL TX Power in dBm, Band B
        rsp = Read_UL_TX_Power_B( rdata );
        if( rsp == RSP_SUCCESS ) 
        {
          dLen = strlen( rdata );
          rdata[dLen++] = COMMA;
          rsp = Read_DL_TX_Power_B( &rdata[dLen] );
          if( rsp == RSP_SUCCESS ) 
          {
            replyID = REPLY_ID_SUCCESS; 
          }      
          else replyID = REPLY_ID_FAILED;
        }
        else replyID = REPLY_ID_FAILED;
        break;
        
      default: // Unsupported Command
        rsp = RSP_NOT_SUPPORTED;
        replyID = REPLY_ID_FAILED;
        break;
    }
  } 

  // Just in case, check if we populated the replyID:  
  if( replyID == REPLY_ID_NONE )
  {
    // Jinoj, leave this code alone.  I will re-design this function later.
    rsp = RSP_GENERAL_ERROR; 
    replyID = REPLY_ID_FAILED;
  }
  
  // If failure, override response data field with the error code:
  if( rsp != RSP_SUCCESS )
  {
    memset( rdata, 0, sizeof(rdata));
    sprintf( rdata, "%d", (int)rsp ); // Error code in decimal, not hex
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"SMS MT Error=%d", (int)rsp );
  }
  
  // Send Reponse: Based on Conn_mode (MQTT or SMS).
  if( rdata[0] == 0 )
  {
    rsp = send_MO_MSG( cmd, msgID, replyID, 0, NULL, conn_mode ); // Ack without data
    if( rsp == RSP_SMS_MO_FAILED ) send_MO_MSG( cmd, msgID, replyID, 0, NULL, conn_mode ); // Send again
  }
  else 
  {
    rsp = send_MO_MSG( cmd, msgID, replyID, strlen(rdata), rdata, conn_mode ); // Ack with data
    if( rsp == RSP_SMS_MO_FAILED ) send_MO_MSG( cmd, msgID, replyID, strlen(rdata), rdata, conn_mode ); // Send again
  }
    
  return( rsp );
}


/////////////////////////////////////////////////////////////////////////////
// Function: send_MO_MSG()
//
// Parameters: char  *cmd: ascii string of hex command (null terminated)
//             char  *msgID: ascii string of hex MessageID (null terminated)
//             replyID_t rpyID: type of SMS reply: ACk, NAK, Unsol     
//             char  *data: ascii string of the data (null terminated)
//             UINT8 dLen: length of the data string.
//             mode: connection method (SMS or MQTT)
//
// Return: response_t -> response enum type
//
// Description: This function formats and transmits a Mobile Originated 
// SMS or MQTT message to the RMS server.   If this is an Unsolicited Message,
// set the msgID pointer to Null, and it will generate a sequential msgID.
// 
/////////////////////////////////////////////////////////////////////////////
response_t send_MO_MSG( char *cmd, char *msgID, replyID_t rpyID, UINT8 dLen, 
                        char *data, conn_meth_t mode )
{
  static UINT8 msgIDcnt = 0x00;
  char         coded_msg[200];
  char         msg[SMS_MAX_LENGTH];
  char         str[CMD_PKT_MAX_DATA_PAYLOAD];
  UINT8        i;
  int          len;
  crc          crc_cal;
  msg_queue_t  p1_msg;
#ifdef SMS_PREFIX
  char         prefix[200];
  
  memset( prefix, 0, sizeof(prefix) );
#endif  
  memset( msg, 0, sizeof( msg ));
  memset( coded_msg, 0, sizeof(coded_msg) );
  
   // Add IMEI:
  strncpy( msg, globals.module.imei, SMS_HDR_IMEI_LEN );
  i = SMS_HDR_IMEI_LEN;
  
  // Command ID:
  if( cmd == NULL || strlen( cmd ) != SMS_HDR_CMD_LEN ) return( RSP_SMS_FORMAT_ERROR );
  strncpy( &msg[i], cmd, SMS_HDR_CMD_LEN );
  i += SMS_HDR_CMD_LEN;
    
  // Message ID:
  if( msgID == NULL )
  {  
    msgIDcnt++;
    msg[i++] = n_h_a( msgIDcnt >> 4 );   // MSN
    msg[i++] = n_h_a( msgIDcnt & 0x0F ); // LSN
  }
  else 
  {
    if( strlen( msgID ) != SMS_HDR_MSG_ID_LEN ) return( RSP_SMS_FORMAT_ERROR );
    strncpy( &msg[i], msgID, SMS_HDR_MSG_ID_LEN );
    i += SMS_HDR_MSG_ID_LEN;
  }

  // Reply Id:
  msg[i++] = rpyID;

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
    if( dLen > SMS_MO_MAX_DATA_LEN ||
        dLen > strlen( data )) return( RSP_SMS_FORMAT_ERROR );
    strncpy( &msg[i], data, dLen );
    i += dLen;
  }    
    
  // CRC
  crc_cal = crcFast( msg, i );
  msg[i++] = n_h_a((UINT8)(crc_cal >> 12));
  msg[i++] = n_h_a((UINT8)((crc_cal & 0x0F00) >> 8));
  msg[i++] = n_h_a((UINT8)((crc_cal & 0x00F0) >> 4));
  msg[i++] = n_h_a((UINT8)(crc_cal & 0x000F));


  // Because of a bug in the Telit special character translator,
  // it cannot handle the "@" character right now.
  if( strchr( msg, ATSIGN ) == 0 )
  {
    FromASCIIToRaw( coded_msg, msg, i ); // Telit translator for special chars
    len = strlen( coded_msg );
    if( len > SMS_MAX_LENGTH ) return( RSP_SMS_DATA_PAYLOAD_ERROR );
  }
  else
  {
    strncpy( coded_msg, msg, i );
  }
  
#ifdef SMS_PREFIX  
  // Add Pre-Fix if compiled in:
  strcpy( prefix, sms_prefix );
  strcat( prefix, coded_msg );
  strcpy( coded_msg, prefix );
#endif
  
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"SMS MO:%s", coded_msg );
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"SMS mode:%d", mode );

  if( mode == CONN_METHOD_SMS_ONLY  )
  {
    if( m2m_sms_send_SMS( globals.module.sms_addr, coded_msg ) ) 
    {
      memset( str, 0, sizeof( str));
      sprintf( str, "MO SMS cmd:%s", cmd );
      NE_debug( str );
      NE_debug( "SMS SENT....!");
      return( RSP_SUCCESS );
    }
    else
    {
      NE_debug( "SMS sending failed...!");
      return( RSP_SMS_MO_FAILED );
    }
  }
  else if( mode == CONN_METHOD_MQTT_ONLY )
  {
    len = strlen( msg );
    return( mqtt_publish_response( msg, len ) ); 
  }
  else
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Unable to send msg through mqtt/sms");
    NE_debug( "SMS failed MQTT/SMS");
    return( RSP_SMS_MO_FAILED );
  }
}


/////////////////////////////////////////////////////////////////////////////
// Function: send_unsol_rsp_msg()
//
// Parameters: char  *cmd: ascii string of hex command (null terminated)
//             int  msgID: MessageID 
//             response_t: response code enum type
//             replyID_t:  unsolicted reply type
//
// Return: response_t -> response enum type
//
// Description: This is a wrapper function that formats and transmits
// an unsolicited Mobile Originated SMS or MQTT message with a known MsgID
// to the RMS server with a data payload of the response code enum.
// If the caller does not have a known msgID, set it to greater than
// 0xFF, and the send_MO_MGS() function will generate a new one.
// 
/////////////////////////////////////////////////////////////////////////////
response_t send_unsol_rsp_msg( char *cmd, int msgID, response_t rsp, replyID_t rpyID )
{
  char  msgID_str[SMS_HDR_MSG_ID_LEN+1];
  char  data[6], buf[SMS_MO_MAX_DATA_LEN];
  UINT8 dLen; 
  
  memset( msgID_str, 0, sizeof( msgID_str ));
  memset( data, 0, sizeof( data ));
  memset( buf, 0, sizeof( buf ));

  sprintf( data, "%d", (int)rsp );
  NE_debug( data );

  strcpy( buf, data );
  dLen = strlen( buf );
  buf[dLen++] = COMMA; 

  // epoch time
  rsp = epoch_time( &buf[dLen]);
  if( rsp != RSP_SUCCESS ) return( rsp );
  dLen = strlen( buf );
  
  if( dLen > SMS_MO_MAX_DATA_LEN )
  {
    NE_debug("FTP Resp Len exceeded");
    return( RSP_GENERAL_ERROR );
  }
  
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"rpyID:%d", rpyID );
  
  if( msgID >= 0 || msgID <= 0xFF ) 
  {
    msgID_str[0] = n_h_a( msgID >> 4 );   // MSN
    msgID_str[1] = n_h_a( msgID & 0x0F ); // LSN
    NE_debug( msgID_str );
    return( send_MO_MSG( cmd, msgID_str, rpyID, dLen, buf, globals.module.conn_method ) );  // TO send responses with id success(A) or failure(F)
  }
  else
  {
    return( send_MO_MSG( cmd, NULL, rpyID, dLen, buf, globals.module.conn_method ) );
  }
}

/////////////////////////////////////////////////////////////////////////////
// Function: delete_sms()
//
// Parameters: INT32 message_index
//
// Return: none
//
// Description: This function deletes a MT SMS from memory.
//
/////////////////////////////////////////////////////////////////////////////
void delete_sms( INT32 index )
{
  if( m2m_sms_delete_message( index ) )
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "SMS: Deleted Msg at index:%d",index);
  }
  else
  {  
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "SMS: Failed to delete Msg at index:%d",index);
  } 
}


/////////////////////////////////////////////////////////////////////////////
// Function: crcFast()
//
// Parameters: unsigned char const message[], int nBytes
//
// Return: crc value
//
// Description: Calculates the CRC value for the array of bytes 
// 
/////////////////////////////////////////////////////////////////////////////
crc crcFast(unsigned char const message[], int nBytes)
{
  crc	         remainder = INITIAL_REMAINDER;
  unsigned char  data;
  int            byte;

  //Divide the message by the polynomial, a byte at a time.
  for (byte = 0; byte < nBytes; ++byte)
  {
    data = REFLECT_DATA(message[byte]) ^ (remainder >> (WIDTH - 8));
    remainder = crcTable[data] ^ (remainder << 8);
  }

  // The final remainder is the CRC.
  return (REFLECT_REMAINDER(remainder) ^ FINAL_XOR_VALUE);

}


/////////////////////////////////////////////////////////////////////////////
// Function: crcInit()
//
// Parameters: none
//
// Return: none
//
// Description: Initializes the crc engine for all SMS.  
// 
/////////////////////////////////////////////////////////////////////////////
void crcInit( void )
{
  crc           remainder;
  int           dividend;
  unsigned char bit;

  // Compute the remainder of each possible dividend.
  for (dividend = 0; dividend < 256; ++dividend)
  {
    // Start with the dividend followed by zeros.
    remainder = dividend << (WIDTH - 8);

    // Perform modulo-2 division, a bit at a time.
    for (bit = 8; bit > 0; --bit)
    {
      // Try to divide the current data bit.
      if (remainder & TOPBIT)
      {
        remainder = (remainder << 1) ^ POLYNOMIAL;
      }
      else
      {
        remainder = (remainder << 1);
      }
    }

    // Store the result into the table.
    crcTable[dividend] = remainder;
  }
}
 

/////////////////////////////////////////////////////////////////////////////
// Function: reflect()
//
// Parameters: unsigned long data, unsigned char nBits
//
// Return: unsigned long -> The reflection of the original data.
//
// Description: Reorder the bits of a binary sequence, by reflecting
// them about the middle position.  No checking is done that nBits <= 32.
// 
/////////////////////////////////////////////////////////////////////////////
static unsigned long reflect(unsigned long data, unsigned char nBits)
{
  unsigned long  reflection = 0x00000000;
  unsigned char  bit;

  // Reflect the data about the center bit.
  for (bit = 0; bit < nBits; ++bit)
  {
    // If the LSB bit is set, set the reflection of it.
    if (data & 0x01)
    {
      reflection |= (1 << ((nBits - 1) - bit));
    }
    data = (data >> 1);
  }
  return (reflection);
}

