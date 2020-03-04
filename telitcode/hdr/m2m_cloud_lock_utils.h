/*
 * m2m_cloud_lock_utils.h
 *
 *  Created on: 01/dic/2014
 *      Author: FabioPi
 */

#ifndef M2M_CLOUD_LOCK_UTILS_H_
#define M2M_CLOUD_LOCK_UTILS_H_

	#include "m2m_cloud_utils.h"


/*  	 LOCKS  FUNCTIONS 	   */

	int initTestTaskLock(void); //5Barz
	M2M_API_RESULT lockTestTaskWait(UINT32 timeout);
	M2M_API_RESULT lockTestTaskLock(void);
	M2M_API_RESULT unLockTestTaskLock(void);


	int initDWStatusLock(void);
	M2M_API_RESULT waitDWStatusValue(UINT32 timeout);
	M2M_API_RESULT unLockDWStatusLock(void);

	int initDWDATALock(void);
	M2M_API_RESULT lockDWDATALock(UINT32 timeout);
	M2M_API_RESULT unLockDWDATALock(void);

	int initDWSendLock(void);
	M2M_API_RESULT waitDWSendLock(UINT32 timeout);
	M2M_API_RESULT unLockDWSendLock(void);

	int initDWRingQueueLock(void);
	M2M_API_RESULT lockDWRingQueueLock(UINT32 timeout);
	M2M_API_RESULT unLockDWRingQueueLock(void);

	int initDWMethodsLock(void);
	M2M_API_RESULT lockDWMethodsLock(UINT32 timeout);
	M2M_API_RESULT unLockDWMethodsLock(void);


#endif /* M2M_CLOUD_LOCK_UTILS_H_ */
