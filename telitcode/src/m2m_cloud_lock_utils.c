/*
 * m2m_cloud_lock_utils.c
 *
 *  Created on: 01/dic/2014
 *      Author: FabioPi
 */
#include "m2m_cloud_lock_utils.h"

/*  	 LOCKS  VARIABLES 	   */

M2M_T_OS_LOCK 	dwSendLock;
M2M_T_OS_LOCK 	dwStatusLock;
M2M_T_OS_LOCK	dwDATALock;
M2M_T_OS_LOCK	dwRingQueueLock;
M2M_T_OS_LOCK	dwMethodsLock;

M2M_T_OS_LOCK   TestTaskLock;  //5Barz
int initTestTaskLock(void)
{
	TestTaskLock = m2m_os_lock_init(M2M_OS_LOCK_IPC); //M2M_OS_LOCK_IPC
	return (TestTaskLock)? M2M_CLOUD_SUCCESS: M2M_CLOUD_FAILURE;
}
M2M_API_RESULT lockTestTaskLock(void)
{
	return m2m_os_lock_lock(TestTaskLock);
}
M2M_API_RESULT lockTestTaskWait(UINT32 timeout)
{
	return m2m_os_lock_wait(TestTaskLock, timeout);
}
M2M_API_RESULT unLockTestTaskLock(void)
{
	return m2m_os_lock_unlock(TestTaskLock);
}




/*  	 LOCKS  FUNCTIONS 	   */

int initDWStatusLock(void)
{
	dwStatusLock = m2m_os_lock_init(M2M_OS_LOCK_IPC);
	return (dwStatusLock)? M2M_CLOUD_SUCCESS: M2M_CLOUD_FAILURE;
}

M2M_API_RESULT waitDWStatusValue(UINT32 timeout)
{
	return m2m_os_lock_wait(dwStatusLock, timeout);
}

M2M_API_RESULT unLockDWStatusLock(void)
{
	return m2m_os_lock_unlock(dwStatusLock);
}

/////

int initDWDATALock(void)
{
	dwDATALock = m2m_os_lock_init(M2M_OS_LOCK_CS);
	return (dwDATALock)? M2M_CLOUD_SUCCESS: M2M_CLOUD_FAILURE;
}

M2M_API_RESULT lockDWDATALock(UINT32 timeout)
{
	return m2m_os_lock_wait(dwDATALock, timeout);
}

M2M_API_RESULT unLockDWDATALock(void)
{
	return m2m_os_lock_unlock(dwDATALock);
}


/////

int initDWSendLock(void)
{
	dwSendLock = m2m_os_lock_init(M2M_OS_LOCK_IPC);
	return (dwSendLock)? M2M_CLOUD_SUCCESS: M2M_CLOUD_FAILURE;
}

M2M_API_RESULT waitDWSendLock(UINT32 timeout)
{
	return m2m_os_lock_wait(dwSendLock, timeout);

}

M2M_API_RESULT unLockDWSendLock(void)
{
	return m2m_os_lock_unlock(dwSendLock);
}


/////

int initDWRingQueueLock(void)
{
	dwRingQueueLock = m2m_os_lock_init(M2M_OS_LOCK_CS);
	return (dwRingQueueLock)? M2M_CLOUD_SUCCESS: M2M_CLOUD_FAILURE;
}

M2M_API_RESULT lockDWRingQueueLock(UINT32 timeout)
{
	return m2m_os_lock_wait(dwRingQueueLock, timeout);

}

M2M_API_RESULT unLockDWRingQueueLock(void)
{
	return m2m_os_lock_unlock(dwRingQueueLock);
}


/////

int initDWMethodsLock(void)
{
	dwMethodsLock = m2m_os_lock_init(M2M_OS_LOCK_CS);
	return (dwMethodsLock)? M2M_CLOUD_SUCCESS: M2M_CLOUD_FAILURE;
}

M2M_API_RESULT lockDWMethodsLock(UINT32 timeout)
{
	return m2m_os_lock_wait(dwMethodsLock, timeout);

}

M2M_API_RESULT unLockDWMethodsLock(void)
{
	return m2m_os_lock_unlock(dwMethodsLock);
}
