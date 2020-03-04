/*************************************************************************
                    5BARz International Incorporated. 

                 (C) Copyright 2017. All Rights Reserved.

Filename: paho_task.h

Description: This header file contains interfaces for functions to
initialize and maintain a MQTT task and client

*************************************************************************/
#ifndef HDR_PAHO_TASK_H_
#define HDR_PAHO_TASK_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include "client.h"
#include "sms.h"


/* Paho Client type and macro definitions */
#define CON_MODE_LEN            2
#define MQTT_TASK_PRIORTIY      6
#define PORT_LEN	              6
#define URL_LEN                 100
#define CLIENT_BUFF_SIZE        1000
#define CLIENT_BUF_SIZE         100
#define	MQTT_CLIENT_ID          "5BARZ_CLIENT"    /**< MQTT Client ID */
#define TOPIC_PUBLISH           "5BARZ_RMS"

/* global function prototype declarations */
response_t MQTT_client_connect(INT32 msgID, bool PDP_Active);
response_t MQTT_ping_failed( char *msg );

/* global variable declarations */
Network network;
Client  client;

typedef unsigned short  crc;
/* global inline function definitions */


/////////////////////////////////////////////////////////////////////////////
// Function: MQTT_InitProc()
//
// Parameters:    type:	event id
//                param1: addition info
//                param2: addition info
//
// Returns:       None.
//
// Description: This function is the start of MQTT connection and its functions.
//
/////////////////////////////////////////////////////////////////////////////
INT32 MQTT_InitProc( INT32 type, INT32 param1, INT32 param2 );


/////////////////////////////////////////////////////////////////////////////
// Function: MQTT_start()
//
// Parameters:   none.
//
// Return:       response_t -> response enum type
//
// Description: This function initializes MQTT connection session by calling
// the MQTT task with instructions.
//
/////////////////////////////////////////////////////////////////////////////
response_t MQTT_start(void);


/////////////////////////////////////////////////////////////////////////////
// Function: MQTT_stop()
//
// Parameters:  none.
//
// Return:       response_t -> response enum type
//
// Description: Disconnect from the MQTT Client and switch to SMS.
//
/////////////////////////////////////////////////////////////////////////////
response_t MQTT_stop(void);


/////////////////////////////////////////////////////////////////////////////
// Function: Disconnect_RMS()
//
// Parameters:  NONE.
//
// Return:       response_t -> response enum type
//
// Description: Disconnect from the MQTT Client and switch to SMS.
//
/////////////////////////////////////////////////////////////////////////////
response_t Disconnect_RMS(void);


/////////////////////////////////////////////////////////////////////////////
// Function: mqtt_send_response()
//
// Parameters: response_t -> response enum type
//             INT32      -> the original Message ID
//
// Return: none
//
// Description: This function Publish to the RMS with MQTT response
// code to indicate the state of the current MQTT connection.
//
/////////////////////////////////////////////////////////////////////////////
void mqtt_send_response( response_t rsp, INT32 msgID );


/////////////////////////////////////////////////////////////////////////////
// Function: mqtt_session_failed()
//
// Parameters: response_t -> response enum type
//             INT32      -> the original Message ID
//
// Return: none
//
// Description: This function handles a failed MQTT session by Publishing a
// MQTT response and ending the PDP session.
//
/////////////////////////////////////////////////////////////////////////////
void mqtt_session_failed( response_t rsp, INT32 msgID );


/////////////////////////////////////////////////////////////////////////////
// Function: Initialize_MQTT()
//
// Parameters: none
//
// Return: init_result_t:
//  INIT_SUCCESS,       Initialization success
//  INIT_FAIL_SOFTWARE, Initernal Software failure
//  INIT_FAIL_MQTT    ,MQTT task creation failed
//
// Description: This function initializes all necessary software
// for proper MQTT operation.
//
/////////////////////////////////////////////////////////////////////////////
init_result_t Initialize_MQTT(void);


/////////////////////////////////////////////////////////////////////////////
// Function: MQTT_cell_service_change()
//
// Parameters: bool
//
// Return: none
//
// Description: This function informs the change in reg.status  when device is in MQTT mode
//              and connects to PPP if cell signal is arrived.
/////////////////////////////////////////////////////////////////////////////
void MQTT_cell_service_change(bool cell_service );


/////////////////////////////////////////////////////////////////////////////
// Function: Is_MQTT_Connected()
//
// Parameters: none
//
// Return: bool -> true if connected
//
// Description: This function returns true if connected to the RMS
//
/////////////////////////////////////////////////////////////////////////////
bool Is_MQTT_Connected( void );


/////////////////////////////////////////////////////////////////////////////
// Function: MQTT_task_manager()
//
// Parameters: INT32 - intertask parameter
//
// Return: none
//
// Description: This function controls and monitors the current PPP
// data link and MQTT data links
//
/////////////////////////////////////////////////////////////////////////////
void MQTT_task_manager( INT32 param1 );


/////////////////////////////////////////////////////////////////////////////
// Function: user_timeout_handler()
//
// Parameters: none
//
// Return: none
//
// Description: This function the Timer handler that is used to ping
//              the MQTT broker periodically.
//
/////////////////////////////////////////////////////////////////////////////
void user_timeout_handler( void *arg );


/////////////////////////////////////////////////////////////////////////////
// Function: cycle_timeout_handler()
//
// Parameters: none
//
// Return: none
//
// Description: This Timer handler that is called to trigger the MQTT
// receive message.
//
/////////////////////////////////////////////////////////////////////////////
void cycle_timeout_handler( void *arg );


/////////////////////////////////////////////////////////////////////////////
// Function: wait_timeout_handler()
//
// Parameters: none
//
// Return: none
//
// Description: This Timer handler that is called to trigger a wait
//              for receive message.
//
/////////////////////////////////////////////////////////////////////////////
void wait_timeout_handler( void *arg );


#ifdef __cplusplus
}
#endif

#endif /* HDR_PAHO_TASK_H_ */
