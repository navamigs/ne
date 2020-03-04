/*************************************************************************
                    5BARz International Incorporated.

                 (C) Copyright 2016. All Rights Reserved.

Filename: m2m_cloud_methods_handler.c

*************************************************************************/

#include "m2m_cloud_api.h"
#include "m2m_cloud_methods_api.h"


/*========================
 * the cloud_user_method.h header file contains user functions callable from m2m_cloud_method_handler() callback below.
 * It is an example only, the user logic could be directly written inside the m2m_cloud_method_handler() code .
 *======================= */
#include "cloud_user_method.h"
#ifdef FEATURE_5BARZ
#include "cmd_pkt.h"
#include "utilities.h"
#include "fs_file.h"

/*==================================================================================================
                            GLOBAL VARIABLES PROTOTYPES
==================================================================================================*/
M2M_CLOUD_GLOBALS globals;

#endif // FEATURE_5BARZ

/*===========================================================================================
 * handler for DeviceWise Cloud Methods execution. This CallBack will be automatically called
 * whenever the module (already connected to the cloud) receives a method execution message.
 * It is up to the user to implement its logic.
 * ==========================================================================================*/
int m2m_cloud_method_handler(char methodKey[DW_FIELD_LEN], char methodID[DW_FIELD_LEN], char methodDataFields[DW_FIELDS_NUM][DW_FIELD_LEN])
{
	int i, res = M2M_CLOUD_FAILURE, rows = 0;

	dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Starting method handler..\n");

	//print method fields
    for (i = 0; i < DW_FIELDS_NUM; i++)
	{
		if (methodDataFields[i][0] != '\0') {
			rows ++;
			dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Fields: %s\n", methodDataFields[i]);

		}
	}

    //Data elaboration and method execution..
    // fields: id, error, len, methodID, thing_def_key, method_def_key, notify_var_key, notify_var_value, notify_var_key, notify_var_value, notify_var_key, notify_var_value

    //check method key value, in order to choose which handler function should be used
 

  return res;
}
