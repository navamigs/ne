#ifndef M2M_CLOUD_METHODS_UTILS_H_
#define M2M_CLOUD_METHODS_UTILS_H_
/*************************************************************************
                    5Barz International Incorporated. 

                 (C) Copyright 2016. All Rights Reserved.

Filename: cloud_user_method.h

Description: This header file contains interfaces to functions that
handle data exchange with the Cloud for the Client App.


*************************************************************************/

#ifdef __cplusplus
 extern "C" {
#endif

#include "m2m_cloud_methods_api.h"

#ifdef FEATURE_5BARZ
#include <stdbool.h>   
  


/////////////////////////////////////////////////////////////////////////////
// Function: Reset_NE()
//
// Parameters: none
//
// Return: response_t -> response enum type
//
// Description: This function sends a command packet to reset the NE,
// which usually resets the Telit module too, but not always.
//
/////////////////////////////////////////////////////////////////////////////
response_t Reset_NE( void );


/////////////////////////////////////////////////////////////////////////////
// Function: Reset_Module()
//
// Parameters: int -> delay
//
// Return: response_t -> response enum type
//
// Description: This function sends a command packet to NE that causes
// a power down and power up cycle of the Telit Module.  Takes 10 seconds.
//
/////////////////////////////////////////////////////////////////////////////
response_t Reset_Module( int reset_delay );


/////////////////////////////////////////////////////////////////////////////
// Function: Cloud_Enable_NE()
//
// Parameters: none
//
// Return: response_t -> response enum type
//
// Description: This function sends a command packet to enable the NE.
//
/////////////////////////////////////////////////////////////////////////////
response_t Cloud_Enable_NE( void );


/////////////////////////////////////////////////////////////////////////////
// Function: Cloud_Enable_NE_A_only()
//
// Parameters: none
//
// Return: response_t -> response enum type
//
// Description: This function sends a command packet to enable only Band A
// of the NE.  Band B will be disabled.   This function only works for dual
// band NEs only.
//
/////////////////////////////////////////////////////////////////////////////
response_t Cloud_Enable_NE_A_only( void );


/////////////////////////////////////////////////////////////////////////////
// Function: Cloud_Enable_NE_B_only()
//
// Parameters: none
//
// Return: response_t -> response enum type
//
// Description: This function sends a command packet to enable only Band B
// of the NE.  Band A will be disabled.   This function only works for dual
// band NEs only.
//
/////////////////////////////////////////////////////////////////////////////
response_t Cloud_Enable_NE_B_only( void );


/////////////////////////////////////////////////////////////////////////////
// Function: Cloud_Disable_NE()
//
// Parameters: none
//
// Return: response_t -> response enum type
//
// Description: This function sends a command packet to disable the NE.
//
/////////////////////////////////////////////////////////////////////////////
response_t Cloud_Disable_NE( void );


/////////////////////////////////////////////////////////////////////////////
// Function: Set_UL_Channel_A()
//
// Parameters: 1 dimensional char string array
//
// Return: response_t -> response enum type
//
// Description: This function sends a command packet to update the Uplink
// channel (and this automatically updates the Downlink channel too) on 
// Band A the NE.
//
/////////////////////////////////////////////////////////////////////////////
response_t Set_UL_Channel_A(char buffer[SMS_MT_MAX_DATA_LEN]);


/////////////////////////////////////////////////////////////////////////////
// Function: Set_UL_Channel_B()
//
// Parameters: 1 dimensional char string array
//
// Return: response_t -> response enum type
//
// Description: This function sends a command packet to update the Uplink
// channel (and this automatically updates the Downlink channel too) for
// Band B (UARCFN Band 8 - 900Mhz) on the NE.  
//
// Note: This is for dual band NEs only.
//
/////////////////////////////////////////////////////////////////////////////
response_t Set_UL_Channel_B( char buffer[SMS_MT_MAX_DATA_LEN] );


/////////////////////////////////////////////////////////////////////////////
// Function: Read_Temperature()
//
// Parameters: pointer to a char buffer where the Temperature is placed
//             placed.  Min Size 6 chars
//
// Return: response_t -> response enum type
//
// Description: This function reads the NE's Temperature, and returns
// a pointer to a null-terminated string containing the float value.
//
/////////////////////////////////////////////////////////////////////////////
response_t Read_Temperature( char *temp );


/////////////////////////////////////////////////////////////////////////////
// Function: Read_Alarms()
//
// Parameters: pointer to a byte where alarm code will be placed (NULL if 
//             not used).
//             pointer to a char buffer where the Alarm Code integer is
//             placed (NULL if not used).
//
// Return: response_t -> response enum type
//
// Description: This function reads the NE's Alarm state, and returns
// the Alarm code in two ways: a pointer to a byte, and formats
// the integer data into a null-terminated string.
//
/////////////////////////////////////////////////////////////////////////////
response_t Read_Alarms( UINT8 *code, char *code_str );


/////////////////////////////////////////////////////////////////////////////
// Function: Read_NE_Cloud_Enabled()
//
// Parameters: pointer to a bool where Enabled state is placed (NULL if 
//             not used).   True = Enabled, False = Disabled
//             pointer to a char buffer where Enable State integer is
//             placed (NULL if not used).  "1" = Enabled, "0" = Disabled
//
// Return: response_t -> response enum type
//
// Description: This function reads the NE's Enabled/Disabled status, and 
// returns the state in two ways: a pointer to a bool, and pointer to a 
// string that contains an ascii integer 1 or 0.
//
/////////////////////////////////////////////////////////////////////////////
response_t Read_NE_Cloud_Enabled( bool *status, char *status_str );


/////////////////////////////////////////////////////////////////////////////
// Function: Read_UL_Activity_State_A()
//
// Parameters: pointer to a bool where Activity state is placed (NULL if 
//             not used).   True = Dummy Load, False = Antenna
//             pointer to a char buffer where Enable State integer is
//             placed (NULL if not used).  "1" = Dummy Load, "0" = Antenna
//
// Return: response_t -> response enum type
//
// Description: This function reads the NE's UL Activity state, and returns
// the state in two ways: a pointer to a bool, and pointer to a string that
// contains an ascii integer 1 or 0.  Band A.
//
/////////////////////////////////////////////////////////////////////////////
response_t Read_UL_Activity_State_A( bool *state, char *state_str );


/////////////////////////////////////////////////////////////////////////////
// Function: Read_UL_Activity_State_B()
//
// Parameters: pointer to a bool where Activity state is placed (NULL if 
//             not used).   True = Dummy Load, False = Antenna
//             pointer to a char buffer where Enable State integer is
//             placed (NULL if not used).  "1" = Dummy Load, "0" = Antenna
//
// Return: response_t -> response enum type
//
// Description: This function reads the NE's UL Activity state, and returns
// the state in two ways: a pointer to a bool, and pointer to a string that
// contains an ascii integer 1 or 0.
//
// Note: This is for dual band NEs only.
//
/////////////////////////////////////////////////////////////////////////////
response_t Read_UL_Activity_State_B( bool *state, char *state_str );


/////////////////////////////////////////////////////////////////////////////
// Function: Read_UL_Channel_A()
//
// Parameters: pointer to a UINT16 where Channel Number is placed (NULL if 
//             not used).
//             pointer to a char buffer where the Channel Number integer is
//             placed (NULL if not used).  Min Size 8 chars
//             pointer to a char buffer where the Channel Frequency float is
//             placed (NULL if not used).  Min Size 8 chars

// Return: response_t -> response enum type
//
// Description: This function reads the NE's UARFCN Uplink Channel Number, 
// and returns the Channel number in 3 ways: a pointer to a 16-bit unsigned,
// a pointer to a string of the integer channel number, a pointer to a 
// string of the Frequency in Mhz XXXX.X representing a float.
//
// Note: This function is for Band_1 channels only, Band_A
//
/////////////////////////////////////////////////////////////////////////////
response_t Read_UL_Channel_A( UINT16 *chan, char *chan_str, char *freq_str );


/////////////////////////////////////////////////////////////////////////////
// Function: Read_DL_Channel_A()
//
// Parameters: pointer to a UINT16 where Channel Number is placed (NULL if 
//             not used).
//             pointer to a char buffer where the Channel Number integer is
//             placed (NULL if not used).  Min Size 8 chars
//             pointer to a char buffer where the Channel Frequency float is
//             placed (NULL if not used).  Min Size 8 chars
//
// Return: response_t -> response enum type
//
// Description: This function reads the NE's UARFCN Downlink Channel Number, 
// and returns the Channel number in 3 ways: a pointer to a 16-bit unsigned,
// a pointer to a string of the integer channel number, a pointer to a 
// string of the Frequency in Mhz XXXXX.X representing a float.
//
// Note: This function is for Band_1 channels only, Band_A
//
/////////////////////////////////////////////////////////////////////////////
response_t Read_DL_Channel_A( UINT16 *chan, char *chan_str, char *freq_str );


/////////////////////////////////////////////////////////////////////////////
// Function: Read_UL_Channel_B()
//
// Parameters: pointer to a UINT16 where Channel Number is placed (NULL if 
//             not used).
//             pointer to a char buffer where the Channel Number integer is
//             placed (NULL if not used).  Min Size 8 chars
//             pointer to a char buffer where the Channel Frequency float is
//             placed (NULL if not used).  Min Size 8 chars
//
// Return: response_t -> response enum type
//
// Description: This function reads the NE's UARFCN Uplink Channel Number, 
// and returns the Channel number in 3 ways: a pointer to a 16-bit unsigned,
// a pointer to a string of the integer channel number, a pointer to a 
// string of the Frequency in Mhz XXXX.X representing a float.
//
// Note: This function is for Band B (Band_8), dual band NEs only.
//
/////////////////////////////////////////////////////////////////////////////
response_t Read_UL_Channel_B( UINT16 *chan, char *chan_str, char *freq_str );


/////////////////////////////////////////////////////////////////////////////
// Function: Read_DL_Channel_B()
//
// Parameters: pointer to a UINT16 where Channel Number is placed (NULL if 
//             not used).
//             pointer to a char buffer where the Channel Number integer is
//             placed (NULL if not used).  Min Size 8 chars
//             pointer to a char buffer where the Channel Frequency float is
//             placed (NULL if not used).  Min Size 8 chars
//
// Return: response_t -> response enum type
//
// Description: This function reads the NE's UARFCN Downlink Channel Number, 
// and returns the Channel number in 3 ways: a pointer to a 16-bit unsigned,
// a pointer to a string of the integer channel number, a pointer to a 
// string of the Frequency in Mhz XXXX.X representing a float.
//
// Note: This function is for Band B (Band_8), dual band NEs only.
//
/////////////////////////////////////////////////////////////////////////////
response_t Read_DL_Channel_B( UINT16 *chan, char *chan_str, char *freq_str );


/////////////////////////////////////////////////////////////////////////////
// Function: Read_UL_Gain_A()
//
// Parameters: pointer to a char buffer where the Gain is placed
//             placed.  Min Size 5 chars
//
// Return: response_t -> response enum type
//
// Description: This function reads the NE's Band A Uplink Gain, and returns
// a pointer to a null-terminated string containing the float value.
//
/////////////////////////////////////////////////////////////////////////////
response_t Read_UL_Gain_A( char *gain_str );


/////////////////////////////////////////////////////////////////////////////
// Function: Read_DL_Gain_A()
//
// Parameters: pointer to a char buffer where the Gain is placed
//             placed.  Min Size 5 chars
//
// Return: response_t -> response enum type
//
// Description: This function reads the NE's Band A Downlink Gain, and returns
// a pointer to a null-terminated string containing the float value.
//
/////////////////////////////////////////////////////////////////////////////
response_t Read_DL_Gain_A( char *gain_str );


/////////////////////////////////////////////////////////////////////////////
// Function: Read_UL_Gain_B()
//
// Parameters: pointer to a char buffer where the Gain is placed
//             placed.  Min Size 5 chars
//
// Return: response_t -> response enum type
//
// Description: This function reads the NE's Uplink Gain, and returns
// a pointer to a null-terminated string containing the float value.
//
// Note: This function is for Band B, dual band NEs only.
//
/////////////////////////////////////////////////////////////////////////////
response_t Read_UL_Gain_B( char *gain_str );


/////////////////////////////////////////////////////////////////////////////
// Function: Read_DL_Gain_B()
//
// Parameters: pointer to a char buffer where the Gain is placed
//             placed.  Min Size 5 chars
//
// Return: response_t -> response enum type
//
// Description: This function reads the NE's Downlink Gain, and returns
// a pointer to a null-terminated string containing the float value.
//
// Note: This function is for Band B, dual band NEs only.
//
/////////////////////////////////////////////////////////////////////////////
response_t Read_DL_Gain_B( char *gain_str );


/////////////////////////////////////////////////////////////////////////////
// Function: Read_UL_RSSI_A()
//
// Parameters: pointer to a signed int where RSSI is placed.  RSSI will be
//             a negative number.  (NULL if not used).
//             pointer to a char buffer where the RSSI integer is placed 
//            (NULL if not used).  Min Size 6 chars
//
// Return: response_t -> response enum type
//
// Description: This function reads the NE's Uplink RSSI, and returns the 
// RSSI in 2 ways: a pointer to a signed integer, and a pointer to a string 
// of the signed integer RSSI value.  Band A.
//
/////////////////////////////////////////////////////////////////////////////
response_t Read_UL_RSSI_A( int *rssi, char *rssi_str );


/////////////////////////////////////////////////////////////////////////////
// Function: Read_DL_RSSI_A()
//
// Parameters: pointer to a signed int where RSSI is placed.  RSSI will be
//             a negative number.  (NULL if not used).
//             pointer to a char buffer where the RSSI integer is placed 
//            (NULL if not used).  Min Size 6 chars
//
// Return: response_t -> response enum type
//
// Description: This function reads the NE's Downlink RSSI, and returns the 
// RSSI in 2 ways: a pointer to a signed integer, and a pointer to a string 
// of the signed integer RSSI value.  Band A.
//
/////////////////////////////////////////////////////////////////////////////
response_t Read_DL_RSSI_A( int *rssi, char *rssi_str );


/////////////////////////////////////////////////////////////////////////////
// Function: Read_UL_RSSI_B()
//
// Parameters: pointer to a signed int where RSSI is placed.  RSSI will be
//             a negative number.  (NULL if not used).
//             pointer to a char buffer where the RSSI integer is placed 
//            (NULL if not used).  Min Size 6 chars
//
// Return: response_t -> response enum type
//
// Description: This function reads the NE's Uplink RSSI, and returns the 
// RSSI in 2 ways: a pointer to a signed integer, and a pointer to a string 
// of the signed integer RSSI value.
//
// Note: This function is for Band B, dual band NEs only.
//
/////////////////////////////////////////////////////////////////////////////
response_t Read_UL_RSSI_B( int *rssi, char *rssi_str );


/////////////////////////////////////////////////////////////////////////////
// Function: Read_DL_RSSI_B()
//
// Parameters: pointer to a signed int where RSSI is placed.  RSSI will be
//             a negative number.  (NULL if not used).
//             pointer to a char buffer where the RSSI integer is placed 
//            (NULL if not used).  Min Size 6 chars
//
// Return: response_t -> response enum type
//
// Description: This function reads the NE's Downlink RSSI, and returns the 
// RSSI in 2 ways: a pointer to a signed integer, and a pointer to a string 
// of the signed integer RSSI value.
//
// Note: This function is for Band B, dual band NEs only.
//
/////////////////////////////////////////////////////////////////////////////
response_t Read_DL_RSSI_B( int *rssi, char *rssi_str );


/////////////////////////////////////////////////////////////////////////////
// Function: Read_UL_TX_Power_A()
//
// Parameters: pointer to a char buffer where the string of the signed TX 
//             Power value is placed (could be negative number).  Min Size
//             is CMD_PKT_MAX_DATA_PAYLOAD chars.
//
// Return: response_t -> response enum type
//
// Description: This function reads the NE's Band A Uplink TX Power in dBm, 
// and returns the value in as a string.
//
/////////////////////////////////////////////////////////////////////////////
response_t Read_UL_TX_Power_A( char *tx_pwr_str );


/////////////////////////////////////////////////////////////////////////////
// Function: Read_DL_TX_Power_A()
//
// Parameters: pointer to a char buffer where the string of the signed TX 
//             Power value is placed (could be negative number).  Min Size
//             is CMD_PKT_MAX_DATA_PAYLOAD chars.
//
// Return: response_t -> response enum type
//
// Description: This function reads the NE's Band A Downlink TX Power in dBm,
// and returns the value in as a string.
//
/////////////////////////////////////////////////////////////////////////////
response_t Read_DL_TX_Power_A( char *tx_pwr_str );


/////////////////////////////////////////////////////////////////////////////
// Function: Read_LED_Status()
//
// Parameters: pointer to a unsigned int where LED status is placed.  (NULL if not used).
//             pointer to a char buffer where the LED status is placed 
//            (NULL if not used). Min Size 6 chars
//
// Return: response_t -> response enum type
//
// Description: This function reads the NE's LED status, and returns the 
// led Status in 2 ways: a pointer to an integer, and a pointer to a string 
// of the integer.
//
// Note the following return Values:
// 0 => 1 Amber LED lit
// 1 => 1 Green LED lit
// 2 => 2 Green LEDs lit
// 3 => 3 Green LEDs lit
// 4 => Bouncing Ball state (NE is booting or tuning)
// 5 => Fault, Red LED Blink code 1 (Cloud Disable)
// 6 => Fault, Red LED Blink code 2 (RF Stability)
// 7 => Fault, Red LED Blink code 3 (No GSM Signal)
// 8 => Fault, Red LED Blink code 4 (Temperature)
// 9 => Fault, Red LED Blink code 5 (Module Failure, HW, SW, or No SIM)
// 0xFF => invalid state of LEDs, Response code will be set to RSP_GENERAL_ERROR
//
/////////////////////////////////////////////////////////////////////////////
response_t Read_LED_Status( UINT8 *led, char *led_str );


/////////////////////////////////////////////////////////////////////////////
// Function: Read_NE_Cal_item()
//
// Parameters: UINT16  -> item number, max is 505
//             UINT16* -> pointer to the returned data
//             char*    ->pointer to a char buffer where data is placed
//
// Return: response_t -> response enum type
//
// Description: This function reads a Calibration item from the NE's flash
// and returns the data as a 16-bit unsigned value and/or an integer string.
//
/////////////////////////////////////////////////////////////////////////////
response_t Read_NE_Cal_item( UINT16 item, UINT16 *data, char *data_str );


/////////////////////////////////////////////////////////////////////////////
// Function: Write_NE_Cal_item()
//
// Parameters: UINT16 -> item number, range 1 - 505
//             UINT16 -> data to be written
//
// Return: response_t -> response enum type
//
// Description: This function writes a unsigned 16-bit value to a Calibration
// Item within the NE's flash.
//
/////////////////////////////////////////////////////////////////////////////
response_t Write_NE_Cal_item( UINT16 item, UINT16 data );


/////////////////////////////////////////////////////////////////////////////
// Function: epoch_time()
//
// Parameters: pointer to a char buffer where epochtime is placed
//
// Return: response_t -> response enum type
//
// Description: This function gathers date and sends when requested
// SMS or MQTT message payload.
//
/////////////////////////////////////////////////////////////////////////////
response_t epoch_time(char *time_str);


/////////////////////////////////////////////////////////////////////////////
// Function: PeriodicPerformanceData()
//
// Parameters: char* -> buffer to place data for "read" operations,
//                      set to NULL for unsolicited messages
//
// Return: response_t -> response enum type
//
// Description: This function gathers various performance data stats
// and packages them up into a comma separated string for an unsolicited
// SMS or MQTT message payload.
//
// Payload="<enabled>,<Temp>,<UL rssi>,<DL rssi>,<UL gain>,<DL gain>,
// <UL tx pwr>,<DL tx pwr>,<UL activity>,<Alarm>,<epoch time>,<>
//
/////////////////////////////////////////////////////////////////////////////
response_t PeriodicPerformanceData( char *data_buf );


/////////////////////////////////////////////////////////////////////////////
// Function: Bootup_perf_basic1()
//
// Parameters: none
//
// Return: response_t -> response enum type
//
// Description: This function gathers various stats about the module
// and packages them up into a comma separated string for an unsolicited
// SMS or MQTT message payload.  This message is typically sent at
// boot-up once, after cell service if established
//
/////////////////////////////////////////////////////////////////////////////
response_t Bootup_perf_basic1( void );


/////////////////////////////////////////////////////////////////////////////
// Function: Bootup_perf_basic2()
//
// Parameters: none
//
// Return: response_t -> response enum type
//
// Description: This function gathers various stats about the module
// and packages them up into a comma separated string for an unsolicited
// SMS or MQTT message payload.  This message is typically sent at
// boot-up once, after cell service if established
//
/////////////////////////////////////////////////////////////////////////////
response_t Bootup_perf_basic2( void );


/////////////////////////////////////////////////////////////////////////////
// Function: Bootup_perf_basic3()
//
// Parameters: none
//
// Return: response_t -> response enum type
//
// Description: This function gathers various status about the module
// and packages them up into a comma separated string for an unsolicited
// SMS or MQTT message payload.  This message is typically sent at
// boot-up once, after cell service if established in device not activated state.
//
/////////////////////////////////////////////////////////////////////////////
response_t Bootup_perf_basic3(void);


/////////////////////////////////////////////////////////////////////////////
// Function: Activate_NE()
//
// Parameters: none
//
// Return: response_t -> response enum type
//
// Description: This function processes an OTA command to activate the NE.
//
/////////////////////////////////////////////////////////////////////////////
response_t Activate_NE( void );


////////////////////////////////////////////////////////////////////////////
// Function: PeriodicLocationData()
//
// Parameters: none
//
// Return: response_t -> response enum type
//
// Description: This function finds location information about the module
// and packages them up into a comma separated string for an unsolicited
// SMS or MQTT message payload.
//
/////////////////////////////////////////////////////////////////////////////
response_t PeriodicLocationData( void );


/////////////////////////////////////////////////////////////////////////////
// Function: FTP_version()
//
// Parameters: 1 dimensional char string array
//             1 dimensional char string array
//
// Return: response_t -> response enum type
//
// Description: This function sends the current and previous version of 
// Telit application to RMS.
//
/////////////////////////////////////////////////////////////////////////////
response_t FTP_version(char version[APP_VER_LEN],char msg_id[SMS_HDR_MSG_ID_LEN+1]);


/////////////////////////////////////////////////////////////////////////////
// Function: FTP_Error_Resp()
//
// Parameters:  1 dimensional char string array
//
// Return: response_t -> response enum type
//
// Description: This function sends the error code of unable to
// download along with current version to RMS.
//
/////////////////////////////////////////////////////////////////////////////
response_t FTP_Error_Resp(char msg_id[SMS_HDR_MSG_ID_LEN+1]);


#endif // FEATURE_5BARZ 

#ifdef __cplusplus
}
#endif

#endif /* M2M_CLOUD_METHODS_UTILS_H_ */
