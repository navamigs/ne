/*
 * m2m_cloud_types.h
 *
 *  Created on: 09/gen/2015
 *      Author: FabioPi
 */

#ifndef M2M_CLOUD_TYPES_H_
#define M2M_CLOUD_TYPES_H_

	typedef enum
	{
		M2M_FALSE,
		M2M_TRUE
	} M2M_CLOUD_BOOL;

	typedef enum
	{
		M2M_CLOUD_LOG_NONE,   	/*No messages are displayed*/
		M2M_CLOUD_LOG_ERRORS,	/*Only Errors are displayed*/
		M2M_CLOUD_LOG_LOG,		/*Errors and main log messages are displayed*/
		M2M_CLOUD_LOG_VERBOSE,	/*Errors and all log messages are displayed*/
		M2M_CLOUD_LOG_DEBUG		/*All messages are displayed*/

	} M2M_CLOUD_DBG_LEVEL;

	typedef enum
	{
		M2M_CLOUD_MAX_LENGTH_EXCEEDED = -8,			/* if a command string exceeds the maximum length (400 B) */
		M2M_CLOUD_MISSING_PARAMETER = -7,			/* if using a function without a needed parameter */
		M2M_CLOUD_MISSING_INIT = -6,				/* in case of not initiated cloud application */
		M2M_CLOUD_UNALLOCATED_BUFFERS = -5,			/* in case of not allocated message string.*/
		M2M_CLOUD_TIMEOUT = -4,						/* in case of timeout.*/
		M2M_CLOUD_WRONG_MESSAGE_ID_RESPONSE = -3,	/* in case of wrong message ID response.*/
		M2M_CLOUD_WRONG_RESPONSE =-2,				/* in case of wrong response (e.g, an ERROR was received).*/
		M2M_CLOUD_FAILURE = -1,						/* in case of failure sending the message.*/
		M2M_CLOUD_SUCCESS = 1						/* in case of success.*/
	} M2M_CLOUD_T_API_RETURNS;

	typedef enum
	{
		M2M_CLOUD_DW_DISABLED,
		M2M_CLOUD_DW_ENABLED
	} M2M_CLOUD_REMOTE_AT_STATE;

	typedef enum
	{
		M2M_AZ1=1,
		M2M_AZ2
	} M2M_CLOUD_AZ_LOGPORT;

	typedef enum
	{
		AT0,
		AT1,
		AT2
	} M2M_AT_PARSER_N;

	typedef enum
	{
		IMEI,  /*IMEI is used if not SIM card nor CDMA ID are installed*/
		CCID_ESN
	} M2M_CLOUD_DEVICE_ID_SELECTOR;

	typedef enum
	{
		M2M_CLOUD_SSL_OFF, 		/*Use this as default value*/
		M2M_CLOUD_SSL_ON
	} M2M_CLOUD_SECURITY_FLAG;

	typedef enum
	{
		M2M_CLOUD_DISABLED,
		M2M_CLOUD_LAZY, 		/*Reconnect on next send and every 3600 seconds.*/
		M2M_CLOUD_MODERATE,		/*Reconnect 120 seconds, then every 3600 seconds after the first day. (Use this as default value) */
		M2M_CLOUD_AGGRESSIVE	/*Reconnect every 120 seconds.*/
	} M2M_CLOUD_AUTO_RECONNECT;

	typedef enum
	{
		M2M_CLOUD_FIFO, 		/*Use this as default value*/
		M2M_CLOUD_LIFO
	} M2M_CLOUD_OVERFLOW_HANDLING;


	typedef enum
	{
		DW_LOCK_TIMEOUT= -4, /*unable to acquire lock to get the status*/
		DW_ERROR= -1,
		DW_DISCONNECTED,
		DW_TRYING_TO_CONNECT,
		DW_CONNECTED,
		DW_WAITING_TO_CONNECT
	} M2M_CLOUD_DWSTATUS;


	typedef enum
	{
		M2M_CLOUD_METHOD_MSG_ID,  			/*0*/
		M2M_CLOUD_METHOD_MSG_ERROR,			/*1*/
		M2M_CLOUD_METHOD_MSG_LEN, 			/*2*/
		M2M_CLOUD_METHOD_MSG_METHOD_ID,		/*3*/
		M2M_CLOUD_METHOD_MSG_THING_KEY, 	/*4*/
		M2M_CLOUD_METHOD_MSG_METHOD_KEY 	/*5*/
	} M2M_CLOUD_METHOD_PARAMS;

	typedef enum
	{
		M2M_CLOUD_HEAP_S = 64,  /*64KB*/
		M2M_CLOUD_HEAP_M = 128,	/*128KB*/
		M2M_CLOUD_HEAP_L = 512	/*512KB*/
	}M2M_CLOUD_HEAP_SIZE;




#endif /* M2M_CLOUD_TYPES_H_ */
