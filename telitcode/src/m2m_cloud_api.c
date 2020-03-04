/*
 * m2m_cloud_api.c
 *
 *  Created on: 17/nov/2014
 *      Author: FabioPi
 */

#include <stdlib.h>

#include "m2m_cloud_api.h"
#ifdef FEATURE_5BARZ
#include <stdarg.h>

char att_key[] = "rms.set.val"; 
//char att_key[] = "thing.attr.set";
#endif // FEATURE_5BARZ


// -- GLOBAL DATA MATRIX DEFINITION-- //
extern char dwDATAMatrix[DW_FIELDS_NUM][DW_FIELD_LEN];



// -- GLOBAL VARIABLES STRUCT -- //
extern M2M_CLOUD_GLOBALS globals;


extern int regTimeout;

char * certificate;






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
void m2m_cloud_set_global_timeout(int timeout)
{
	globals.timeout = timeout;
}



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
int m2m_cloud_wait_network_registration(void)
{
	M2M_T_NETWORK_REG_STATUS_INFO statusInfo;
	M2M_T_TIMER_HANDLE regTimer;

	if ( globals.dwIsInit == M2M_FALSE ){
		//init() has not been previously called!
		dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_FALSE, " ERROR: Call m2m_cloud_init() function first!\n" );
		return M2M_CLOUD_MISSING_INIT;
	}

	regTimeout = 0;

	regTimer = m2m_timer_create(registration_timeout_handler, NULL);

	m2m_timer_start(regTimer, globals.timeout);

	while(1){
		if (regTimeout == 1)
		{
			dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_FALSE, "Registration Timeout!.\n");
			return M2M_CLOUD_TIMEOUT;

		}
		else
		{
			m2m_network_get_reg_status(&statusInfo);

			dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Status: %d\n", statusInfo.status);

			if (statusInfo.status == 1 || statusInfo.status == 5)
			{
				dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Registered.\n");
				m2m_timer_free(regTimer); /* stop the timer if everything is ok  */
				return M2M_CLOUD_SUCCESS;
			}

			m2m_os_sleep_ms(2000);
		}
	}


}

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
int m2m_cloud_pdp_context_activate(char *APN, char* username, char* password)
{
	char cmd[64];
	struct M2M_T_RTC_TIMEVAL tv;
	int res;
	UINT32 start = 0, now = 0;

	if (!APN){
		return M2M_CLOUD_UNALLOCATED_BUFFERS;
	}


	memset(cmd, 0,sizeof(cmd));
	sprintf(cmd, "AT+CGDCONT=1,\"IP\",\"%s\"\r", APN);
	res = dwSendATCommand(cmd, M2M_TRUE);
	if (res == 1){
		if (username && password){
			sprintf(cmd, "AT#SGACT=1,1,\"%s\",\"%s\"\r", username, password);
		}
		else {
			sprintf(cmd, "AT#SGACT=1,1\r");
		}

		dwSendATCommand(cmd, M2M_FALSE);

		while ( getCloudRespState() == 2){
			m2m_os_sleep_ms(500);
		}


		m2m_get_timeofday(&tv, NULL);
		start = tv.tv_sec;

		while (getCloudRespState() == -1)
		{
			m2m_get_timeofday(&tv, NULL);
			now = tv.tv_sec;

			dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"ms_count_diff = %d\n", now-start);
			if (((now - start) * 1000) > globals.timeout){ //timeout
				return M2M_CLOUD_TIMEOUT;

			}

			res = dwSendATCommand("AT#SGACT=1,0\r", M2M_TRUE);
			m2m_os_sleep_ms(1000);
			res = dwSendATCommand("AT#SGACT=1,1\r", M2M_FALSE);
			while ( getCloudRespState() == 2){
				m2m_os_sleep_ms(500);
			}
		}

		return M2M_CLOUD_SUCCESS;

	}
	else return M2M_CLOUD_FAILURE;

}

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
int m2m_cloud_pdp_context_deactivate(void)
{
	if( dwSendATCommand("AT#SGACT=1,0\r", M2M_TRUE) == 1)
		return M2M_CLOUD_SUCCESS;
	else
		return M2M_CLOUD_FAILURE;
}

/* =================================================================================================
 *
 * DESCRIPTION:     Initializes the Telit M2MCloud library
 *
 * PARAMETERS:      azLogPort:		The Logic Port that will be used by the library (values 1 to 2)
 *                  atParser: 		The AT parser instance that will be linked to the Logic Port (values 0 to 2)
 *                  debug: 			Sets the debug output level: DW_NONE, DW_LOG, DW_DEBUG, DW_ERRORS
 *                  usbLogChannel:	Sets the debug output usb channel. see M2M_USB_CH enum for available values.
 *                  				Use USB_CH_NONE to print to UART.
 *
 * RETURNS:         M2M_CLOUD_SUCCESS in case of success, M2M_CLOUD_FAILURE in case of failure.
 *
 * PRE-CONDITIONS:  None.
 *
 * POST-CONDITIONS: None.
 *
 * ============================================================================================== */
int m2m_cloud_init(M2M_CLOUD_AZ_LOGPORT azLogPort, M2M_AT_PARSER_N atParser, M2M_CLOUD_HEAP_SIZE heapSize, M2M_CLOUD_DBG_LEVEL debug, M2M_USB_CH usbLogChannel)
{
	int res;

	//initialize the global variables structure
	globals.cloudStatus = DW_ERROR;
	globals.cloudRespState = 2;
	globals.dwIsInit = M2M_FALSE;
	globals.dwDebugLevel = M2M_CLOUD_LOG_NONE;
	globals.USB_LOG = USB_CH_NONE;
	globals.CACertificatePath = NULL;

	globals.timeout = 20000; //20 seconds timeout

	//setting the log state
	setDWDebugState(debug);

	//setting the USB log channel
	setDWUSBLogChannel(usbLogChannel);

	/*heap size of 64KB*/
	res = m2m_os_mem_pool(heapSize * 1024);
	if (res != 1)
	{
		dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_FALSE, "Error Setting the memory Pool! Returned value: %d\n", res);
		return res;
	}

	//initializing the cloud operations semaphores
	res = initDWStatusLock();
	if (res != 1)
	{
		dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_FALSE, "Error initializing DWSTATUS lock.\n" );
		return M2M_CLOUD_FAILURE;
	}

	res = initDWDATALock();
	if (res != 1)
	{
		dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_FALSE, "Error initializing DWDATA lock.\n" );
		return M2M_CLOUD_FAILURE;
	}
	res = initDWRingQueueLock();
	if (res != 1)
	{
		dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_FALSE, "Error initializing DWRINGQUEUE lock.\n" );
		return M2M_CLOUD_FAILURE;
	}
	res = initDWSendLock();
	if (res != 1)
	{
		dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_FALSE, "Error initializing DWSEND lock.\n" );
		return M2M_CLOUD_FAILURE;
	}




/*res = initDWMethodsLock();
	 * if (res != 1)
	{
		dwPrintDebug(M2M_CLOUD_LOG_ERRORS, DW_FALSE, "Error initializing methods lock.\n" );
		return M2M_CLOUD_FAILURE;
	}
	 */



	res = initDwRingQueue();
	if (res != 1)
	{
		dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_FALSE, "Error initializing DWRingQueue .\n" );
		return M2M_CLOUD_FAILURE;
	}

	dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Queue Init done.\n" );



	//saving the logic port that the library will use
	setCloudLogPort(azLogPort);


	//TODO add a parameter to the init function, in order to give an option to the user
	setDWRingAction(M2M_CLOUD_AUTORETRIEVE);

	// Inizialization of a task that will manage module responses to AT commands (it is implemented in the file m2m_cloud_task_callbacks.c ).
	setM2M_Cloud_ReceiveAtTaskID(m2m_os_create_task(M2M_OS_TASK_STACK_L, M2M_CLOUD_AT_RESPONSE_TASK_PRIORITY, M2M_OS_TASK_MBOX_M /*50 msgs message box*/, M2M_dwMsgReceive /*callback function*/ ));
	res = getM2M_Cloud_ReceiveAtTaskID();
	if( res <= 0){
		return res;
	}


	// Inizialization of a second task that will manage dwring messages (it is implemented in the file m2m_cloud_task_callbacks.c ).
	setM2M_Cloud_ParseMessageTaskID(m2m_os_create_task(M2M_OS_TASK_STACK_L, M2M_CLOUD_PARSE_MESSAGE_TASK_PRIORITY, M2M_OS_TASK_MBOX_M /*50 msgs message box*/, M2M_dwParseMessage /*callback function*/ ));
	res = getM2M_Cloud_ParseMessageTaskID();
	if( res <= 0){
		return res;
	}

	// Inizialization of a third task that will manage methods (it is implemented in the file m2m_cloud_task_callbacks.c ).
	setM2M_Cloud_MethodsHandlerTaskID(m2m_os_create_task(M2M_OS_TASK_STACK_L, M2M_CLOUD_METHODS_TASK_PRIORITY, M2M_OS_TASK_MBOX_S /*10 msgs message box*/, M2M_dwMethodsHandlerTask /*callback function*/ ));
	res = getM2M_Cloud_MethodsHandlerTaskID();
	if( res <= 0){
		return res;
	}

	//initializing dw AT command instance e.g. AZ2 connected to AT2
	res = m2m_os_iat_set_at_command_instance(azLogPort /*AZ logical port (AZ1,AZ2)*/, atParser/*AT parser number (AT0,AT1,AT2)*/);
	dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"RES_at_command_instance: %d\n", res);

	if (res != 1)
	{
		return res;
	}

	globals.dwIsInit = M2M_TRUE;
    
	return res;
}


/* =================================================================================================
 *
 * DESCRIPTION:     Configures the Certificate file Path
 *
 * PARAMETERS:      certificatePath:	Path of the CA Certificate file (PEM format) in the module's filesystem
 *
 * RETURNS:         M2M_CLOUD_SUCCESS in case of success
 * 					M2M_CLOUD_FAILURE in case of failure
 * 					M2M_CLOUD_MISSING_PARAMETER if the file is not in DER format
 *
 * POST-CONDITIONS: None.
 *
 * ============================================================================================== */
int m2m_cloud_CACertInit(char *certificatePath){
	if (strstr(certificatePath, ".crt") || strstr(certificatePath, ".der")){
		globals.CACertificatePath = m2m_os_mem_alloc(sizeof(char) * 20);
		strcpy(globals.CACertificatePath, certificatePath);
		if (strlen(globals.CACertificatePath) > 0){
			return M2M_CLOUD_SUCCESS;
		}
		else return M2M_CLOUD_FAILURE;
	}

	else
		return M2M_CLOUD_MISSING_PARAMETER;
}



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
 * RETURNS:         M2M_CLOUD_SUCCESS in case of success, M2M_CLOUD_FAILURE in case of failure, M2M_CLOUD_MISSING_INIT if m2m_cloud_init or m2m_cloud_CACert_Init were not called earlier.
 *
 * PRE-CONDITIONS:  m2m_cloud_init() function must have been successfully called earlier. If SSL is required, m2m_cloud_CACertInit() function must have been called earlier.
 *
 * POST-CONDITIONS: None.
 *
 * ============================================================================================== */
int m2m_cloud_config(char *serverUrl, M2M_CLOUD_DEVICE_ID_SELECTOR deviceIDSelector, char *appToken, M2M_CLOUD_SECURITY_FLAG security, int heartBeat, M2M_CLOUD_AUTO_RECONNECT autoReconnect, M2M_CLOUD_OVERFLOW_HANDLING handle, int atrunInstanceId, int serviceTimeout)
{
	int res;
	M2M_T_FS_HANDLE certFile_handle = NULL;
	UINT32 certSize = 0;
	INT8 SSLStatus = -1;
	char module[128];
	char cmd[1500];  //this array will contain the AT command to be sent

	if ( globals.dwIsInit == M2M_FALSE ){
		//init() has not been previously called!
		dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_FALSE, " ERROR: Call m2m_cloud_init() function first!\n" );
		return M2M_CLOUD_MISSING_INIT;
	}


	//if( deviceIDSelector < 0 ) deviceIDSelector = IMEI; //default value
	if( heartBeat < 10 ) heartBeat = 60; //default value
	if (heartBeat > 86400) heartBeat = 86400; //keep value inside boundaries


	//if( autoReconnect < 0) autoReconnect = MODERATE; //default value
	//if( handle < 0 ) handle = FIFO; //default value

	if( atrunInstanceId < 0 ) atrunInstanceId = 4; //default value
	if( atrunInstanceId > 4 ) atrunInstanceId = 4; //keep value inside boundaries

	if( serviceTimeout < 1 ) serviceTimeout = 5; //default value
	if( serviceTimeout > 120 ) serviceTimeout = 120; //keep value inside boundaries



	//check for SSL enable status
	res = dwSendATCommand("AT#SSLEN?\r", M2M_TRUE);

	if (res != 1 ) {
		dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_FALSE, "SSL check failed. Aborting...");
		m2m_os_sys_reset(0);
	}
	if (getCloudRespState() != 0) {
		unLockDWDATALock();
		return M2M_CLOUD_FAILURE;
	}

	//get the SSL status
	SSLStatus = checkSSLStatus(dwDATAMatrix[0]);
	unLockDWDATALock();


	//handle security configuration
	if (security == M2M_TRUE)
	{


		dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "SSL status: %d\n", SSLStatus);

		if (globals.CACertificatePath != NULL)
		{
			if (SSLStatus == 0){
				strcpy(cmd,"AT#SSLEN=1,1\r");
				res = dwSendATCommand(cmd, M2M_TRUE);
				if (res != 1 ) return res;
				if (getCloudRespState() != 0) return M2M_CLOUD_FAILURE;
			}

			if (strstr(globals.CACertificatePath,".crt"))
			{

				//Configuring SSL
				m2m_info_get_model(module);
				if(strstr(module,"GE910"))
				{
					strcpy(cmd,"AT#SSLSECCFG=1,0,1\r");
				}
				else
				{
					strcpy(cmd,"AT#SSLSECCFG=1,0,1,1\r");
				}
				res = dwSendATCommand(cmd, M2M_TRUE);
				if (res != 1 ) return res;
				if (getCloudRespState() != 0) return M2M_CLOUD_FAILURE;

				//get the certificate from PEM file....
				certFile_handle = m2m_fs_open(globals.CACertificatePath, M2M_FS_OPEN_READ);

				if(NULL != certFile_handle)
				{
					certSize = m2m_fs_get_size_with_handle(certFile_handle);
					//dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "filesize: %d\n", certSize);

					certificate = m2m_os_mem_alloc(sizeof(char) * (certSize + 3));
					memset(certificate,0,sizeof(certificate));


					res = m2m_fs_read(certFile_handle, certificate, certSize);

					m2m_fs_close(certFile_handle);

					if (res == certSize)
					{

						dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Certificate correctly read from %s file\n", globals.CACertificatePath );

						m2m_os_mem_free(globals.CACertificatePath);
						memset(cmd,0,sizeof(cmd));

						//Sending CA certificate data
						sprintf(cmd, "AT#SSLSECDATA=1,1,1,%d\r",certSize);
						res = dwSendATCommand(cmd, M2M_TRUE);
						if (res != 1 ) {
							m2m_os_mem_free(certificate);
							return res;
						}
						if (getCloudRespState() != 0) {
							m2m_os_mem_free(certificate);
							return M2M_CLOUD_FAILURE;
						}

						certificate[certSize] = 0x1A;
						//dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "certificate: %s\n", certificate);

						res = dwSendATData(certificate, certSize + 1, M2M_TRUE);
						dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "res: %d\n", res);

						if (res != 1 ) {
							m2m_os_mem_free(certificate);
							return res;
						}
						if (getCloudRespState() != 0) {
							m2m_os_mem_free(certificate);
							return M2M_CLOUD_FAILURE;
						}
						m2m_os_mem_free(certificate);

					}
					else
					{
						dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_FALSE,"Error reading the certificate file");

						m2m_os_mem_free(globals.CACertificatePath);
						m2m_os_mem_free(certificate);
						return M2M_CLOUD_FAILURE;
					}

				}
				else
				{
					m2m_os_mem_free(globals.CACertificatePath);

					dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_FALSE, "Error opening the certificate file");
				}

			} //end of IF .crt

			else if (strstr(globals.CACertificatePath,".der")) //not a .crt, check for .der
			{

				//Configuring SSL
				strcpy(cmd,"AT#SSLSECCFG=1,0,1,0\r");
				res = dwSendATCommand(cmd, M2M_TRUE);
				if (res != 1 ) return res;
				if (getCloudRespState() != 0) return M2M_CLOUD_FAILURE;

				//get the certificate from DER file....
				certFile_handle = m2m_fs_open(globals.CACertificatePath, M2M_FS_OPEN_READ);

				if(NULL != certFile_handle)
				{
					certSize = m2m_fs_get_size_with_handle(certFile_handle);
					//dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "filesize: %d\n", certSize);

					certificate = m2m_os_mem_alloc(sizeof(char) * (certSize + 2));
					memset(certificate,0,sizeof(certificate));


					res = m2m_fs_read(certFile_handle, certificate, certSize);

					m2m_fs_close(certFile_handle);

					if (res == certSize)
					{

						dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Certificate correctly read from %s file\n", globals.CACertificatePath );

						m2m_os_mem_free(globals.CACertificatePath);
						memset(cmd,0,sizeof(cmd));

						//Sending CA certificate data
						sprintf(cmd, "AT#SSLSECDATA=1,1,1,%d\r",certSize);
						res = dwSendATCommand(cmd, M2M_TRUE);
						if (res != 1 ) {
							m2m_os_mem_free(certificate);
							return res;
						}
						if (getCloudRespState() != 0) {
							m2m_os_mem_free(certificate);
							return M2M_CLOUD_FAILURE;
						}

						res = dwSendATData(certificate, certSize, M2M_TRUE);
						dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "res: %d\n", res);

						if (res != 1 ) {
							m2m_os_mem_free(certificate);
							return res;
						}
						if (getCloudRespState() != 0) {
							m2m_os_mem_free(certificate);
							return M2M_CLOUD_FAILURE;
						}
						m2m_os_mem_free(certificate);

					}
					else
					{
						dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_FALSE,"Error reading the certificate file");

						m2m_os_mem_free(globals.CACertificatePath);
						m2m_os_mem_free(certificate);
						return M2M_CLOUD_FAILURE;
					}

				}
				else
				{
					m2m_os_mem_free(globals.CACertificatePath);

					dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_FALSE, "Error opening the certificate file");
				}

			} //not a GE910

		} //certificate path not null
		else
		{
				//CACertInit() had not been previously called
				dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_FALSE, " ERROR: Call m2m_cloud_CACertInit() function first!\n" );
				return M2M_CLOUD_MISSING_INIT;
		}
	}
	else if (SSLStatus == 1)
	{
		strcpy(cmd,"AT#SSLEN=1,0\r");
		res = dwSendATCommand(cmd, M2M_TRUE);
		if (res != 1 ) return res;
		if (getCloudRespState() != 0) return M2M_CLOUD_FAILURE;

	}


	// SENDING AT#DWCFG command //
	setDWCFGString(cmd, serverUrl, deviceIDSelector, appToken, security, heartBeat, autoReconnect, handle, atrunInstanceId, serviceTimeout);

	res = dwSendATCommand(cmd, M2M_TRUE);
	if (res != 1 ) return res;

	return (getCloudRespState() == 0)? M2M_CLOUD_SUCCESS : M2M_CLOUD_FAILURE;
}



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
int m2m_cloud_remoteAT(M2M_CLOUD_REMOTE_AT_STATE state)
{
	int res;
	char cmd[15];


	sprintf(cmd, "AT#DWEN=0,%d\r", (int) state);

	res = dwSendATCommand(cmd, M2M_TRUE);

	if (res != 1 ) return res;
	return (getCloudRespState() == 0)? M2M_CLOUD_SUCCESS : M2M_CLOUD_FAILURE;
}


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
int m2m_cloud_connect(M2M_CLOUD_BOOL isBlocking)
{
	int res;

	res = dwSendATCommand(m2m_cloudDWConn, isBlocking);
	if (res != 1 ) return res;
	return (getCloudRespState() == 0)? M2M_CLOUD_SUCCESS : M2M_CLOUD_FAILURE;

}


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
int m2m_cloud_disconnect(M2M_CLOUD_BOOL isBlocking)
{
	int res;

	res = dwSendATCommand(m2m_cloudDWDisconn, isBlocking);
	if (res != 1 ) return res;
	return (getCloudRespState() == 0)? M2M_CLOUD_SUCCESS : M2M_CLOUD_FAILURE;

}


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
M2M_CLOUD_DWSTATUS m2m_cloud_status(void)
{

	M2M_API_RESULT statusLockResult;

	dwSendATCommand(m2m_cloudDWStat, M2M_TRUE);


	statusLockResult = waitDWStatusValue(globals.timeout);  //wait until the other task unlocks the lock

	if (statusLockResult == M2M_API_RESULT_SUCCESS)
	{
		return getDWStatus();

	}
	else return DW_LOCK_TIMEOUT;

}



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
int m2m_cloud_thing_attr_get(char* attrKey, char* timestamp, char* attrValue)
{
	int res= M2M_CLOUD_FAILURE;
	char cmd[128];
	//char tmp[DW_FIELD_LEN];
	M2M_API_RESULT dwSendLockResult;

	if (!attrValue || !attrKey ) { //not allocated destination buffer
		setLastMsgID(0);
		unLockDWDATALock();
		return M2M_CLOUD_UNALLOCATED_BUFFERS;
	}

	sprintf(cmd, "AT#DWSEND=0,thing.attr.get,key,%s\r", attrKey);
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"%s", cmd);
	res = dwSendATCommand(cmd, M2M_TRUE);
	if (res != 1) {
		setLastMsgID(0);
		unLockDWDATALock();
		return res;
	}

	//wait until the dwdata struct is filled
	dwSendLockResult = waitDWSendLock(globals.timeout);
	if (dwSendLockResult == M2M_API_RESULT_SUCCESS)
	{

		dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"MsgID: %s\n", dwDATAMatrix[0]);
		dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"lastMsgID: %d\n", getLastMsgID());


		if (atoi(dwDATAMatrix[0]) == getLastMsgID())
		{ //check if this is the message response we are waiting for
			dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Correct MsgID.\n");

			//if a timestamp allocated buffer was given
			if (timestamp){
				if(searchFieldValue(dwDATAMatrix, "ts", timestamp)  && searchFieldValue(dwDATAMatrix, "value", attrValue))
				{
					dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"Timestamp and attribute value successfully get.\n");
					//Unlocking the DWDATA lock (locked by dwParseDWData() ), now the global data structure will be available for the next writing//

					res = M2M_CLOUD_SUCCESS;
				}
				else
				{
					//OK was not received
					res = M2M_CLOUD_WRONG_RESPONSE;
				}
			}
			else
			{
				if(searchFieldValue(dwDATAMatrix, "value", attrValue))
				{
					dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"Attribute value successfully get.\n");
					//Unlocking the DWDATA lock (locked by dwParseDWData() ), now the global data structure will be available for the next writing//

					res = M2M_CLOUD_SUCCESS;
				}
				else
				{
					//OK was not received
					res = M2M_CLOUD_WRONG_RESPONSE;
				}
			}
		}
		else
		{
			//wrong msg id
			res = M2M_CLOUD_WRONG_MESSAGE_ID_RESPONSE;
		}
	}
	else
	{
		//failure acquiring lock
		res = M2M_CLOUD_TIMEOUT;
	}
	setLastMsgID(0);
	unLockDWDATALock();
	return res;
}


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
                             char* imei_no,  char* thingimei)
#else 
int m2m_cloud_thing_attr_set(char* attrKey, char* attrValue)
#endif // FEATURE_5BARZ 
{
	int res= M2M_CLOUD_FAILURE;
#ifdef FEATURE_5BARZ
	char cmd[330];
#else
	char cmd[128];
#endif // FEATURE_5BARZ 
    
	M2M_API_RESULT dwSendLockResult;

#ifdef FEATURE_5BARZ
	if (!attrValue1 || !attrKey1) { //not allocated char buffers
#else
	if (!attrValue || !attrKey) { //not allocated char buffers
#endif // FEATURE_5BARZ 
			setLastMsgID(0);
			unLockDWDATALock();
			return M2M_CLOUD_UNALLOCATED_BUFFERS;
		}

#ifdef FEATURE_5BARZ
	sprintf(cmd, "AT#DWSEND=0,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r",att_key,    \
                  attrKey1, attrValue1, attrKey2, attrValue2, attrKey3, attrValue3, attrKey4, attrValue4, \
                  attrKey5, attrValue5, attrKey6, attrValue6, attrKey7, attrValue7, attrKey8, attrValue8, \
                  imei_no, thingimei );
    
	dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"%s",cmd);
	res = dwSendATCommand(cmd, M2M_FALSE);
#else
	sprintf(cmd, "AT#DWSEND=0,thing.attr.set,key,%s,value,%s\r", attrKey, attrValue);
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"%s", cmd);
	res = dwSendATCommand(cmd, M2M_TRUE);
#endif // FEATURE_5BARZ 
	if (res != 1) {
		setLastMsgID(0);
		unLockDWDATALock();
		return res;
	}


	//wait until the dwdata struct is filled
	dwSendLockResult = waitDWSendLock(globals.timeout);
	if (dwSendLockResult == M2M_API_RESULT_SUCCESS)
	{
		dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"MsgID: %s\n", dwDATAMatrix[0]);
		dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"lastMsgID: %d\n", getLastMsgID());


		if (atoi(dwDATAMatrix[0]) == getLastMsgID())
		{ //check if this is the message response we are waiting for
			dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Correct MsgID.\n");

			if(strstr(dwDATAMatrix[3], "OK\r\n"))
			{
				res = M2M_CLOUD_SUCCESS;
			}
			else
			{
				//OK was not received
				res = M2M_CLOUD_WRONG_RESPONSE;
			}
		}
		else
		{
			//wrong msg id
			res = M2M_CLOUD_WRONG_MESSAGE_ID_RESPONSE;
		}

	}
	else
	{
		//failure acquiring lock
		res = M2M_CLOUD_TIMEOUT;
	}
	setLastMsgID(0);
	unLockDWDATALock();
	return res;

}



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
int m2m_cloud_thing_attr_unset(char* attrKey)
{
	int res= M2M_CLOUD_FAILURE;
	char cmd[128];
	M2M_API_RESULT dwSendLockResult;

	if ( !attrKey) { //not allocated char buffer
		setLastMsgID(0);
		unLockDWDATALock();
		return M2M_CLOUD_UNALLOCATED_BUFFERS;
	}

	sprintf(cmd, "AT#DWSEND=0,thing.attr.unset,key,%s\r", attrKey);
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"%s", cmd);
	res = dwSendATCommand(cmd, M2M_TRUE);

	if (res != 1) {
		setLastMsgID(0);
		unLockDWDATALock();
		return res;
	}


	//wait until the dwdata struct is filled
	dwSendLockResult = waitDWSendLock(globals.timeout);
	if (dwSendLockResult == M2M_API_RESULT_SUCCESS)
	{
		dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"MsgID: %s\n", dwDATAMatrix[0]);
		dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"lastMsgID: %d\n", getLastMsgID());


		if (atoi(dwDATAMatrix[0]) == getLastMsgID())
		{ //check if this is the message response we are waiting for
			dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Correct MsgID.\n");

			if(strstr(dwDATAMatrix[3], "OK\r\n"))
			{
				res = M2M_CLOUD_SUCCESS;
			}
			else
			{
				//OK was not received
				res = M2M_CLOUD_WRONG_RESPONSE;
			}
		}
		else
		{
			//wrong msg id
			res = M2M_CLOUD_WRONG_MESSAGE_ID_RESPONSE;
		}

	}
	else
	{
		//failure acquiring lock
		res = M2M_CLOUD_TIMEOUT;
	}
	setLastMsgID(0);
	unLockDWDATALock();
	return res;

}



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
int m2m_cloud_property_publish(char* propertyKey, char* propertyValue, char* timestamp, char* correlationID, M2M_CLOUD_BOOL isBlocking)
{
	int res;
	char cmd[400];
	char ts[40];
	char corrID[128];
	M2M_API_RESULT dwSendLockResult;

	memset(ts,0,sizeof(ts));
	memset(corrID,0,sizeof(corrID));

	if (timestamp != NULL ){
		sprintf(ts, ",ts,%s",timestamp );
	}
	if (correlationID != NULL ){
		sprintf(corrID, ",corrId,%s", correlationID );
	}



	sprintf(cmd, "AT#DWSEND=0,property.publish,key,%s,value,%s%s%s\r", propertyKey, propertyValue, ts, corrID);
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"%s", cmd);
	res = dwSendATCommand(cmd, isBlocking);

	if (res != 1) {
		setLastMsgID(0);
		unLockDWDATALock();
		return res;
	}


	//wait until the dwdata struct is filled
	dwSendLockResult = waitDWSendLock(globals.timeout);
	if (dwSendLockResult == M2M_API_RESULT_SUCCESS)
	{
		dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"MsgID: %s\n", dwDATAMatrix[0]);
		dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"lastMsgID: %d\n", getLastMsgID());


		if (atoi(dwDATAMatrix[0]) == getLastMsgID())
		{ //check if this is the message response we are waiting for
			dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Correct MsgID.\n");

			if(strstr(dwDATAMatrix[3], "OK\r\n"))
			{
				res = M2M_CLOUD_SUCCESS;
			}
			else
			{
				//OK was not received
				res = M2M_CLOUD_WRONG_RESPONSE;
			}
		}
		else
		{
			//wrong msg id
			res = M2M_CLOUD_WRONG_MESSAGE_ID_RESPONSE;
		}

	}
	else
	{
		//failure acquiring lock
		res = M2M_CLOUD_TIMEOUT;
	}
	setLastMsgID(0);
	unLockDWDATALock();
	return res;
}



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
int m2m_cloud_alarm_publish(char* alarmKey, UINT32 state, char* message, double* latitude, 
                            double* longitude, char* timestamp, char* correlationID, 
                            M2M_CLOUD_BOOL republish, M2M_CLOUD_BOOL isBlocking )

{
	int res;
	char cmd[512];

	char ts[40];
	char corrID[128];
	char msg[256];
	char lat[16];
	char lng[16];
	char repub[16];

	M2M_API_RESULT dwSendLockResult;

	memset(ts,0,sizeof(ts));
	memset(corrID,0,sizeof(corrID));
	memset(msg,0,sizeof(msg));
	memset(lat,0,sizeof(lat));
	memset(lng,0,sizeof(lng));
	memset(repub,0,sizeof(repub));

	if (timestamp != NULL ){
		sprintf(ts, ",ts,%s",timestamp );
	}
	if (correlationID != NULL ){
		sprintf(corrID, ",corrId,%s", correlationID );
	}
	if (message != NULL ){
		sprintf(msg, ",msg,\"%s\"",message );
	}
	if (latitude != NULL ){
		sprintf(lat, ",lat,%f",*latitude );
	}
	if (longitude != NULL ){
		sprintf(lng, ",lng,%f",*longitude );
	}
	if (republish){
		sprintf(repub, ",republish,true");
	}
	else {
		sprintf(repub, ",republish,false");
	}


	sprintf(cmd, "AT#DWSEND=0,alarm.publish,key,%s,state,%d%s%s%s%s%s%s\r", alarmKey, state, msg, lat, lng, repub, ts, corrID);

	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"%s", cmd);
	res = dwSendATCommand(cmd, isBlocking);

	if (res != 1) {
		setLastMsgID(0);
		unLockDWDATALock();
		return res;
	}


	//wait until the dwdata struct is filled
	dwSendLockResult = waitDWSendLock(globals.timeout);
	if (dwSendLockResult == M2M_API_RESULT_SUCCESS)
	{
		dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"MsgID: %s\n", dwDATAMatrix[0]);
		dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"lastMsgID: %d\n", getLastMsgID());


		if (atoi(dwDATAMatrix[0]) == getLastMsgID())
		{ //check if this is the message response we are waiting for
			dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Correct MsgID.\n");

			if(strstr(dwDATAMatrix[3], "OK\r\n"))
			{
				res = M2M_CLOUD_SUCCESS;
			}
			else
			{
				//OK was not received
				res = M2M_CLOUD_WRONG_RESPONSE;
			}
		}
		else
		{
			//wrong msg id
			res = M2M_CLOUD_WRONG_MESSAGE_ID_RESPONSE;
		}
	}
	else
	{
		//failure acquiring lock
		res = M2M_CLOUD_TIMEOUT;
	}
	setLastMsgID(0);
	unLockDWDATALock();
	return res;
}


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
int m2m_cloud_alarm_history_last(char* alarmKey, int* alarmState)
{
	int res= M2M_CLOUD_FAILURE;
	int msgID;
	char cmd[256];
	char buffer[32];
	char* startPos = 0;
	char* endPos = 0;
	char stateStr[5];

	M2M_API_RESULT dwSendLockResult;

	if (!alarmKey || !alarmState ) { //not allocated destination buffer
		setLastMsgID(0);
		unLockDWDATALock();
		return M2M_CLOUD_UNALLOCATED_BUFFERS;
	}
	sprintf(cmd, "{\"cmd\":{\"command\": \"alarm.history\",\"params\": {\"key\": \"%s\",\"records\": 1}}}\r", alarmKey);
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"cmd: %s\n", cmd);

	sprintf(buffer, "AT#DWSENDR=%d\r", strlen(cmd) -1 ); //removing the CR character
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"buffer: %s\n", buffer);

	res = dwSendATCommand(buffer, M2M_TRUE);
	if (res != 1) {
		setLastMsgID(0);
		unLockDWDATALock();
		return res;
	}
	m2m_os_sleep_ms(1000);
	res = dwSendATCommand(cmd, M2M_TRUE);

	if (res != 1) {
		setLastMsgID(0);
		unLockDWDATALock();
		return res;
	}


	//wait until the dwdata struct is filled
	dwSendLockResult = waitDWSendLock(globals.timeout);
	if (dwSendLockResult == M2M_API_RESULT_SUCCESS)
	{
		msgID = getLastMsgID();
		dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"MsgID: %d\n", msgID);
		dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"lastMsgID: %d\n", getLastMsgID());


		if (msgID == getLastMsgID())
		{ //check if this is the message response we are waiting for
			dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Correct MsgID.\n");


			if (strstr(getRawResponse(),"\"success\":true"))
			{
			        startPos = strstr(getRawResponse(), "\"state\":") + 8;
			        endPos = strstr(startPos, ",");
			        strncat(stateStr, startPos, endPos - startPos);
			        //dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Alarm State: %s\n",stateStr);

			        *alarmState = atoi(stateStr);
			}
		}
		else
		{
			//wrong msg id
			res = M2M_CLOUD_WRONG_MESSAGE_ID_RESPONSE;
		}
	}
	else
	{
		//failure acquiring lock
		res = M2M_CLOUD_TIMEOUT;
	}
	setLastMsgID(0);
	unLockDWDATALock();
	return res;
}


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
int m2m_cloud_log_publish( char* message, int* level, M2M_CLOUD_BOOL isGlobal, char* timestamp, char* correlationID, M2M_CLOUD_BOOL isBlocking)
{
	int res;
	char cmd[400];

	char ts[40];
	char corrID[128];
	char globalStr[20];
	char levelStr[16];

	M2M_API_RESULT dwSendLockResult;

	memset(ts,0,sizeof(ts));
	memset(corrID,0,sizeof(corrID));
	memset(globalStr,0,sizeof(globalStr));
	memset(levelStr,0,sizeof(levelStr));

	if(!message){
		setLastMsgID(0);
		unLockDWDATALock();
		return M2M_CLOUD_UNALLOCATED_BUFFERS;
	}

	if (timestamp != NULL ){
		sprintf(ts, ",ts,%s",timestamp );
	}

	if (correlationID != NULL ){
		sprintf(corrID, ",corrId,%s", correlationID );  //correlationID max length: 128 - 8 = 120 characters string
	}

	if(level != NULL){
		sprintf(levelStr, ",level,%d", *(level) );	//level max length: 16 - 7 = 9 chars (999999999 max) -> 10^(10)  -1
	}

	switch(isGlobal)
	{
		case M2M_TRUE:
			sprintf(globalStr, ",global,true");
			break;
		case M2M_FALSE:
		default:
			sprintf(globalStr, ",global,false");
			break;
	}
	//message max length ("guard" length, it could be longer): 400 - 128 - 40 - 20 - 16 - 31 (static string of AT#DWSEND command) = 165 characters

	/* =====================================================================================================
	 * the sum of correlationID and message strings length MUST be at MOST 285 characters long.
	 * So, if correlationID is 120 chars long, message max length (see above) will be 165.
	 * If a correlation ID of 10 chars is used, message max length will be 275.
	 * if NO correlation ID is used, message max length will be 293
	 * if NO timestamp is used, and a correlationID of 120 chars is used, message max length will be 205.
	 * if NO timestamp nor correlationID is used, message max length will be 333
	 ====================================================================================================== */


	sprintf(cmd, "AT#DWSEND=0,log.publish,msg,\"%s\"%s%s%s%s\r", message, levelStr, globalStr, ts, corrID);
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"%s", cmd);
	res = dwSendATCommand(cmd, isBlocking);

	if (res != 1) {
		setLastMsgID(0);
		unLockDWDATALock();
		return res;
	}


	//wait until the dwdata struct is filled
	dwSendLockResult = waitDWSendLock(globals.timeout);
	if (dwSendLockResult == M2M_API_RESULT_SUCCESS)
	{
		dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"MsgID: %s\n", dwDATAMatrix[0]);
		dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"lastMsgID: %d\n", getLastMsgID());


		if (atoi(dwDATAMatrix[0]) == getLastMsgID())
		{ //check if this is the message response we are waiting for
			dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Correct MsgID.\n");

			if(strstr(dwDATAMatrix[3], "OK\r\n"))
			{
				res = M2M_CLOUD_SUCCESS;
			}
			else
			{
				//OK was not received
				res = M2M_CLOUD_WRONG_RESPONSE;
			}
		}
		else
		{
			//wrong msg id
			res = M2M_CLOUD_WRONG_MESSAGE_ID_RESPONSE;
		}

	}
	else
	{
		//failure acquiring lock
		res = M2M_CLOUD_TIMEOUT;
	}
	setLastMsgID(0);
	unLockDWDATALock();
	return res;
}


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
int m2m_cloud_location_publish(double latitude, double longitude, float* heading, float* altitude, float* speed, float* fixAcc, char* fixType, char* timestamp, char* correlationID, M2M_CLOUD_BOOL isBlocking)
{
	int res;
	char cmd[400];

	char ts[40];
	char corrID[128];
	char head[30];
	char alt[30];
	char spd[30];
	char fixA[30];
	char fixT[32];
	M2M_API_RESULT dwSendLockResult;

	memset(ts,0,sizeof(ts));
	memset(corrID,0,sizeof(corrID));
	memset(head,0,sizeof(head));
	memset(alt,0,sizeof(alt));
	memset(spd,0,sizeof(spd));
	memset(fixA,0,sizeof(fixA));
	memset(fixT,0,sizeof(fixT));

	if (timestamp != NULL ){
		sprintf(ts, ",ts,%s",timestamp );
	}
	if (correlationID != NULL ){
		sprintf(corrID, ",corrId,%s", correlationID );
	}

	if (heading != NULL ){
		sprintf(head, ",heading,%g",*heading );
	}
	if (altitude != NULL ){
		sprintf(alt, ",altitude,%g",*altitude );
	}
	if (speed != NULL ){
		sprintf(spd, ",speed,%g",*speed );
	}
	if (fixAcc != NULL ){
		sprintf(fixA, ",fixAcc,%g",*fixAcc );
	}
	if (fixType != NULL ){
			sprintf(fixT, ",fixType,%s",fixType );
	}

	sprintf(cmd, "AT#DWSEND=0,location.publish,lat,%f,lng,%f%s%s%s%s%s%s%s\r", latitude, longitude, head, alt, spd, fixA, fixT, ts, corrID);

	//dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"%s", cmd);
	res = dwSendATCommand(cmd, isBlocking);

	if (res != 1) {
		setLastMsgID(0);
		unLockDWDATALock();
		return res;
	}


	//wait until the dwdata struct is filled
	dwSendLockResult = waitDWSendLock(globals.timeout);
	if (dwSendLockResult == M2M_API_RESULT_SUCCESS)
	{
		dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"MsgID: %s\n", dwDATAMatrix[0]);
		dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"lastMsgID: %d\n", getLastMsgID());


		if (atoi(dwDATAMatrix[0]) == getLastMsgID())
		{ //check if this is the message response we are waiting for
			dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Correct MsgID.\n");

			if(strstr(dwDATAMatrix[3], "OK\r\n"))
			{
				res = M2M_CLOUD_SUCCESS;
			}
			else
			{
				//OK was not received
				res = M2M_CLOUD_WRONG_RESPONSE;
			}
		}
		else
		{
			//wrong msg id
			res = M2M_CLOUD_WRONG_MESSAGE_ID_RESPONSE;
		}

	}
	else
	{
		//failure acquiring lock
		res = M2M_CLOUD_TIMEOUT;
	}
	setLastMsgID(0);
	unLockDWDATALock();
	return res;
}



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
int m2m_cloud_location_current(double* latitude, double* longitude, float* heading, float* altitude, float* speed, float* fixAcc, char* fixType)
{
	int res= M2M_CLOUD_FAILURE;
	char cmd[128];
	char lat[16];
	char lng[16];
	char buffer[64];

	M2M_API_RESULT dwSendLockResult;


	if (!latitude || !longitude) { //not allocated destination buffers
		setLastMsgID(0);
		unLockDWDATALock();
		return M2M_CLOUD_UNALLOCATED_BUFFERS;
	}

	memset(buffer, 0, sizeof(buffer));
	memset(lat,0,sizeof(lat));
	memset(lng,0,sizeof(lng));

	sprintf(cmd, "AT#DWSEND=0,location.current\r");
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"%s", cmd);
	res = dwSendATCommand(cmd, M2M_TRUE);

	if (res != 1) {
		setLastMsgID(0);
		unLockDWDATALock();
		return res;
	}


	//wait until the dwdata struct is filled
	dwSendLockResult = waitDWSendLock(globals.timeout);
	if (dwSendLockResult == M2M_API_RESULT_SUCCESS)
	{

		dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"MsgID: %s\n", dwDATAMatrix[0]);
		dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"lastMsgID: %d\n", getLastMsgID());


		if (atoi(dwDATAMatrix[0]) == getLastMsgID())
		{ //check if this is the message response we are waiting for
			dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Correct MsgID.\n");

			if(searchFieldValue(dwDATAMatrix, "lat", lat) && searchFieldValue(dwDATAMatrix, "lng", lng))
			{
				*latitude = strtod(lat, NULL);
				dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"latitude value was successfully get.\n");

				*longitude = strtod(lng, NULL);
				dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"longitude value was successfully get.\n");
				res = M2M_CLOUD_SUCCESS;
			}
			else
			{
				res = M2M_CLOUD_WRONG_RESPONSE;
			}

			//search for optional fields

			if(heading){
				if(searchFieldValue(dwDATAMatrix, "heading", buffer))
				{
					*heading = strtod(buffer,NULL);
					memset(buffer, 0, sizeof(buffer));
				}
				else *heading = 0.0;
			}
			if(altitude){
				if(searchFieldValue(dwDATAMatrix, "altitude", buffer))
				{
					*altitude = strtod(buffer,NULL);
					memset(buffer, 0, sizeof(buffer));
				}
				else *altitude = 0.0;
			}

			if(speed){
				if(searchFieldValue(dwDATAMatrix, "speed", buffer))
				{
					*speed = strtod(buffer,NULL);
					memset(buffer, 0, sizeof(buffer));
				}
				else *speed = 0.0;
			}

			if(fixAcc){
				if(searchFieldValue(dwDATAMatrix, "fixAcc", buffer))
				{
					*fixAcc = strtod(buffer,NULL);
					memset(buffer, 0, sizeof(buffer));
				}
				else *fixAcc = 0.0;
			}

			if(fixType){
				if(searchFieldValue(dwDATAMatrix, "fixType", buffer))
				{
					strcpy(fixType, buffer);
					memset(buffer, 0, sizeof(buffer));
				}
				else fixType = NULL;
			}

		}
		else
		{
			//wrong msg id
			res = M2M_CLOUD_WRONG_MESSAGE_ID_RESPONSE;
		}
	}
	else
	{
		//failure acquiring lock
		res = M2M_CLOUD_TIMEOUT;
	}
	setLastMsgID(0);
	unLockDWDATALock();
	return res;
}




/* =================================================================================================
 *
 * DESCRIPTION:     Allows to send and receive raw cloud data, for TR50 APIs not supported by AT#DW* commands
 *
 * PARAMETERS:		jsonCmd: 		Pointer to the already allocated buffer containing the JSON object to be sent.
 * 									Max length: 1500 Bytes. If CR or LF characters are present, the function will replace them with spaces.
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
int m2m_cloud_raw_data(char* jsonCmd, char** jsonResponse, M2M_CLOUD_BOOL isBlocking)
{
	int res= M2M_CLOUD_FAILURE;
	int len = 0;
	int msgID;
	char buffer[32];
	char* startPos = NULL;
	char* tmpCRLF = NULL;


	M2M_API_RESULT dwSendLockResult;

	if (!jsonCmd ) { //not allocated destination buffer
		setLastMsgID(0);
		unLockDWDATALock();
		return M2M_CLOUD_UNALLOCATED_BUFFERS;
	}

	/*=================================================================================
	 * Removing the <CR> and <LF> inside the jsonCmd string, replacing them with spaces.
	 *
	 ==================================================================================*/
	tmpCRLF = strstr(jsonCmd, "\r");
	while (tmpCRLF){
		*tmpCRLF = ' ';
		tmpCRLF = strstr(jsonCmd, "\r");
	}
	tmpCRLF = strstr(jsonCmd, "\n");
	while (tmpCRLF){
		*tmpCRLF = ' ';
		tmpCRLF = strstr(jsonCmd, "\n");
	}



	sprintf(buffer, "AT#DWSENDR=%d\r", strlen(jsonCmd));
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"buffer: %s\n", buffer);

	res = dwSendATCommand(buffer, isBlocking);
	if (res != 1) {
		setLastMsgID(0);
		unLockDWDATALock();
		return res;
	}
	m2m_os_sleep_ms(1000);
	res = dwSendATCommand(jsonCmd, isBlocking);

	if (res != 1) {
		setLastMsgID(0);
		unLockDWDATALock();
		return res;
	}


	//wait until the dwdata element is filled
	dwSendLockResult = waitDWSendLock(globals.timeout);
	if (dwSendLockResult == M2M_API_RESULT_SUCCESS)
	{
		msgID = getLastMsgID();
		dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"MsgID: %d\n", msgID);
		dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"lastMsgID: %d\n", getLastMsgID());


		if (msgID == getLastMsgID())
		{ //check if this is the message response we are waiting for
			dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Correct MsgID.\n");


			if (dwDATAMatrix[0][0] != '\0' ){
				startPos = strstr(dwDATAMatrix[0], "{");
				len = strlen(startPos) -2 ; //remove CR+LF at the end of the string
				dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Response length: %d\n" , len);

			}

			if (len > 0){
				if (*jsonResponse){  //the external buffer is already initialized
					m2m_os_mem_free(*jsonResponse);

				}

				*jsonResponse = (char*) m2m_os_mem_alloc( ( len + 1 ) * sizeof(char));

				if (!*jsonResponse)
				{
					dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_FALSE,"Error allocating the response buffer!");
					setLastMsgID(0);
					unLockDWDATALock();
					return M2M_CLOUD_FAILURE;
				}

				memset(*jsonResponse,0,len + 1);

				strncpy(*jsonResponse, startPos, len);

				res = (strlen(*jsonResponse) == len)? M2M_CLOUD_SUCCESS : M2M_CLOUD_FAILURE;
				//dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "res: %d\nint: --%s--\n",res,*jsonResponse);
			}

		}
		else
		{
			//wrong msg id
			res = M2M_CLOUD_WRONG_MESSAGE_ID_RESPONSE;
		}
	}
	else
	{
		//failure acquiring lock
		res = M2M_CLOUD_TIMEOUT;
	}

	setLastMsgID(0);
	unLockDWDATALock();
	return res;

}




/* =================================================================================================
 *
 * DESCRIPTION:     Creates a Time string of local time according to RFC3339 format
 * 					( e.g. 2014-12-10T16:39:57.434+01:00 for UTC+1 )
 *
 * PARAMETERS:     	buffer: previously allocated (at least 28 Bytes) destination buffer for the Time String
 * 					timestamp: timestamp as number of seconds since EPOCH (must be > 0).
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
int m2m_cloud_time_RFC3339(char *buffer, time_t timestamp, int useMillisecs)
{

	int result = 0;
	time_t time_tmp = 0;
	M2M_T_RTC_DATE dateStruct;
	M2M_T_RTC_TIME timeStruct;
	struct M2M_T_RTC_TIMEVAL tv;
	struct tm* calendarStruct;
	char millisString[6]; //milliseconds
	char tzString[7];


	int integerPart, remainderPart, sign;


#if 0
	int timeZone;
	struct M2M_T_RTC_TIMEZONE tz;


	if (! buffer) return M2M_CLOUD_UNALLOCATED_BUFFERS;


	memset(tzString, 0, sizeof(tzString));
	memset(millisString, 0, sizeof(millisString));
	memset(buffer, 0, sizeof(buffer));



	if (timestamp == 0){  //use current time, not a given timestamp
		result =  m2m_get_timeofday(&tv, &tz);
		timeZone = tz.tz_tzone;  /* -11:45 to +12:00 time shift */

		time_tmp = (time_t) tv.tv_sec + 2; //WORKAROUND: adding 2 seconds for elaboration otherwise the server will respond with an expiration error

		if (useMillisecs >0 ){
			sprintf(millisString, ".%d", tv.tv_msec);
		}

		calendarStruct = localtime( &time_tmp );


		timeStruct.hour = (CHAR)calendarStruct->tm_hour;
		timeStruct.minute = (CHAR)calendarStruct->tm_min;
		timeStruct.second = (CHAR)calendarStruct->tm_sec; /*0..60 (to allow leap seconds)*/

		dateStruct.day = (CHAR)calendarStruct->tm_mday;
		dateStruct.month = (CHAR)(calendarStruct->tm_mon + 1);
		dateStruct.year = (CHAR)(calendarStruct->tm_year - 100); /* years since 2000 */

		integerPart = timeZone / 4; // get the hours

		//use Daylight Saving Time correction (number of hours shift) to increment timezone hours
		integerPart += tz.tz_dst;  /*tz_dst:  0, 1, 2 */

		remainderPart = timeZone % 4;
		sign = (timeZone > 0) - (timeZone < 0);

		switch(sign)
		{
			case 0:
				sprintf(tzString,"Z");
				break;
			case 1:
				result = sprintf(tzString, "+%02d:%02d", integerPart, remainderPart*15);
				break;
			case -1:
				sprintf(tzString, "-%02d:%02d", -1 * integerPart, -1 * remainderPart * 15);
				break;
		}

	}
	else {  //use the UTC given timestamp as input

		time_tmp = timestamp;
		//time_tmp -= (timeZone * 15 * 60)  + (tz.tz_dst * 3600);
		calendarStruct = localtime( &time_tmp );

		if (useMillisecs > 0 ){
			sprintf(millisString, ".%d", useMillisecs);  //if a timestamp is given, useMillisecs will contain the milliseconds to use.
		}

		timeStruct.hour = (CHAR)calendarStruct->tm_hour;
		timeStruct.minute = (CHAR)calendarStruct->tm_min;
		timeStruct.second = (CHAR)calendarStruct->tm_sec; //0..60 (to allow leap seconds)//

		dateStruct.day = (CHAR)calendarStruct->tm_mday;
		dateStruct.month = (CHAR)(calendarStruct->tm_mon + 1);
		dateStruct.year = (CHAR)(calendarStruct->tm_year - 100); // years since 2000 //

		sprintf(tzString,"Z");

	}


#else
	if (! buffer) return M2M_CLOUD_UNALLOCATED_BUFFERS;


	memset(tzString, 0, sizeof(tzString));
	memset(millisString, 0, sizeof(millisString));
	memset(buffer, 0, sizeof(buffer));


	if (timestamp == 0){
		return M2M_CLOUD_FAILURE;
	}
	else {  //use the UTC given timestamp as input

		time_tmp = timestamp;
		//time_tmp -= (timeZone * 15 * 60)  + (tz.tz_dst * 3600);
		calendarStruct = localtime( &time_tmp );

		if (useMillisecs > 0 ){
			sprintf(millisString, ".%d", useMillisecs);  //if a timestamp is given, useMillisecs will contain the milliseconds to use.
		}

		timeStruct.hour = (CHAR)calendarStruct->tm_hour;
		timeStruct.minute = (CHAR)calendarStruct->tm_min;
		timeStruct.second = (CHAR)calendarStruct->tm_sec; //0..60 (to allow leap seconds)//

		dateStruct.day = (CHAR)calendarStruct->tm_mday;
		dateStruct.month = (CHAR)(calendarStruct->tm_mon + 1);
		dateStruct.year = (CHAR)(calendarStruct->tm_year - 100); // years since 2000 //

		sprintf(tzString,"Z");

	}


#endif


	sprintf(buffer, "%04d-%02d-%02dT%02d:%02d:%02d%s%s", (dateStruct.year + 2000), dateStruct.month, dateStruct.day, timeStruct.hour, timeStruct.minute, timeStruct.second, millisString, tzString);

	return result;

}


#ifdef FEATURE_5BARZ
/* =================================================================================================
*
* FUNCTION:        m2m_cloud_thing_attr_set_one_param()
*
* DESCRIPTION:     Sets a thing attribute, adding it to the thing model
*
* PARAMETERS:      attrKey: String indicating the attribute key to set to value mentioned
*                  attrValue: value to be stored in that attribute
*                  attrKey1: IMEI Number name to be added.
*                  attrValue1: IMEI number to be added in this list.
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
int m2m_cloud_thing_attr_set_one_param( char* attrKey, char* attrValue )
{
  int res = M2M_CLOUD_FAILURE;
  char cmd[128];
  M2M_API_RESULT dwSendLockResult;
  
  if (!attrValue || !attrKey) { //not allocated char buffers
    setLastMsgID(0);
    unLockDWDATALock();
    return M2M_CLOUD_UNALLOCATED_BUFFERS;
  }
  
  sprintf(cmd, "AT#DWSEND=0,%s,%s,%s,%s,%s\r",att_key,attrKey, attrValue,"IMEI_No",globals.module.imei_quote );
  dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"%s", cmd);
  
  res = dwSendATCommand(cmd, M2M_TRUE);
  if (res != 1) {
    setLastMsgID(0);
    unLockDWDATALock();
    return res;
  }

  //wait until the dwdata struct is filled
  dwSendLockResult = waitDWSendLock(10000);
  if (dwSendLockResult == M2M_API_RESULT_SUCCESS)
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"MsgID: %s\n", dwDATAMatrix[0]);
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"lastMsgID: %d\n", getLastMsgID());

    if (atoi(dwDATAMatrix[0]) == getLastMsgID())
    { //check if this is the message response we are waiting for:
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Correct MsgID.\n");

      if(strstr(dwDATAMatrix[3], "OK\r\n"))
      {
        res = M2M_CLOUD_SUCCESS;
      }
      else
      {
        //OK was not received
        res = M2M_CLOUD_WRONG_RESPONSE;
      }
    }
    else
    {
      //wrong msg id
      res = M2M_CLOUD_WRONG_MESSAGE_ID_RESPONSE;
    }
  }
  else
  {
    //failure acquiring lock
    res = M2M_CLOUD_TIMEOUT;
  }
  
  setLastMsgID(0);
  unLockDWDATALock();
  return res;
}


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
int m2m_cloud_thing_attr_set_two_param( char* attrKey1, char* attrValue1, char* attrKey2, char* attrValue2 )
{
  int res= M2M_CLOUD_FAILURE;
  char cmd[128];
  M2M_API_RESULT dwSendLockResult;

  if (!attrValue1 || !attrKey1) { //not allocated char buffers
    setLastMsgID(0);
    unLockDWDATALock();
    return M2M_CLOUD_UNALLOCATED_BUFFERS;
  }

  sprintf(cmd, "AT#DWSEND=0,%s,%s,%s,%s,%s,%s,%s\r",att_key,attrKey1,attrValue1,attrKey2,attrValue2,"IMEI_No",globals.module.imei_quote);
  dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"%s", cmd);
  res = dwSendATCommand(cmd, M2M_TRUE);
  if (res != 1) {
    setLastMsgID(0);
    unLockDWDATALock();
    return res;
  }


  //wait until the dwdata struct is filled
  dwSendLockResult = waitDWSendLock(10000);
  if (dwSendLockResult == M2M_API_RESULT_SUCCESS)
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"MsgID: %s\n", dwDATAMatrix[0]);
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"lastMsgID: %d\n", getLastMsgID());

    if (atoi(dwDATAMatrix[0]) == getLastMsgID())
    { //check if this is the message response we are waiting for
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Correct MsgID.\n");

      if(strstr(dwDATAMatrix[3], "OK\r\n"))
      {
        res = M2M_CLOUD_SUCCESS;
      }
      else
      {
        //OK was not received
        res = M2M_CLOUD_WRONG_RESPONSE;
      }
    }
    else
    {
      //wrong msg id
      res = M2M_CLOUD_WRONG_MESSAGE_ID_RESPONSE;
    }
  }
  else
  {
    //failure acquiring lock
    res = M2M_CLOUD_TIMEOUT;
  }
  
  setLastMsgID(0);
  unLockDWDATALock();
  return res;
}


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
                                          char* attrValue2, char* attrKey3,   char* attrValue3 )
{
  int res= M2M_CLOUD_FAILURE;
  char cmd[128];
  M2M_API_RESULT dwSendLockResult;

  if (!attrValue1 || !attrKey1) { //not allocated char buffers
    setLastMsgID(0);
    unLockDWDATALock();
    return M2M_CLOUD_UNALLOCATED_BUFFERS;
  }

  sprintf(cmd, "AT#DWSEND=0,%s,%s,%s,%s,%s,%s,%s,%s,%s\r",att_key,attrKey1,attrValue1,attrKey2,
          attrValue2,attrKey3,attrValue3,"IMEI_No",globals.module.imei_quote);
  
  dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"%s", cmd);
  res = dwSendATCommand(cmd, M2M_TRUE);
  if (res != 1) {
    setLastMsgID(0);
    unLockDWDATALock();
    return res;
  }

  //wait until the dwdata struct is filled
  dwSendLockResult = waitDWSendLock(10000);
  if (dwSendLockResult == M2M_API_RESULT_SUCCESS)
  {   
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"MsgID: %s\n", dwDATAMatrix[0]);
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"lastMsgID: %d\n", getLastMsgID());

    if (atoi(dwDATAMatrix[0]) == getLastMsgID())
    { //check if this is the message response we are waiting for
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Correct MsgID.\n");

      if(strstr(dwDATAMatrix[3], "OK\r\n"))
      {
        res = M2M_CLOUD_SUCCESS;
      }
      else
      {
        //OK was not received
        res = M2M_CLOUD_WRONG_RESPONSE;
      }
    }
    else
    {
      //wrong msg id
      res = M2M_CLOUD_WRONG_MESSAGE_ID_RESPONSE;
    }
  }
  else
  {
    //failure acquiring lock
    res = M2M_CLOUD_TIMEOUT;
  }

  setLastMsgID(0);
  unLockDWDATALock();
  return res;
}



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
                                        char* attrKey5, char* attrValue5 )
{
  int res= M2M_CLOUD_FAILURE;
  char cmd[330];
    
  M2M_API_RESULT dwSendLockResult;

  if( !attrValue1 || !attrKey1 ) { //not allocated char buffers
	setLastMsgID(0);
    unLockDWDATALock();
    return M2M_CLOUD_UNALLOCATED_BUFFERS;
  }

  sprintf( cmd, "AT#DWSEND=0,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r",att_key,    \
           attrKey1, attrValue1, attrKey2, attrValue2, attrKey3, attrValue3, attrKey4, attrValue4,  \
           attrKey5, attrValue5, "IMEI_No",globals.module.imei_quote );

  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"%s",cmd);
  res = dwSendATCommand(cmd, M2M_FALSE);

  if (res != 1) {
    setLastMsgID(0);
    unLockDWDATALock();
    return res;
  }

  //wait until the dwdata struct is filled
  dwSendLockResult = waitDWSendLock(globals.timeout);
  if (dwSendLockResult == M2M_API_RESULT_SUCCESS)
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"MsgID: %s\n", dwDATAMatrix[0]);
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"lastMsgID: %d\n", getLastMsgID());

    if (atoi(dwDATAMatrix[0]) == getLastMsgID())
    { //check if this is the message response we are waiting for
        dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Correct MsgID.\n");
        if(strstr(dwDATAMatrix[3], "OK\r\n")) res = M2M_CLOUD_SUCCESS;
          else res = M2M_CLOUD_WRONG_RESPONSE; //OK was not received
    }
    else
    {
      //wrong msg id
      res = M2M_CLOUD_WRONG_MESSAGE_ID_RESPONSE;
    }
  }
  else
  {
    //failure acquiring lock
    res = M2M_CLOUD_TIMEOUT;
  
  }
  setLastMsgID(0);
  unLockDWDATALock();
  return res;
}


/* =================================================================================================
 *
 * DESCRIPTION:     Sets the value of the thing attribute associated with the given key.
 *
 * PARAMETERS:      attrKey: String indicating the attribute key to get
 * 					attrValue: String containing the value to be set
 *                  count:count of key_value pair sets added.
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
int m2m_attr_set(char* attrKey1, char* attrValue1,int count,...)
{
	va_list args;
	int i, res= M2M_CLOUD_FAILURE;
	char cmd[512];
	char key[128];
	char value[128];

	M2M_API_RESULT dwSendLockResult;
	memset(cmd,0,strlen(cmd));
	memset(key,0,strlen(key));
	memset(value,0,strlen(value));

		if (!attrValue1 || !attrKey1) { //not allocated char buffers
		setLastMsgID(0);
		unLockDWDATALock();
		return M2M_CLOUD_UNALLOCATED_BUFFERS;
		}

	va_start(args, count);
	/* =======in case of failure, there is only an optional error message=======*/

	/* ======in case of success, retrieve key value couples ======*/
	sprintf(cmd, "AT#DWSEND=0,%s,%s,%s",att_key,attrKey1,attrValue1);  //insert the beginning of the command string

		if( count > 0)
		{
			for (i=0; i < count; i++)
			{
			strcpy(key, va_arg( args, char * ));
			strcpy(value, va_arg( args, char * ));

				if (strlen(key)> 0 && strlen(value) > 0)
				{
				//append the key value couple to the command string
				strcat(cmd, ",");
				strcat(cmd, key);
				strcat(cmd, ",");
				strcat(cmd, value);
				}
				else return M2M_CLOUD_MISSING_PARAMETER;
			}
		va_end(args);
		}
	if (strlen(cmd) > 400) return M2M_CLOUD_MAX_LENGTH_EXCEEDED;
	dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"%s",cmd);
	strcat(cmd, "\r");
	res = dwSendATCommand(cmd, M2M_TRUE);
	if (res != 1) {
	setLastMsgID(0);
	unLockDWDATALock();
	return res;
    }


	//wait until the dwdata struct is filled
	dwSendLockResult = waitDWSendLock(10000);
		if (dwSendLockResult == M2M_API_RESULT_SUCCESS)
		{
		dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"MsgID: %s\n", dwDATAMatrix[0]);
		dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"lastMsgID: %d\n", getLastMsgID());


		if (atoi(dwDATAMatrix[0]) == getLastMsgID())
		{ //check if this is the message response we are waiting for
			dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Correct MsgID.\n");

			if(strstr(dwDATAMatrix[3], "OK\r\n"))
			{
				res = M2M_CLOUD_SUCCESS;
			}
			else
			{
				//OK was not received
				res = M2M_CLOUD_WRONG_RESPONSE;
			}
		}
		else
		{
			//wrong msg id
			res = M2M_CLOUD_WRONG_MESSAGE_ID_RESPONSE;
		}

		}
	else
	{
	//failure acquiring lock
	res = M2M_CLOUD_TIMEOUT;
	}
	setLastMsgID(0);
	unLockDWDATALock();
	return res;
}


#endif // FEATURE_5BARZ
