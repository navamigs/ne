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
#include "m2m_sms_api.h"
#include "m2m_network_api.h"


#include "m2m_cloud_defines.h" // 5Barz
#include "ftp.h"               // 5Barz
#include "sms.h"               // 5Barz
#include "m2m_cloud_types.h"   // 5Barz


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
extern M2M_T_OS_LOCK     PDPLock;

/*==================================================================================================
                            LOCAL FUNCTIONS IMPLEMENTATION
==================================================================================================*/

/*==================================================================================================
                            GLOBAL FUNCTIONS IMPLEMENTATION
==================================================================================================*/

/* =================================================================================================
 *
 * DESCRIPTION:     Handles events related to PDP/GPRS connection. Refer to m2m_pdp_activate() API
 *
 * PARAMETERS:      event: event id.
 *                  param: one of two:
 *                  1. In case of socket event (M2M_SOCKET_EVENT_SOCKET_BREAK,M2M_SOCKET_EVENT_SOCKET_FAIL), the socket ID.
 *                  2. In case of PDP M2M_SOCKET_EVENT_PDP_ACTIVE event, the ip address.
 *                  param1: In case of PDP M2M_SOCKET_EVENT_PDP_IPV6_ACTIVE event, the ipV6 address
 *
 * RETURNS:         None.
 *
 * PRE-CONDITIONS:  None.
 *
 * POST-CONDITIONS: None.
 *
 * IMPORTANT NOTES: This function support multiple sockets.
 * 					This function runs on the modem task and hence must not run any complex/heavy code.
 * ============================================================================================== */
 
void M2M_onNetEvent (M2M_NETWORK_EVENT event, UINT32 param, M2M_SOCKET_BSD_IN6_ADDR *param1)
{

  switch (event)  /* PDP related*/
  { 
    case M2M_SOCKET_EVENT_PDP_ACTIVE:
      dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"M2M_SOCKET_EVENT_PDP_ACTIVE.\n");
      m2m_os_lock_unlock(PDPLock);
      break;
      
    case M2M_SOCKET_EVENT_PDP_BREAK:
      dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"M2M_SOCKET_EVENT_PDP_BREAK\n");
      break;
      
    case M2M_SOCKET_EVENT_PDP_DEACTIVE:
      dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"M2M_SOCKET_EVENT_PDP_DEACTIVE\n");
      break;
      
    case M2M_SOCKET_EVENT_PDP_ACTIVATION_FAIL:
      dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"M2M_SOCKET_EVENT_PDP_ACTIVATION_FAIL!\n");
      m2m_os_lock_unlock(PDPLock);
      break;
      
    case M2M_SOCKET_EVENT_PDP_IPV6_ACTIVE:
      dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"M2M_SOCKET_EVENT_PDP_IPV6_ACTIVE: %s\n",
      m2m_socket_bsd_addr_str_ip6((M2M_SOCKET_BSD_IPV6_ADDR *)param1));
      break;
      
    default:
      dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"M2M NET EVENT: %d\n", event);
  }
}


/* =================================================================================================
 *
 * DESCRIPTION:     Handles events indicating the registration status.
 *					Refer to m2m_network_enable_registration_location_unsolicited() API.
 *
 * PARAMETERS:      status:				1 for success.
 *                  location_area_code:	Location Area Code
 *                  cell_id:			cell ID
 *
 * RETURNS:         None.
 *
 * PRE-CONDITIONS:  None.
 *
 * POST-CONDITIONS: None.
 *
 * IMPORTANT NOTES: This function runs on the modem task and hence must not run any complex/heavy code.
 * ============================================================================================== */
 
void M2M_onRegStatusEvent ( UINT16 status,  UINT8* location_area_code, CHAR* cell_id, UINT16 Act )
{

	/* write code fulfilling the requirements of the M2M project */

}



/* =================================================================================================
 *
 * DESCRIPTION:     Handles events indicating an SMS has been received.
 *					Refer to m2m_sms_enable_new_message_indication () API
 *
 * PARAMETERS:      mem:			message storage
 *                  message_index:	message index
 *
 * RETURNS:         None.
 *
 * PRE-CONDITIONS:  None.
 *
 * POST-CONDITIONS: None.
 *
 * IMPORTANT NOTES: This function runs on the modem task and hence must not run any complex/heavy code.
 * ============================================================================================== */
 
void M2M_onMsgIndEvent ( CHAR *mem,  UINT32 message_index )
{
#ifdef FEATURE_5BARZ
  mt_sms_callback( mem, message_index ); 
#endif // FEATURE_5BARZ   
}

/* =================================================================================================
 *
 * DESCRIPTION:     Handles events related to IPv6 raw
 *
 * PARAMETERS:
 *                  param:  length of the received packet
 *
 * RETURNS:         None.
 *
 * PRE-CONDITIONS:  None.
 *
 * POST-CONDITIONS: None.
 *
 * This function runs on the modem task and hence must not run any complex/heavy code.
 * ============================================================================================== */
void M2M_onIP6RawEvent (UINT16 len)
{  
}

