/*
 * m2m_cloud_ring_utils.h
 *
 *  Created on: 01/dic/2014
 *      Author: FabioPi
 */

#ifndef M2M_CLOUD_RING_UTILS_H_
#define M2M_CLOUD_RING_UTILS_H_

	#include "m2m_cloud_utils.h"

	/*	RINGS UTILITIES	*/

	int initDwRingQueue(void);
	void initDwRingStruct(M2M_CLOUD_DWRING_STRUCT *rs);
	int pushToRingQueue(M2M_CLOUD_DWRING_STRUCT rs);
	int dwFillRingStruct(const char *str, M2M_CLOUD_DWRING_STRUCT *dwRing);
	int searchMsgTypeFromMsgIDInRingQueue(int msgId);
	M2M_CLOUD_DWRING_STRUCT searchMsgIdInRingQueue(int msgId);

	void setDWRingAction(M2M_CLOUD_DWRING_ACTION action);
	M2M_CLOUD_DWRING_ACTION getDWRingAction(void);

#endif /* M2M_CLOUD_RING_UTILS_H_ */
