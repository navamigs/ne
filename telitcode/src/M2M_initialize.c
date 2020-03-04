/*==================================================================================================
                            INCLUDE FILES
==================================================================================================*/

#include <stdio.h>
#include <string.h>

#include "m2m_type.h"
#include "m2m_cb_app_func.h"
#include "m2m_clock_api.h"
#include "m2m_fs_api.h"
#include "m2m_hw_api.h"
#include "m2m_os_api.h"
#include "m2m_os_lock_api.h"
#include "m2m_socket_api.h"
#include "m2m_timer_api.h"
#include "m2m_i2c_api.h"

/*==================================================================================================
                            LOCAL CONSTANT DEFINITION
==================================================================================================*/

/*==================================================================================================
                            LOCAL TYPES DEFINITION
==================================================================================================*/

/*==================================================================================================
                            LOCAL FUNCTION PROTOTYPES
==================================================================================================*/

/*==================================================================================================
                            GLOBAL FUNCTIONS PROTOTYPES
==================================================================================================*/
extern void		M2M_main ( INT32 argc, CHAR argv[M2M_ARGC_MAX][M2M_ARGV_MAXTOKEN + 1] );

extern void 	M2M_suspend(void);		/* NOT YET IMPLEMENTED */
extern void		M2M_resume(void);		/* NOT YET IMPLEMENTED */

extern INT32	M2M_msgProc1(INT32 type, INT32 param1, INT32 param2);
extern void   	M2M_msgProcCompl ( INT8 procId, INT32 type, INT32 result );
extern INT32 	M2M_Cloud_onReceiveResultCmd(CHAR *atCmd, INT32 len, UINT16 logPort);
extern void 	M2M_onKeyEvent(INT32 val1, INT32 val2);
extern void 	M2M_onNetEvent (M2M_NETWORK_EVENT event, UINT32 param, M2M_SOCKET_BSD_IN6_ADDR *param1);
extern void   	M2M_onIP6RawEvent (UINT16 len);
extern void 	M2M_onWakeup(void);
extern void 	M2M_onHWTimer(INT32 timer_id);
extern void 	M2M_onInterrupt (INT32 gpio);

extern M2M_T_GPIO_CONFIG* M2M_initGPIO(INT32 *gpio_len);	/* NOT YET IMPLEMENTED */
extern INT32	M2M_onAppUpgradeAvailable (void);			/* NOT YET IMPLEMENTED */
extern void		M2M_onI2CEvent (M2M_I2C_EVENT event, UINT8 sda_pin, UINT8 scl_pin, UINT32 address, UINT8 reg_addr, UINT8* buffer, UINT32 length);/* NOT YET IMPLEMENTED */

extern void		M2M_onRegStatusEvent (UINT16 status,  UINT8* location_area_code, CHAR* cell_id,UINT16 Act);
extern void		M2M_onMsgIndEvent (CHAR* mem, UINT32 message_index);
extern INT32 	M2M_cmdShell (INT32 cmd, INT32 argc, CHAR argv[M2M_ARGC_MAX][M2M_ARGV_MAXTOKEN + 1] );

/*==================================================================================================

                            LOCAL MACROS
==================================================================================================*/

/*==================================================================================================
                            LOCAL VARIABLES
==================================================================================================*/

/*==================================================================================================
                            GLOBAL VARIABLES
==================================================================================================*/

/*==================================================================================================
                            LOCAL FUNCTIONS IMPLEMENTATION
==================================================================================================*/

/*==================================================================================================
                            GLOBAL FUNCTIONS IMPLEMENTATION
==================================================================================================*/

/* =================================================================================================
 *
 * DESCRIPTION:     DESCRIPTION:    THIS FUNCTION IS NOT IMPLEMENTED, DO NOT USE IT.
 *
 * PARAMETERS:      /
 *
 * RETURNS:         /
 *
 * PRE-CONDITIONS:  /
 *
 * POST-CONDITIONS: /
 *
 * IMPORTANT NOTES: /
 * ============================================================================================== */

INT32 M2M_onAppUpgradeAvailable (void)
{

	/* THIS FUNCTION IS NOT IMPLEMENTED, DO NOT USE IT */

	/* just to avoid warnings */
	return 0;
}

/* =================================================================================================
 *
 * DESCRIPTION:     Entry point of the M2M application
 *
 * PARAMETERS:      proc - structure consists on callback functions to be called from the Telit module (to the M2M).
 *
 * RETURNS:         None.
 *
 * PRE-CONDITIONS:  None.
 *
 * POST-CONDITIONS: None.
 *
 * IMPORTANT NOTES: Function name and prototype must not be changed otherwise application will
 *                  not run. Function names M2M_* can be changed according to the developer
 * ============================================================================================== */

void InitUserInterface(M2M_T_USER_CB *proc)
{
	proc->m2m_cb_initialize               = M2M_main;
	proc->m2m_cb_suspend                  = M2M_suspend;
	proc->m2m_cb_resume                   = M2M_resume;
	proc->m2m_cb_msg_proc_task[0]         = M2M_msgProc1;
	proc->m2m_cb_msg_proc_release         = M2M_msgProcCompl;
	proc->m2m_cb_on_receive_at_cmd_result = M2M_Cloud_onReceiveResultCmd;
	proc->m2m_cb_on_key_event             = M2M_onKeyEvent;
	proc->m2m_cb_on_net_event             = M2M_onNetEvent;   
	proc->m2m_cb_on_ip6_raw_event         = M2M_onIP6RawEvent;
	proc->m2m_cb_on_wakeup                = M2M_onWakeup;
	proc->m2m_cb_on_hw_timer              = M2M_onHWTimer;
	proc->m2m_cb_on_interrupt             = M2M_onInterrupt;
	proc->m2m_cb_init_gpio                = M2M_initGPIO;
	proc->m2m_cb_on_upgrade_available     = M2M_onAppUpgradeAvailable;
	proc->m2m_cb_on_i2c_event             = M2M_onI2CEvent;
	proc->m2m_cb_on_reg_status_event      = M2M_onRegStatusEvent;
    proc->m2m_cb_on_msg_ind_event         = M2M_onMsgIndEvent;
	proc->m2m_cb_shell                    = M2M_cmdShell;
}

/* =================================================================================================
 *
 * DESCRIPTION:     DESCRIPTION:    THIS FUNCTION IS NOT IMPLEMENTED, DO NOT USE IT.
 *
 * PARAMETERS:      /
 *
 * RETURNS:         /
 *
 * PRE-CONDITIONS:  /
 *
 * POST-CONDITIONS: /
 *
 * IMPORTANT NOTES: /
 * ============================================================================================== */

M2M_T_GPIO_CONFIG* M2M_initGPIO (INT32 *gpio_len)
{
  M2M_T_GPIO_CONFIG *pnt;

  /* THIS FUNCTION IS NOT IMPLEMENTED, DO NOT USE IT */

  /* just to avoid warnings */
  return pnt;
}
