/*************************************************************************
                    5BARz International Incorporated.

                 (C) Copyright 2016. All Rights Reserved.

Filename: cmd_pkt.c

Description: This file contains functions to read and write to the NE
via the serial interface

*************************************************************************/

#include "cmd_pkt.h"
#include "m2m_hw_api.h"
#include "m2m_cloud_types.h"
#include "fs_file.h"

// Global Variables:
M2M_CLOUD_GLOBALS  globals;

#define RX_QUEUE_SIZE     15
#define NOT_FOUND         -1
#define NE_DELAY          50     // 50 msec

// Local Types:
typedef struct
{
  UINT16        cmd_pkt;
  UINT8         dlen;
  UINT8         data[CMD_PKT_MAX_DATA_PAYLOAD];
  bool          nak;
  volatile bool lock;
} rx_queue_t;

typedef struct
{
  int         last;
  rx_queue_t  item[RX_QUEUE_SIZE];
} rx_queue_struct_t;



// Local Variables:
M2M_T_HW_UART_IO_HW_OPTIONS uart_settings;
M2M_T_HW_UART_HANDLE        hUART;
M2M_T_OS_LOCK               hLOCK;

rx_queue_struct_t           rx_queue;

// Local Prototypes:
static INT32 uart_read_cb( M2M_T_HW_UART_HANDLE handle, char *buf, INT32 len );
int   search_rx_queue( UINT16 cmd_pkt );
void  Free_rx_queue_item( int index );
void  ProcessRevCmdPkt_command( UINT16 cmd_pkt, UINT8 *buf, UINT8 dlen );
UINT8 format_cmd_pkt( UINT8 *buf, bool ack, UINT16 cmd_pkt, UINT8 *data, UINT8 dLen );


/////////////////////////////////////////////////////////////////////////////
// Function: ReadCmdPacket()
//
// Parameters: UINT16 - the command packet ID number
//             UINT8* - pointer to the string of returned data
//             UINT8  - length of the return data string
//
// Return: response_t -> response enum type
//
// Description: This function Sends a Read request command packet to the
// NE, parses the response, and returns the data as a string.
//
/////////////////////////////////////////////////////////////////////////////
response_t ReadCmdPacket(UINT16 cmd_pkt, UINT8 *data, UINT8 *length)
{
  CHAR    buf_q[CMD_PKT_MAX_LEN], buf_r[CMD_PKT_MAX_DATA_PAYLOAD];
  UINT8   len, i;
  INT32   sent;
  UINT16  error, checksum_q;
  bool    nak;
  int     index;

  // Init:
  len = 0;
  checksum_q = 0;
  *length = 0;

  // Format Request Packet:
  buf_q[len++] = STX;
  buf_q[len++] = n_h_a((UINT8)(cmd_pkt >> 12));
  buf_q[len++] = n_h_a((UINT8)((cmd_pkt & 0x0F00) >> 8));
  buf_q[len++] = n_h_a((UINT8)((cmd_pkt & 0x00F0) >> 4));
  buf_q[len++] = n_h_a((UINT8)(cmd_pkt & 0x000F));

  // Calculate checksum:
  for (i = 0; i < len; i++)
  {
    checksum_q += buf_q[i];
  }
  checksum_q &= 0x00FF;
  buf_q[len++] = n_h_a((UINT8)(checksum_q >> 4));
  buf_q[len++] = n_h_a((UINT8)(checksum_q & 0x000F));
  buf_q[len++] = ETX;

  // TX the Request packet:
  if( m2m_hw_uart_write(hUART, buf_q, len, &sent ) != M2M_HW_UART_RESULT_SUCCESS )
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "CMD_PKT Read: TX failed");
	  return( RSP_NE_COMM_ERROR );
  }
  
  // Look for RX packet in the queue with 50msec timeout:
  for( i = 0; i < NE_DELAY; i++)
  {    
    index = search_rx_queue( cmd_pkt );
    if( index != NOT_FOUND ) break;
    m2m_os_sleep_ms(1);
  }

  if( index != NOT_FOUND ) // Found our RX packet
  {
    rx_queue.item[index].lock = true; // Lock it for reading
    nak = rx_queue.item[index].nak;
    len = rx_queue.item[index].dlen;
    if( len > 0 ) 
    {
      memset( buf_r, 0, CMD_PKT_MAX_DATA_PAYLOAD );
      memcpy( buf_r, rx_queue.item[index].data, len );
    }
    Free_rx_queue_item( index );
  }
  else // Nothing in the queue
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Error: response timeout from NE:%4X",cmd_pkt);
    return( RSP_NE_COMM_ERROR );
  } 
  
  // Process the returned data:
  if( !nak && len > 0 ) // Normal STX packet
  {
    memcpy( data, buf_r, len );
    *length = len; // set the data length ptr
    return( RSP_SUCCESS );
  }    
  
  if( nak && len > 0 ) // NAK packet
  {
    error  = a_h_n(buf_r[0]) * 0x1000;
    error += a_h_n(buf_r[1]) * 0x100;
    error += a_h_n(buf_r[2]) * 0x10;
    error += a_h_n(buf_r[3]);

    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "CMD_PKT Read Command failed %04X", error );
    if( error == CMD_PKT_ERROR_READ_FAIL )      return ( RSP_NE_READ_FAILED );
    if( error == CMD_PKT_ERROR_WRITE_FAIL )     return ( RSP_NE_WRITE_FAILED );
    if( error == CMD_PKT_ERROR_PARM_OUT_RANGE ) return ( RSP_PARAMETER_OUT_OF_RANGE );
    if( error == CMD_PKT_ERROR_NOT_SUPPORTED )  return ( RSP_NOT_SUPPORTED );
    if( error == CMD_PKT_ERROR_PARSING )        return ( RSP_NE_COMM_ERROR );
  }
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "CMD_PKT Read failed without NAK, cmd=%04X",cmd_pkt);
  return ( RSP_NE_COMM_ERROR );
  
}


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
                                  UINT8 *data, UINT8 *length )
{
  CHAR    buf_q[CMD_PKT_MAX_LEN], buf_r[CMD_PKT_MAX_LEN];
  UINT8   len, i;
  INT32   sent;
  UINT16  error, checksum_q;
  int     index;
  bool    nak;

  // Init:
  len = 0;
  checksum_q = 0;
  *length = 0;

  if( pyld_len > CMD_PKT_MAX_DATA_PAYLOAD )
  {
    return( RSP_PARAMETER_OUT_OF_RANGE );
  }

  // Format Request Packet:
  buf_q[len++] = STX;
  buf_q[len++] = n_h_a((UINT8)(cmd_pkt >> 12));
  buf_q[len++] = n_h_a((UINT8)((cmd_pkt & 0x0F00) >> 8));
  buf_q[len++] = n_h_a((UINT8)((cmd_pkt & 0x00F0) >> 4));
  buf_q[len++] = n_h_a((UINT8)(cmd_pkt & 0x000F));

  // Insert Data payload:
  for (i = 0; i < pyld_len; i++)
  {
    buf_q[len++] = payload[i];
  }
  
  // Calculate checksum:
  for (i = 0; i < len; i++)
  {
    checksum_q += buf_q[i];
  }
  checksum_q &= 0x00FF;
  buf_q[len++] = n_h_a((UINT8)(checksum_q >> 4));
  buf_q[len++] = n_h_a((UINT8)(checksum_q & 0x000F));
  buf_q[len++] = ETX;

  // TX the Request packet:
  if( m2m_hw_uart_write(hUART, buf_q, len, &sent ) != M2M_HW_UART_RESULT_SUCCESS )
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "CMD_PKT Read: TX failed");
    return( RSP_NE_COMM_ERROR );
  }

  // Look for RX packet in the queue with 50msec timeout:
  for( i = 0; i < NE_DELAY; i++)
  {    
    index = search_rx_queue( cmd_pkt );
    if( index != NOT_FOUND ) break;
    m2m_os_sleep_ms(1);
  }

  if( index != NOT_FOUND ) // Found our RX packet
  {
    rx_queue.item[index].lock = true; // Lock it for reading
    nak = rx_queue.item[index].nak;
    len = rx_queue.item[index].dlen;
    if( len > 0 ) 
    {
      memset( buf_r, 0, CMD_PKT_MAX_DATA_PAYLOAD );
      memcpy( buf_r, rx_queue.item[index].data, len );
    }
    Free_rx_queue_item( index );
  }
  else // Nothing in the queue
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Error: response timeout from NE:%4X",cmd_pkt);
    return( RSP_NE_COMM_ERROR );
  } 
  
  // Process the returned data:
  if( !nak && len > 0 ) // Normal STX packet
  {
    memcpy( data, buf_r, len );
    *length = len; // set the data length ptr
    return( RSP_SUCCESS );
  }    
  
  if( buf_r[0] == NAK && len > CMD_PKT_MIN_LEN )
  {
    error  = a_h_n(buf_r[0]) * 0x1000;
    error += a_h_n(buf_r[1]) * 0x100;
    error += a_h_n(buf_r[2]) * 0x10;
    error += a_h_n(buf_r[3]);

    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "CMD_PKT Read Command failed %04X", error );
    if( error == CMD_PKT_ERROR_READ_FAIL )      return ( RSP_NE_READ_FAILED );
    if( error == CMD_PKT_ERROR_WRITE_FAIL )     return ( RSP_NE_WRITE_FAILED );
    if( error == CMD_PKT_ERROR_PARM_OUT_RANGE ) return ( RSP_PARAMETER_OUT_OF_RANGE );
    if( error == CMD_PKT_ERROR_NOT_SUPPORTED )  return ( RSP_NOT_SUPPORTED );
    if( error == CMD_PKT_ERROR_PARSING )        return ( RSP_NE_COMM_ERROR );
  }
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "CMD_PKT Read failed without NAK, cmd=%04X",cmd_pkt);
  return ( RSP_NE_COMM_ERROR );
  
}


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
response_t WriteCmdPacket(UINT16 cmd_pkt, UINT8 *szData, UINT8 length)
{
  CHAR   buf_q[CMD_PKT_MAX_LEN], buf_r[CMD_PKT_MAX_DATA_PAYLOAD];
  UINT8  len, i;
  INT32  sent;
  UINT16 cmd_r, checksum_q;
  UINT16 error;
  int    index;
  bool   nak;

  // Init:
  len = 0;
  checksum_q = 0;

  if (length > CMD_PKT_MAX_DATA_PAYLOAD)
  {
    return( RSP_PARAMETER_OUT_OF_RANGE );
  }

  // Format Request Packet:
  buf_q[len++] = STX;
  buf_q[len++] = n_h_a((UINT8)(cmd_pkt >> 12));
  buf_q[len++] = n_h_a((UINT8)((cmd_pkt & 0x0F00) >> 8));
  buf_q[len++] = n_h_a((UINT8)((cmd_pkt & 0x00F0) >> 4));
  buf_q[len++] = n_h_a((UINT8)(cmd_pkt & 0x000F));

  // Insert Data payload:
  for (i = 0; i < length; i++)
  {
    buf_q[len++] = szData[i];
  }

  // Calculate checksum:
  for (i = 0; i < len; i++)
  {
    checksum_q += buf_q[i];
  }
  checksum_q &= 0x00FF;
  buf_q[len++] = n_h_a((UINT8)(checksum_q >> 4));
  buf_q[len++] = n_h_a((UINT8)(checksum_q & 0x000F));
  buf_q[len++] = ETX;

  // TX the Request packet:
  if( m2m_hw_uart_write(hUART, buf_q, len, &sent ) != M2M_HW_UART_RESULT_SUCCESS )
  {
    //m2m_os_lock_unlock(hLOCK);
  	return ( RSP_NE_COMM_ERROR );
  }
  
  // No response for debug messages:
  if( cmd_pkt == CMD_PKT_SEND_DEBUG_MSG ) return( RSP_SUCCESS );

  // Look for RX packet in the queue with 50msec timeout:
  for( i = 0; i < NE_DELAY; i++)
  {    
    index = search_rx_queue( cmd_pkt );
    if( index != NOT_FOUND ) break;
    m2m_os_sleep_ms(1);
  }
        
  if( index != NOT_FOUND ) // Found our RX packet
  {
    rx_queue.item[index].lock = true; // Lock it for reading
    nak = rx_queue.item[index].nak;
    len = rx_queue.item[index].dlen;
    if( len > 0 ) 
    {
      memset( buf_r, 0, CMD_PKT_MAX_DATA_PAYLOAD );
      memcpy( buf_r, rx_queue.item[index].data, len );
    }
    Free_rx_queue_item( index );
  }
  else // Nothing in the queue
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Error: response timeout from NE:%4X",cmd_pkt);
    return( RSP_NE_COMM_ERROR );
  } 

  if( !nak ) // STX
  {
    return( RSP_SUCCESS );
  }    
  
  if( nak && len > CMD_PKT_MIN_LEN ) // NAK
  {
    error  = a_h_n(buf_r[0]) * 0x1000;
    error += a_h_n(buf_r[1]) * 0x100;
    error += a_h_n(buf_r[2]) * 0x10;
    error += a_h_n(buf_r[3]);

    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "CMD_PKT Wr Command failed %04X", error );
    if( error == CMD_PKT_ERROR_READ_FAIL )      return ( RSP_NE_READ_FAILED );
    if( error == CMD_PKT_ERROR_WRITE_FAIL )     return ( RSP_NE_WRITE_FAILED );
    if( error == CMD_PKT_ERROR_PARM_OUT_RANGE ) return ( RSP_PARAMETER_OUT_OF_RANGE );
    if( error == CMD_PKT_ERROR_NOT_SUPPORTED )  return ( RSP_NOT_SUPPORTED );
    if( error == CMD_PKT_ERROR_PARSING )        return ( RSP_NE_COMM_ERROR );
  }
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "CMD_PKT Write failed without NAK, cmd=%04X",cmd_pkt);
  return ( RSP_NE_COMM_ERROR );
}  
  

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
response_t WriteCmdPkt16(UINT16 cmd_pkt, UINT16 data)
{
  UINT8 szData[4];

  szData[0] = n_h_a((UINT8)(data >> 12));
  szData[1] = n_h_a((UINT8)((data & 0x0F00) >> 8));
  szData[2] = n_h_a((UINT8)((data & 0x00F0) >> 4));
  szData[3] = n_h_a((UINT8)(data & 0x000F));

  return(WriteCmdPacket(cmd_pkt, szData, 4));
}


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
response_t WriteCmdPkt0(UINT16 cmd_pkt)
{
  return(WriteCmdPacket(cmd_pkt, NULL, 0));
}


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
response_t ReadCmdPkt16(UINT16 cmd_pkt, UINT16 *data)
{
  UINT8      length;
  UINT16     sub;
  UINT8      szNUM[CMD_PKT_MAX_DATA_PAYLOAD] = "\0";
  response_t rsp;

  rsp = ReadCmdPacket(cmd_pkt, szNUM, &length);

  if( rsp == RSP_SUCCESS )
  {
    if (length != 4) return( RSP_GENERAL_ERROR );

    sub =  a_h_n(szNUM[0]) * 0x1000;
    sub += a_h_n(szNUM[1]) * 0x100;
    sub += a_h_n(szNUM[2]) * 0x10;
    sub += a_h_n(szNUM[3]);

    *data = sub;
  }
  return( rsp );
}


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
UINT8 n_h_a(UINT8 num)
{
  if (num < 0x0A)      return(num + NUM0);
  else if (num < 0x10) return(num + NUM7);
  else return(NUM0);
}


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
UINT8 a_h_n(UINT8 hex)
{
  if (hex >= NUM0   && hex <= NUM9)   return (hex - NUM0);
  if (hex >= CHAR_A && hex <= CHAR_F) return (hex - NUM7);
  if (hex >= CHAR_a && hex <= CHAR_f) return (hex - CHAR_W);
  return 0;
}


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
bool isHex(char hex)
{
  if (hex >= NUM0   && hex <= NUM9)   return( true );
  if (hex >= CHAR_A && hex <= CHAR_F) return( true );
  if (hex >= CHAR_a && hex <= CHAR_f) return( true );
  return( false );
}


/////////////////////////////////////////////////////////////////////////////
// Function: Init_uart()
//
// Parameters: none
//
// Return: bool - true if successful
//
// Description: This function opens and configures the uart for general use.
//
/////////////////////////////////////////////////////////////////////////////
bool Init_uart(void)
{
  hUART = m2m_hw_uart_open();

  if (hUART != M2M_HW_UART_HANDLE_INVALID)
  {
    // Turn Tx/Rx Blocking Off:
    if(m2m_hw_uart_ioctl (hUART, M2M_HW_UART_IO_BLOCKING_SET, (INT32) M2M_HW_UART_IO_BLOCKING_OFF) != M2M_HW_UART_RESULT_SUCCESS) return false;
    //debug: add code here to set baud, 8-n-1, no HW handshack etc

    // Turn off flow control:
    m2m_hw_uart_ioctl( hUART, M2M_USB_HW_OPTIONS_GET, (INT32)&uart_settings);
    uart_settings.flow_ctrl = 0; // flow control OFF
    m2m_hw_uart_ioctl( hUART, M2M_USB_HW_OPTIONS_SET, (INT32)&uart_settings);
    
    m2m_hw_uart_ioctl( hUART, M2M_HW_UART_IO_RCV_FUNC, (INT32)uart_read_cb );

    //hLOCK = m2m_os_lock_init(M2M_OS_LOCK_CS); maybe not using a lock, could change
    
    // Init the entire RX queue:
    memset( &rx_queue, 0, sizeof( rx_queue ) );
    rx_queue.last = RX_QUEUE_SIZE - 1;
    globals.module.rev_cmd_pkt = false;
    return true;
  }
  return false;
}


/////////////////////////////////////////////////////////////////////////////
// Function: uart_read_cb()
//
// Parameters: handle -> UART handle
//             char* buf -> data received on the uart port
//             INT32 len -> length of the data
//
// Return: INT32 
//
// Description: This function is a call back for UART1 that is connected
// to the NE.  It's runs in the lower level modem task level, much like
// an ISR.   It receives bytes from the port, parses for cmd_pkts, and
// places them into the rx_cmd_pkt queue for further processing.
//
/////////////////////////////////////////////////////////////////////////////
static INT32 uart_read_cb( M2M_T_HW_UART_HANDLE handle, char *buf, INT32 len )
{
  int    i, j, last;
  UINT16 checksum_c, checksum_r;
  static char buf_cp[CMD_PKT_MAX_LEN];
  static int  k = 0;
  static bool start = false;
  
  for( i = 0; i < len; i++ )
  {
    if( buf[i] == STX || buf[i] == NAK )
    {
      if( start == true ) dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "RX_CB error: broken cmd_pkt, starting fresh");
      k = 0;
      start = true;
      buf_cp[k++] = buf[i];
    }
    else if( start )
    {
      if( k < CMD_PKT_MAX_LEN )
      {
        buf_cp[k++] = buf[i];
      }
      else // Error, ran off the end of the cmd_pkt length, scrap cmd_pkt
      {
        dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "RX_CB error: ran off the length of cmd_pkt, discarted" );
        start = false;
      }

      if( buf[i] == ETX ) // End of cmd_pkt?
      {  
        start = false;
        if( k >= CMD_PKT_MIN_LEN && k <= CMD_PKT_MAX_LEN )
        {
          // Calculate and compare Checksum:
          checksum_c = 0;
          for( j = 0; j < k - 3; j++ )
          {
            checksum_c += buf_cp[j];  // Calc checksum
          }
          checksum_c &= 0x00FF;
          checksum_r =  a_h_n(buf_cp[k - 3]) * 0x10; // MSN
          checksum_r += a_h_n(buf_cp[k - 2]);        // LSN

          if (checksum_c == checksum_r)
          {
            last = rx_queue.last; // use a local variable
            // Find the next unlocked item within the queue:
            for( j = 0; j < (RX_QUEUE_SIZE + 1); j++ )
            {
              last = ++last % RX_QUEUE_SIZE;
              if( rx_queue.item[last].lock == true )
              {
                dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "RX_CB: queue locked, index,cmd=%d,%04X", last, rx_queue.item[last].cmd_pkt );
              }
              else break; 
            }
         
            // Check for overflow of queue, overwrite if needed:
            if( rx_queue.item[last].cmd_pkt != 0 )
            {  
              dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "RX_CB: queue overflow, index,cmd=%d,%04X", last, rx_queue.item[last].cmd_pkt );
              //memset( &rx_queue.item[last], 0, sizeof( rx_queue_t ));
            }
          
            // Lock down item for writing and update:
            rx_queue.item[last].lock = true;
            if( buf_cp[0] == NAK ) rx_queue.item[last].nak = true;
              else rx_queue.item[last].nak = false;
            memset( rx_queue.item[last].data, 0, CMD_PKT_MAX_DATA_PAYLOAD );
            //memcpy( rx_queue.item[last].data, buf_cp, k );

            // Got data payload?
            if( k > CMD_PKT_MIN_LEN )
            {  
              memcpy( rx_queue.item[last].data, &buf_cp[CMD_PKT_DATA_START_POS], k-CMD_PKT_MIN_LEN );
              rx_queue.item[last].dlen = k - CMD_PKT_MIN_LEN;
            }
            else rx_queue.item[last].dlen = 0;
            
            //rx_queue.item[last].len = k;
            rx_queue.item[last].cmd_pkt =  a_h_n(buf_cp[1]) * 0x1000;
            rx_queue.item[last].cmd_pkt += a_h_n(buf_cp[2]) * 0x100;
            rx_queue.item[last].cmd_pkt += a_h_n(buf_cp[3]) * 0x10;
            rx_queue.item[last].cmd_pkt += a_h_n(buf_cp[4]);

            rx_queue.item[last].lock = false; // Free Lock
            rx_queue.last = last;
            // Signal code that a new reverse cmd_pkt arrived: 
            if( rx_queue.item[last].cmd_pkt >= CMD_PKT_START_OF_REVERSE_CMD_PKT )
              globals.module.rev_cmd_pkt = true;
          
            dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "RX_CB new cmd_pkt, last=%d,%04X", last, rx_queue.item[last].cmd_pkt );
          }
          else // Bad checksum
            dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "RX_CB error: cmd_pkt checksum failed" );
        }
        else // Error, wrong length:
          dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "RX_CB error: length of cmd_pkt incorrect" );
      }
    }
    else dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "RX_CB throw out:%02X:%c", buf[i], buf[i] );
  }
  return( M2M_HW_UART_RESULT_SUCCESS );
}                      


/////////////////////////////////////////////////////////////////////////////
// Function: search_rx_queue()
//
// Parameters: UINT16 -> cmd_pkt
//
// Return: int -> index within the RX queue where cmd_pkt found. 
//
// Description: This function searches the RX queue for the requested
// cmd_pkt response that is unlocked.  The search starts at the "last" 
// item and looks backwards and will loop around until it searches the 
// entire queue length.  If it cannot find the cmd_pkt, it returns NOT_FOUND.
// This function is intended for regular cmd_pkts responses and reverse
// cmd_pkt, not logging cmd_pkts.
//
/////////////////////////////////////////////////////////////////////////////
int search_rx_queue( UINT16 cmd_pkt )
{
  int index;
  int i;
  
  index = rx_queue.last;
  
  for( i = 0; i < RX_QUEUE_SIZE; i++ )
  { 
    if( rx_queue.item[index].cmd_pkt == cmd_pkt && 
        rx_queue.item[index].lock == false )
    { 
      return( index );
    }
    if( index == 0 ) index = RX_QUEUE_SIZE;
    index--;
  }
  return( NOT_FOUND );
}


/////////////////////////////////////////////////////////////////////////////
// Function: Free_rx_queue_item()
//
// Parameters: int -> item within rx to be freed
//
// Return: none. 
//
// Description: This function will blank out an item within the RX
// Queue with zeros, thus unlocking it and freeing it up for future use.
//
/////////////////////////////////////////////////////////////////////////////
void Free_rx_queue_item( int index )
{
  if( index >= 0 && index < RX_QUEUE_SIZE )
  {
    rx_queue.item[index].lock = true; // Lock it
    rx_queue.item[index].cmd_pkt = 0;
    rx_queue.item[index].dlen = 0;
    rx_queue.item[index].nak = false;
    memset( rx_queue.item[index].data, 0, CMD_PKT_MAX_DATA_PAYLOAD );
    rx_queue.item[index].lock = false;
  }
}


/////////////////////////////////////////////////////////////////////////////
// Function: ProcessRevCmdPacket()
//
// Parameters: none
//
// Return: none
//
// Description: This function finds & processes the most recent reverse 
// cmd_pkt on the RX Queue.
//
/////////////////////////////////////////////////////////////////////////////
void ProcessRevCmdPacket( void )
{
  int    i, index;
  UINT8  dlen;
  UINT8  buf_r[CMD_PKT_MAX_DATA_PAYLOAD];
  UINT16 cmd_pkt;
  
  // Search the queue, backwards in time, and process all reverse cmd_pkts:
  index = rx_queue.last;
  for( i = 0; i < RX_QUEUE_SIZE; i++ )
  { 
    index = ++index % RX_QUEUE_SIZE;
    if( rx_queue.item[index].cmd_pkt >= CMD_PKT_START_OF_REVERSE_CMD_PKT && 
        rx_queue.item[index].lock == false )
    { 
      
      rx_queue.item[index].lock = true; // Lock it for reading
      memcpy( buf_r, rx_queue.item[index].data, rx_queue.item[index].dlen );
      dlen = rx_queue.item[index].dlen;
      cmd_pkt = rx_queue.item[index].cmd_pkt;
      Free_rx_queue_item( index );
      dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Found reverse cmd_pkt=%04X on queue at %d", cmd_pkt, index ); 
      
      if( cmd_pkt == CMD_PKT_RV_OTA_LOG_DATA ) 
      {
        if( globals.module.logging == true )
        {
          NewLogEntry( buf_r, dlen );
        } 
      }
      if( cmd_pkt >= CMD_PKT_RV_WRITE_CARRIER_NAME )
      {
        ProcessRevCmdPkt_command( cmd_pkt, buf_r, dlen );
      }
    }
  }
  
  // Search the queue 1 more time, see if anything new arrived:
  for( i = 0; i < RX_QUEUE_SIZE; i++ )
  { 
    if( rx_queue.item[i].cmd_pkt >= CMD_PKT_START_OF_REVERSE_CMD_PKT && 
        rx_queue.item[i].lock == false )
    { 
      globals.module.rev_cmd_pkt = true;
      return;
    }
  }

  globals.module.rev_cmd_pkt = false;  
  return;
}


/////////////////////////////////////////////////////////////////////////////
// Function: ProcessRevCmdPkt_command()
//
// Parameters: none
//
// Return: none
//
// Description: This function processes a reverse cmd_pkt from the NE and 
// sends a response back to the NE.
//
/////////////////////////////////////////////////////////////////////////////
void ProcessRevCmdPkt_command( UINT16 cmd_pkt, UINT8 *buf, UINT8 dlen )
{
  response_t rsp;
  char       str[CMD_PKT_MAX_DATA_PAYLOAD+1];
  UINT8      data_r[CMD_PKT_MAX_DATA_PAYLOAD];
  UINT8      buf_r[CMD_PKT_MAX_LEN];
  UINT8      len_r, dlen_r, i;
  UINT16     error;
  INT32      sent;
  bool       ack;
  M2M_T_NETWORK_CURRENT_NETWORK op;
  
  // Init:
  memset( str, 0, sizeof( str ) );
  memset( data_r, 0, CMD_PKT_MAX_DATA_PAYLOAD );
  i = 0;
  
  switch( cmd_pkt )
  {
    case CMD_PKT_RV_READ_CARRIER_NAME: // 0xF101
    case CMD_PKT_TN_READ_CARRIER_NAME: // 0xF701 - Tunneling
      rsp = read_config_file_item( CFG_FILE_CARRIER_NAME, str, NULL );
      break;
      
    case CMD_PKT_RV_READ_RMS_ACTIVATE: // 0xF103
    case CMD_PKT_TN_READ_RMS_ACTIVATE: // 0xF703 - Tunneling
      rsp = read_config_file_item( CFG_FILE_ITEM_RMS_ACTIVATE, str, NULL );
      break;
      
    case CMD_PKT_RV_READ_CONN_METHOD: // 0xF105
    case CMD_PKT_TN_READ_CONN_METHOD: // 0xF705 - Tunneling
      rsp = read_config_file_item( CFG_FILE_ITEM_CONN_METHOD, str, NULL );
      break;
      
    case CMD_PKT_RV_READ_APN: // 0xF107
    case CMD_PKT_TN_READ_APN: // 0xF707 - Tunneling
      rsp = read_config_file_item( CFG_FILE_ITEM_APN, str, NULL );
      break;
      
    case CMD_PKT_RV_WRITE_SMS_SHORT_CODE: // 0xF108
    case CMD_PKT_TN_WRITE_SMS_SHORT_CODE: // 0xF708 - Tunneling
      if( dlen > 0 && dlen < CMD_PKT_MAX_DATA_PAYLOAD )
      {  
        memcpy( str, buf, dlen );
        rsp = update_config_file( CFG_FILE_ITEM_SMS_SHORT_CODE, str );
      }
      else rsp = RSP_PARAMETER_OUT_OF_RANGE;
      break;
      
    case CMD_PKT_RV_READ_SMS_SHORT_CODE: // 0xF109
    case CMD_PKT_TN_READ_SMS_SHORT_CODE: // 0xF709 - Tunneling
      rsp = read_config_file_item( CFG_FILE_ITEM_SMS_SHORT_CODE, str, NULL );
      break;
      
    case CMD_PKT_RV_READ_MQTT_PASSWORD: // 0xF10B
    case CMD_PKT_TN_READ_MQTT_PASSWORD: // 0xF70B - Tunneling
      rsp = read_config_file_item( CFG_FILE_ITEM_MQTT_PASSWORD, str, NULL );
      break;

    case CMD_PKT_RV_READ_MQTT_URL: // 0xF10D
    case CMD_PKT_TN_READ_MQTT_URL: // 0xF70D - Tunneling
      rsp = read_config_file_item( CFG_FILE_ITEM_MQTT_URL, str, NULL );
      break;

    case CMD_PKT_RV_READ_MQTT_HEARTBEAT: // 0xF10F
    case CMD_PKT_TN_READ_MQTT_HEARTBEAT: // 0xF70F - Tunneling
      rsp = read_config_file_item( CFG_FILE_ITEM_MQTT_HEARTBEAT, str, NULL );
      break;

    case CMD_PKT_RV_READ_PERF_INTERVAL: // 0xF111
    case CMD_PKT_TN_READ_PERF_INTERVAL: // 0xF711 - Tunneling
      rsp = read_config_file_item( CFG_FILE_ITEM_PERF_INTERVAL, str, NULL );
      break;

    case CMD_PKT_RV_READ_LOC_INTERVAL: // 0xF113
    case CMD_PKT_TN_READ_LOC_INTERVAL: // 0xF713 - Tunneling
      rsp = read_config_file_item( CFG_FILE_ITEM_LOC_INTERVAL, str, NULL );
      break;

    case CMD_PKT_RV_READ_TCA_VERSION: // 0xF115
    case CMD_PKT_TN_READ_TCA_VERSION: // 0xF715 - Tunneling
      rsp = read_config_file_item( CFG_FILE_ITEM_TCA_VERSION, str, NULL );
      break;
      
    case CMD_PKT_RV_READ_LOG_FTP_URL: // 0xF117
    case CMD_PKT_TN_READ_LOG_FTP_URL: // 0xF717 - Tunneling
      rsp = read_config_file_item( CFG_FILE_ITEM_LOG_FTP_URL, str, NULL );
      break;
      
    case CMD_PKT_RV_READ_LOG_FTP_PATH: // 0xF119
    case CMD_PKT_TN_READ_LOG_FTP_PATH: // 0xF719 - Tunneling
      rsp = read_config_file_item( CFG_FILE_ITEM_LOG_PATH, str, NULL );
      break;
      
    case CMD_PKT_RV_READ_LOG_FTP_USERID: // 0xF11B
    case CMD_PKT_TN_READ_LOG_FTP_USERID: // 0xF71B - Tunneling
      rsp = read_config_file_item( CFG_FILE_ITEM_LOG_FTP_USERID, str, NULL );
      break;
      
    case CMD_PKT_RV_READ_LOG_FTP_PASSWORD: // 0xF11D
    case CMD_PKT_TN_READ_LOG_FTP_PASSWORD: // 0xF71D - Tunneling
      rsp = read_config_file_item( CFG_FILE_ITEM_LOG_FTP_PASSWD, str, NULL );
      break;
      
    case CMD_PKT_RV_READ_LOG_FTP_START: // 0xF11F
    case CMD_PKT_TN_READ_LOG_FTP_START: // 0xF71F - Tunneling
      rsp = read_config_file_item( CFG_FILE_ITEM_LOG_START, str, NULL );
      break;
      
    case CMD_PKT_RV_READ_LOG_FTP_STOP: // 0xF121      
    case CMD_PKT_TN_READ_LOG_FTP_STOP: // 0xF721 - Tunneling
      rsp = read_config_file_item( CFG_FILE_ITEM_LOG_STOP, str, NULL );
      break;
 
    ///////////////////////////////////
    //   Tunneling Command Packets   //
    ///////////////////////////////////
    case CMD_PKT_TN_READ_MODULE_RSSI: // 0xF722
      if( CellService() )
      {
        rsp = Read_GSM_RSSI( str );
        if( rsp == RSP_SUCCESS ) break;
      }
      str[0] = NUM0; // Zero = No GSM Service
      str[1] = 0; // Null terminate, in case str[] scrambled
      rsp = RSP_SUCCESS;
      break;
      
    case CMD_PKT_TN_READ_MODULE_CHANNEL: // 0xF723
      if( CellService() ) sprintf( str, "%d", globals.module.GSM_channel );
        else str[0] = NUM0; // Zero = No GSM Service
      rsp = RSP_SUCCESS;
      break;      
      
    case CMD_PKT_TN_READ_MODULE_OPERATER: // 0xF724
      if( CellService() )
      {
        if( m2m_network_get_currently_selected_operator( &op ) )
        {
          strcpy( str, op.longAlphanumeric );
          rsp = RSP_SUCCESS;
          break; 
        }
      }
      str[0] = NUM0; // Zero = No GSM Service or No Operator Name
      rsp = RSP_SUCCESS;
      break;
      
    case CMD_PKT_TN_READ_MODULE_FIRMWARE: // 0xF725
      strcpy( str, globals.module.firmware ); // Module firmware version
      rsp = RSP_SUCCESS;
      break;
      
    case CMD_PKT_TN_READ_TCA_SOFTWARE: // 0xF726  
      strcpy( str, software_version ); // TCA software version
      rsp = RSP_SUCCESS;
      break;
      
    case CMD_PKT_TN_READ_PHONE_NUMBER: // 0xF727      
      strcpy( str, globals.module.phone_num ); // TCA software version
      rsp = RSP_SUCCESS;
      break;
      

      
    default:
      rsp = RSP_NOT_SUPPORTED;
      break;
  }
  
  if( rsp == RSP_SUCCESS )
  {
    ack = true;
    dlen_r = strlen( str );

    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Rev cmd_pkt worked: len=%d",dlen_r);

    if( dlen_r > 0 )
    {
      if( dlen_r > CMD_PKT_MAX_DATA_PAYLOAD ) dlen_r = CMD_PKT_MAX_DATA_PAYLOAD; 
      memcpy( data_r, str, dlen_r );
    }
  }
  else // Bad response, copy error code into data payload:
  {
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Rev cmd_pkt error=%d",rsp);
    ack = false;
    error = (UINT16)rsp;
    data_r[0] = n_h_a((UINT8)(error >> 12));
    data_r[1] = n_h_a((UINT8)((error & 0x0F00) >> 8));
    data_r[2] = n_h_a((UINT8)((error & 0x00F0) >> 4));
    data_r[3] = n_h_a((UINT8)(error & 0x000F));    
    dlen_r = 4;
  }
  
  len_r = format_cmd_pkt( buf_r, ack, cmd_pkt, data_r, dlen_r );
  
   // TX the Request packet:
  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "Sending RevCmdPkt response to NE:%04X", cmd_pkt ); 
  m2m_hw_uart_write( hUART, buf_r, len_r, &sent );
 
}


/////////////////////////////////////////////////////////////////////////////
// Function: format_cmd_pkt()
//
// Parameters: UINT8 *buf -> buffer that the formated cmd_pkt is placed, must
//                           be minimum size of CMD_PKT_MAX_LEN.
//               bool ack -> true for normal, false for NAKs with error code
//         UINT16 cmd_pkt -> the cmd_pkt command ID
//            UINT8 *data -> ptr to data payload, NULL if no data.
//             UINT8 dLen -> length of data payload, zero if no data.
//
// Return: length of formatted cmd_pkt
//
// Description: This function formats a complete cmd_pkt for transmission
//
/////////////////////////////////////////////////////////////////////////////
UINT8 format_cmd_pkt( UINT8 *buf, bool ack, UINT16 cmd_pkt, UINT8 *data, UINT8 dLen )
{
  UINT8  j, i = 0;
  UINT16 checksum = 0;
  
  if( buf == NULL ) return( 0 );
  
  // Format Command Packet:
  if( ack == true ) buf[i++] = STX;
    else buf[i++] = NAK;
  
  buf[i++] = n_h_a((UINT8)(cmd_pkt >> 12));
  buf[i++] = n_h_a((UINT8)((cmd_pkt & 0x0F00) >> 8));
  buf[i++] = n_h_a((UINT8)((cmd_pkt & 0x00F0) >> 4));
  buf[i++] = n_h_a((UINT8)(cmd_pkt & 0x000F));

  // Insert data payload if present:
  if( data != NULL && dLen > 0 && dLen <= CMD_PKT_MAX_DATA_PAYLOAD )
  {
    for( j = 0; j < dLen; j++ )
    {
      buf[i++] = data[j];
    }
  }
  
  // Calculate checksum:
  for( j = 0; j < i; j++ )
  {
    checksum += buf[j];
  }
  checksum &= 0x00FF;
  buf[i++] = n_h_a((UINT8)(checksum >> 4));
  buf[i++] = n_h_a((UINT8)(checksum & 0x000F));
  buf[i++] = ETX;  
  return( i );
}









