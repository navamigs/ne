/*
 * m2m_cloud_api.h
 *
 *  Created on: 17/nov/2014
 *      Author: FabioPi
 */

#ifndef M2M_CLOUD_API_H_
#define M2M_CLOUD_API_H_

	#include "m2m_cloud_utils.h"


	/* =================================================================================================
	 *
	 * DESCRIPTION:     Sets up the global network timeout used by all APIs.
	 * 					Please Note: m2m_cloud_init() sets it to 20 seconds by default, this API allows the user to set a different value.
	 *
	 * PARAMETERS:      timeout:	The timeout value, expressed in milliseconds.
	 *
	 * RETURNS:         None.
	 *
	 * PRE-CONDITIONS:  None.
	 *
	 * POST-CONDITIONS: None.
	 *
	 * ============================================================================================== */
	void m2m_cloud_set_global_timeout(int timeout);

	/* =================================================================================================
	 *
	 * DESCRIPTION:     Waits until the module registers to the network, or until timeout (see m2m_cloud_set_global_timeout)
	 *
	 * PARAMETERS:      None.
	 * RETURNS:         M2M_CLOUD_SUCCESS in case of successful registration, M2M_CLOUD_TIMEOUT in case of timeout, M2M_CLOUD_MISSING_INIT if m2m_cloud_init or m2m_cloud_CACert_Init were not called earlier.
	 *
	 * PRE-CONDITIONS:  m2m_cloud_init() function must have been successfully called earlier.
	 *
	 * POST-CONDITIONS: None.
	 *
	 * ============================================================================================== */
	int m2m_cloud_wait_network_registration(void);

	/* =================================================================================================
	 *
	 * DESCRIPTION:     Sets up the PDP context and activates it
	 *
	 * PARAMETERS:      APN:		The string containing the Access Point Name
	 * 					username: 	The username string (NULL if not needed)
	 * 					password: 	The password string (NULL if not needed)
	 *
	 * RETURNS:         M2M_CLOUD_SUCCESS in case of success,
	 * 					M2M_CLOUD_FAILURE in case of failure,
	 * 					M2M_CLOUD_TIMEOUT if after 15 seconds the PDP context has not been activated yet
	 *
	 * PRE-CONDITIONS:  None.
	 *
	 * POST-CONDITIONS: None.
	 *
	 * ============================================================================================== */
	int m2m_cloud_pdp_context_activate(char *APN, char* username, char* password);

	/* =================================================================================================
	 *
	 * DESCRIPTION:     Disables the PDP context
	 *
	 * PARAMETERS:      None
	 *
	 * RETURNS:         M2M_CLOUD_SUCCESS in case of success,
	 * 					M2M_CLOUD_FAILURE in case of failure,
	 *
	 * PRE-CONDITIONS:  None.
	 *
	 * POST-CONDITIONS: None.
	 *
	 * ============================================================================================== */
	int m2m_cloud_pdp_context_deactivate(void);

	/* =================================================================================================
	 *
	 * DESCRIPTION:     Initializes the Telit M2MCloud library
	 *
	 * PARAMETERS:      azLogPort:		The Logic Port that will be used by the library (values 1 to 2)
	 *                  atParser: 		The AT parser instance that will be linked to the Logic Port (values 0 to 2)
	 *                  debug: 			Sets the debug output level: DW_NONE, DW_LOG, DW_DEBUG, DW_ERRORS
	 *                  usbLogChannel:	Sets the debug output usb channel. see M2M_USB_CH enum for available values
	 *
	 * RETURNS:         M2M_CLOUD_SUCCESS in case of success, M2M_CLOUD_FAILURE in case of failure.
	 *
	 * PRE-CONDITIONS:  None.
	 *
	 * POST-CONDITIONS: None.
	 *
	 * ============================================================================================== */
	int m2m_cloud_init(M2M_CLOUD_AZ_LOGPORT azLogPort, M2M_AT_PARSER_N atParser, M2M_CLOUD_HEAP_SIZE heapSize, M2M_CLOUD_DBG_LEVEL debug, M2M_USB_CH usbLogChannel);


	/* =================================================================================================
	 *
	 * DESCRIPTION:     Configures the Certificate file Path
	 *
	 * PARAMETERS:      certificatePath:	Path of the CA Certificate file (DER format) in the module's filesystem
	 *
	 * RETURNS:         M2M_CLOUD_SUCCESS in case of success
	 * 					M2M_CLOUD_FAILURE in case of failure
	 * 					M2M_CLOUD_MISSING_PARAMETER if the file is not in DER format
	 *
	 * POST-CONDITIONS: None.
	 *
	 * ============================================================================================== */
	int m2m_cloud_CACertInit(char *certificatePath);

	/* =================================================================================================
	 *
	 * DESCRIPTION:     Configures the connection parameters to the M2MCloud server
	 *
	 * PARAMETERS:      serverUrl:			String parameter indicating the URL of the device Wise service instance in address:port format.
	 *                  deviceIDSelector: 	Which type of device ID selector will be used. Basically IMEI if not SIM card nor CDMA ID are installed, CCID_ESN otherwise.
	 *                  appToken: 			The secure application token provided in the Management Portal, typically a string of 16 characters.
	 *                  security: 			Flag indicating if the SSL encryption is enabled. Values:  SSL_OFF (default) / SSL_ON
	 *                  heartBeat: 			If no packets are received in the number of seconds specified in the heartbeat field, a heartbeat message will be sent to keep the connection alive.
	 *                  		   			Default: 60,  Range: 10 - 86400
	 *                  autoReconnect:		Flag indicating if the connection manager should automatically reconnect to the service.
	 *                  overflowHandling:	Flag indicating if the way to handle overflows in data management.
	 *                  atrunInstanceId:	AT instance that will be used by the service to run the AT Command. Default 4, Range 0 – 4
	 *                  serviceTimeout:		It defines in seconds the maximum time interval for a service request to the server.	Default 5, Range 1 – 120
	 *
	 * RETURNS:         M2M_CLOUD_SUCCESS in case of success, M2M_CLOUD_FAILURE in case of failure, M2M_CLOUD_MISSING_INIT if m2m_cloud_init was not called earlier.
	 *
	 * PRE-CONDITIONS:  m2m_cloud_init() function must have been successfully called earlier.
	 *
	 * POST-CONDITIONS: None.
	 *
	 * ============================================================================================== */
	int m2m_cloud_config(char *serverUrl, M2M_CLOUD_DEVICE_ID_SELECTOR deviceIDSelector, char *appToken, M2M_CLOUD_SECURITY_FLAG security, int heartBeat, M2M_CLOUD_AUTO_RECONNECT autoReconnect, M2M_CLOUD_OVERFLOW_HANDLING handle, int atrunInstanceId, int serviceTimeout);


	/* =================================================================================================
	 *
	 * DESCRIPTION:     Configures the remote AT feature
	 *
	 * PARAMETERS:      enable:	Sets whether the remote AT would be enabled or not. Values: M2M_CLOUD_DW_DISABLED, M2M_CLOUD_DW_ENABLED
	 *
	 * RETURNS:         M2M_CLOUD_SUCCESS in case of success, M2M_CLOUD_FAILURE in case of failure.
	 *
	 * PRE-CONDITIONS:  m2m_cloud_config() function should be called before this one.
	 *
	 * POST-CONDITIONS: None.
	 *
	 * ============================================================================================== */
	int m2m_cloud_remoteAT(M2M_CLOUD_REMOTE_AT_STATE state);


	/* =================================================================================================
	 *
	 * DESCRIPTION:     Connects the module to the cloud server.
	 *
	 * PARAMETERS:      isBlocking:	if M2M_TRUE, the function waits for the response, otherwise it returns immediately
	 *
	 * RETURNS:         M2M_CLOUD_SUCCESS in case of success, M2M_CLOUD_FAILURE in case of failure.
	 *
	 * PRE-CONDITIONS:  A PDP context must have been set up on context 1. All configuration APIs (init, config, remoteAT) must be called before this one.
	 *
	 * POST-CONDITIONS: None.
	 *
	 * ============================================================================================== */
	int m2m_cloud_connect(M2M_CLOUD_BOOL isBlocking);


	/* =================================================================================================
	 *
	 * DESCRIPTION:     Disconnects the module from the cloud server
	 *
	 * PARAMETERS:      isBlocking:	if M2M_TRUE, the function waits for the response, otherwise it returns immediately
	 *
	 * RETURNS:         M2M_CLOUD_SUCCESS in case of success, M2M_CLOUD_FAILURE in case of failure.
	 *
	 * PRE-CONDITIONS:  m2m_cloud_connect() function must have been successfully called earlier.
	 *
	 * POST-CONDITIONS: None.
	 *
	 * ============================================================================================== */
	int m2m_cloud_disconnect(M2M_CLOUD_BOOL isBlocking);


	/* =================================================================================================
	 *
	 * DESCRIPTION:     Gets the cloud connection status of the module
	 *
	 * PARAMETERS:      None.
	 *
	 * RETURNS:         M2M_CLOUD_SUCCESS in case of success, M2M_CLOUD_FAILURE in case of failure, DW_LOCK_TIMEOUT in case of timeout.
	 *
	 * PRE-CONDITIONS:  None.
	 *
	 * POST-CONDITIONS: None.
	 *
	 * ============================================================================================== */
	M2M_CLOUD_DWSTATUS m2m_cloud_status(void);



	////////////////////////////////////////////////////////////////////////////////////////////

	/* =======================
	 *  ATTRIBUTE APIs
	 * =======================*/

	/* =================================================================================================
	 *
	 * DESCRIPTION:     Gets the value of the thing attribute associated with the given key.
	 *
	 * PARAMETERS:      attrKey: String indicating the attribute key to get
	 * 					timestamp: allocated buffer that will be filled with the timestamp of the last attribute modification. NULL if not required.
	 * 					attrValue: allocated buffer that will be filled with the current attribute value
	 *
	 * RETURNS:         M2M_CLOUD_SUCCESS in case of success,
	 *					M2M_CLOUD_FAILURE in case of failure sending the message,
	 * 					M2M_CLOUD_WRONG_RESPONSE in case of wrong response (e.g, an ERROR was received),
	 * 					M2M_CLOUD_WRONG_MESSAGE_ID_RESPONSE in case of wrong message ID response,
	 * 					M2M_CLOUD_TIMEOUT in case of timeout.
	 * 					M2M_CLOUD_UNALLOCATED_BUFFERS in case of not allocated message string.
	 *
	 * PRE-CONDITIONS:  The module must be connected to the cloud server. The attribute key must be present in the thing definition.
	 *
	 * POST-CONDITIONS: None.
	 *
	 * ============================================================================================== */
	int m2m_cloud_thing_attr_get(char* attrKey, char* timestamp, char* attrValue);



	/* =================================================================================================
	 *
	 * DESCRIPTION:     Sets the value of the thing attribute associated with the given key.
	 *
	 * PARAMETERS:      attrKey: String indicating the attribute key to get
	 * 					attrValue: String containing the value to be set
	 *
	 * RETURNS:         M2M_CLOUD_SUCCESS in case of success,
	 *					M2M_CLOUD_FAILURE in case of failure sending the message,
	 * 					M2M_CLOUD_WRONG_RESPONSE in case of wrong response (e.g, an ERROR was received),
	 * 					M2M_CLOUD_WRONG_MESSAGE_ID_RESPONSE in case of wrong message ID response,
	 * 					M2M_CLOUD_TIMEOUT in case of timeout.
	 * 					M2M_CLOUD_UNALLOCATED_BUFFERS in case of not allocated input/output buffers.
	 *
	 * PRE-CONDITIONS:  The module must be connected to the cloud server. The attribute key must be present in the thing definition, or the "Auto def attributes" option in the module thing definition must be flagged.
	 *
	 * POST-CONDITIONS: None.
	 *
	 * ============================================================================================== */
#ifdef FEATURE_5BARZ
int m2m_cloud_thing_attr_set(char* attrKey1, char* attrValue1, char* attrKey2,  char* attrValue2,  \
                             char* attrKey3, char* attrValue3, char* attrKey4,  char* attrValue4,  \
                             char* attrKey5, char* attrValue5, char* attrKey6,  char* attrValue6,  \
                             char* attrKey7, char* attrValue7, char* attrKey8,  char* attrValue8,  \
                             char* imei_no,  char* thingimei);
#else
	int m2m_cloud_thing_attr_set(char* attrKey, char* attrValue);
#endif // FEATURE_5BARZ


	/* =================================================================================================
	 *
	 * DESCRIPTION:     Unsets a thing attribute, removing it from the thing model
	 *
	 * PARAMETERS:      attrKey: String indicating the attribute key to unset
	 *
	 * RETURNS:        	M2M_CLOUD_SUCCESS in case of success,
	 *					M2M_CLOUD_FAILURE in case of failure sending the message,
	 * 					M2M_CLOUD_WRONG_RESPONSE in case of wrong response (e.g, an ERROR was received),
	 * 					M2M_CLOUD_WRONG_MESSAGE_ID_RESPONSE in case of wrong message ID response,
	 * 					M2M_CLOUD_TIMEOUT in case of timeout.
	 * 					M2M_CLOUD_UNALLOCATED_BUFFERS in case of not allocated input buffers.
	 *
	 * PRE-CONDITIONS:  The module must be connected to the cloud server. The attribute key must be present in the thing definition.
	 *
	 * POST-CONDITIONS: None.
	 *
	 * ============================================================================================== */
	int m2m_cloud_thing_attr_unset(char* attrKey);

	///////////////////////////////////////////////////////////////

	/* =======================
	 *  PUBLISH APIs
	 * =======================*/


	/* =================================================================================================
	 *
	 * DESCRIPTION:     Publishes a thing property value
	 *
	 * PARAMETERS:      propertyKey: String indicating the property key to publish (mandatory)
	 * 					propertyValue: String indicating the property value (mandatory)
	 * 					timestamp: String formatted according RFC3339 time standard (see m2m_cloud_time_RFC3339 API). NULL to use current time
	 * 					correlationID: String containing a correlation ID for the property publish. NULL if not needed
	 * 					isBlocking: If M2M_TRUE, waits for a response, if M2M_FALSE returns immediately.
	 *
	 * RETURNS:         M2M_CLOUD_SUCCESS in case of success,
	 * 					M2M_CLOUD_FAILURE in case of failure sending the message,
	 * 					M2M_CLOUD_WRONG_RESPONSE in case of wrong response (e.g, an ERROR was received),
	 * 					M2M_CLOUD_WRONG_MESSAGE_ID_RESPONSE in case of wrong message ID response,
	 * 					M2M_CLOUD_TIMEOUT in case of timeout.
	 *
	 * PRE-CONDITIONS:  The module must be connected to the cloud server. The property key must be present in the thing definition, or the "Auto def properties" option in the module thing definition must be flagged.
	 *
	 * POST-CONDITIONS: None.
	 *
	 * ============================================================================================== */
	int m2m_cloud_property_publish(char* propertyKey, char* propertyValue, char* timestamp, char* correlationID, M2M_CLOUD_BOOL isBlocking);


	/* =================================================================================================
	 *
	 * DESCRIPTION:     Publishes an alarm in the cloud thing
	 *
	 * PARAMETERS:      alarmKey: String indicating the alarm key to publish (mandatory)
	 * 					state: Integer indicating the alarm state (mandatory)
	 * 					message: String containing the message associated with the alarm state. NULL if not needed.
	 * 					latitude: Pointer to an already allocated float variable containing the latitude of the module. NULL if not needed
	 * 					longitude: Pointer to an already allocated float variable containing the longitude of the module. NULL if not needed
	 * 					timestamp: String formatted according RFC3339 time standard (see m2m_cloud_time_RFC3339 API). NULL to use current time
	 * 					correlationID: String containing a correlation ID for the alarm publish. NULL if not needed
	 * 					republish: If M2M_TRUE, the alarm will be republished even if the state is the same as the previous one.
	 * 					isBlocking: If M2M_TRUE, waits for a response, if M2M_FALSE returns immediately.
	 *
	 * RETURNS:         M2M_CLOUD_SUCCESS in case of success,
	 * 					M2M_CLOUD_FAILURE in case of failure sending the message,
	 * 					M2M_CLOUD_WRONG_RESPONSE in case of wrong response (e.g, an ERROR was received),
	 * 					M2M_CLOUD_WRONG_MESSAGE_ID_RESPONSE in case of wrong message ID response,
	 * 					M2M_CLOUD_TIMEOUT in case of timeout.
	 *
	 * PRE-CONDITIONS:  The module must be connected to the cloud server.
	 * 					The alarm key must be present in the thing definition.
	 * 					The alarm state must be present in the thing definition.
	 *
	 * POST-CONDITIONS: None.
	 *
	 * ============================================================================================== */
	int m2m_cloud_alarm_publish(char* alarmKey, UINT32 state, char* message, double* latitude, double* longitude, char* timestamp, char* correlationID, M2M_CLOUD_BOOL republish, M2M_CLOUD_BOOL isBlocking);


	/* =================================================================================================
		 *
		 * DESCRIPTION:     Retrieves the last state of the given alarm
		 *
		 * PARAMETERS:      alarmKey: String indicating the alarm key to retrieve (mandatory)
		 * 					alarmState: Pointer to an already allocated integer variable that will be filled with the state(mandatory)
		 *
		 * RETURNS:         M2M_CLOUD_SUCCESS in case of success,
		 * 					M2M_CLOUD_FAILURE in case of failure sending the message,
		 * 					M2M_CLOUD_WRONG_RESPONSE in case of wrong response (e.g, an ERROR was received),
		 * 					M2M_CLOUD_WRONG_MESSAGE_ID_RESPONSE in case of wrong message ID response,
		 * 					M2M_CLOUD_TIMEOUT in case of timeout.
		 *
		 * PRE-CONDITIONS:  The module must be connected to the cloud server.
		 * 					The alarm key must be present in the thing definition.
		 *
		 * POST-CONDITIONS: None.
		 *
		 * ============================================================================================== */
	int m2m_cloud_alarm_history_last(char* alarmKey, int* alarmState);

	/* =================================================================================================
	 *
	 * DESCRIPTION:     Publishes a log message in the cloud server.
	 *
	 * PARAMETERS:      message: 		String containing the log message (Mandatory)
	 * 					level: 			Pointer to an already allocated integer variable containing the log level. NULL for default (2).
	 * 					isGlobal: 		If M2M_TRUE, the message will be visible for all the cloud organization. If M2M_FALSE, the log will be associated to the module only
	 * 					timestamp: 		String formatted according RFC3339 time standard (see m2m_cloud_time_RFC3339 API). NULL to use current time.
	 * 					correlationID:	String containing a correlation ID for the log publish. NULL if not needed
	 * 					isBlocking: 	If M2M_TRUE, waits for a response, if M2M_FALSE returns immediately.
	 *
	 * RETURNS:         M2M_CLOUD_SUCCESS in case of success,
	 *					M2M_CLOUD_FAILURE in case of failure sending the message,
	 * 					M2M_CLOUD_WRONG_RESPONSE in case of wrong response (e.g, an ERROR was received),
	 * 					M2M_CLOUD_WRONG_MESSAGE_ID_RESPONSE in case of wrong message ID response,
	 * 					M2M_CLOUD_TIMEOUT in case of timeout.
	 * 					M2M_CLOUD_UNALLOCATED_BUFFERS in case of not allocated message buffer.
	 *
	 * PRE-CONDITIONS:  The module must be connected to the cloud server.
	 *
	 * POST-CONDITIONS: None.
	 *
	 * ============================================================================================== */
	int m2m_cloud_log_publish( char* message, int* level, M2M_CLOUD_BOOL isGlobal, char* timestamp, char* correlationID, M2M_CLOUD_BOOL isBlocking);



	/* =================================================================================================
	 *
	 * DESCRIPTION:     Publishes the location of the module, given latitude and longitude values (other parameters are optional)
	 *
	 * PARAMETERS:      latitude: 		Double precision float indicating the latitude of the module
	 * 					longitude: 		Double precision float indicating the longitude of the module
	 * 					heading: 		pointer to an already allocated float variable containing the heading of the module. NULL if not needed.
	 * 					altitude: 		Pointer to an already allocated float variable containing the altitude of the module. NULL if not needed.
	 * 					speed: 			Pointer to an already allocated float variable containing the speed of the module. NULL if not needed.
	 * 					fixAcc:			Pointer to an already allocated float variable containing the fix accuracy of the module position. NULL if not needed.
	 * 					fixType:		String indicating the type of Fix (e.g: gps, gnss, manual, coarse, m2m_locate). NULL if not needed.
	 * 					timestamp: 		String formatted according RFC3339 time standard ( see m2m_cloud_time_RFC3339 API). NULL to use current time
	 * 					correlationID:	String containing a correlation ID for the location publish. NULL if not needed
	 * 					isBlocking: 	If M2M_TRUE, waits for a response, if M2M_FALSE returns immediately.
	 *
	 * RETURNS:         M2M_CLOUD_SUCCESS in case of success,
	 *					M2M_CLOUD_FAILURE in case of failure sending the message,
	 * 					M2M_CLOUD_WRONG_RESPONSE in case of wrong response (e.g, an ERROR was received),
	 * 					M2M_CLOUD_WRONG_MESSAGE_ID_RESPONSE in case of wrong message ID response,
	 * 					M2M_CLOUD_TIMEOUT in case of timeout.
	 *
	 * PRE-CONDITIONS:  The module must be connected to the cloud server.
	 *
	 * POST-CONDITIONS: None.
	 *
	 * ============================================================================================== */
	int m2m_cloud_location_publish(double latitude, double longitude, float* heading, float* altitude, float* speed, float* fixAcc, char* fixType, char* timestamp, char* correlationID, M2M_CLOUD_BOOL isBlocking);



	/* =================================================================================================
	 *
	 * DESCRIPTION:     Gets the last published module location from the server.
	 *
	 * PARAMETERS:      latitude: 	Pointer to an already allocated double variable that will be filled with the latitude value. (Mandatory)
	 * 					longitude: 	Pointer to an already allocated double variable that will be filled with the longitude value. (Mandatory)
	 * 					heading: 	Pointer to an already allocated float variable that will be filled with the heading value. NULL if not needed.
	 * 					altitude: 	Pointer to an already allocated float variable that will be filled with the altitude value. NULL if not needed.
	 * 					speed:		Pointer to an already allocated float variable that will be filled with the speed value. NULL if not needed.
	 * 					fixAcc:		Pointer to an already allocated float variable that will be filled with the fix Accuracy value. NULL if not needed.
	 * 					fixType:	Already allocated char buffer that will be filled with the fix Type. NULL if not needed.
	 *
	 * RETURNS:         M2M_CLOUD_SUCCESS in case of success,
	 *					M2M_CLOUD_FAILURE in case of failure sending the message,
	 * 					M2M_CLOUD_WRONG_RESPONSE in case of wrong response (e.g, an ERROR was received),
	 * 					M2M_CLOUD_WRONG_MESSAGE_ID_RESPONSE in case of wrong message ID response,
	 * 					M2M_CLOUD_TIMEOUT in case of timeout.
	 * 					M2M_CLOUD_UNALLOCATED_BUFFERS in case of not allocated message buffer.
	 *
	 * PRE-CONDITIONS:  The module must be connected to the cloud server. A module position must have been already published.
	 *
	 * POST-CONDITIONS: None.
	 *
	 * ============================================================================================== */
	int m2m_cloud_location_current(double* latitude, double* longitude, float* heading, float* altitude, float* speed, float* fixAcc, char* fixType);



	/* =================================================================================================
	 *
	 * DESCRIPTION:     Allows to send and receive raw cloud data, for TR50 APIs not supported by AT#DW* commands
	 *
	 * PARAMETERS:		jsonCmd: 		Pointer to the already allocated buffer containing the JSON object to be sent.
	 * 									Max length: 1500 Bytes. MUST not contain CR+LF in the middle of the string.
	 *					jsonResponse:	Pointer to the pointer to the buffer that will be filled with the cloud response (as a JSON object).
	 *									Max response length: 1500 Bytes.
	 *									The buffer will be allocated by the function itself, so IT IS UP TO THE USER to release
	 *									the allocated block, using m2m_os_mem_free()
	 *					isBlocking: 	If M2M_TRUE, waits for a response, if M2M_FALSE returns immediately.
	 *
	 * RETURNS:         M2M_CLOUD_SUCCESS in case of success,
	 * 					M2M_CLOUD_FAILURE in case of failure sending the message,
	 * 					M2M_CLOUD_WRONG_RESPONSE in case of wrong response (e.g, an ERROR was received),
	 * 					M2M_CLOUD_WRONG_MESSAGE_ID_RESPONSE in case of wrong message ID response,
	 * 					M2M_CLOUD_TIMEOUT in case of timeout.
	 *
	 * PRE-CONDITIONS:  The module must be connected to the cloud server.
	 *
	 * POST-CONDITIONS: None.
	 *
	 * ============================================================================================== */
	int m2m_cloud_raw_data(char* jsonCmd, char** jsonResponse, M2M_CLOUD_BOOL isBlocking);

	/* =================================================================================================
	 *
	 * DESCRIPTION:     Creates a Time string of local time according to RFC3339 format
	 * 					( e.g. 2014-12-10T16:39:57.434+01:00 for UTC+1 )
	 *
	 * PARAMETERS:     	buffer: previously allocated (at least 28 Bytes) destination buffer for the Time String
	 * 					timestamp: timestamp as number of seconds since EPOCH. If 0, current time will be used.
	 * 					useMillisecs: 0: the string will contain seconds only. 1: the string will contain seconds.milliseconds
	 *
	 * RETURNS:         M2M_CLOUD_SUCCESS in case of success,
	 * 					M2M_CLOUD_FAILURE in case of failure,
	 * 					M2M_CLOUD_UNALLOCATED_BUFFERS in case of not allocated buffer
	 *
	 * PRE-CONDITIONS:  None.
	 *
	 * POST-CONDITIONS: None.
	 *
	 * ============================================================================================== */
	int m2m_cloud_time_RFC3339(char *buffer, time_t timestamp, int useMillisecs);

#ifdef FEATURE_5BARZ
/* =================================================================================================
*
* FUNCTION:        m2m_cloud_thing_attr_set_one_param()
*
* DESCRIPTION:     Sets a thing attribute, adding it to the thing model
*
* PARAMETERS:      attrKey: String indicating the attribute key to set to value mentioned
*                  attrValue: value to be stored in that attribute
*
* RETURNS:        	M2M_CLOUD_SUCCESS in case of success,
*					M2M_CLOUD_FAILURE in case of failure sending the message,
* 					M2M_CLOUD_WRONG_RESPONSE in case of wrong response (e.g, an ERROR was received),
* 					M2M_CLOUD_WRONG_MESSAGE_ID_RESPONSE in case of wrong message ID response,
* 					M2M_CLOUD_TIMEOUT in case of timeout.
* 					M2M_CLOUD_UNALLOCATED_BUFFERS in case of not allocated input buffers.
*
* PRE-CONDITIONS:  The module must be connected to the cloud server. The attribute key sets in the thing definition.
*
* POST-CONDITIONS: None.
*
* ============================================================================================== */
int m2m_cloud_thing_attr_set_one_param(char* attrKey, char* attrValue);


/* =================================================================================================
*
* FUNCTION:        m2m_cloud_thing_attr_set_two_param()
*
* DESCRIPTION:     sets a thing attribute, adding it to data
*
* PARAMETERS:      attrKey: String indicating the attribute key to set
*                  attrValue: value to be stored in that attribute
*                  attrKey1: String indicating the attribute key to set
*                  attrValue1: value to be stored in that attribute
*
* RETURNS:        	M2M_CLOUD_SUCCESS in case of success,
*					M2M_CLOUD_FAILURE in case of failure sending the message,
* 					M2M_CLOUD_WRONG_RESPONSE in case of wrong response (e.g, an ERROR was received),
* 					M2M_CLOUD_WRONG_MESSAGE_ID_RESPONSE in case of wrong message ID response,
* 					M2M_CLOUD_TIMEOUT in case of timeout.
* 					M2M_CLOUD_UNALLOCATED_BUFFERS in case of not allocated input buffers.
*
* PRE-CONDITIONS:  The module must be connected to the cloud server. The attribute key sets in the thing definition.
*
* POST-CONDITIONS: None.
*
* ============================================================================================== */
int m2m_cloud_thing_attr_set_two_param( char* attrKey1, char* attrValue1, char* attrKey2, char* attrValue2 );


/* =================================================================================================
*
* FUNCTION:        m2m_cloud_thing_attr_set_three_param()
*
* DESCRIPTION:     sets a thing attribute, adding it to data
*
* PARAMETERS:      attrKey: String indicating the attribute key to set
*                  attrValue: value to be stored in that attribute
*                  attrKey1: String indicating the attribute key to set
*                  attrValue1: value to be stored in that attribute
*                  attrKey2: String indicating the attribute key to set
*                  attrValue2: value to be stored in that attribute
*
* RETURNS:        	M2M_CLOUD_SUCCESS in case of success,
*					M2M_CLOUD_FAILURE in case of failure sending the message,
* 					M2M_CLOUD_WRONG_RESPONSE in case of wrong response (e.g, an ERROR was received),
* 					M2M_CLOUD_WRONG_MESSAGE_ID_RESPONSE in case of wrong message ID response,
* 					M2M_CLOUD_TIMEOUT in case of timeout.
* 					M2M_CLOUD_UNALLOCATED_BUFFERS in case of not allocated input buffers.
*
* PRE-CONDITIONS:  The module must be connected to the cloud server. The attribute key sets in the thing definition.
*
* POST-CONDITIONS: None.
*
* ============================================================================================== */
int m2m_cloud_thing_attr_set_three_param( char* attrKey1,   char* attrValue1, char* attrKey2, 
                                          char* attrValue2, char* attrKey3,   char* attrValue3 );


/* =================================================================================================
 *
 * DESCRIPTION:     Sets the value of the thing attribute associated with the given key.
 *
 * PARAMETERS:      attrKey: String indicating the attribute key to get
 * 					attrValue: String containing the value to be set
*                   attrKeyN: String indicating the attribute key to set
*                   attrValueN: value to be stored in that attribute
 *
 * RETURNS:         M2M_CLOUD_SUCCESS in case of success,
 *					M2M_CLOUD_FAILURE in case of failure sending the message,
 * 					M2M_CLOUD_WRONG_RESPONSE in case of wrong response (e.g, an ERROR was received),
 * 					M2M_CLOUD_WRONG_MESSAGE_ID_RESPONSE in case of wrong message ID response,
 * 					M2M_CLOUD_TIMEOUT in case of timeout.
 * 					M2M_CLOUD_UNALLOCATED_BUFFERS in case of not allocated input/output buffers.
 *
 * PRE-CONDITIONS:  The module must be connected to the cloud server. The attribute key must be present in the thing definition, or the "Auto def attributes" option in the module thing definition must be flagged.
 *
 * POST-CONDITIONS: None.
 *
 * ============================================================================================== */
int m2m_cloud_thing_attr_set_five_param(char* attrKey1, char* attrValue1, char* attrKey2,  char* attrValue2,  \
                                        char* attrKey3, char* attrValue3, char* attrKey4,  char* attrValue4,  \
                                        char* attrKey5, char* attrValue5 );                   


int m2m_attr_set(char* attrKey1, char* attrValue1, int count,...);

#endif // FEATURE_5BARZ
    
#endif /* M2M_CLOUD_API_H_ */


