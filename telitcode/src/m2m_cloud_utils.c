/*
 * m2m_cloud_defs.c
 *
 *  Created on: 17/nov/2014
 *      Author: FabioPi
 */

/*==================================================================================================
                            INCLUDE FILES
==================================================================================================*/

#include "m2m_cloud_utils.h"


#include <math.h>
#include <stdlib.h>
#include <stdarg.h>


// -- GLOBAL DATA MATRIX DEFINITION-- //
char dwDATAMatrix[DW_FIELDS_NUM][DW_FIELD_LEN];

// -- GLOBAL VARIABLES STRUCT -- //
M2M_CLOUD_GLOBALS globals;

static M2M_T_FS_HANDLE logfile_handle = NULL;
M2M_T_OS_LOCK USBlock_handle[USB_INSTANCES];
M2M_T_OS_LOCK cloudRespLock, dwSendATCmdLock;
M2M_T_OS_LOCK lock_handle = NULL;

int regTimeout;

// -- Functions -- //


INT32 getM2M_Cloud_ReceiveAtTaskID(void)
{
	return globals.M2M_CLOUD_AT_RESPONSE_TASK_ID;
}

void setM2M_Cloud_ReceiveAtTaskID(INT32 id)
{
	globals.M2M_CLOUD_AT_RESPONSE_TASK_ID = id;
}

INT32 getM2M_Cloud_ParseMessageTaskID(void)
{
	return globals.M2M_CLOUD_PARSE_MESSAGE_TASK_ID;
}

void setM2M_Cloud_ParseMessageTaskID(INT32 id)
{
	globals.M2M_CLOUD_PARSE_MESSAGE_TASK_ID = id;
}

INT32 getM2M_Cloud_MethodsHandlerTaskID(void)
{
	return globals.M2M_CLOUD_METHODS_HANDLER_TASK_ID;
}

void setM2M_Cloud_MethodsHandlerTaskID(INT32 id)
{
	globals.M2M_CLOUD_METHODS_HANDLER_TASK_ID = id;
}



M2M_CLOUD_AZ_LOGPORT getCloudLogPort(){

	return globals.cloud_LogPortN;
}

void setCloudLogPort(M2M_CLOUD_AZ_LOGPORT port){

	globals.cloud_LogPortN = port;
}


M2M_CLOUD_DWSTATUS getDWStatus(void)
{
	return globals.cloudStatus;
}

void setDWStatus(M2M_CLOUD_DWSTATUS isConn)
{
	globals.cloudStatus = isConn;
}


INT8 getCloudRespState(void)
{
	INT8 state;
	if (NULL == cloudRespLock) {
	cloudRespLock = m2m_os_lock_init(M2M_OS_LOCK_CS);
	}
	m2m_os_lock_lock(cloudRespLock);
	state = globals.cloudRespState;

	m2m_os_lock_unlock(cloudRespLock);
	return state;

}

void setCloudRespState(INT8 status)
{
	if (NULL == cloudRespLock) {
		cloudRespLock = m2m_os_lock_init(M2M_OS_LOCK_CS);
	}
	m2m_os_lock_lock(cloudRespLock);
	globals.cloudRespState = status;
	m2m_os_lock_unlock(cloudRespLock);
}

M2M_CLOUD_DBG_LEVEL getDWDebugState(void)
{
	return globals.dwDebugLevel;
}

void setDWDebugState(M2M_CLOUD_DBG_LEVEL debugLev)
{
	globals.dwDebugLevel = debugLev;
}


/* =================================================================================================
 *
 * DESCRIPTION:     Sets the USB channel used for log messages
 *
 * PARAMETERS:      ch:		The USB channel
 *
 * RETURNS:         None.
 *
 * PRE-CONDITIONS:  None.
 *
 * POST-CONDITIONS: None.
 *
 * ============================================================================================== */
void setDWUSBLogChannel(M2M_USB_CH ch)
{
	globals.USB_LOG = ch;
}




INT32 M2M_Cloud_onReceiveResultCmd(CHAR *atCmd, INT32 len, UINT16 logPort){
	// ======== CLOUD MESSAGES HANDLER ======== //
	if(logPort == getCloudLogPort() )
	{
		m2m_os_send_message_to_task(getM2M_Cloud_ReceiveAtTaskID(), 0, (long)atCmd, (long)NULL);
	}
	else if ( !strstr(atCmd,"#DWDATA: ") && !strstr(atCmd,"#DWRDATA: ") && !strstr(atCmd,"#DWRING: "))
	{

		//Not cloud related response: call normal receive callback
		M2M_onReceiveResultCmd(atCmd,len,logPort);
	}

	return 1;
}





void setContextCommand(char* cmd, char *APN){
	sprintf(cmd, "AT+CGDCONT=1,\"IP\",\"%s\"\r", APN);
}

/*
 * Creates the AT#DWCFG string using the parameters
 * */

void setDWCFGString(char* cmd, char *openURL, M2M_CLOUD_DEVICE_ID_SELECTOR deviceIDSelector, char *appToken, M2M_CLOUD_SECURITY_FLAG security, int heartBeat, M2M_CLOUD_AUTO_RECONNECT autoReconnect, M2M_CLOUD_OVERFLOW_HANDLING handle, int atrunInstanceId, int serviceTimeout){
	//TODO: add unused_1, unused_2, unused_3 to parameters list
	sprintf(cmd, "AT#DWCFG=%s,%d,%s,%d,%d,%d,%d,%d,%d\r", openURL, deviceIDSelector, appToken, security, heartBeat, autoReconnect, handle, atrunInstanceId, serviceTimeout);
}


/*
 	void setDWCfgCommand(char* cmd, char *openURL, char *appToken, int enRemoteAT){
	sprintf(cmd, "AT#DWCFG=%s,0,%s;#DWEN=0,%d\r", openURL, appToken, enRemoteAT);
}
*/

void ATResponse_timeout_handler(void *arg)
{
	setCloudRespState(M2M_CLOUD_TIMEOUT);
}

void registration_timeout_handler(void *arg)
{
	regTimeout = 1;
}


INT32 dwSendATCommand(const char *cmd, M2M_CLOUD_BOOL isBlocking){
	INT32 res;
	M2M_T_TIMER_HANDLE ATRespTimer;


	if (NULL == dwSendATCmdLock) {
		dwSendATCmdLock = m2m_os_lock_init(M2M_OS_LOCK_CS);
	}

	m2m_os_lock_lock(dwSendATCmdLock);

	if (isBlocking){
		ATRespTimer = m2m_timer_create(ATResponse_timeout_handler, NULL);
		m2m_timer_start(ATRespTimer, globals.timeout);  /* start the timer with the defined timeout  */
	}

	do
	{
		setCloudRespState(2);
		if (getDWDebugState() >= M2M_CLOUD_LOG_VERBOSE)
		{ //echo the command to send
			internalPrint("#VERBOSE SND--%s\n\r",cmd);

		}
		res = m2m_os_iat_send_at_command( (CHAR *)cmd, (UINT16)getCloudLogPort()); //Sending AT Command

		m2m_os_sleep_ms(200);
		if (getDWDebugState() >= M2M_CLOUD_LOG_DEBUG)
		{
			internalPrint("#DEBUG m2m_os_iat_send_at_command-- Res: %d\n\r",res);
		}

		while ( getCloudRespState() == 2 && isBlocking ) m2m_os_sleep_ms(500);

		if (getCloudRespState() != 0) {
			m2m_os_sleep_ms(2000);
		}
	}
	while (getCloudRespState() == -1 && isBlocking );


	if (getCloudRespState() == M2M_CLOUD_TIMEOUT) {
		res = M2M_CLOUD_TIMEOUT;
	}

	m2m_timer_free(ATRespTimer); /* stop the timer if everything is ok  */
	m2m_os_lock_unlock(dwSendATCmdLock);

	return res;
}

INT32 dwSendATData(const char *cmd, INT32 dataLen, M2M_CLOUD_BOOL isBlocking){
	INT32 res;
	M2M_T_TIMER_HANDLE ATRespTimer;


	if (NULL == dwSendATCmdLock) {
		dwSendATCmdLock = m2m_os_lock_init(M2M_OS_LOCK_CS);
	}

	m2m_os_lock_lock(dwSendATCmdLock);

	if (isBlocking){
		ATRespTimer = m2m_timer_create(ATResponse_timeout_handler, NULL);
		m2m_timer_start(ATRespTimer, globals.timeout);  /* start the timer with 10 secs timeout  */
	}

	do
	{
		setCloudRespState(2);

		res = m2m_os_iat_send_atdata_command( (CHAR *)cmd, dataLen, (UINT16)getCloudLogPort()); //Sending AT Command

		m2m_os_sleep_ms(200);
		if (getDWDebugState() >= M2M_CLOUD_LOG_DEBUG)
		{
          internalPrint("#DEBUG: m2m_os_iat_send_atdata_command-- Res: %d\n",res);

		}

		while ( getCloudRespState() == 2 && isBlocking ) m2m_os_sleep_ms(500);

		if (getCloudRespState() != 0) {
			m2m_os_sleep_ms(2000);
		}
	}
	while (getCloudRespState() == -1 && isBlocking );


	if (getCloudRespState() == M2M_CLOUD_TIMEOUT) {
		res = M2M_CLOUD_TIMEOUT;
	}

	m2m_timer_free(ATRespTimer); /* stop the timer if everything is ok  */
	m2m_os_lock_unlock(dwSendATCmdLock);

	return res;
}

M2M_CLOUD_DWSTATUS dwCheckStatus(char *response){
	char dwStatusFields[7][16];
	M2M_CLOUD_DWSTATUS res = DW_ERROR;
	char strToFind[] ="#DWSTATUS: ";

	char *dwStatPtr = strstr( response, strToFind );
	//USB_Print(USB_CH_DEFAULT, "dwstatpointer: %s\n\n", dwStatPtr);

	if ( dwStatPtr > 0 )  //check if the response is a DWSTATUS one
	{
		//USB_Print(USB_CH_DEFAULT, "Parsing AT#DWSTATUS response...\n");
		memset(dwStatusFields, 0, sizeof(dwStatusFields));
		dwParseFields(dwStatPtr + strlen(strToFind), (char **) dwStatusFields, 7, 16);

		res = (M2M_CLOUD_DWSTATUS) atoi( dwStatusFields[0]);
	}

	return res;
}


UINT32 dwGetIpAddress(char *response){
	char * start = NULL, *end = NULL;
	char ipAddress[20];

	start = strstr(response, "#SGACT: ") + 8;
	if (start > 0){
		end = strstr(start, "\r\n\r\n");
	    strncpy(ipAddress, start, end-start);

	    if (strlen(ipAddress) > 0)
	    {
	    	return m2m_socket_bsd_inet_addr(ipAddress);
	    }
	    else return 0;
	}
	else return 0;

}

int dwParseDWData(char *response){
	int res = -1;
	char strToFind[] = "#DWDATA: ";
	char *dwdataPtr = strstr( response, strToFind );

	if ( dwdataPtr > 0 )  //check if the response is a DWDATA one
	{

		memset(dwDATAMatrix, 0, sizeof(dwDATAMatrix));

		dwParseFields(dwdataPtr + strlen(strToFind), (char **) dwDATAMatrix, DW_FIELDS_NUM, DW_FIELD_LEN);
		res = 1;

		/*Unlocking the lock*/
		//unLockDWDATALock();  //it will be unlocked elsewhere
	}
	return res;
}


int dwParseFields(char *src, char **fields, int rows, int cols){
    char *ptr;
    int i = 0;
    const char separator[2] = ",";
    if ( !fields || !src )
    {
    	dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_FALSE,"Not initialized arrays!");
        return -1;
    }

    ptr = strtok (src,separator);
    while (ptr != NULL) {
        strcpy((char *) fields  + ( i * cols), ptr);
        i++;
        ptr = strtok (NULL, ",");
    }
    if (i > 0) return 0;
    else return i;
}


INT8 checkSSLStatus(char * response){
	INT8 SSLStatus = -1;
	char * check;
	check = strstr(response,"#SSLEN: 1,");
	if (check)
	{
		SSLStatus = atoi(check + 10);

	}
	return SSLStatus;
}


int checkATResponse(char *response, int args,  ...){
    int i, check = 0;

    va_list stringsToCheck;
    va_start ( stringsToCheck, args );

    for (i=0; i < args; i++)
    {
        check = check || ( strstr( response, va_arg( stringsToCheck, char * ) ) );

        if( check > 0 ) break;
    }
    va_end ( stringsToCheck );
    return check;
}


void internalPrint( const CHAR *fmt, ... )
{
	va_list arg;
	CHAR	buf[512];

	va_start(arg, fmt);
	vsnprintf(buf , sizeof(buf), fmt, arg);
	if(globals.USB_LOG > USB_CH_NONE) //USB channel: print on USB
	{
		USB_Print(globals.USB_LOG, buf);
	}
    else //Use USB_CH_NONE to print to UART
    {
    	//PrintToUart(buf);
      PrintToUart("%s",buf); //5Barz
    }



	va_end(arg);

}

//##################################################################################################################################
/**
 *  \brief Print directy on the main UART
 *
 *
 *  \param [in] fmt : parameters to print
 *  \param [in] ...  : ...
 *  \return no returned value: if unsuccessfully for any reason, it does not print anything
 *
 */
//##################################################################################################################################
void PrintToUart ( const CHAR *fmt, ... )
{
	INT32	sent;
	va_list arg;
	CHAR	buf[512];
  
#ifdef FEATURE_5BARZ  
  // We do not want any activity on the UART port because
  // it will interfer with cmd_pkt communication
  return;
#else  
	M2M_T_HW_UART_HANDLE local_fd = M2M_HW_UART_HANDLE_INVALID;

	if (NULL == lock_handle) {
		lock_handle = m2m_os_lock_init(M2M_OS_LOCK_CS);
	}

	m2m_os_lock_lock(lock_handle);

	va_start(arg, fmt);
	vsnprintf(buf , sizeof(buf), fmt, arg);

	/* Get a UART handle first */
	local_fd = m2m_hw_uart_open();

	if (M2M_HW_UART_HANDLE_INVALID != local_fd) {
		m2m_hw_uart_write(local_fd, buf, strlen(buf), &sent);
		//m2m_hw_uart_write(local_fd, "\r\n", 2, &sent);

		/* in case of concurrency using m2m_hw_uart... comment the next API to avoid the closing */
		m2m_hw_uart_close(local_fd);
	}

	va_end(arg);
	m2m_os_lock_unlock(lock_handle);
#endif // FEATURE_5BARZ
}


//##################################################################################################################################
/**
 *  \brief Print as PrintToUart but using a specified USB channel
 *
 *  \param [in] ch: USB channel where to print
 *  \param [in] fmt : parameters to prints
 *  \param [in] ...  : ...
 *  \return no returned value: if unsuccessfully for any reason, it does not print anything
 *
 *  \details Using channel:USB_CH_DEFAULT is the same as PrintToUart_USB.  It uses channel assigned to instance USER_USB_INSTANCE_0
 *                      See PrintToUart_USB in the header file
 */
//##################################################################################################################################
 void USB_Print (M2M_USB_CH ch, const CHAR *fmt, ... )
{
	INT32	sent;
	va_list arg;
	CHAR	buf[256];
    USER_USB_INSTANCE_T  UsbInstance;


  M2M_T_HW_USB_HANDLE local_USBfd = M2M_HW_USB_UART_HANDLE_INVALID_PORT;
  UsbInstance = m2m_hw_usb_get_instance(ch);
  if (UsbInstance >= USER_USB_INSTANCE_ERR)                  // no istance found
  {
     UsbInstance = m2m_hw_usb_get_instance(USB_CH_NONE);  // search for new istance
  	 if (UsbInstance >= USER_USB_INSTANCE_ERR)
     	return;                                               // no istances available
  }

	if (NULL == USBlock_handle[UsbInstance]) {
		USBlock_handle[UsbInstance] = m2m_os_lock_init(M2M_OS_LOCK_CS);
	}

	m2m_os_lock_lock(USBlock_handle[UsbInstance]);

	va_start(arg, fmt);
	vsnprintf(buf, sizeof(buf)-1, fmt, arg);

	/* Get a USB handle first */
	if ( M2M_API_RESULT_SUCCESS != m2m_hw_usb_open(ch, &local_USBfd) )
  {
    //destroy lock related to USB that can not be opened
    m2m_os_lock_destroy(USBlock_handle[UsbInstance]);
    //free for later use, if any
    USBlock_handle[UsbInstance] = NULL;
    return;
  }

   (void)m2m_hw_usb_write ( local_USBfd, buf, strlen(buf), &sent );

	 //(void)m2m_hw_usb_write(local_USBfd, "\r\n", 2, &sent);

		/* in case of concurrency using m2m_hw_usb... comment the next API to avoid the closing */
		(void)m2m_hw_usb_close(local_USBfd);


	va_end(arg);
	m2m_os_lock_unlock(USBlock_handle[UsbInstance]);
}



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
void dwPrintDebug(M2M_CLOUD_DBG_LEVEL level, M2M_CLOUD_BOOL wrap, const CHAR *fmt, ...){
	va_list arg;
	CHAR	buf[234];
	CHAR 	outbuf[256];

	if (getDWDebugState() >= level)
		{
		va_start(arg, fmt);
		memset(buf,0,sizeof(buf));
		memset(outbuf,0,sizeof(outbuf));

		vsnprintf(buf, sizeof(buf)-1, fmt, arg);


		switch(level)
		{
			case M2M_CLOUD_LOG_ERRORS:
					strcat(outbuf,"#ERRORS: ");
				break;
			case M2M_CLOUD_LOG_LOG:
					strcat(outbuf,"#LOG: ");
				break;
			case M2M_CLOUD_LOG_VERBOSE:
					strcat(outbuf,"#VERBOSE: ");
				break;
			case M2M_CLOUD_LOG_DEBUG:
					strcat(outbuf,"#DEBUG: ");
				break;
		}
		strcat(outbuf,buf);

		//if wrap is true, the log will have an ending string
		//if( wrap ) strcat(outbuf,"\n--END LOG\n");
		strcat(outbuf,"\n\r"); //5Barz

		//internalPrint(outbuf);
    internalPrint("%s",outbuf); //5Barz

	}
	va_end(arg);

}

void logToFile(int enable, char logfile_name[16], const CHAR *fmt, ...)
{
	CHAR	buf[200];
	va_list arg;
	if (enable){
		va_start(arg, fmt);
		memset(buf,0,sizeof(buf));
		vsnprintf(buf, sizeof(buf)-1, fmt, arg);

		logfile_handle = m2m_fs_open(logfile_name, M2M_FS_OPEN_APPEND);
		if(NULL != logfile_handle)
		{
			m2m_fs_write(logfile_handle, buf, strlen(buf));

			m2m_fs_close(logfile_handle);
		}
	}

}




void setLastMsgID(int msgID)
{
	//TODO add lock wait (unlock in APIs)
	globals.DW_LAST_MSG_ID = msgID;
}

int getLastMsgID(void)
{
	return globals.DW_LAST_MSG_ID;
	//TODO add lock wait (locked in APIs)
}


int getdwSendMsgID(char *response)
{
	char *start, *end;
    char msgIDStr[5];
    int msgID = -1;
    start = strstr(response,"#DWSEND: ");
    if(start){
    	end = strstr(start, "\r");
    	if(end){
    		strncpy(msgIDStr, start + 9, end - start - 9 );
    		msgID = atoi(msgIDStr);
    	}
    }
    else
    {
		start = strstr(response,"#DWSENDR: ");
		if(start){
			end = strstr(start, "\r");
			if(end){
				strncpy(msgIDStr, start + 10, end - start - 10 );
				msgID = atoi(msgIDStr);
			}
		}
    }
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"Retrieved msgID: %d\n", msgID);
    return msgID;
}


int searchFieldValue(char buffer[DW_FIELDS_NUM][DW_FIELD_LEN], char *strToSearch, char *result){
    int i, ret = -1;

    if ( !buffer || !result )
    {
    	dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_FALSE,"Not initialized arrays!");
		return -2;
    }

    for (i=3; i< DW_FIELDS_NUM;i++){
    	if( ( strstr( buffer[i], strToSearch) ) ){
			memset(result, 0, sizeof(result));
			strcpy(result, buffer[i + 1]);
			ret = 1;
		}
    }

    return ret;
}


int getGPSPosition(char *response, DW_GPSPositionData *position)
{
    //int i;
    int res;
	char gpsacpFields[11][30];
    char strToFind[] ="$GPSACP: ";
    char tmpbuf[10];
    double tmpcoord = 0.0;
    char *gpsacpPtr = strstr( response, strToFind );
    struct tm timeStruct;

#if 0
    // variables needed to get local offset with respect to UTC
    int localOffset = 0;
    struct M2M_T_RTC_TIMEVAL tv;
    struct M2M_T_RTC_TIMEZONE tz;
#endif

   //char* GPSACPFIELDS[] = {"UTC","latitude","longitude","hdop","altitude","fix","cog","spkm","spkn","date","nsat"};

    //reset of the structure
    position -> latitude = 0.0;
    position -> longitude = 0.0;
    position -> altitude = 0.0;
	position -> speed = 0.0;
	position -> cog = 0.0;  //course over ground
	position -> nsat = 0;
	position -> fix = 0;
	//USB_Print(USB_CH_DEFAULT, "pointer: %s\n\n", gpsacpPtr);

    if ( gpsacpPtr > 0 )  //check if the response is a $GPSACP one
    {
        if (strstr( response, ",,,,,1,,,,," ))  //no data available
        {
            position -> fix = -1;
            return -1;
        }
        //USB_Print(USB_CH_DEFAULT, "Parsing AT$GPSACP response...\n");
        dwParseFields(gpsacpPtr + strlen(strToFind), (char **) gpsacpFields, 11, 30);

        res = atoi(gpsacpFields[5]);  //FIX type
        if (res == 3)  //3D FIX
        {
        	 //Calculating Latitude
			memset(tmpbuf,0,sizeof(tmpbuf));

			strncpy(tmpbuf, gpsacpFields[1],2); 	// 	degree integer part;
			tmpcoord = strtod(tmpbuf,NULL); 		//	converting to double
			strncpy(tmpbuf, gpsacpFields[1] + 2,7); //	minute float part
			tmpcoord += strtod(tmpbuf,NULL) / 60;  	//	sum up the converted minute part to the degree integer part -> WGS84 standard
			if (gpsacpFields[1][9] == 'S'){
				tmpcoord = -1 * tmpcoord;
			}
			position -> latitude = tmpcoord;

			//Calculating Longitude
			memset(tmpbuf,0,sizeof(tmpbuf));
			tmpcoord = 0.0;
			strncpy(tmpbuf, gpsacpFields[2],3);		// degree integer part;
			tmpcoord = strtod(tmpbuf,NULL); 		//	converting to double
			strncpy(tmpbuf, gpsacpFields[2] + 3,7); //	minute float part
			tmpcoord += strtod(tmpbuf,NULL) / 60;  	//	sum up the converted minute part to the degree integer part -> WGS84 standard
			if (gpsacpFields[2][10] == 'W'){
				tmpcoord = -1 * tmpcoord;
			}

			position -> longitude = tmpcoord;


			memset(tmpbuf,0,sizeof(tmpbuf));
			strncpy(tmpbuf, gpsacpFields[0],2);
			timeStruct.tm_hour = atoi(tmpbuf);

			strncpy(tmpbuf, gpsacpFields[0] + 2,2);
			timeStruct.tm_min = atoi(tmpbuf);

			strncpy(tmpbuf, gpsacpFields[0] +4,2);
			timeStruct.tm_sec = atoi(tmpbuf);

			strncpy(tmpbuf, gpsacpFields[0] + 7,3);

			position -> time.tv_msec = atoi(tmpbuf);


			memset(tmpbuf,0,sizeof(tmpbuf));
			strncpy(tmpbuf, gpsacpFields[9],2);
			timeStruct.tm_mday = atoi(tmpbuf);
			strncpy(tmpbuf, gpsacpFields[9] + 2,2);
			timeStruct.tm_mon = atoi(tmpbuf) - 1; //months go from 0 to 11
			strncpy(tmpbuf, gpsacpFields[9] + 4,2);
			timeStruct.tm_year = 100 + atoi(tmpbuf); //years since 1900

#if 0
			/* ======= Getting Local Time Offset=======*/
			m2m_get_timeofday(&tv, &tz);
			localOffset = tz.tz_tzone * 15 * 60;  //number of offset seconds according to localtime
			//localOffset += tz.tz_dst * 3600;  //adding daylight saving seconds shift to the offset
			USB_Print(USB_CH_DEFAULT, "offset: %d\n", localOffset);
			//timeStruct.tm_sec += localOffset;
			timeStruct.tm_isdst = tz.tz_dst;

			USB_Print(USB_LOG, "seconds: %d\n", timeStruct.tm_sec);
#endif
			position -> time.tv_sec = mktime(&timeStruct);



            position -> altitude = strtod(gpsacpFields[4], NULL);
            position -> speed = strtod(gpsacpFields[7], NULL);
            position -> nsat = atoi(gpsacpFields[10]);
            position -> cog = strtod(gpsacpFields[6], NULL);  //course over ground
            position -> fix = 3;

            /*
            for (i=0; i < 11; i++)
            {
                USB_Print(USB_CH_DEFAULT, "gpsacpFields[%d]\t-> ", i);
                USB_Print(USB_CH_DEFAULT, "%s: %s\n", GPSACPFIELDS[i], gpsacpFields[i]);
            }
            */
        }
        else
        {
        	position -> fix = res;
        	return -2;
        }
    }
    return res;
}


void setRawResponse(char *response)
{
	strncpy(dwDATAMatrix[0], response, DW_FIELD_LEN);
}

char *getRawResponse(void)
{
	return dwDATAMatrix[0];
}










