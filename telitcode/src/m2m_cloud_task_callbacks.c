/*
 * m2m_cloud_task_callbacks.c
 *
 *  Created on: 17/nov/2014
 *      Author: FabioPi
 */

#include <stdlib.h>
#include "m2m_cloud_utils.h"
#include "m2m_cloud_methods_api.h"
#ifdef FEATURE_5BARZ
#include "cmd_pkt.h"
#include "fs_file.h"
#include "utilities.h"
#endif // FEATURE_5BARZ 

extern char dwDATAMatrix[DW_FIELDS_NUM][DW_FIELD_LEN];

M2M_CLOUD_GLOBALS globals; //5Barz

INT32 M2M_dwMsgReceive(INT32 type, INT32 param1, INT32 param2)
{
  char  *start;
	char* response = (char*)param1;
	M2M_CLOUD_DWSTATUS dwStatus;

	//dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Response:%s", response);

	if(checkATResponse(response, 5, "OK\r\n", "CONNECT\r\n", ">", "#DWDATA", "#DWRDATA"))
	{
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Response:%s", response ); //module response
	  setCloudRespState(0);
	}
	else if(checkATResponse(response, 3, "BUSY\r\n", "ERROR\r\n", "NO CARRIER\r\n"))
	{
	  dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_TRUE, "%s", response ); //module response
	  setCloudRespState(-1);
	}


	if(strstr(response, "#DWRING"))
	{
		m2m_os_send_message_to_task(getM2M_Cloud_ParseMessageTaskID(), M2M_CLOUD_DWRING_TYPE, (long)response, getDWRingAction());
	}
	else if(strstr(response, "#DWDATA"))
	{
		m2m_os_send_message_to_task(getM2M_Cloud_ParseMessageTaskID(), M2M_CLOUD_DWDATA_TYPE, (long)response, (INT32)NULL);
	}
	else if(strstr(response, "#DWRDATA")){
		m2m_os_send_message_to_task(getM2M_Cloud_ParseMessageTaskID(), M2M_CLOUD_DWRDATA_TYPE, (long)response, (INT32)NULL);
	}
	else if (strstr(response, "#DWSEND:") || strstr(response, "#DWSENDR:") )
	{
		//retrieves the last msgID
		setLastMsgID(getdwSendMsgID(response));
	}
	else if (strstr(response, "#SSLEN:")){
		lockDWDATALock(10000);
		strcpy(dwDATAMatrix[0],response);
	}
	else if(strstr(response, "#CGSN:")) // Get IMEI
	{
	  m2m_os_send_message_to_task(getM2M_Cloud_ParseMessageTaskID(), M2M_CLOUD_AT_CMD_CGSN_TYPE, (long)response, (INT32)NULL);
	}
	else if(strstr(response, "#CGMM:")) // Get Model Designation
	{
	  m2m_os_send_message_to_task(getM2M_Cloud_ParseMessageTaskID(), M2M_CLOUD_AT_CMD_CGMM_TYPE, (long)response, (INT32)NULL);
	}
	else if(strstr(response, "#CGMR:")) // Get Firmware
	{
	  m2m_os_send_message_to_task(getM2M_Cloud_ParseMessageTaskID(), M2M_CLOUD_AT_CMD_CGMR_TYPE, (long)response, (INT32)NULL);
	}
    else if( start = strstr(response, "#QSS:")) // SIM Present
	{
    // In this case, we process the reponse string here.  This eliminates problems observed
    // with false "no SIM" errors on boot-up caused by transient unsolicited AT commands - 5Barz
    if( *(start+8) == NUM1 ) globals.module.SIM_present = true;
	}
    else if(strstr(response, "#ADC:")) // Get 12V A/D reading
	{
	  m2m_os_send_message_to_task(getM2M_Cloud_ParseMessageTaskID(), M2M_CLOUD_AT_CMD_ADC_TYPE, (long)response, (INT32)NULL);
	}
  else if(strstr(response, "+CNUM:"))
  {
	  m2m_os_send_message_to_task(getM2M_Cloud_ParseMessageTaskID(), M2M_CLOUD_AT_CMD_CNUM_TYPE, (long)response, (INT32)NULL);
  }
  else if(strstr(response, "+CUSD:")) // Get Phone_number using ussd codes
  {
  	m2m_os_send_message_to_task(getM2M_Cloud_ParseMessageTaskID(), M2M_CLOUD_AT_CMD_MOB_TYPE, (long)response, (INT32)NULL);
 	}
  else if(strstr(response, "#AGPSRING:")) // Get location
  {
    m2m_os_send_message_to_task(getM2M_Cloud_ParseMessageTaskID(), M2M_CLOUD_AT_CMD_LOC_TYPE, (long)response, (INT32)NULL);
  }
  else if(strstr(response, "#ADC:")) // Get location
  {
    m2m_os_send_message_to_task(getM2M_Cloud_ParseMessageTaskID(), M2M_CLOUD_AT_CMD_ADC_TYPE, (long)response, (INT32)NULL);
  }
    
	//ipAddr = dwGetIpAddress(response, ipAddr);

	dwStatus = dwCheckStatus(response);
	if ( dwStatus > DW_ERROR )  // -1 => not a DWSTATUS response
	{
		setDWStatus(dwStatus);
		unLockDWStatusLock();
	}

	return 0;
}





INT32 M2M_dwParseMessage(INT32 type, INT32 param1, INT32 param2)
{
  char *start, *end;
  char *response = (char*)param1;
  char string[100];
  M2M_CLOUD_DWRING_STRUCT dwRingStruct;
  M2M_T_NETWORK_CURRENT_NETWORK curr_net;
  char *array[7];
  char dwRcvBuff[30];
  int  i=0, dwRing, msgType, val=0,pos;
  DW_GPSPositionData loc;
  int ret = -1;
  msg_queue_t msg;
  static UINT32 sec;
#ifdef FEATURE_5BARZ
  char str[GENERAL_RETURN_STR_LEN];
#endif // FEATURE_5BARZ   


	switch(type)
	{
		case M2M_CLOUD_DWRING_TYPE:
			dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "RSP--%s", response ); //module response

			dwRing = dwFillRingStruct(response, &dwRingStruct );
			if (dwRing == 0 )
			{
					ret = dwRingStruct.msgID;
					msgType = dwRingStruct.msgType;
					pushToRingQueue( dwRingStruct );

					/*Get the lock to avoid concurrent writings in the fields structure*/
					lockDWDATALock(10000);

					switch(param2)
					{
						case M2M_CLOUD_AUTORETRIEVE:  //automatically retrieves the message from the queue, thus making the corresponding queue slot free for another message.
							if (msgType == 2) //raw JSON message
							{
								sprintf(dwRcvBuff,"AT#DWRCVR=%d\r", ret);
								dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "RCV--Receiving RAW DW message.. %s\n", dwRcvBuff );
							}
							else
							{
								sprintf(dwRcvBuff,"AT#DWRCV=%d\r", ret);
								dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "RCV--Receiving DW message.. %s\n", dwRcvBuff );
							}

							dwSendATCommand(dwRcvBuff, M2M_TRUE);
							break;
						case M2M_CLOUD_LEAVE: //messages are left in queue
							dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "RCV--DW msgID: %d\n", ret );
							break;
					}
			}
			break;

		case M2M_CLOUD_DWDATA_TYPE:

			ret = dwParseDWData(response);

			if(ret == 1)
			{
				dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Parsing dwdata\n", response ); //module response

				//searching in the RingQueue for the present received message
				msgType = searchMsgTypeFromMsgIDInRingQueue(atoi(dwDATAMatrix[0]));

				dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "MsgType: %d\n", msgType );

				switch(msgType)
				{
					case 0: //response to a type 0 format message (api command execution message)
							/*Unlocking the DWDATA lock (locked by case M2M_CLOUD_DWRING_TYPE ), now the global data structure will be available for the next writing*/
						unLockDWDATALock();
						break;
					case 1:
						dwPrintDebug(M2M_CLOUD_LOG_VERBOSE, M2M_FALSE, "DWDATA-- Type: 0; MsgID: %s; Error: %s; Len: %s\nDATA: ", dwDATAMatrix[0],  dwDATAMatrix[1], dwDATAMatrix[2]);
						if (getDWDebugState() >= M2M_CLOUD_LOG_VERBOSE)
						{
							for (i=3; i< DW_FIELDS_NUM; i++)
							{
								if (dwDATAMatrix[i][0] != '\0') {
									internalPrint("%s | ", dwDATAMatrix[i]);

								}
							}
							internalPrint("\n");

						}

						if( getLastMsgID() != 0)
						{
							unLockDWSendLock();
						}

						break;
					case 2:
						/*Unlocking the DWDATA lock (locked by case M2M_CLOUD_DWRING_TYPE ), now the global data structure will be available for the next writing*/
						unLockDWDATALock();
						break;

					case 3: //Message incoming from method execution request
						dwPrintDebug(M2M_CLOUD_LOG_VERBOSE, M2M_FALSE, "Method execution request incoming!\n");
						dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"Free Space at start: %d\n", m2m_os_get_mem_info((UINT32 *)NULL));
						m2m_os_send_message_to_task(getM2M_Cloud_MethodsHandlerTaskID(), 0, (INT32)NULL, (INT32)NULL);

						break;
					default:
						dwPrintDebug(M2M_CLOUD_LOG_ERRORS,M2M_FALSE, "Error! Unable to recognize DWDATA Message Type! Value: %d", dwDATAMatrix[0]);
						unLockDWDATALock();
						break;
				}
			}
			break;

		case M2M_CLOUD_DWRDATA_TYPE:

			if( getLastMsgID() != 0)
			{
				dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"raw message: %s\n", response);
				setRawResponse(response);
				unLockDWSendLock();
			}
			break;

    case M2M_CLOUD_AT_CMD_CGSN_TYPE:  // Get IMEI
      start = strstr(response,"#CGSN: "); // 7 chars
      if(start)
      {
        end = strstr(start, "\r");
        if( end )
        {
          if( end-start-7 < IMEI_STR_LEN )
          {
            strncpy(globals.module.imei, start + 7, end-start-7 );      
            dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"IMEI=%s", globals.module.imei);
            sprintf( globals.module.imei_quote, "\"%s\"", globals.module.imei );
            dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"IMEIq=%s", globals.module.imei_quote);
            ret = 1;
            break;
          }
        }
      }
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Could Not Parse AT#CGSN");
      break;
      
    case M2M_CLOUD_AT_CMD_CGMM_TYPE:  // Get Model
      start = strstr(response,"#CGMM: "); // 7 chars
      if(start)
      {
        end = strstr(start, "\r");
    	  if(end)
        {
          if( end-start-7 < MODEL_STR_LEN )
          {
    		    strncpy(globals.module.model, start + 7, end-start-7 );      
            dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Model=%s", globals.module.model);
            ret = 1;
            break;
          }
        }
      }
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Could Not Parse AT#CGMM");
      break;
      
    case M2M_CLOUD_AT_CMD_CGMR_TYPE:  // Get Firmware
      start = strstr(response,"#CGMR: "); // 7 chars
      if(start)
      {
        end = strstr(start, "\r");
        if(end)
        {
          if( end-start-7 < FIRMWARE_STR_LEN )
          {
            strncpy(globals.module.firmware, start + 7, end-start-7 );      
            dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Firmware=%s", globals.module.firmware);
            ret = 1;
            break;
          }
        }
      }
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Could Not Parse AT#CGMR");
      break;

    case M2M_CLOUD_AT_CMD_ADC_TYPE:  // Get 12V A/D reading
      start = strstr(response,"#ADC: "); // 6 chars
      if( start )
      {
        end = strstr(start, "\r");
        if( end && strlen(start+6) < GENERAL_RETURN_STR_LEN )
        {  
          strncpy( str, start + 6, end-start-6 );
          val = atoi( str );
          if( val < ADC_THRESHOLD_12V )
          {
            if( globals.power_state == PWR_STATE_12V_ON )
            {  
              dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Lost 12V, Going to Wait state");
              globals.power_state = PWR_STATE_12V_OFF_WAIT;
              sec = globals.module.sec_running;
            }
            else if( globals.power_state == PWR_STATE_12V_OFF_WAIT )
            {
              if( globals.module.sec_running >= sec + LOST_POWER_WAIT_TIME ) // ~10 seconds
              {
                dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Lost 12V, Done waiting, Shut Off state");
                globals.power_state = PWR_STATE_12V_OFF_SHUTDOWN;
                msg.cmd = MSQ_QUEUE_CMD_12V_LOST;
                PushToMsgQueue( msg );
              }
              else dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Lost 12V, In Waiting state");
            }
          }
          else // 12V Power Good:
          {  
            if( globals.power_state == PWR_STATE_12V_OFF_WAIT )
            {  
              dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"12V Restored, cancel Wait state");
              globals.power_state = PWR_STATE_12V_ON;
            }
          }
          dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Voltage1=%d",val);
          ret = 1;
          break;
        }
      }
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Could Not Parse AT#ADC=1,2");
      break;
  
    case M2M_CLOUD_AT_CMD_CNUM_TYPE: // Get the Phone Number  
      start = strstr(response,"+CNUM:"); // 7 chars
      if( start )
      {
        start = strstr( start,",\"" );
        if( start )
        {
          start += 2; // skip over ,"
          end = strstr( start, "\"" );
          if( end && ((end-start) < PHONE_NUM_LEN) )
          {
            strncpy( globals.module.phone_num, start, end-start ); 
            dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"CNUM=%s",globals.module.phone_num);
            ret = 1;
            break;
          }
        }
      }
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Could Not Parse AT+CNUM:->%s<-",response);
      break;
      
    case M2M_CLOUD_AT_CMD_MOB_TYPE:  // Get Phone_number via network
      start = strstr(response,"+CUSD:"); // 6 chars
      if( start )
      {
        dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "response_print--%s",response);
        dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Operator:%s.\n",globals.module.Operator);
        if(strncmp(globals.module.Operator,"VODAFONE",8)==0)
        {
          dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "response--%s",response);
          end = strstr( response, "for");
          if(end)
          {
            strncpy(dwRcvBuff,end+4,10);
            memset( globals.module.phone_num, 0, sizeof( globals.module.phone_num ));
            sprintf( globals.module.phone_num, "%s", dwRcvBuff );
            dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "phone_num--%s",globals.module.phone_num);
          }
        }
        else if(strncmp(globals.module.Operator,"AIRTEL",6)==0)
        {
          dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "response--%s",response);
          end = strstr( response, "is");
          if(end)
          {
            strncpy(dwRcvBuff,end+4,10);
            memset( globals.module.phone_num, 0, sizeof( globals.module.phone_num ));
            sprintf(globals.module.phone_num,"%s",dwRcvBuff);
            dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "phone_num--%s",globals.module.phone_num);
          }
        }
        ret = 1;
        break;
      }
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Could Not Parse AT+CUSD");
      break;

    case M2M_CLOUD_AT_CMD_LOC_TYPE:  // Get location
    	start=strstr(response,"#AGPSRING: ");
    	if( start )
    	{
    	dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "response:%s\n",response);
    	memset( string, 0, sizeof(string));
    	strncpy( string, response, strlen(response));
    	end=xstrtok(string, ",",&pos);
    	while(end!= NULL)
    	{
    		  if(pos==0 )
    		 {
    		   i++;
    		   if(i==2) // if we get 2 comma's without response
    		   {
    			 dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Location not found\n");
    			 NE_debug("No location found");
    		     break;
    		   }
    		 }
    	 array[val++] =end;
    	 end=xstrtok(NULL, ",",&pos);
    	}
    	if(pos!=0)
    	{
		strcpy(globals.module.location.latitude,array[1]);
        strcpy(globals.module.location.longitude,array[2]);
        globals.module.location.accuracy=atoi(array[4]);
		dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "latitude response at read:%s\n", globals.module.location.latitude);
		dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "longitude response at read: %s\n",globals.module.location.longitude);
		dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "accuracy response at read: %d\n",globals.module.location.accuracy);
		break;
    	}
      }

		dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Could Not Parse AT#AGPSSND");
		break;
  }

  return ret;
}



INT32 M2M_dwMethodsHandlerTask(INT32 type, INT32 param1, INT32 param2)
{
	int res;
	char (*dwMethodMatrix)[DW_FIELD_LEN] = m2m_os_mem_alloc(sizeof(char) * DW_FIELDS_NUM * DW_FIELD_LEN);

	memcpy(dwMethodMatrix, dwDATAMatrix, sizeof(dwDATAMatrix));
	dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"Free Space after allocation: %d\n", m2m_os_get_mem_info((UINT32 *)NULL));

	/*Unlocking the DWDATA lock (locked by M2M_dwParseMessage() ), now the global data structure will be available for the next writing*/
	unLockDWDATALock();

	res = m2m_cloud_method_handler(dwMethodMatrix[M2M_CLOUD_METHOD_MSG_METHOD_KEY], dwMethodMatrix[M2M_CLOUD_METHOD_MSG_METHOD_ID], dwMethodMatrix);

	m2m_os_mem_free(dwMethodMatrix);

	dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"Free Space: %d\n", m2m_os_get_mem_info((UINT32 *)NULL));


	return res;

}
