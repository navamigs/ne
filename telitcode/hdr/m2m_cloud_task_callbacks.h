/*
 * m2m_cloud_task_callbacks.h
 *
 *  Created on: 01/dic/2014
 *      Author: FabioPi
 */

#ifndef M2M_CLOUD_TASK_CALLBACKS_H_
#define M2M_CLOUD_TASK_CALLBACKS_H_
	INT32 M2M_dwMsgReceive(INT32 type, INT32 param1, INT32 param2);
	INT32 M2M_dwParseMessage(INT32 type, INT32 param1, INT32 param2);
	INT32 M2M_dwMethodsHandlerTask(INT32 type, INT32 param1, INT32 param2);

	INT32 M2M_TestTask(INT32 type, INT32 param1, INT32 param2); //5Barz

#endif /* M2M_CLOUD_TASK_CALLBACKS_H_ */
