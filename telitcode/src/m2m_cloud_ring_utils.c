/*
 * m2m_cloud_ring_utils.c
 *
 *  Created on: 01/dic/2014
 *      Author: FabioPi
 */

#include "m2m_cloud_ring_utils.h"
#include <stdlib.h>

// -- GLOBAL VARIABLES STRUCT -- //
extern M2M_CLOUD_GLOBALS globals;

void initDwRingStruct(M2M_CLOUD_DWRING_STRUCT *rs){
	rs->msgID = 0;
	rs->msgType = 0;
	rs->msgLen = 0;
}

int initDwRingQueue(void){
	int i, res = -1;
	M2M_CLOUD_DWRING_STRUCT rs;
	M2M_API_RESULT lockResult;

	initDwRingStruct( &rs );

	lockResult = lockDWRingQueueLock(10000);
	if (lockResult == M2M_API_RESULT_SUCCESS)
	{
		for (i = 0; i < 20; i++)
		{
			globals.dw_RING_QUEUE.ringQueue[i] = rs;
		}
		globals.dw_RING_QUEUE.last = 0;
		res = 1;
	}

	unLockDWRingQueueLock();

	dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "RingQueueLock unlocked.\n");
	return res;
}

int pushToRingQueue(M2M_CLOUD_DWRING_STRUCT rs){
	int res;
	lockDWRingQueueLock(10000);

	globals.dw_RING_QUEUE.ringQueue[globals.dw_RING_QUEUE.last] = rs;

	globals.dw_RING_QUEUE.last = (globals.dw_RING_QUEUE.last + 1 ) % 20;

	unLockDWRingQueueLock();
	dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "RingQueueLock unlocked.\n");

	return res;
}

int dwFillRingStruct(const char *str, M2M_CLOUD_DWRING_STRUCT *dwRing){
    char fields[DW_FIELDS_NUM][DW_FIELD_LEN];
    char *ringPtr = strstr(str,"#DWRING");

    if (ringPtr > 0)
    {
        dwParseFields(ringPtr + 9, (char **) fields, DW_FIELDS_NUM, DW_FIELD_LEN);

        dwRing -> msgType = atoi(fields[0]);
        dwRing -> msgID = atoi(fields[1]);
        dwRing -> msgLen = atoi(fields[2]);

        return 0;
    }
    else return -1;
}

int searchMsgTypeFromMsgIDInRingQueue(int msgId){
	int i;
	int res = -1;


	for (i = 0; i < 20; i++)
	{
		if (globals.dw_RING_QUEUE.ringQueue[i].msgID == msgId){
			res = globals.dw_RING_QUEUE.ringQueue[i].msgType;
		}
	}

	return res;
}

M2M_CLOUD_DWRING_STRUCT searchMsgIdInRingQueue(int msgId){
	int i;

	M2M_CLOUD_DWRING_STRUCT rs;
	initDwRingStruct( &rs );

	for (i = 0; i < 20; i++)
	{
		if (globals.dw_RING_QUEUE.ringQueue[i].msgID == msgId){
			rs = globals.dw_RING_QUEUE.ringQueue[i];
		}
	}

	return rs;
}


void setDWRingAction(M2M_CLOUD_DWRING_ACTION action)
{
	//TODO: lock!

	globals.DW_RING_ACTION = action;
	//printDebug(M2M_CLOUD_LOG_DEBUG, DW_FALSE,"Ring Action: %d\n", DW_RING_ACTION);

}

M2M_CLOUD_DWRING_ACTION getDWRingAction(void)
{
	return globals.DW_RING_ACTION;
}
