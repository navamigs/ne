/*
 * cloud_user_method.c
 *
 *  Created on: 16/gen/2015
 *      Author: FabioPi
 */

#include <stdlib.h>
#include "cloud_user_method.h"
#include "cmd_pkt.h"
#include "sms.h"
#include "fs_file.h"
#include "utilities.h"


/* ===========================================================================================
*
*	 This source file contains a sample method execution function, callable
*	 from m2m_cloud_method_handler() callback (see m2m_cloud_method_handler.c source file).
*
* ===========================================================================================*/

#ifdef FEATURE_5BARZ

#define UARCFN_LOWEST_CHANNEL_BAND1_UL   9612     // Channel 9612 => 1922.4 MHz
#define UARCFN_HIGHEST_CHANNEL_BAND1_UL  9888     // Channel 9888 => 1977.6 MHz
#define UARCFN_LOWEST_CHANNEL_BAND1_DL   10562    // Channel 10562 => 2112.4 MHz
#define UARCFN_HIGHEST_CHANNEL_BAND1_DL  10838    // Channel 10838 => 2167.6 MHz

#define UARCFN_LOWEST_CHANNEL_BAND8_DL   2937     // Channel 2937 => 927.4 MHz
#define UARCFN_HIGHEST_CHANNEL_BAND8_DL  3088     // Channel 3088 => 957.6 MHz
#define UARCFN_LOWEST_CHANNEL_BAND8_UL   2712     // Channel 2712 => 882.4 MHz
#define UARCFN_HIGHEST_CHANNEL_BAND8_UL  2863     // Channel 2863 => 912.6 MHz


M2M_CLOUD_GLOBALS globals;

// Local Function Prototypes:
response_t Read_GSM_RSSI( char *rssi_str );


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
response_t Reset_NE( void )
{
  response_t rsp;
  
  rsp = WriteCmdPkt0( CMD_PKT_RESET_NE );
  if( rsp == RSP_SUCCESS )
  {
    dwPrintDebug( M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Reset NE" );
    NE_debug("Reset NE" );  
  }
  return( rsp );  
}


/////////////////////////////////////////////////////////////////////////////
// Function: Reset_Module()
//
// Parameters: none
//
// Return: response_t -> response enum type
//
// Description: This function sends a command packet to NE that causes
// a power down and power up cycle of the Telit Module.  Takes 10 seconds.
// Range: 0 to 65535 seconds (~18.2 hours)
//
/////////////////////////////////////////////////////////////////////////////
response_t Reset_Module( int reset_delay )
{
  response_t rsp;
  INT32      reset;
    
  if( reset_delay <= 0 )
  {
    rsp = WriteCmdPkt0( CMD_PKT_RESET_MODULE );
    if( rsp == RSP_SUCCESS )
    {
      dwPrintDebug( M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Reset Module" );
      NE_debug("Reset Module" );  
    }
    return( rsp );  
  }

  // Start the Timer for non-zero delays, range 1 to 65535 seconds
  if( reset_delay > 0xFFFF ) reset_delay = 0xFFFF;
  reset = reset_delay * 1000;
  m2m_timer_start( timer_restart, reset );
  return( RSP_SUCCESS );
}


/////////////////////////////////////////////////////////////////////////////
// Function: Cloud_Enable_NE()
//
// Parameters: none
//
// Return: response_t -> response enum type
//
// Description: This function sends a command packet to enable the NE.
// It enables all available bands on the NE.
//
/////////////////////////////////////////////////////////////////////////////
response_t Cloud_Enable_NE( void )
{
  response_t rsp;
  
  rsp = WriteCmdPkt0( CMD_PKT_SET_CLOUD_ENABLE_ALL );
  if( rsp == RSP_SUCCESS )
  {
    dwPrintDebug( M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Enabling NE" );
    NE_debug("Enabling NE" );  
  }
  return( rsp );  
}


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
response_t Cloud_Enable_NE_A_only( void )
{
  response_t rsp;
  
  if( globals.module.NE_boardID != TITAN_BAND1_BAND8 ) return ( RSP_NOT_SUPPORTED );
  
  rsp = WriteCmdPkt0( CMD_PKT_SET_CLOUD_ENABLE_A_ONLY );
  if( rsp == RSP_SUCCESS )
  {
    dwPrintDebug( M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Enabling Band A of NE" );
    NE_debug("Enabling NE Band A" );  
  }
  return( rsp );  
}


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
response_t Cloud_Enable_NE_B_only( void )
{
  response_t rsp;
  
  if( globals.module.NE_boardID != TITAN_BAND1_BAND8 ) return ( RSP_NOT_SUPPORTED );
  
  rsp = WriteCmdPkt0( CMD_PKT_SET_CLOUD_ENABLE_B_ONLY );
  if( rsp == RSP_SUCCESS )
  {
    dwPrintDebug( M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Enabling Band B of NE" );
    NE_debug("Enabling NE Band B" );  
  }
  return( rsp );  
}


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
response_t Cloud_Disable_NE( void )
{
  response_t rsp;
  
  rsp = WriteCmdPkt0( CMD_PKT_SET_CLOUD_DISABLE );
  if( rsp == RSP_SUCCESS )
  {
    dwPrintDebug( M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Disabling NE" );
    NE_debug("Disabling NE" );  
  }
  return( rsp );
}


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
response_t Activate_NE( void )
{
  response_t rsp;
  msg_queue_t msg;

  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "%s",globals.module.RMS_Activate);
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "IN activation status\n");
  
  // Sending 0E00,0E01,0E02,0E03 during first activation.
  if( !strcmp( globals.module.RMS_Activate, "N" ) || 
      !strcmp( globals.module.RMS_Activate, "n" ) )  
  {
    rsp = Cloud_Enable_NE();
    if( rsp != RSP_SUCCESS ) return( rsp );
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "RMS Activating\n");
    globals.module.cloud_enabled = true;
    update_config_file( CFG_FILE_ITEM_RMS_ACTIVATE, "Y\0" ); 
    NE_debug( "Module Activated" );
    msg.cmd = MSQ_QUEUE_CMD_BASIC_MSG1;
    PushToMsgQueue( msg );    
    msg.cmd = MSQ_QUEUE_CMD_BASIC_MSG2;
    PushToMsgQueue( msg );    
    Req_location_info();
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "latitude response at read:%s\n", globals.module.location.latitude);
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "longitude response at read: %s\n",globals.module.location.longitude);
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "accuracy response at read: %d\n",globals.module.location.accuracy);
    Bootup_perf_basic3();
    msg.cmd = MSQ_QUEUE_CMD_PERF_MSG;
    PushToMsgQueue( msg );
    if( atoi( globals.module.perf_intvl ) > 0 ) Start_Perf_Data_timer();
    if( atoi( globals.module.Loc_intvl ) > 0 ) Start_Loc_Data_timer();    
  }
  else
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Already activated\n");
    rsp = RSP_NE_ALREADY_DONE;
  }

  return( rsp );
}


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
response_t Set_UL_Channel_A( char buffer[SMS_MT_MAX_DATA_LEN] )
{
  int        chan = 0;
  response_t rsp;
  
  chan = atoi(buffer);
  
  // Check Band 1 limits:
  if( chan < UARCFN_LOWEST_CHANNEL_BAND1_UL ||
      chan > UARCFN_HIGHEST_CHANNEL_BAND1_UL ) return( RSP_PARAMETER_OUT_OF_RANGE );

  rsp = WriteCmdPkt16( CMD_PKT_WRITE_CHAN_A_UL, (UINT16)chan );
  
  if( rsp == RSP_SUCCESS )
  {
    dwPrintDebug( M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Set UL channel=%d", chan );
    NE_debug_var("New UL Channel=", chan );  
  } 
  return( rsp );
}


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
response_t Set_UL_Channel_B( char buffer[SMS_MT_MAX_DATA_LEN] )
{
  int        chan = 0;
  response_t rsp;
  
  if( globals.module.NE_boardID != TITAN_BAND1_BAND8 ) return ( RSP_NOT_SUPPORTED );
  
  chan = atoi(buffer);
  
  // Check Band 8 limits:
  if( chan < UARCFN_LOWEST_CHANNEL_BAND8_UL ||
      chan > UARCFN_HIGHEST_CHANNEL_BAND8_UL ) return( RSP_PARAMETER_OUT_OF_RANGE );

  rsp = WriteCmdPkt16( CMD_PKT_WRITE_CHAN_B_UL, (UINT16)chan );
  
  if( rsp == RSP_SUCCESS )
  {
    dwPrintDebug( M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Set Band B UL channel=%d", chan );
    NE_debug_var("New B_UL Channel=", chan );  
  } 
  return( rsp );
}


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
response_t Read_Temperature( char *temp )
{
  int        len;
  UINT16     temp16;
  response_t rsp;

  rsp = ReadCmdPkt16( CMD_PKT_READ_TEMPERATURE, &temp16 );
  
  if( rsp == RSP_SUCCESS )
  {
    sprintf( temp, "%d\0\0", temp16 );
    len = strlen( temp );
    if( len > 0 && len < (RESPONSE_ARRAY_LEN_MAX - 2) ) 
    {
      temp[len]   = temp[len-1];
      temp[len-1] = PERIOD;
    }
    else rsp = RSP_GENERAL_ERROR;
  }
  return( rsp );  
}
  
  
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
response_t Read_Alarms( UINT8 *code, char *code_str )
{
  UINT16     data16;
  response_t rsp;
  
  rsp = ReadCmdPkt16( CMD_PKT_READ_ALARM_CODE, &data16 );

  if( rsp == RSP_SUCCESS )
  {
    if( code != NULL ) *code = (UINT8)(0x00FF & data16);
    if( code_str != NULL ) sprintf( code_str, "%d\0", data16 );
  }
  return( rsp );
}


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
response_t Read_NE_Cloud_Enabled( bool *status, char *status_str )
{
  UINT16     data16;
  response_t rsp;
  
  rsp = ReadCmdPkt16( CMD_PKT_READ_CLOUD_ENABLE, &data16 );

  if( rsp == RSP_SUCCESS )
  {
    if( status != NULL )
    {
      if( globals.module.NE_boardID == TITAN_BAND1_BAND8 ) // dual band
      {
        if( data16 == 1 || data16 == 2 || data16 == 3 ) *status = true; 
        else if( data16 == 0 ) *status = false;
        else return( RSP_NE_READ_FAILED );  
      }
      else // Single band Atlas
      {
        if( data16 == 1 ) *status = true;
        else if( data16 == 0 ) *status = false;
        else return( RSP_NE_READ_FAILED );  
      }
    }
    if( status_str != NULL ) 
    {  
      if( data16 <= 3 ) sprintf( status_str, "%d\0", data16 );   
      else return( RSP_NE_READ_FAILED );
    } 
  }
  return( rsp );
}


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
response_t Read_UL_Activity_State_A( bool *state, char *state_str )
{
  UINT16     data16;
  response_t rsp;
  
  rsp = ReadCmdPkt16( CMD_PKT_READ_ACTVTY_DET_A_UL, &data16 );

  if( rsp == RSP_SUCCESS )
  {
    if( state != NULL )
    {
      if( data16 == 1 ) *state = true;
      else if( data16 == 0 ) *state = false;
      else return( RSP_NE_READ_FAILED );  
    }
    if( state_str != NULL ) 
    {  
      if( data16 == 1 || data16 == 0 ) sprintf( state_str, "%d\0", data16 );   
      else return( RSP_NE_READ_FAILED );
    } 
  }
  return( rsp );
}


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
response_t Read_UL_Activity_State_B( bool *state, char *state_str )
{
  UINT16     data16;
  response_t rsp;
  
  if( globals.module.NE_boardID != TITAN_BAND1_BAND8 ) return ( RSP_NOT_SUPPORTED );
    
  rsp = ReadCmdPkt16( CMD_PKT_READ_ACTVTY_DET_B_UL, &data16 );

  if( rsp == RSP_SUCCESS )
  {
    if( state != NULL )
    {
      if( data16 == 1 ) *state = true;
      else if( data16 == 0 ) *state = false;
      else return( RSP_NE_READ_FAILED );  
    }
    if( state_str != NULL ) 
    {  
      if( data16 == 1 || data16 == 0 ) sprintf( state_str, "%d\0", data16 );   
      else return( RSP_NE_READ_FAILED );
    } 
  }
  return( rsp );
}


/////////////////////////////////////////////////////////////////////////////
// Function: Read_UL_Channel()
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
// Note: This function is for Band_1 channels only, Band_A
//
/////////////////////////////////////////////////////////////////////////////
response_t Read_UL_Channel_A( UINT16 *chan, char *chan_str, char *freq_str )
{
  UINT8      len;
  UINT16     data16 = 0;
  response_t rsp;
  
  rsp = ReadCmdPkt16( CMD_PKT_READ_CHAN_A_UL, &data16 );
  
  if( data16 < UARCFN_LOWEST_CHANNEL_BAND1_UL || 
      data16 > UARCFN_HIGHEST_CHANNEL_BAND1_UL ) return( RSP_NE_READ_FAILED );

  if( rsp == RSP_SUCCESS )
  {
    if( chan != NULL ) *chan = data16; 
    if( chan_str != NULL ) sprintf( chan_str, "%d\0", data16 );
    
    if( freq_str )
    {
      data16 *= 2;
      sprintf( freq_str, "%d\0\0", data16 );
      len = strlen( freq_str );
      if( len > 0 && len < (RESPONSE_ARRAY_LEN_MAX - 2) ) 
      {
        freq_str[len]   = freq_str[len-1];
        freq_str[len-1] = PERIOD;
      }
    }
  }
  return( rsp );
}


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
response_t Read_DL_Channel_A( UINT16 *chan, char *chan_str, char *freq_str )
{
  UINT8      len;
  UINT16     data16 = 0;
  response_t rsp;
  
  rsp = ReadCmdPkt16( CMD_PKT_READ_CHAN_A_DL, &data16 );
  
  if( data16 < UARCFN_LOWEST_CHANNEL_BAND1_DL || 
      data16 > UARCFN_HIGHEST_CHANNEL_BAND1_DL ) return( RSP_NE_READ_FAILED );

  if( rsp == RSP_SUCCESS )
  {
    if( chan != NULL ) *chan = data16; 
    if( chan_str != NULL ) sprintf( chan_str, "%d\0", data16 );   
    if( freq_str )
    {
      data16 *= 2;
      sprintf( freq_str, "%d\0\0", data16 );
      len = strlen( freq_str );
      if( len > 0 && len < (RESPONSE_ARRAY_LEN_MAX - 2) ) 
      {
        freq_str[len]   = freq_str[len-1];
        freq_str[len-1] = PERIOD;
      }
    }
  }
  return( rsp );
}


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
response_t Read_UL_Channel_B( UINT16 *chan, char *chan_str, char *freq_str )
{
  UINT8      len;
  UINT16     data16 = 0;
  response_t rsp;
  
  if( globals.module.NE_boardID != TITAN_BAND1_BAND8 ) return ( RSP_NOT_SUPPORTED );
  
  rsp = ReadCmdPkt16( CMD_PKT_READ_CHAN_B_UL, &data16 );
  
  if( data16 < UARCFN_LOWEST_CHANNEL_BAND8_UL || 
      data16 > UARCFN_HIGHEST_CHANNEL_BAND8_UL ) return( RSP_NE_READ_FAILED );

  if( rsp == RSP_SUCCESS )
  {
    if( chan != NULL ) *chan = data16; 
    if( chan_str != NULL ) sprintf( chan_str, "%d\0", data16 );
    
    if( freq_str ) 
    {
      data16 = 2 * data16 + 3400;
      sprintf( freq_str, "%d\0\0", data16 ); 
      len = strlen( freq_str );
      if( len > 0 && len < (RESPONSE_ARRAY_LEN_MAX - 2) ) 
      {
        freq_str[len]   = freq_str[len-1];
        freq_str[len-1] = PERIOD;
      }
    }
  }
  return( rsp );
}


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
response_t Read_DL_Channel_B( UINT16 *chan, char *chan_str, char *freq_str )
{
  UINT8      len;
  UINT16     data16 = 0;
  response_t rsp;
  
  if( globals.module.NE_boardID != TITAN_BAND1_BAND8 ) return ( RSP_NOT_SUPPORTED );  
  
  rsp = ReadCmdPkt16( CMD_PKT_READ_CHAN_B_DL, &data16 );
  
  if( data16 < UARCFN_LOWEST_CHANNEL_BAND8_DL || 
      data16 > UARCFN_HIGHEST_CHANNEL_BAND8_DL ) return( RSP_NE_READ_FAILED );

  if( rsp == RSP_SUCCESS )
  {
    if( chan != NULL ) *chan = data16; 
    if( chan_str != NULL ) sprintf( chan_str, "%d\0", data16 );   
    if( freq_str )
    {
      data16 = 2 * data16 + 3400;
      sprintf( freq_str, "%d\0\0", data16 );
      len = strlen( freq_str );
      if( len > 0 && len < (RESPONSE_ARRAY_LEN_MAX - 2) ) 
      {
        freq_str[len]   = freq_str[len-1];
        freq_str[len-1] = PERIOD;
      }
    }
  }
  return( rsp );
}


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
response_t Read_UL_Gain_A( char *gain_str )
{
  UINT8      len;
  UINT16     data16;
  response_t rsp;
  
  rsp = ReadCmdPkt16( CMD_PKT_READ_GAIN_A_UL, &data16 );

  if( rsp == RSP_SUCCESS )
  {
    if( gain_str != NULL ) 
    {
      sprintf( gain_str, "%d\0\0", data16 );
      len = strlen( gain_str );
      if( len > 0 && len < (RESPONSE_ARRAY_LEN_MAX - 2) ) 
      {
        gain_str[len]   = gain_str[len-1];
        gain_str[len-1] = PERIOD;
      }
    }
  }
  return( rsp );
}


/////////////////////////////////////////////////////////////////////////////
// Function: Read_DL_Gain_A()
//
// Parameters: pointer to a char buffer where the Gain is placed
//             placed.  Min Size 5 chars.
//
// Return: response_t -> response enum type
//
// Description: This function reads the NE's Band A Downlink Gain, and returns
// a pointer to a null-terminated string containing the float value.
//
/////////////////////////////////////////////////////////////////////////////
response_t Read_DL_Gain_A( char *gain_str )
{
  UINT8      len;
  UINT16     data16;
  response_t rsp;
  
  rsp = ReadCmdPkt16( CMD_PKT_READ_GAIN_A_DL, &data16 );

  if( rsp == RSP_SUCCESS )
  {
    if( gain_str != NULL ) 
    {
      sprintf( gain_str, "%d\0\0", data16 );
      len = strlen( gain_str );
      if( len > 0 && len < (RESPONSE_ARRAY_LEN_MAX - 2) ) 
      {
        gain_str[len]   = gain_str[len-1];
        gain_str[len-1] = PERIOD;
      }
    }
  }
  return( rsp );
}


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
response_t Read_UL_Gain_B( char *gain_str )
{
  UINT8      len;
  UINT16     data16;
  response_t rsp;
  
  if( globals.module.NE_boardID != TITAN_BAND1_BAND8 ) return ( RSP_NOT_SUPPORTED );  
  
  rsp = ReadCmdPkt16( CMD_PKT_READ_GAIN_B_UL, &data16 );

  if( rsp == RSP_SUCCESS )
  {
    if( gain_str != NULL ) 
    {
      sprintf( gain_str, "%d\0\0", data16 );
      len = strlen( gain_str );
      if( len > 0 && len < (RESPONSE_ARRAY_LEN_MAX - 2) ) 
      {
        gain_str[len]   = gain_str[len-1];
        gain_str[len-1] = PERIOD;
      }
    }
  }
  return( rsp );
}


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
response_t Read_DL_Gain_B( char *gain_str )
{
  UINT8      len;
  UINT16     data16;
  response_t rsp;
  
  if( globals.module.NE_boardID != TITAN_BAND1_BAND8 ) return ( RSP_NOT_SUPPORTED );  
  
  rsp = ReadCmdPkt16( CMD_PKT_READ_GAIN_B_DL, &data16 );

  if( rsp == RSP_SUCCESS )
  {
    if( gain_str != NULL ) 
    {
      sprintf( gain_str, "%d\0\0", data16 );
      len = strlen( gain_str );
      if( len > 0 && len < (RESPONSE_ARRAY_LEN_MAX - 2) ) 
      {
        gain_str[len]   = gain_str[len-1];
        gain_str[len-1] = PERIOD;
      }
    }
  }
  return( rsp );
}


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
response_t Read_UL_RSSI_A( int *rssi, char *rssi_str )
{
  UINT16     data16;
  response_t rsp;
  
  rsp = ReadCmdPkt16( CMD_PKT_READ_RSSI_A_UL, &data16 );
  
  if( rsp == RSP_SUCCESS )
  {
    // Report as a Negative Number:
    if( rssi != NULL ) *rssi = -1 * (int)data16;
    if( rssi_str != NULL ) sprintf( rssi_str, "-%d\0", data16 );
  }
  return( rsp );
}


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
response_t Read_DL_RSSI_A( int *rssi, char *rssi_str )
{
  int        RSSI;
  UINT16     data16;
  response_t rsp;
  
  rsp = ReadCmdPkt16( CMD_PKT_READ_RSSI_A_DL, &data16 );

  if( rsp == RSP_SUCCESS )
  {
    // Report as a Negative Number:
    if( rssi != NULL ) *rssi = -1 * (int)data16;
    if( rssi_str != NULL ) sprintf( rssi_str, "-%d\0", data16 );
  }
  
  return( rsp );
}


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
response_t Read_UL_RSSI_B( int *rssi, char *rssi_str )
{
  UINT16     data16;
  response_t rsp;
  
  if( globals.module.NE_boardID != TITAN_BAND1_BAND8 ) return ( RSP_NOT_SUPPORTED );
  
  rsp = ReadCmdPkt16( CMD_PKT_READ_RSSI_A_UL, &data16 );
  
  if( rsp == RSP_SUCCESS )
  {
    // Report as a Negative Number:
    if( rssi != NULL ) *rssi = -1 * (int)data16;
    if( rssi_str != NULL ) sprintf( rssi_str, "-%d\0", data16 );
  }
  return( rsp );
}


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
response_t Read_DL_RSSI_B( int *rssi, char *rssi_str )
{
  int        RSSI;
  UINT16     data16;
  response_t rsp;
  
  if( globals.module.NE_boardID != TITAN_BAND1_BAND8 ) return ( RSP_NOT_SUPPORTED );
  
  rsp = ReadCmdPkt16( CMD_PKT_READ_RSSI_A_DL, &data16 );

  if( rsp == RSP_SUCCESS )
  {
    // Report as a Negative Number:
    if( rssi != NULL ) *rssi = -1 * (int)data16;
    if( rssi_str != NULL ) sprintf( rssi_str, "-%d\0", data16 );
  }
  
  return( rsp );
}


/////////////////////////////////////////////////////////////////////////////
// Function: Read_GSM_RSSI()
//
// Parameters: pointer to a char buffer where the RSSI integer is placed 
//             Min Size 5 chars  (range -115 to -51 dBm)
//
// Return: response_t -> response enum type
//
// Description: This function reads the RSSI of the GSM signal of
// the Telit module.  You must have coverage to get an accurate
// reading, otherwise this function returns -115 dBm.
//
/////////////////////////////////////////////////////////////////////////////
response_t Read_GSM_RSSI( char *rssi_str )
{
  int rssi, ber;
  
  if( globals.Reg_state == REG_STATE_CAMPING_HOME ||
      globals.Reg_state == REG_STATE_CAMPING_ROAM )
  {
    if( m2m_network_get_signal_strength( &rssi, &ber ) )
    {
      // RSSI needs converting, see AT+CSQ
      if( rssi >= 0 && rssi <= 31 )
      {
        rssi = rssi * 2 - 113;
      }
      else rssi = -115;
    }
    else rssi = -115;
  }
  else rssi = -115;
  
  
  if( rssi_str != NULL )
  {
    sprintf( rssi_str, "%d\0", rssi );
    return( RSP_SUCCESS );
  }
  else return( RSP_PARAMETER_OUT_OF_RANGE );
}


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
response_t Read_UL_TX_Power_A( char *tx_pwr_str )
{
  UINT8      data[CMD_PKT_MAX_DATA_PAYLOAD];
  UINT8      length;
  response_t rsp;
  
  memset( data, 0, CMD_PKT_MAX_DATA_PAYLOAD );
  rsp = ReadCmdPacket( CMD_PKT_READ_TX_PWR_A_UL, data, &length );
  
  if( rsp == RSP_SUCCESS )
  {
    strncpy( tx_pwr_str, (char *)data, length );
  }
  return( rsp );
}


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
response_t Read_DL_TX_Power_A( char *tx_pwr_str )
{
  UINT8      data[CMD_PKT_MAX_DATA_PAYLOAD];
  UINT8      length;
  response_t rsp;
  
  memset( data, 0, CMD_PKT_MAX_DATA_PAYLOAD );
  rsp = ReadCmdPacket( CMD_PKT_READ_TX_PWR_A_DL, data, &length );
  
  if( rsp == RSP_SUCCESS )
  {
    strncpy( tx_pwr_str, (char *)data, length );
  }
  return( rsp );
}


/////////////////////////////////////////////////////////////////////////////
// Function: Read_UL_TX_Power_B()
//
// Parameters: pointer to a char buffer where the string of the signed TX 
//             Power value is placed (could be negative number).  Min Size
//             is CMD_PKT_MAX_DATA_PAYLOAD chars.
//
// Return: response_t -> response enum type
//
// Description: This function reads the NE's Uplink TX Power in dBm, and 
// returns the value in as a string.
//
// Note: This is for dual band NEs only.
//
/////////////////////////////////////////////////////////////////////////////
response_t Read_UL_TX_Power_B( char *tx_pwr_str )
{
  UINT8      data[CMD_PKT_MAX_DATA_PAYLOAD];
  UINT8      length;
  response_t rsp;
  
  if( globals.module.NE_boardID != TITAN_BAND1_BAND8 ) return ( RSP_NOT_SUPPORTED );
  
  memset( data, 0, CMD_PKT_MAX_DATA_PAYLOAD );
  rsp = ReadCmdPacket( CMD_PKT_READ_TX_PWR_B_UL, data, &length );
  
  if( rsp == RSP_SUCCESS )
  {
    strncpy( tx_pwr_str, (char *)data, length );
  }
  return( rsp );
}


/////////////////////////////////////////////////////////////////////////////
// Function: Read_DL_TX_Power_B()
//
// Parameters: pointer to a char buffer where the string of the signed TX 
//             Power value is placed (could be negative number).  Min Size
//             is CMD_PKT_MAX_DATA_PAYLOAD chars.
//
// Return: response_t -> response enum type
//
// Description: This function reads the NE's Downlink TX Power in dBm, and 
// returns the value in as a string.
//
// Note: This is for dual band NEs only.
//
/////////////////////////////////////////////////////////////////////////////
response_t Read_DL_TX_Power_B( char *tx_pwr_str )
{
  UINT8      data[CMD_PKT_MAX_DATA_PAYLOAD];
  UINT8      length;
  response_t rsp;
  
  if( globals.module.NE_boardID != TITAN_BAND1_BAND8 ) return ( RSP_NOT_SUPPORTED );
  
  memset( data, 0, CMD_PKT_MAX_DATA_PAYLOAD );
  rsp = ReadCmdPacket( CMD_PKT_READ_TX_PWR_B_DL, data, &length );
  
  if( rsp == RSP_SUCCESS )
  {
    strncpy( tx_pwr_str, (char *)data, length );
  }
  return( rsp );
}


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
response_t Read_LED_Status( UINT8 *led, char *led_str )
{
  UINT16     data16;
  response_t rsp;
  
  rsp = ReadCmdPkt16( CMD_PKT_READ_LED_STATE, &data16 );

  if( rsp == RSP_SUCCESS )
  {
    if( led != NULL ) *led = (UINT8)(0x00FF & data16);
    if( led_str != NULL ) sprintf( led_str, "%d\0", data16 );
    if( data16 == 0xFF ) rsp = RSP_GENERAL_ERROR;
  }
  
  return( rsp );
}


/////////////////////////////////////////////////////////////////////////////
// Function: Read_NE_Cal_item()
//
// Parameters: UINT16  -> item number, max is 505
//             UINT16* -> pointer to the returned data
//
// Return: response_t -> response enum type
//
// Description: This function reads a Calibration item from the NE's flash
// and returns the data as a 16-bit unsigned value and/or an integer string.
//
/////////////////////////////////////////////////////////////////////////////
response_t Read_NE_Cal_item( UINT16 item, UINT16 *data, char *data_str )
{
  UINT8      len;
  UINT16     sub;
  response_t rsp;
  UINT8      payload[4];
  UINT8      rdata[CMD_PKT_MAX_DATA_PAYLOAD];
  UINT8      *dLen;
  
  // In this case, the NE wants to use a 4 digit HEX number
  // for both the payload and returned data
  len = 0;
  payload[len++] = n_h_a((UINT8)(item >> 12));
  payload[len++] = n_h_a((UINT8)((item & 0x0F00) >> 8));
  payload[len++] = n_h_a((UINT8)((item & 0x00F0) >> 4));
  payload[len++] = n_h_a((UINT8)(item & 0x000F));
  
  rsp = ReadCmdPacket_payload( CMD_PKT_READ_FLASH_ITEM, payload, len, rdata, dLen );  
  if( rsp == RSP_SUCCESS )
  {
    if( *dLen == 4 ) 
    {
      sub =  a_h_n(rdata[0]) * 0x1000;
      sub += a_h_n(rdata[1]) * 0x100;
      sub += a_h_n(rdata[2]) * 0x10;
      sub += a_h_n(rdata[3]);
      if( data != NULL ) *data = sub;
      if( data_str != NULL ) sprintf( data_str, "%d\0", sub );
    }
    else
    {  
      rsp = RSP_NE_READ_FAILED;
    }
  }
  return( rsp );  
}


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
response_t Write_NE_Cal_item( UINT16 item, UINT16 data )
{
  UINT8      len;
  UINT8      payload[8];
  
  if( item == 0 || item > ATLAS_MAX_CAL_ITEM ) return( RSP_PARAMETER_OUT_OF_RANGE );
  
  // In this case, the NE wants to use a 4 digit HEX number
  // for both the item number and data.  Not comma spearated!
  len = 0;
  payload[len++] = n_h_a((UINT8)(item >> 12));
  payload[len++] = n_h_a((UINT8)((item & 0x0F00) >> 8));
  payload[len++] = n_h_a((UINT8)((item & 0x00F0) >> 4));
  payload[len++] = n_h_a((UINT8)(item & 0x000F));
  payload[len++] = n_h_a((UINT8)(data >> 12));
  payload[len++] = n_h_a((UINT8)((data & 0x0F00) >> 8));
  payload[len++] = n_h_a((UINT8)((data & 0x00F0) >> 4));
  payload[len++] = n_h_a((UINT8)(data & 0x000F));
  
  return( WriteCmdPacket( CMD_PKT_WRITE_FLASH_ITEM, payload, len ) );
}


/////////////////////////////////////////////////////////////////////////////
// Function: epoch_time()
//
// Parameters: pointer to a char buffer where epochtime is placed
//
// Return: response_t -> response enum type
//
// Description: This function gathers epoch time and sends when requested
//
/////////////////////////////////////////////////////////////////////////////
response_t epoch_time( char *time_str )
{
  struct M2M_T_RTC_TIMEVAL millisec;

  if( m2m_get_timeofday( &millisec, 0 ) != 0 ) return( RSP_GENERAL_ERROR );

  if( time_str != NULL )
  {
    sprintf( time_str, "%d", millisec.tv_sec );
    //dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE, "time_str_epoch:%s", time_str);
    return( RSP_SUCCESS );
  }
  
  return( RSP_PARAMETER_OUT_OF_RANGE );
}


/////////////////////////////////////////////////////////////////////////////
// Function: PeriodicPerformanceData()
//
// Parameters: char* -> buffer to place data for "read" operations,
//                      set to NULL for unsolicited messages.
//
// Return: response_t -> response enum type
//
// Description: This function gathers various performance data stats
// and packages them up into a comma separated string for an unsolicited
// or Read command SMS/MQTT message payload.
//
// Payload=<Board_ID>,<enabled>,<Alarm>,<LED Status>,<GSM RSSI>,<Temp>,
// <A_UL rssi>,<A_DL rssi>,<A_UL gain>,<A_DL gain>,<A_UL tx pwr>,  <- Band A
// <A_DL tx pwr>,<A_UL activity>,                                  <- Band A
// <B_UL rssi>,<B_DL rssi>,<B_UL gain>,<B_DL gain>,<B_UL tx pwr>,  <- Band B
// <B_DL tx pwr>,<B_UL activity>,                                  <- Band B
// <epoch time>
//
// Note: Band B data is only present for dual band NEs (BoardID indicates)
//
/////////////////////////////////////////////////////////////////////////////
response_t PeriodicPerformanceData( char *data_buf )
{
  UINT8      i = 0;
  int        rssi;
  response_t rsp;
  char       buf[255];
  
  memset( buf, 0 , sizeof( buf ));
  
  // Board ID:
  sprintf( buf, "%d", globals.module.NE_boardID ); // Board ID
  i = strlen( buf );
  buf[i++] = COMMA;
  
  // NE Cloud Enabled Status:
  rsp = Read_NE_Cloud_Enabled( NULL, &buf[i] ); // Enabled Status
  if( rsp != RSP_SUCCESS ) return( rsp );
  i = strlen( buf );
  buf[i++] = COMMA;
  
  // Alarms:
  rsp = Read_Alarms( NULL, &buf[i] );           // Alarm Status
  if( rsp != RSP_SUCCESS ) return( rsp );
  i = strlen( buf );
  buf[i++] = COMMA;
  
  // LED status:
  rsp = Read_LED_Status( NULL, &buf[i] );       // LED status  
  if( rsp != RSP_SUCCESS ) return( rsp );
  i = strlen( buf );
  buf[i++] = COMMA;
  
  // GSM RSSI:
  rsp = Read_GSM_RSSI( &buf[i] );               // GSM RSSI
  if( rsp != RSP_SUCCESS ) return( rsp );
  i = strlen( buf );  
  buf[i++] = COMMA;
  
  // Temperature:
  rsp = Read_Temperature( &buf[i] );            // Temperature
  if( rsp != RSP_SUCCESS ) return( rsp );
  i = strlen( buf );
  buf[i++] = COMMA;
  
  // A_UL & A_DL RSSI:
  rsp = Read_UL_RSSI_A( NULL, &buf[i] );        // A_UL RSSI
  if( rsp != RSP_SUCCESS ) return( rsp );
  i = strlen( buf );
  buf[i++] = COMMA;
  rsp = Read_DL_RSSI_A( NULL, &buf[i] );        // A_DL RSSI
  if( rsp != RSP_SUCCESS ) return( rsp );
  i = strlen( buf );
  buf[i++] = COMMA;

  // A_UL & A_DL Gains:
  rsp = Read_UL_Gain_A( &buf[i] );              // A_UL Gain
  if( rsp != RSP_SUCCESS ) return( rsp );
  i = strlen( buf );
  buf[i++] = COMMA;
  rsp = Read_DL_Gain_A( &buf[i] );              // A_DL Gain
  if( rsp != RSP_SUCCESS ) return( rsp );
  i = strlen( buf );
  buf[i++] = COMMA;
  
  // A_UL & A_DL Tx Power:
  rsp = Read_UL_TX_Power_A( &buf[i] );          // A_UL TX Power
  if( rsp != RSP_SUCCESS ) return( rsp );
  i = strlen( buf );
  buf[i++] = COMMA;
  rsp = Read_DL_TX_Power_A( &buf[i] );          // A_DL TX Power
  if( rsp != RSP_SUCCESS ) return( rsp );
  i = strlen( buf );
  buf[i++] = COMMA;
  
  // A_UL Activity Detection:  (0=Antenna, 1=dummyload)
  rsp = Read_UL_Activity_State_A( NULL, &buf[i] );
  if( rsp != RSP_SUCCESS ) return( rsp );
  i = strlen( buf );
  buf[i++] = COMMA;
  
  if( globals.module.NE_boardID == TITAN_BAND1_BAND8 )
  {
    // B_UL & B_DL RSSI:
    rsp = Read_UL_RSSI_B( NULL, &buf[i] );      // B_UL RSSI
    if( rsp != RSP_SUCCESS ) return( rsp );
    i = strlen( buf );
    buf[i++] = COMMA;
    rsp = Read_DL_RSSI_B( NULL, &buf[i] );      // B_DL RSSI
    if( rsp != RSP_SUCCESS ) return( rsp );
    i = strlen( buf );
    buf[i++] = COMMA;

    // B_UL & B_DL Gains:
    rsp = Read_UL_Gain_B( &buf[i] );            // B_UL Gain
    if( rsp != RSP_SUCCESS ) return( rsp );
    i = strlen( buf );
    buf[i++] = COMMA;
    rsp = Read_DL_Gain_B( &buf[i] );            // B_DL Gain
    if( rsp != RSP_SUCCESS ) return( rsp );
    i = strlen( buf );
    buf[i++] = COMMA;
  
    // B_UL & B_DL Tx Power:
    rsp = Read_UL_TX_Power_B( &buf[i] );        // B_UL TX Power
    if( rsp != RSP_SUCCESS ) return( rsp );
    i = strlen( buf );
    buf[i++] = COMMA;
    rsp = Read_DL_TX_Power_B( &buf[i] );        // B_DL TX Power
    if( rsp != RSP_SUCCESS ) return( rsp );
    i = strlen( buf );
    buf[i++] = COMMA;
  
    // B_UL Activity Detection:  (0=Antenna, 1=dummyload)
    rsp = Read_UL_Activity_State_B( NULL, &buf[i] );
    if( rsp != RSP_SUCCESS ) return( rsp );
    i = strlen( buf );
    buf[i++] = COMMA;
  }
  
  // Epoch time
  rsp = epoch_time( &buf[i] );
  if( rsp != RSP_SUCCESS ) return( rsp );
  i = strlen( buf );

  if( i > SMS_MO_MAX_DATA_LEN ) return( RSP_GENERAL_ERROR );
  
  // We use data_buf while sending perf_data during "Reads"
  if( data_buf != NULL )
  {                       
    strcpy( data_buf, buf );
    return( rsp );
  }
  
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "PerfData:%s", buf );

  rsp = send_MO_MSG( NE_PERF_DATA, NULL, REPLY_ID_UNSOLCITED, i, buf, globals.module.conn_method );
  if( rsp == RSP_SMS_MO_FAILED )
  {
    // Retry if first MO SMS failed:
	  rsp = send_MO_MSG( NE_PERF_DATA, NULL, REPLY_ID_UNSOLCITED, i, buf, globals.module.conn_method );
  }

  return( rsp );
}


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
response_t Bootup_perf_basic1( void )
{
  UINT8      dLen;
  char       buf[SMS_MO_MAX_DATA_LEN], data[5];
  response_t rsp;
  
  memset( buf, 0, sizeof( buf ));
  memset( data, 0, sizeof( data ));

  strcpy( buf, globals.module.apn ); // APN
  dLen = strlen( buf );
  buf[dLen++] = COMMA;

  strcpy( &buf[dLen], globals.module.HB_intvl ); // MQTT Heartbeat interval
  dLen = strlen( buf );
  buf[dLen++] = COMMA;

  strcpy( &buf[dLen], globals.module.perf_intvl ); // Performance interval
  dLen = strlen( buf );
  buf[dLen++] = COMMA;

  strcpy( &buf[dLen], globals.module.sms_addr ); // SMS_short_code
  dLen = strlen( buf );
  buf[dLen++] = COMMA;

  sprintf(data,"%d",globals.module.conn_method); // Connection Method 
  strcpy( &buf[dLen], data );
  dLen = strlen( buf );
  buf[dLen++] = COMMA;

  strcpy( &buf[dLen], globals.module.firmware ); // Module software version
  dLen = strlen( buf );
  buf[dLen++] = COMMA;

  strcpy( &buf[dLen], software_version );        // TCA software version
  dLen = strlen( buf );
  buf[dLen++] = COMMA;

  strcpy( &buf[dLen], globals.module.ne_version); // NE's software version
  dLen = strlen( buf );
  buf[dLen++] = COMMA;
  
  rsp = epoch_time( &buf[dLen]);                  // EpochTime(current time)
  if( rsp != RSP_SUCCESS ) return( rsp );
  dLen = strlen( buf );
  
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Length_basic1:%d", dLen );

  if( dLen > SMS_MO_MAX_DATA_LEN ) return( RSP_SMS_FORMAT_ERROR );

  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "PerfData_basic1:%s", buf );
  
  rsp = send_MO_MSG( NE_CONF_PART1, NULL, REPLY_ID_UNSOLCITED, dLen, buf ,globals.module.conn_method );
  if( rsp == RSP_SMS_MO_FAILED )
  {
    // Retry if first MO SMS failed:
    rsp = send_MO_MSG( NE_CONF_PART1, NULL, REPLY_ID_UNSOLCITED, dLen, buf ,globals.module.conn_method );
  }

  return( rsp );
}


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
response_t Bootup_perf_basic2( void )
{
  UINT8      dLen = 0;
  char       buf[SMS_MO_MAX_DATA_LEN];
  char       ul_ch[10], dl_ch[10];
  char       ul_frequ[10], dl_frequ[10];
  response_t rsp;

  memset( buf,      0, sizeof( buf ));
  memset( ul_ch,    0, sizeof( ul_ch ));
  memset( ul_frequ, 0, sizeof( ul_frequ ));
  memset( dl_ch,    0, sizeof( dl_ch ));
  memset( dl_frequ, 0, sizeof( dl_frequ ));

  rsp = Read_UL_Channel_A( NULL, &ul_ch[0], &ul_frequ[0]);
  if( rsp != RSP_SUCCESS ) return( rsp );

  rsp = Read_DL_Channel_A( NULL, &dl_ch[0], &dl_frequ[0]);
  if( rsp != RSP_SUCCESS ) return( rsp );

  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "ul_ch:%s\n",&ul_ch[0]);
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "%s\n",&dl_ch[0]);
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "%s\n",&ul_frequ[0]);
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "%s\n",&dl_frequ[0]);

  strcpy( buf, &ul_ch[0] ); // Uplink Channel
  dLen = strlen( buf );
  buf[dLen++] = COMMA;

  strcpy( &buf[dLen], &dl_ch[0] ); // Downlink Channel
  dLen = strlen( buf );
  buf[dLen++] = COMMA;

  strcpy( &buf[dLen], &ul_frequ[0] ); // Uplink Frequency
  dLen = strlen( buf );
  buf[dLen++] = COMMA;

  strcpy( &buf[dLen], &dl_frequ[0] ); // Downlink Frequency
  dLen = strlen( buf );
  buf[dLen++] = COMMA;

  strcpy( &buf[dLen], globals.module.mqttIP ); // MQTTServerURL
  dLen = strlen( buf );
  buf[dLen++] = COMMA;

  strcpy( &buf[dLen], globals.module.mqttPWD ); // MQTT Password
  dLen = strlen( buf );
  buf[dLen++] = COMMA;
  
  rsp = epoch_time( &buf[dLen]);                  // EpochTime(current time)
  if( rsp != RSP_SUCCESS ) return( rsp );
  dLen = strlen( buf );

  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Length_basic2:%d", dLen );

  if( dLen > SMS_MO_MAX_DATA_LEN ) return( RSP_SMS_FORMAT_ERROR );

  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "PerfData_basic2:%s", buf );

  rsp = send_MO_MSG( NE_CONF_PART2, NULL, REPLY_ID_UNSOLCITED, dLen, buf ,globals.module.conn_method );
  if( rsp == RSP_SMS_MO_FAILED )
  {
    // Retry if first MO SMS failed:
    rsp = send_MO_MSG( NE_CONF_PART2, NULL, REPLY_ID_UNSOLCITED, dLen, buf ,globals.module.conn_method );
  }

  return( rsp );
}


/////////////////////////////////////////////////////////////////////////////
// Function: Bootup_perf_basic3()
//
// Parameters: none
//
// Return: response_t -> response enum type
//
// Description: This function gathers various stats about the module
// and packages them up into a comma separated string for an unsolicited
// SMS or MQTT message payload.  This message is typically sent at
// boot-up once, after cell service if established, when the RMS
// Activation has not occurred yet.
//
/////////////////////////////////////////////////////////////////////////////
response_t Bootup_perf_basic3( void )
{
  UINT8      dLen;
  char       buf[SMS_MO_MAX_DATA_LEN], data[20];
  response_t rsp;

  memset( buf, 0, sizeof( buf ));

  strcpy( buf, globals.module.firmware );                 // Telit's firmware version
  dLen = strlen( buf );
  buf[dLen++] = COMMA;

  strcpy( &buf[dLen], software_version );                  // TCA software version
  dLen = strlen( buf );
  buf[dLen++] = COMMA;

  strcpy( &buf[dLen], globals.module.ne_version );         // NE's software version
  dLen = strlen( buf );
  buf[dLen++] = COMMA;

  strcpy( &buf[dLen], globals.module.location.latitude );  // Latitude
  dLen = strlen( buf );
  buf[dLen++] = COMMA;

  strcpy( &buf[dLen], globals.module.location.longitude ); // Longitude
  dLen = strlen( buf );
  buf[dLen++] = COMMA;

  sprintf( data,"%d", globals.module.location.accuracy );  // Accuracy
  strcpy( &buf[dLen], data );
  dLen = strlen( buf );
  buf[dLen++] = COMMA;
  
  rsp = epoch_time( &buf[dLen]);                  // EpochTime(current time)
  if( rsp != RSP_SUCCESS ) return( rsp );
  dLen = strlen( buf );

  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Length_basic3:%d", dLen );

  if( dLen > SMS_MO_MAX_DATA_LEN ) return( RSP_SMS_FORMAT_ERROR );

  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "PerfData_basic3:%s", buf );

  rsp = send_MO_MSG( NE_CONF_PART3, NULL, REPLY_ID_UNSOLCITED, dLen, buf ,globals.module.conn_method );
  if( rsp == RSP_SMS_MO_FAILED )
  {
    // Retry if first MO SMS failed:
    rsp = send_MO_MSG( NE_CONF_PART3, NULL, REPLY_ID_UNSOLCITED, dLen, buf ,globals.module.conn_method );
  }

  return( rsp );
}


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
response_t PeriodicLocationData( void )
{
  UINT8      dLen;
  char       buf[SMS_MO_MAX_DATA_LEN];
  response_t rsp;

  rsp = Req_location_info();
  if( rsp != RSP_SUCCESS ) return( rsp );

  strcpy( buf, globals.module.location.latitude );
  dLen = strlen( buf );
  buf[dLen++] = COMMA;
  strcpy( &buf[dLen], globals.module.location.longitude );
  dLen = strlen( buf );
  buf[dLen++] = COMMA;
  sprintf( &buf[dLen], "%d", globals.module.location.accuracy );
  dLen = strlen( buf );
  buf[dLen++] = COMMA;
  rsp = epoch_time( &buf[dLen] );          // EpochTime(current time)
  if( rsp != RSP_SUCCESS ) return( rsp );
  dLen = strlen( buf );


  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Location_len:%d", dLen );

  if( dLen > SMS_MO_MAX_DATA_LEN ) return( RSP_SMS_FORMAT_ERROR );

  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "LOCATION_UNSOLCITED_DATA:%s", buf );

  rsp = send_MO_MSG( NE_LOC_DATA, NULL, REPLY_ID_UNSOLCITED, dLen, buf ,globals.module.conn_method );
  if( rsp == RSP_SMS_MO_FAILED )
  {
    // Retry if first MO SMS failed:
    rsp = send_MO_MSG( NE_LOC_DATA, NULL, REPLY_ID_UNSOLCITED, dLen, buf ,globals.module.conn_method );
  }
  
  return( rsp );
}


/////////////////////////////////////////////////////////////////////////////
// Function: FTP_version()
//
// Parameters: version
//
// Return: response_t -> response enum type
//
// Description: This function sends the current and previous version of 
// Telit application to RMS.  It will send the msgID stored in the config
// file from the original start FTP msg before re-boot.
//
/////////////////////////////////////////////////////////////////////////////
response_t FTP_version( char version[APP_VER_LEN],
                        char msg_id[SMS_HDR_MSG_ID_LEN+1] )
{
  UINT8      dLen;
  char       buf[SMS_MO_MAX_DATA_LEN];
  response_t rsp;
  
  memset( buf, 0, sizeof( buf ));

  strcpy( buf, version );
  dLen = strlen( buf );
  buf[dLen++] = COMMA;

  strcpy(&buf[dLen], software_version);
  dLen = strlen( buf );
  buf[dLen++] = COMMA;

  rsp = epoch_time( &buf[dLen] );         // EpochTime(current time)
  if( rsp != RSP_SUCCESS ) return( rsp );  
  dLen = strlen( buf );
  
  if( dLen > SMS_MO_MAX_DATA_LEN ) 
  {
    NE_debug("FTP Resp Len exceeded");
    return( RSP_GENERAL_ERROR );
  }

  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Changed Version:%s", buf );
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "msg_id:%s", msg_id );

  rsp = send_MO_MSG( NE_FTP_VER, msg_id, REPLY_ID_UNSOLCITED, dLen, buf,globals.module.conn_method );
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Res_status:%d", rsp );
  if( rsp == RSP_SMS_MO_FAILED ) send_MO_MSG( NE_FTP_VER, msg_id, REPLY_ID_UNSOLCITED, dLen, buf ,globals.module.conn_method );
  
  return( rsp );
}


/////////////////////////////////////////////////////////////////////////////
// Function: FTP_Error_Resp()
//
// Parameters: version
//
// Return: response_t -> response enum type
//
// Description: This function sends the error code if unable to
// download along with current version to RMS.
//
/////////////////////////////////////////////////////////////////////////////
response_t FTP_Error_Resp( char msg_id[SMS_HDR_MSG_ID_LEN+1] )
{
  UINT8      dLen;
  char       buf[SMS_MO_MAX_DATA_LEN], rdata[6];
  response_t rsp;

  memset( buf, 0, sizeof( buf ));
  memset( rdata, 0, sizeof( rdata ));      // Error value

  sprintf( buf, "%d", (int)RSP_FTP_FILE_DOWNLOAD_FAIL );

  strcpy( buf, rdata );
  dLen = strlen( buf );
  buf[dLen++] = COMMA;

  strcpy( &buf[dLen], software_version );  // Current Software Version
  dLen = strlen( buf );
  buf[dLen++] = COMMA;
  
  rsp = epoch_time( &buf[dLen]);          // EpochTime(current time)
  if( rsp != RSP_SUCCESS ) return( rsp );
  dLen = strlen( buf );
  
  if( dLen > SMS_MO_MAX_DATA_LEN )
  {
    NE_debug("FTP Resp Len exceeded");
    return( RSP_GENERAL_ERROR );
  }

  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Res_buffer:%s", buf );
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Msg_id:%s", msg_id );

  rsp = send_MO_MSG( NE_FTP_VER, msg_id, REPLY_ID_FAILED, dLen, buf,globals.module.conn_method );
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Res_status:%d",rsp );
  if( rsp == RSP_SMS_MO_FAILED ) send_MO_MSG( NE_FTP_VER, msg_id, REPLY_ID_FAILED, dLen, buf,globals.module.conn_method );

  return( rsp );
}


#endif // FEATURE_5BARZ
