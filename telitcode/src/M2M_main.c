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
#include "m2m_ssl_api.h"
#include "m2m_sms_api.h"
#include "m2m_network_api.h"


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
 * DESCRIPTION:     M2M_main(...) is the main function and is the M2M application entry point.
 * 					This is the first function called by the control to start M2M application, you should write your code here.
 * 					Best practice would be to do the required initialization for the M2M application, and send messages
 *					to other tasks.
 *
 *
 * PARAMETERS:      The values arguments are read from the m2m_comfig file. The values arguments can be modified by the user
 *                  via the dedicated API.
 *
 * RETURNS:         None.
 *
 * PRE-CONDITIONS:  None.
 *
 * POST-CONDITIONS: None.
 *
 *
 * IMPORTANT NOTES: DO NOT BLOCK THIS FUNCTION: send messages to other M2M application tasks and return.
 * 					This function runs on the modem task and hence must not run any complex/heavy code.
 *
 * ============================================================================================== */

void M2M_main ( INT32 argc, CHAR argv[M2M_ARGC_MAX][M2M_ARGV_MAXTOKEN + 1] )
{

	//starting the task #1
	m2m_os_send_message_to_task(1, 0, (INT32)NULL, (INT32)NULL);

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

void M2M_suspend ( void )
{

 /* THIS FUNCTION IS NOT IMPLEMENTED, DO NOT USE IT */
             /* FOR INTERNAL USE ONLY */
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


void M2M_resume ( void )
{

 /* THIS FUNCTION IS NOT IMPLEMENTED, DO NOT USE IT */
           /* FOR INTERNAL USE ONLY */
}

