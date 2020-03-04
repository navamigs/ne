#ifndef _SMS_H
#define _SMS_H
/*************************************************************************
                    5Barz International Incorporated. 

                 (C) Copyright 2017. All Rights Reserved.

Filename: sms.h

Description: This header file contains interfaces to functions that
handling all the SMS processing for the Client App.

*************************************************************************/
#ifdef __cplusplus
 extern "C" {
#endif 

#include <stdbool.h>
#include "cmd_pkt.h"

// Data types:
typedef enum
{
  REPLY_ID_NONE       = 0,
  REPLY_ID_SUCCESS    = CHAR_A,
  REPLY_ID_UNSOLCITED = CHAR_E,
  REPLY_ID_FAILED     = CHAR_F,
} replyID_t;
   
   
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
bool init_sms( void );


/////////////////////////////////////////////////////////////////////////////
// Function: mt_sms_callback()
//
// Parameters: char* sms message,  UINT32 message_index
//
// Return: none
//
// Description: This function is called from the M2M_onMsgIndEvent() callback
// when a new MT SMS arrvies.   Because it runs in a lower level modem task,
// do not perfrom any major function other placing a message on the Task_1
// message queue.
//
/////////////////////////////////////////////////////////////////////////////
void mt_sms_callback( char *mem,  UINT32 index );


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
// Description: This function formats and transmitts a Mobile Origenated 
// SMS or MQTT message to the RMS server.   If this is an Unsolicited Message,
// set the msgID pointer to Null, and it will generate a sequential msgID.
// 
/////////////////////////////////////////////////////////////////////////////
response_t send_MO_MSG( char *cmd, char *msgID, replyID_t rpyID, UINT8 dLen, 
                        char *data, conn_meth_t mode );


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
void read_mt_sms( INT32 index );


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
void delete_sms( INT32 index );


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
void crcInit( void );


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
void delete_all_SMS( void );


/////////////////////////////////////////////////////////////////////////////
// Function: send_unsolicited_msg()
//
// Parameters: char  *cmd: ascii string of hex command (null terminated)
//             int  msgID: MessageID 
//             response_t: response code enum type
//             replyID_t : replyid type
//
// Return: response_t -> response enum type
//
// Description: This is a wrapper function that formats and transmits
// an unsolicited Mobile Originated SMS or MQTT message with a known MessageID
// to the RMS server with a data payload of the response code enum.
// If the caller does not have a known msgID, set it to greater than
// 0xFF, and the send_MO_MSG() function will generate a new one.
// 
/////////////////////////////////////////////////////////////////////////////
response_t send_unsol_rsp_msg( char *cmd, int msgID, response_t rsp, replyID_t rpyID );


#ifdef __cplusplus
}
#endif

#endif /* _SMS_H */
