/*
 * m2m_cloud_methods_api.h
 *
 *  Created on: 14/gen/2015
 *      Author: FabioPi
 */

#ifndef M2M_CLOUD_METHODS_API_H_
#define M2M_CLOUD_METHODS_API_H_

#include "m2m_cloud_api.h"

/* =================================================================================================
 *
 * DESCRIPTION:     Handler for DeviceWise Cloud Methods.
 * 					This CallBack is automatically called if the module (already connected to the cloud) receives a method execution message.
 *
 * PARAMETERS:      methodKey: 	  		String containing the method key as defined in the thing definition of the module
 * 										(the same value can be obtained with methodDataFields[M2M_CLOUD_METHOD_MSG_METHOD_KEY]
 * 					methodID:			String containing the method instance alphanumeric identifier, which is needed to send an ack response to the cloud server after the method execution
 * 										(the same value can be obtained with methodDataFields[M2M_CLOUD_METHOD_MSG_METHOD_ID]
 * 					methodDataFields: 	String array containing the CSV method execution message splitted into single strings.
 * 										Every string thus contains a field of the csv dwdata message. Refer the M2M_CLOUD_METHOD_PARAMS enum for array indexes meaning.
 * 										N.B. Indexes after M2M_CLOUD_METHOD_MSG_METHOD_KEY will contain optional key-value couples of notification variables.
 *
 * RETURNS:         None.
 *
 * PRE-CONDITIONS:  None.
 *
 * POST-CONDITIONS: None.
 *
 * ============================================================================================== */
int m2m_cloud_method_handler(char methodKey[DW_FIELD_LEN], char methodID[DW_FIELD_LEN], char methodDataFields[DW_FIELDS_NUM][DW_FIELD_LEN]);




/* =================================================================================================
 *
 * DESCRIPTION:     Searches for a field key in the method data matrix, and if it exists, returns the corresponding value
 *
 * PARAMETERS:      buffer:			String array containing the CSV method execution message splitted into single strings.
 * 									Refer to methodDataFields in m2m_cloud_method_handler(...) function signature
 * 					strToSearch: 	String containing the name of the field to search.
 *                  fieldValue: 	Pointer to already allocated char buffer that will be filled with the value of the field.
 *
 * RETURNS:         M2M_CLOUD_SUCCESS in case of success, M2M_CLOUD_FAILURE in case of failure.
 * 					If success, fieldValue will point to the variable containing the wanted value
 *
 * PRE-CONDITIONS:  A method #DWDATA message must have been received earlier.
 *
 * POST-CONDITIONS: None.
 *
 * ============================================================================================== */
int m2m_cloud_method_searchFieldValue(char buffer[DW_FIELDS_NUM][DW_FIELD_LEN], char *strToSearch, char result[DW_FIELD_LEN]);




/* =================================================================================================
 *
 * DESCRIPTION:     Sends a response to the cloud server after a method execution.
 *
 * PARAMETERS:      methodID: 	  		String containing the temporary alphanumeric identifier of the executed method instance, needed to send an ack response to the cloud server after the method execution
 * 					status: 			Integer containing the exit status: if 0, no errors. Otherwise, it represents the error code
 *					completionVars:		Integer indicating the number of optional completion variables (key-value couples).
 *					[...]				The optional parameters are handled in different ways according on the status and completionVars parameters values
 *										- if status is not 0, completionVars will be ignored and the following parameter will contain the error message to send to the server. use "" (empty string) if a message is not needed.
 *										- if status is 0
 *											- if completionVars is > 0, the following parameters will be key-value String couples.
 *											  e.g. completionVars = 1, the function will expect 1 completion variable key and one completion variable value.
 *											- if completionVars is 0, the function will only send an ack message to the server, without any completion variables.
 *											  Any additional parameter will be ignored.
 *
 * RETURNS:         Refer to M2M_CLOUD_T_API_RETURNS enum.
 *
 * PRE-CONDITIONS:  A method #DWDATA message must have been received earlier.
 *
 * POST-CONDITIONS: None.
 *
 * ============================================================================================== */
int m2m_cloud_method_send_response(char* methodID, int status, int completionVars, ...);

#endif /* M2M_CLOUD_METHODS_API_H_ */
