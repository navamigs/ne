/*************************************************************************
                    5BARz International Incorporated.

                 (C) Copyright 2017. All Rights Reserved.

Filename: cmd_pkt.h

Description: This header file contains interfaces to functions to read 
and write to the NE via the serial interface

*************************************************************************/
#ifndef	_CMD_PKT_H
#define _CMD_PKT_H

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
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
#include "m2m_cloud_defines.h"


#define CMD_PKT_PING                      0x1000
#define CMD_PKT_READ_SERIAL_NUMBER        0x1001
#define CMD_PKT_READ_BOARD_ID             0x1002
#define CMD_PKT_RESET_NE                  0x1003
#define CMD_PKT_SET_SERIAL_LOCK           0x1005
#define CMD_PKT_SET_SERIAL_UNLOCK         0x1006
#define CMD_PKT_READ_ALARM_CODE           0x1007
#define CMD_PKT_READ_LED_STATE            0x1008
#define CMD_PKT_SET_MODULE_WATCHDOG       0x1009
#define CMD_PKT_SEND_DEBUG_MSG            0x100A

#define CMD_PKT_RESET_MODULE              0x1100

#define CMD_PKT_READ_CHAN_A_DL            0x2000
#define CMD_PKT_READ_CHAN_A_UL            0x2001
#define CMD_PKT_WRITE_CHAN_A_DL           0x2002
#define CMD_PKT_WRITE_CHAN_A_UL           0x2003

#define CMD_PKT_READ_CHAN_B_DL            0x2004
#define CMD_PKT_READ_CHAN_B_UL            0x2005
#define CMD_PKT_WRITE_CHAN_B_DL           0x2006
#define CMD_PKT_WRITE_CHAN_B_UL           0x2007
   
#define CMD_PKT_READ_FLASH_ITEM           0x3000
#define CMD_PKT_WRITE_FLASH_ITEM          0x3001
#define CMD_PKT_READ_FLASH_LOCK           0x3002
#define CMD_PKT_LOCK_FLASH                0x3003

#define CMD_PKT_READ_TEMPERATURE          0x4012
#define CMD_PKT_READ_SW_VERSION           0x4013
#define CMD_PKT_READ_ACTVTY_DET_A_UL      0x4014
#define CMD_PKT_READ_ACTVTY_DET_B_UL      0x4015
   
#define CMD_PKT_READ_RSSI_A_DL            0x4050
#define CMD_PKT_READ_RSSI_A_UL            0x4051
#define CMD_PKT_READ_RSSI_B_DL            0x4052
#define CMD_PKT_READ_RSSI_B_UL            0x4053

#define CMD_PKT_READ_GAIN_A_DL            0x4054
#define CMD_PKT_READ_GAIN_A_UL            0x4055
#define CMD_PKT_READ_GAIN_B_DL            0x4056
#define CMD_PKT_READ_GAIN_B_UL            0x4057

#define CMD_PKT_READ_TX_PWR_A_DL          0x4058
#define CMD_PKT_READ_TX_PWR_A_UL          0x4059
#define CMD_PKT_READ_TX_PWR_B_DL          0x405A
#define CMD_PKT_READ_TX_PWR_B_UL          0x405B

#define CMD_PKT_NV_WRITE_BOOT_MODE        0x4400
#define CMD_PKT_NV_READ_BOOT_MODE         0x4401
#define CMD_PKT_NV_WRITE_LOG_MASK         0x4402
#define CMD_PKT_NV_READ_LOG_MASK          0x4403
#define CMD_PKT_START_OTA_LOGGING         0x4404
#define CMD_PKT_STOP_OTA_LOGGING          0x4405

#define CMD_PKT_SET_CLOUD_ENABLE_ALL      0x4410
#define CMD_PKT_SET_CLOUD_DISABLE         0x4411
#define CMD_PKT_READ_CLOUD_ENABLE         0x4412
#define CMD_PKT_NOTIFY_CELL_SERVICE       0x4413
#define CMD_PKT_NO_SIM_CARD_PRESENT       0x4414
#define CMD_PKT_MODULE_SOFTWARE_FAILURE   0x4415   
#define CMD_PKT_SET_CLOUD_ENABLE_A_ONLY   0x4416
#define CMD_PKT_SET_CLOUD_ENABLE_B_ONLY   0x4417
   
#define CMD_PKT_ERROR_PARSING             0x9999
#define CMD_PKT_ERROR_READ_FAIL           0x999A
#define CMD_PKT_ERROR_WRITE_FAIL          0x999B
#define CMD_PKT_ERROR_PARM_OUT_RANGE      0x999C
#define CMD_PKT_ERROR_NOT_SUPPORTED       0x999D
   
////////////////////////////////////////////////
//    Define Reverse Flow Command Packets     //   
////////////////////////////////////////////////
#define CMD_PKT_START_OF_REVERSE_CMD_PKT  0xF000   
#define CMD_PKT_RV_OTA_LOG_DATA           0xF001
#define CMD_PKT_RV_WATCHDOG_EXPIRED       0xF002
   
#define CMD_PKT_RV_WRITE_CARRIER_NAME     0xF100
#define CMD_PKT_RV_READ_CARRIER_NAME      0xF101
#define CMD_PKT_RV_WRITE_RMS_ACTIVATE     0xF102
#define CMD_PKT_RV_READ_RMS_ACTIVATE      0xF103
#define CMD_PKT_RV_WRITE_CONN_METHOD      0xF104
#define CMD_PKT_RV_READ_CONN_METHOD       0xF105
#define CMD_PKT_RV_WRITE_APN              0xF106
#define CMD_PKT_RV_READ_APN               0xF107
#define CMD_PKT_RV_WRITE_SMS_SHORT_CODE   0xF108
#define CMD_PKT_RV_READ_SMS_SHORT_CODE    0xF109
#define CMD_PKT_RV_WRITE_MQTT_PASSWORD    0xF10A
#define CMD_PKT_RV_READ_MQTT_PASSWORD     0xF10B
#define CMD_PKT_RV_WRITE_MQTT_URL         0xF10C
#define CMD_PKT_RV_READ_MQTT_URL          0xF10D
#define CMD_PKT_RV_WRITE_MQTT_HEARTBEAT   0xF10E
#define CMD_PKT_RV_READ_MQTT_HEARTBEAT    0xF10F
#define CMD_PKT_RV_WRITE_PERF_INTERVAL    0xF110
#define CMD_PKT_RV_READ_PERF_INTERVAL     0xF111
#define CMD_PKT_RV_WRITE_LOC_INTERVAL     0xF112
#define CMD_PKT_RV_READ_LOC_INTERVAL      0xF113
#define CMD_PKT_RV_WRITE_TCA_VERSION      0xF114
#define CMD_PKT_RV_READ_TCA_VERSION       0xF115
#define CMD_PKT_RV_WRITE_LOG_FTP_URL      0xF116
#define CMD_PKT_RV_READ_LOG_FTP_URL       0xF117
#define CMD_PKT_RV_WRITE_LOG_FTP_PATH     0xF118
#define CMD_PKT_RV_READ_LOG_FTP_PATH      0xF119
#define CMD_PKT_RV_WRITE_LOG_FTP_USERID   0xF11A
#define CMD_PKT_RV_READ_LOG_FTP_USERID    0xF11B
#define CMD_PKT_RV_WRITE_LOG_FTP_PASSWORD 0xF11C
#define CMD_PKT_RV_READ_LOG_FTP_PASSWORD  0xF11D
#define CMD_PKT_RV_WRITE_LOG_FTP_START    0xF11E
#define CMD_PKT_RV_READ_LOG_FTP_START     0xF11F
#define CMD_PKT_RV_WRITE_LOG_FTP_STOP     0xF120
#define CMD_PKT_RV_READ_LOG_FTP_STOP      0xF121

   
////////////////////////////////////////////////
//     Define Tunneling Command Packets       //
////////////////////////////////////////////////
#define CMD_PKT_TN_WRITE_CARRIER_NAME     0xF700
#define CMD_PKT_TN_READ_CARRIER_NAME      0xF701
#define CMD_PKT_TN_WRITE_RMS_ACTIVATE     0xF702
#define CMD_PKT_TN_READ_RMS_ACTIVATE      0xF703
#define CMD_PKT_TN_WRITE_CONN_METHOD      0xF704
#define CMD_PKT_TN_READ_CONN_METHOD       0xF705
#define CMD_PKT_TN_WRITE_APN              0xF706
#define CMD_PKT_TN_READ_APN               0xF707
#define CMD_PKT_TN_WRITE_SMS_SHORT_CODE   0xF708
#define CMD_PKT_TN_READ_SMS_SHORT_CODE    0xF709
#define CMD_PKT_TN_WRITE_MQTT_PASSWORD    0xF70A
#define CMD_PKT_TN_READ_MQTT_PASSWORD     0xF70B
#define CMD_PKT_TN_WRITE_MQTT_URL         0xF70C
#define CMD_PKT_TN_READ_MQTT_URL          0xF70D
#define CMD_PKT_TN_WRITE_MQTT_HEARTBEAT   0xF70E
#define CMD_PKT_TN_READ_MQTT_HEARTBEAT    0xF70F
#define CMD_PKT_TN_WRITE_PERF_INTERVAL    0xF710
#define CMD_PKT_TN_READ_PERF_INTERVAL     0xF711
#define CMD_PKT_TN_WRITE_LOC_INTERVAL     0xF712
#define CMD_PKT_TN_READ_LOC_INTERVAL      0xF713
#define CMD_PKT_TN_WRITE_TCA_VERSION      0xF714
#define CMD_PKT_TN_READ_TCA_VERSION       0xF715
#define CMD_PKT_TN_WRITE_LOG_FTP_URL      0xF716
#define CMD_PKT_TN_READ_LOG_FTP_URL       0xF717
#define CMD_PKT_TN_WRITE_LOG_FTP_PATH     0xF718
#define CMD_PKT_TN_READ_LOG_FTP_PATH      0xF719
#define CMD_PKT_TN_WRITE_LOG_FTP_USERID   0xF71A
#define CMD_PKT_TN_READ_LOG_FTP_USERID    0xF71B
#define CMD_PKT_TN_WRITE_LOG_FTP_PASSWORD 0xF71C
#define CMD_PKT_TN_READ_LOG_FTP_PASSWORD  0xF71D
#define CMD_PKT_TN_WRITE_LOG_FTP_START    0xF71E
#define CMD_PKT_TN_READ_LOG_FTP_START     0xF71F
#define CMD_PKT_TN_WRITE_LOG_FTP_STOP     0xF720
#define CMD_PKT_TN_READ_LOG_FTP_STOP      0xF721

#define CMD_PKT_TN_READ_MODULE_RSSI       0xF722
#define CMD_PKT_TN_READ_MODULE_CHANNEL    0xF723
#define CMD_PKT_TN_READ_MODULE_OPERATER   0xF724
#define CMD_PKT_TN_READ_MODULE_FIRMWARE   0xF725
#define CMD_PKT_TN_READ_TCA_SOFTWARE      0xF726
#define CMD_PKT_TN_READ_PHONE_NUMBER      0xF727
   
#define CMD_PKT_RV_ERROR                  0xFFFF
   


#define CMD_PKT_MAX_DATA_PAYLOAD          50
#define CMD_PKT_MIN_LEN                   8      // Minimum length packet
#define CMD_PKT_MAX_LEN                   (CMD_PKT_MAX_DATA_PAYLOAD + CMD_PKT_MIN_LEN)
#define CMD_PKT_DATA_START_POS            5      // Data payload start postion within packet

#define STX                               0x02  // Start of Transmission    (start of cmd_pkt)
#define ETX                               0x03  // End of Transmission      (end of cmd_pkt)
#define NAK                               0x15  // Negative Acknowledgement (start of cmd_pkt)   
     
#define ATLAS_MAX_CAL_ITEM                505   // Max address offset within Atlas Flash 
   
   

#define TAB      0x09  // Tab
#define CARR_RET 0x0D  // Carriage Return
#define LINEFEED 0x0A  // Line Feed
#define EQUAL    0x3D  // "="
#define QUESTION 0x3F  // "?"
#define SPACE    0x20  // " "
#define COLON    0x3A  // ":"
#define COMMA    0x2C  // ","
#define PERIOD   0X2e  // "."
#define UNDER_   0x5F  // "_"
#define GREATER  0x3E  // ">" 
#define STAR     0x2A  // "*"
#define MINUS    0x2D  // "-"
#define ANDPER   0x26  // "&"
#define ATSIGN   0x40  // "@"
#define FSLASH   0x2F  // "/"   
   
#define CHAR_A   0x41  // "A"
#define CHAR_B   0x42  // "B"
#define CHAR_C   0x43  // "C"   
#define CHAR_D   0x44  // "D"
#define CHAR_E   0x45  // "E"    
#define CHAR_F   0x46  // "F"    
#define CHAR_G   0x47  // "G"
#define CHAR_H   0x48  // "H"
#define CHAR_I   0x49  // "I"
#define CHAR_J   0x4A  // "J"
#define CHAR_K   0x4B  // "K"
#define CHAR_L   0x4C  // "L"
#define CHAR_M   0x4D  // "M" 
#define CHAR_N   0x4E  // "N" 
#define CHAR_O   0x4F  // "O" 
#define CHAR_P   0x50  // "P"
#define CHAR_Q   0x51  // "Q"
#define CHAR_R   0x52  // "R"
#define CHAR_S   0x53  // "S"
#define CHAR_T   0x54  // "T"
#define CHAR_U   0x55  // "U"
#define CHAR_V   0x56  // "V"
#define CHAR_W   0x57  // "W"
#define CHAR_X   0x58  // "X"
#define CHAR_Y   0x59  // "Y"
#define CHAR_Z   0x5A  // "Z"
#define NUM0     0x30  // "0"
#define NUM1     0x31  // "1"
#define NUM2     0x32  // "2"
#define NUM3     0x33  // "3"
#define NUM4     0x34  // "4"
#define NUM5     0x35  // "5"
#define NUM6     0x36  // "6"
#define NUM7     0x37  // "7"
#define NUM8     0x38  // "8"
#define NUM9     0x39  // "9"
   
#define CHAR_a   0x61  // "e"   
#define CHAR_b   0x62  // "b"
#define CHAR_c   0x63  // "c"
#define CHAR_d   0x64  // "d"
#define CHAR_e   0x65  // "e"
#define CHAR_f   0x66  // "f"
#define CHAR_g   0x67  // "g"
#define CHAR_h   0x68  // "h"
#define CHAR_i   0x69  // "i"
#define CHAR_j   0x6A  // "j"
#define CHAR_k   0x6B  // "k"
#define CHAR_m   0x6D  // "m"
#define CHAR_n   0x6E  // "n"
#define CHAR_o   0x6F  // "o"
#define CHAR_l   0x6C  // "l"
#define CHAR_q   0x71  // "q"
#define CHAR_r   0x72  // "r"
#define CHAR_s   0x73  // "s"
#define CHAR_t   0x74  // "t"
#define CHAR_u   0x75  // "u"
#define CHAR_v   0x76  // "v"
#define CHAR_w   0x77  // "w"
#define CHAR_x   0x78  // "x"
#define CHAR_y   0x79  // "y"
#define CHAR_z   0x7A  // "z"
   
 
/////////////////////////////////////////////////////////////////////////////
// Function: Init_uart()
//
// Parameters: none
//
// Return: bool - true if successful
//
// Description: This function opens the uart for general use.
//
/////////////////////////////////////////////////////////////////////////////
bool Init_uart(void);


/////////////////////////////////////////////////////////////////////////////
// Function: ProcessRevCmdPacket()
//
// Parameters: void
//
// Return: void
//
// Description: This function finds & processes the most recent reverse 
// cmd_pkt on the RX Queue.
//
/////////////////////////////////////////////////////////////////////////////
void ProcessRevCmdPacket( void );


/////////////////////////////////////////////////////////////////////////////
// Function: WriteCmdPacket()
//
// Parameters: UINT16 - the command packet ID number
//             LPWSTR - pointer to the string of data to be written
//             UINT8  - length of the data string
//
// Return: response_t -> response enum type
//
// Description: This function Sends a Write request command packet to the NE
// with a data payload, parses the response, and returns response_t type.
//
/////////////////////////////////////////////////////////////////////////////
response_t WriteCmdPacket(UINT16 cmd_pkt, UINT8 *szData, UINT8 length);


/////////////////////////////////////////////////////////////////////////////
// Function: ReadCmdPacket()
//
// Parameters: UINT16 - the command packet ID number
//             UINT8* - pointer to the string of returned data
//             UINT8* - length of the return data string
//
// Return: response_t -> response enum type
//
// Description: This function Sends a Read request command packet to the
// NE, parses the response, and retruns the data as a string.
//
/////////////////////////////////////////////////////////////////////////////
response_t ReadCmdPacket(UINT16 cmd_pkt, UINT8 *data, UINT8 *length);


/////////////////////////////////////////////////////////////////////////////
// Function: ReadCmdPacket_payload()
//
// Parameters: UINT16 - the command packet ID number
//             UINT8* - pointer to the string of payload data
//             UINT8  - length of the payload data string
//             UINT8* - pointer to the string of returned data
//             UINT8  - length of the return data string
//
// Return: response_t -> response enum type
//
// Description: This function Sends a Read request command packet
// with a data payload to the NE, parses the response, and returns 
// the data as a string.
//
/////////////////////////////////////////////////////////////////////////////
response_t ReadCmdPacket_payload( UINT16 cmd_pkt, UINT8 *payload, UINT8 pyld_len,
                                  UINT8 *data, UINT8 *length );


/////////////////////////////////////////////////////////////////////////////
// Function: ReadCmdPkt16()
//
// Parameters: UINT16  - the command packet ID number
//             *UINT16 - pointer to 16 bit data payload read
//
// Return: response_t -> response enum type
//
// Description: This function is a wrapper that sends a Read request command
// packet (with no data payload) to the NE with that returns a 16-bit, 4
// character, hexadecimal data payload returned.  Data gets converted from
// ASCII string representation of a real 16 bit value.
//
/////////////////////////////////////////////////////////////////////////////
response_t ReadCmdPkt16(UINT16 cmd_pkt, UINT16 *data);


/////////////////////////////////////////////////////////////////////////////
// Function: WriteCmdPkt16()
//
// Parameters: UINT16 - the command packet ID number
//             UINT16 - 16 bit data payload to be written
//
// Return: Return: response_t -> response enum type
//
// Description: This function is a wrapper that sends a Write request command
// packet to the NE with a 16-bit data payload.  Data gets converted to an
// ASCII representation of a 4 digit hex number, range 0000 - FFFF.
//
/////////////////////////////////////////////////////////////////////////////
response_t WriteCmdPkt16(UINT16 cmd_pkt, UINT16 data);


/////////////////////////////////////////////////////////////////////////////
// Function: WriteCmdPkt0()
//
// Parameters: UINT16 - the command packet ID number
//
// Return: Return: response_t -> response enum type
//
// Description: This function is a wrapper that sends a Write request command
// packet to the NE with a no data payload.
//
/////////////////////////////////////////////////////////////////////////////
response_t WriteCmdPkt0(UINT16 cmd_pkt);


/////////////////////////////////////////////////////////////////////////////
// Function: n_h_a()
//
// Parameters: byte - numeric value, 0 - 15.
//
// Return: byte: ascii representation of a 4-bit nibble
//
// Description: This function converts a 4-bit nibble into an ascii
// hex value (0-9,A-F).
//
/////////////////////////////////////////////////////////////////////////////
UINT8 n_h_a(UINT8 num);


/////////////////////////////////////////////////////////////////////////////
// Function: a_h_n()
//
// Parameters: byte: ascii representation of a 4-bit nibble
//
// Return: numeric value, 0 - 15.
//
// Description: This function converts an ascii represented hex
// nibble into a numberic 0-15 value.
//
/////////////////////////////////////////////////////////////////////////////
UINT8 a_h_n(UINT8 hex);


/////////////////////////////////////////////////////////////////////////////
// Function: isHex()
//
// Parameters: byte: ascii representation of a 4-bit nibble
//
// Return: bool -> true if char value is an ascii represented hex nibble.
//
// Description: This function tests whether an ascii char is a proper hex 
// nibble value: 0-9, a-f, A-F
//
/////////////////////////////////////////////////////////////////////////////
bool isHex(char hex);


#ifdef __cplusplus
}
#endif

#endif /* _CMD_PKT_H */
