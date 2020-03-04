/*==================================================================================================
                            INCLUDE FILES
==================================================================================================*/

#include <stdio.h>
#include <string.h>

#include "m2m_type.h"
#include "m2m_clock_api.h"
#include "m2m_fs_api.h"
#include "m2m_hw_api.h"
#include "m2m_os_api.h"
#include "m2m_os_lock_api.h"
#include "m2m_socket_api.h"
#include "m2m_timer_api.h"


#include "m2m_cloud_api.h"

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
 * DESCRIPTION:     in general, when the modem makes data available on its output buffer connected to the virtual
 *					serial port identified by "logPort", the M2M_onReceiveResultCmd callback is automatically
 *					started to provide the data to the M2M application. 
 *					It is responsibility of the programmer to write, within the M2M_onReceiveResultCmd function, the
 *					code to fulfill the requirements of the M2M application.
 *					The callback works in both Command and Data mode.
 *
 * PARAMETERS:      atCmd:		data string received from the module.
 *                  len:		length of data string
 *					logPort:	identifies the virtual serial port, see m2m_os_iat_set_at_command_instance function. 
 *
 * RETURNS:         1
 *
 * PRE-CONDITIONS:  None.
 *
 * POST-CONDITIONS: None.
 *
 * IMPORTANT NOTES: This function runs on the modem task (no M2M tasks) and hence must not run any complex/heavy code.
 * 					Use M2M tasks ( M2M_proc1, M2M_proc2, etc. ) to handle a large amount of data.
 * ============================================================================================== */

INT32 M2M_onReceiveResultCmd(CHAR *atCmd, INT32 len, UINT16 logPort)
{
	/* Write code fulfilling the requirements of the M2M project */


  /* just to avoid the warnings */
  return 1;
}



