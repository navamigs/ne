/*************************************************************************
                    5BARz International Incorporated. 

                 (C) Copyright 2016. All Rights Reserved.

Filename: utilities.h

Description: This header file contains interfaces to several utility 
functions to process various connection states, monitor activities, 
and control various state machines.

*************************************************************************/
#ifndef HDR_UTILITIES_H_
#define HDR_UTILITIES_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include "m2m_cloud_defines.h"

// Timer Handlers   
M2M_T_TIMER_HANDLE timer_perf_data_hdl;
M2M_T_TIMER_HANDLE timer_loc_data_hdl;
M2M_T_TIMER_HANDLE timer_restart;

/////////////////////////////////////////////////////////////////////////////
// Function: xstrtok()
//
// Parameters: string, delimiter and count
//
// Return: character pointer of the parameter string given
//
// Description: This function parses the location response from the telit.
//
/////////////////////////////////////////////////////////////////////////////
char *xstrtok(char *line, char *delims, int *count);
/////////////////////////////////////////////////////////////////////////////
// Function: Power_Monitor()
//
// Parameters: none
//
// Return: none
//
// Description: This function monitors the 12v line by sending an AT command 
// to read the ADC.  It waits about 30 seconds after boot-up (to give time
// to charge the super-caps, then queries every 5 seconds.
// 
/////////////////////////////////////////////////////////////////////////////
void Power_Monitor( void );
   
   
/////////////////////////////////////////////////////////////////////////////
// Function: Init_Seconds_timer()
//
// Parameters: none
//
// Return: bool -> true if sucessfull
//
// Description: This function the Initializes the System Timer 
// 
/////////////////////////////////////////////////////////////////////////////
bool Init_Seconds_timer( void );


/////////////////////////////////////////////////////////////////////////////
// Function: Start_Perf_Data_timer()
//
// Parameters: none
//
// Return: bool -> true if sucessfull
//
// Description: This function the Starts the Periodic Performance
// Data Timer.
//
/////////////////////////////////////////////////////////////////////////////
bool Start_Perf_Data_timer( void );


/////////////////////////////////////////////////////////////////////////////
// Function: Start_Loc_Data_timer()
//
// Parameters: none
//
// Return: bool -> true if sucessfull
//
// Description: This function the Starts the Periodic location
// Data Timer.
//
/////////////////////////////////////////////////////////////////////////////
bool Start_Loc_Data_timer( void );


/////////////////////////////////////////////////////////////////////////////
// Function: Start_shutdown_timer()
//
// Parameters: none
//
// Return: none
//
// Description: This function starts a timer that will shutdown the
// whole module (12V power loss).
// 
/////////////////////////////////////////////////////////////////////////////
void Start_shutdown_timer( void );


/////////////////////////////////////////////////////////////////////////////
// Function: seconds_running()
//
// Parameters: none
//
// Return: INT32 -> The number of seconds since boot-up
//
// Description: This function returns the number of seconds running 
// since boot-up.
// 
/////////////////////////////////////////////////////////////////////////////
UINT32 seconds_running( void );


/////////////////////////////////////////////////////////////////////////////
// Function: NE_debug()
//
// Parameters: *char -> debug string
//
// Return: response_t -> response enum type
//
// Description: This function tunnels a short (21 char total) debug string
// to the NE, and the NE displays it on its USB.  The NE's USB port must
// be unlocked and C34=191 or C34=0 to display this log message.
// 
// Note: the max string length is 21 chars, any chars after that will
// be truncated.
//
/////////////////////////////////////////////////////////////////////////////
response_t NE_debug( const char *str );


/////////////////////////////////////////////////////////////////////////////
// Function: NE_debug_var()
//
// Parameters: *char -> debug string
//              int  -> debug data
//
// Return: response_t -> response enum type
//
// Description: This function tunnels a short (21 char total) debug string
// to the NE, and the NE displays it on its USB.  The NE's USB port must
// be unlocked and C34=191 or C34=0 to display this log message.
// 
// Note: the max string length (str + data) is 21 chars.  Plan your
// str length and data length according.  Max str length is 20 chars.
//
/////////////////////////////////////////////////////////////////////////////
response_t NE_debug_var( const char *str, int data );


/////////////////////////////////////////////////////////////////////////////
// Function: Registration_Manager()
//
// Parameters: none
//
// Return: none
//
// Description: This function tracks the current state of network
// registration, roaming or not.
//
/////////////////////////////////////////////////////////////////////////////
void Registration_Manager( void );


/////////////////////////////////////////////////////////////////////////////
// Function: CellService()
//
// Parameters: none
//
// Return: bool -> true if module has cell service, wither home or roam
//
// Description: This function returns whether the module has cell service
// or not.
//
/////////////////////////////////////////////////////////////////////////////
bool CellService( void );


/////////////////////////////////////////////////////////////////////////////
// Function: Req_OTA_phone_num()
//
// Parameters: none
//
// Return: void
//
// Description: This function requests the phone OTA from the network
// via special GSM USSD codes.   This is needed for new SIM cards in
// India because some are programmed OTA.
//
// Note: do not call this function until Cell Service is established.
//
/////////////////////////////////////////////////////////////////////////////
void Req_OTA_phone_num( void );


/////////////////////////////////////////////////////////////////////////////
// Function: Req_location_info()
//
// Parameters: none
//
// Return: response_t -> response enum type
//
// Description: This function starts a PPP session and contacts
// Telit's location services for a location fix.
//
// Note: do not call this function until Cell Service is established.
//
/////////////////////////////////////////////////////////////////////////////
response_t Req_location_info( void );


/////////////////////////////////////////////////////////////////////////////
// Function: Alarm_Monitor()
//
// Parameters: none
//
// Return: none
//
// Description: This function monitors the NE for any Alarms once every
// 3 seconds, and notifies the RMS server accordingly.  It also looks
// for any extended internal TCA Alarms.
//
// Note: This function must always run off the main control loop, it
// queries the NE everys 3 seocnds, and this keeps the NE's special
// watchdog timer that monitors the Telit from resetting the Telit.
//
/////////////////////////////////////////////////////////////////////////////
void Alarm_Monitor( void );


/////////////////////////////////////////////////////////////////////////////
// Function: Timer_Perf_data_handler()
//
// Parameters: none
//
// Return: none
//
// Description: This function the Timer handler that is called to trigger
// a periodic performance data message to be send to the RMS.  This function
// is a call back and runs within the low level modem tasks.
//
/////////////////////////////////////////////////////////////////////////////
void Timer_Perf_data_handler( void *arg );


/////////////////////////////////////////////////////////////////////////////
// Function: Timer_Loc_data_handler()
//
// Parameters: none
//
// Return: none
//
// Description: This function the Timer handler that is called to trigger
// a periodic location message to be send to the RMS.
//
/////////////////////////////////////////////////////////////////////////////
void Timer_Loc_data_handler( void *arg );


/////////////////////////////////////////////////////////////////////////////
// Function: restart_handler()
//
// Parameters: none
//
// Return: none
//
// Description: This handler resets the module
//
/////////////////////////////////////////////////////////////////////////////
void restart_handler( void *arg );


/////////////////////////////////////////////////////////////////////////////
// Function: Init_lost_service_timer()
//
// Parameters: none
//
// Return: none
//
// Description: This function initializes a timer that tracks when GSM 
// service is lost.
// 
/////////////////////////////////////////////////////////////////////////////
void Init_lost_service_timer( void );


/////////////////////////////////////////////////////////////////////////////
// Function: Start_lost_service_timer()
//
// Parameters: none
//
// Return: none
//
// Description: This function starts a timer when GSM cell service is lost
// or never acquired after boot-up.
// 
/////////////////////////////////////////////////////////////////////////////
void Start_lost_service_timer( void );


#ifdef __cplusplus
}
#endif   
   
#endif /* HDR_UTILITIES_H_ */
