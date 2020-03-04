/*
 * m2m_cloud_utils.h
 *
 *  Created on: 17/nov/2014
 *      Author: FabioPi
 */



#ifndef M2M_CLOUD_UTILS_H_
#define M2M_CLOUD_UTILS_H_

	#include <stdio.h>
	#include <string.h>
	#include <time.h>

	#include "m2m_type.h"
	#include "m2m_clock_api.h"
	#include "m2m_fs_api.h"
	#include "m2m_hw_api.h"
	#include "m2m_os_api.h"
	#include "m2m_os_lock_api.h"
	#include "m2m_socket_api.h"
	#include "m2m_timer_api.h"
	#include "m2m_ssl_api.h"
	#include "m2m_sms_api.h"
	#include "m2m_network_api.h"


	#include "m2m_cloud_defines.h"

	#include "m2m_cloud_task_callbacks.h"
	#include "m2m_cloud_ring_utils.h"
	#include "m2m_cloud_lock_utils.h"



	// 	FUNCTIONS DECLARATIONS  //


	/*		TASKS GETTERS & SETTERS		*/

	INT32 getM2M_Cloud_ReceiveAtTaskID(void);
	void setM2M_Cloud_ReceiveAtTaskID(INT32 id);

	INT32 getM2M_Cloud_ParseMessageTaskID(void);
	void setM2M_Cloud_ParseMessageTaskID(INT32 id);

	INT32 getM2M_Cloud_MethodsHandlerTaskID(void);
	void setM2M_Cloud_MethodsHandlerTaskID(INT32 id);


	/*		CLOUD UTILITIES		*/

	M2M_CLOUD_AZ_LOGPORT getCloudLogPort(void);
	void setCloudLogPort(M2M_CLOUD_AZ_LOGPORT port);

	M2M_CLOUD_DWSTATUS getDWStatus(void);
	void setDWStatus(M2M_CLOUD_DWSTATUS isConn);

	INT8 getCloudRespState(void);
	void setCloudRespState(INT8 status);

	M2M_CLOUD_DBG_LEVEL getDWDebugState(void);
	void setDWDebugState(M2M_CLOUD_DBG_LEVEL debugLev);

	void setDWUSBLogChannel(M2M_USB_CH ch);

	INT32 M2M_Cloud_onReceiveResultCmd(CHAR *atCmd, INT32 len, UINT16 logPort);

	//normal AT response callback
	INT32 M2M_onReceiveResultCmd(CHAR *atCmd, INT32 len, UINT16 logPort);

	void setContextCommand(char* cmd, char *APN);
	void setDWCFGString(char* cmd, char *openURL, M2M_CLOUD_DEVICE_ID_SELECTOR deviceIDSelector, char *appToken, M2M_CLOUD_SECURITY_FLAG security, int heartBeat, M2M_CLOUD_AUTO_RECONNECT autoReconnect, M2M_CLOUD_OVERFLOW_HANDLING handle, int atrunInstanceId, int serviceTimeout);
	//void setDWCfgCommand(char* cmd, char *openURL, char *appToken, int enRemoteAT);


	void registration_timeout_handler(void *arg);

		/*   UTILITIES   */

	INT32 dwSendATCommand(const char *cmd, M2M_CLOUD_BOOL isBlocking);

	INT32 dwSendATData(const char *cmd, INT32 dataLen, M2M_CLOUD_BOOL isBlocking);

	int dwParseFields(char *src, char **fields, int rows, int cols);

	INT8 checkSSLStatus(char * response);

	M2M_CLOUD_DWSTATUS dwCheckStatus(char *response);

	int checkATResponse(char *response, int args,  ...);
	int dwParseDWData(char *response);

	void internalPrint( const CHAR *fmt, ... );
	void PrintToUart ( const CHAR *fmt, ... );
	void USB_Print (M2M_USB_CH ch, const CHAR *fmt, ... );



	/* =================================================================================================
	 *
	 * DESCRIPTION:     Prints the passed string (and potential parameters, like printf) depending on the log level of the code
	 *
	 * PARAMETERS:      level:	the debug level DW_DBG_LEVEL. Can be
	 *	 					DW_LOG_NONE (no log messages are printed),
	 *						DW_LOG_ERRORS (only error messages are printed),
	 *						DW_LOG_LOG (normal logs and error messages are printed),
	 *						DW_LOG_VERBOSE (normal logs, verbose logs and error messages are printed),
	 *						DW_LOG_DEBUG (all log messages are printed)
	 *					wrap: if DW_TRUE, an END string will be attached at the end of the message.
	 *                  fmt: the format string that will be printed.
	 *					... : potential parameter that will be embedded in the fmt string (like the printf behavior)
	 * RETURNS:         None.
	 *
	 * ============================================================================================== */
	void dwPrintDebug(M2M_CLOUD_DBG_LEVEL level, M2M_CLOUD_BOOL wrap, const CHAR *fmt, ...);

	void logToFile(int enable, char logfile_name[16], const CHAR *fmt, ...);

	void setLastMsgID(int msgID);
	int getLastMsgID(void);
	int getdwSendMsgID(char *response);

	//char* getdwDWDataField(int field, char result[DW_FIELD_LEN]);
	char* getDWDataField(int field);
	char getDWDataChar(int field, int column);

	int searchFieldValue(char buffer[DW_FIELDS_NUM][DW_FIELD_LEN], char *strToSearch, char *result);

	int getGPSPosition(char *response, DW_GPSPositionData *position);

	void setRawResponse(char *response);
	char *getRawResponse(void);

#endif /* M2M_CLOUD_UTILS_H_ */
