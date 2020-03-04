/*
 * m2m_cloud_methods_api.c
 *
 *  Created on: 01/dic/2014
 *      Author: FabioPi
 */
#include <stdlib.h>
#include <stdarg.h>

#include "m2m_cloud_methods_api.h"


int m2m_cloud_method_searchFieldValue(char buffer[DW_FIELDS_NUM][DW_FIELD_LEN], char *strToSearch, char result[DW_FIELD_LEN]){
    int i, ret = M2M_CLOUD_FAILURE;

    if ( !buffer || !result )
    {
    	dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_FALSE,"Not initialized arrays!");
		return M2M_CLOUD_UNALLOCATED_BUFFERS;
    }

    for (i=0; i< DW_FIELDS_NUM;i++){
    	if( ( i > M2M_CLOUD_METHOD_MSG_METHOD_KEY)  &&  ( strstr( buffer[i], strToSearch) ) ){
			memset(result, 0, DW_FIELD_LEN);
			strcpy(result, buffer[i + 1]);
			ret = M2M_CLOUD_SUCCESS;
		}
    }

    return ret;
}

int m2m_cloud_method_send_response(char* methodID, int status, int completionVars, ...)
{
	va_list args;
	int i, res= M2M_CLOUD_FAILURE;
	char cmd[512];
	char key[128];
	char value[128];

	va_start(args, completionVars);

	/* =======in case of failure, there is only an optional error message=======*/
	if (status != 0)
	{
		strcpy(value, va_arg( args, char * ));

		if (strlen(value) > 0)
		{
			sprintf(cmd, "AT#DWSEND=3,%s,%d,%s\r", methodID, status, value);
		}
		else
		{
			sprintf(cmd, "AT#DWSEND=3,%s,%d\r", methodID, status);
		}
	}

	/* ======in case of success, retrieve key value couples ======*/
	else
	{
		sprintf(cmd, "AT#DWSEND=3,%s,0", methodID );  //insert the beginning of the command string
		if( completionVars > 0)
		{
			for (i=0; i < completionVars; i++){
				strcpy(key, va_arg( args, char * ));
				strcpy(value, va_arg( args, char * ));
				//dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "key: %s, value: %s\n", key, value);

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
		strcat(cmd, "\r");
	}
	//dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "%s\n", cmd);

	if (strlen(cmd) > 400) return M2M_CLOUD_MAX_LENGTH_EXCEEDED;


	dwSendATCommand(cmd, M2M_FALSE);

	if (getLastMsgID() == 0)
	{ //check if this is the message response we are waiting for
		dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Correct MsgID.\n");

		if(getCloudRespState() == 0) //an OK was received
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

	return res;
}



