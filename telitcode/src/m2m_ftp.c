/**
 * Fabio Pintus <fabio.pintus@telit.com>
 *
**/

#include "m2m_ftp.h"
#include <stdarg.h>
#include "m2m_cloud_types.h"
#include "m2m_cloud_defines.h"
/*UTILS*/
TRANSFER_STATUS getTransferStatus(void);

DATA_TYPE last_type;

static int s_received226 = 0;

DATA_TYPE get_last_type(void)
{
	return last_type;
}

void set_last_type(DATA_TYPE type)
{
	last_type = type;
}

void set_received_transfer_complete(int status)
{
	s_received226 = status;
}

int get_received_transfer_complete(void)
{
	return s_received226;

}

void * m2m_os_mem_calloc(UINT32 nitems, UINT32 size)
{

	void *pointer;
	pointer = m2m_os_mem_alloc(nitems * size);
	if (!pointer)
		return NULL;

	memset(pointer, 0 , nitems*size);

	return pointer;

}



int waitRegistration(UINT32 timeout)
{

	M2M_T_NETWORK_REG_STATUS_INFO reg_info;
	UINT32 timer = 0;
	while (1) {
		//first check the timeout
		if (timer >= timeout) {
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"cannot get a network registration!\n");
			return 0;
		}
		if (m2m_network_get_reg_status(&reg_info)) {
			if (reg_info.status == 1 || reg_info.status == 5)

				return 1;
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Reg status: %d\n", reg_info.status);


			m2m_os_sleep_ms(2000);
			timer +=2000;
		}
	}
}


INT32 m2m_socket_bsd_recv_timeout(M2M_SOCKET_BSD_SOCKET socket, void *p_buf, INT32 len, INT32 flags, UINT32 timeout)
{
	INT32 ret;
	int total = 0, n;
	UINT32 count = 0;
	INT32 avail = 0;
	INT32 toRead = 0;

	//memset(buf,0,sizeof(buf));

	if (!p_buf) {
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_socket_bsd_recv_timeout(): output buffer not initialized\n");
		return M2M_FTP_UNALLOCATED_BUFF_ERROR;
	}
	while (total < len && count <= timeout) {
		if ((ret = m2m_socket_bsd_recv_data_size(socket, &avail)) < 0) {
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_socket_bsd_recv_timeout() - error getting recv_data_size. ret: %d\n", ret);
			return ret; //error receiving the pending data size
		}

		//dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_socket_bsd_recv_timeout() - available bytes: %d\n", avail);

		toRead = ((avail + total) > len)? len-total: avail;
		//dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"bytes to be Read: %d\n", toRead);

		/* Perform a read only if data is actually pending */
		if (toRead > 0)	{
			n = m2m_socket_bsd_recv(socket, p_buf + total, toRead, flags);
			if (n < 0) {
				dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_socket_bsd_recv_timeout() - error in m2m_socket_bsd_recv. ret: %d\n", n);
				return n;  /* failure receiving from the socket */
			}
			total += n;
		//	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_socket_bsd_recv_timeout() - received: %s", buf);
		}
		m2m_os_sleep_ms(100);
		count += 100;
	}

	return total;

}

// utility functions

bool m2m_ftp_serialize(const Message *p_msg, String p_result)
{
	int res_len = 0;
	if (!p_msg || !p_result)		/* Exit, nothing can be done */
		return false;


	/*  Create the output string */
	memset(p_result,0,sizeof(p_result));

	sprintf(p_result, "%s %s\r\n", p_msg->m_verb, p_msg->m_param);

	#ifndef NODEBUG
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"serialize(): result='%s'\n", p_result);
	#endif

	return true;
}

bool m2m_ftp_deserialize(const String p_str, Message *p_result)
{
	if (p_str == NULL) {	/* Exit, nothing can be done */
		return false;
	}
	if (strlen(p_str) == 0) {
		return false;
	}

// parse serialized string
	strncpy(p_result->m_verb, p_str, M2M_FTP_VERB_SIZE);
	strncpy(p_result->m_param, &p_str[M2M_FTP_VERB_SIZE], M2M_FTP_PARAMETER_SIZE);

	#ifndef NODEBUG
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"deserialize(): message {verb='%s',param='%s'}\n", p_result->m_verb, p_result->m_param);
	#endif

	return true;
}


bool m2m_ftp_send(const M2M_SOCKET_BSD_SOCKET socket, const Message *p_query)
{

	char buf[M2M_FTP_MESSAGE_SIZE]; //default 1024
	int total = 0, n;

	int toSend = 0;
	/* serialize message */
	if (!m2m_ftp_serialize(p_query, buf)) {
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_send(): Message serialization failed.\n");
		return false;
	}

	toSend = strlen(buf);
	// handle partial transmissions
	while (total < toSend ) {
		if ((n = m2m_socket_bsd_send( socket, buf + total, toSend - total, 0)) < 0 )	{
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_send(), errno: %d", m2m_socket_errno());
			return false;
		}
		total += n;

		#ifndef NODEBUG
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_send(): bytes sent=%d, remaining=%d\n", total, toSend - total);
		#endif
	}

	return true;
}



bool m2m_ftp_recv(const M2M_SOCKET_BSD_SOCKET socket, Message *p_response, UINT32 timeout, String p_expected)
{
	char buf[M2M_FTP_MESSAGE_SIZE];
	char code[5];
	char expected_code[5];

	int total = 0, n;
	UINT32 count = 0;

	UINT32 int_timeout = 500, int_delay = 800;


	memset(buf,0,sizeof(buf));
	memset(code,0,sizeof(code));
	memset(expected_code,0,sizeof(expected_code));

	strncpy(expected_code, p_expected, 4);

	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"in receive fun\n");


	while (total < M2M_FTP_MESSAGE_SIZE) {

		/* 2 seconds timeout */
		if ((n = m2m_socket_bsd_recv_timeout( socket, buf, M2M_FTP_MESSAGE_SIZE, 0, int_timeout)) < 0) {
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_recv(): errno: %d",  m2m_socket_errno());
			return false;
		}

		total += n;
//5barz		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"in receing length...\n");
		#ifndef NODEBUG
//5barz		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_recv(): bytes recv=%d, remaining=%d\n", total, M2M_FTP_MESSAGE_SIZE - total);
//5barz		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_recv(): buffer='%s'\n", buf);
		#endif

		/* check for the string only if it was passed. */
		if (p_expected) {
			if (strstr(buf, p_expected)) {
				/* The expected string was received. Exit the loop. */
#ifndef NODEBUG
				dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"completed data reception\n");
#endif
				return m2m_ftp_deserialize(buf, p_response);
			}


			/* We are not sure the server message is exactly the same we are expecting. So, check also for response code. */
			if (strstr(buf, expected_code)) {
			/* The expected code was received. Exit the loop. */
#ifndef NODEBUG
				dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Received expected code\n");
#endif
				m2m_ftp_clear_buffer(socket);
				return m2m_ftp_deserialize(buf, p_response);
			}


		/* Check if the buffer contains an error code (to avoid waiting for timeout)*/
			if ( total >= 4) {

				strncpy(code,buf,4);

				if (atoi(code) >= M2M_FTP_CODES_SYNTAX_ERROR_SERIES) {
					/* Clear the socket buffer */
					m2m_ftp_clear_buffer(socket);
					dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_recv(): An error Code was received: %d. Exiting.\n", atoi(code));
					break;
				}

			}



			if (strstr(p_expected, M2M_FTP_SERVER_RESP_TRANSF_COMPL) ||
					strstr(p_expected, M2M_FTP_SERVER_CODE_AS_STRING(M2M_FTP_CODES_SERVER_TRSF_CMPL))) {
				if (getTransferStatus() == RECEIVING) {
					/**
					 * There is a second socket receiving data (ASCII or BINARY)
					 * in Passive mode. If the socket is active, extend the timeout
					 */
				#ifndef NODEBUG
					dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_recv(): Timeout extended\n");

				#endif
					timeout += 2 * (int_timeout + int_delay);
				}
			}
		}/* End if (expected) */


		m2m_os_sleep_ms(int_delay); //wait 3 seconds

		/* Verify if the timeout expired */
		count += (int_timeout + int_delay);
		if (count > timeout) {
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_recv(): Reached timeout.\n");
			break;
		}
	}

	/* We exited from the loop either for timeout or for reached SIFTP_MESSAGE_SIZE, but we did not find the expected response. */
	if (p_expected) {
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_recv(): Expected string was not received!\n");
		//m2m_ftp_deserialize(buf, p_response);  //try to deserialize the message, if any was received
		return false;
	}

	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_recv(): Completed data reception\n");
	return m2m_ftp_deserialize(buf, p_response);

}

bool m2m_ftp_query(const M2M_SOCKET_BSD_SOCKET socket, const Message *p_query, Message *p_response, UINT32 timeout, String p_expected)
{
	return m2m_ftp_send(socket, p_query) && m2m_ftp_recv(socket, p_response, timeout, p_expected);
}


TRANSFER_STATUS m2m_ftp_get_last_download_status(void)
{
	return getTransferStatus();
}


bool m2m_ftp_clear_buffer(const M2M_SOCKET_BSD_SOCKET socket)
{

	char buf[M2M_FTP_MESSAGE_SIZE];
	UINT32 int_timeout = 2000;

	memset(buf,0,sizeof(buf));

	/* 2 seconds timeout */
	if ((m2m_socket_bsd_recv_timeout( socket, buf, M2M_FTP_MESSAGE_SIZE, 0, int_timeout)) < 0) {
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_clearBuffer(): errno: %d",  m2m_socket_errno());
		return false;
	}
#ifndef NODEBUG
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_clearBuffer(): Buffer cleared\n");
#endif
	return true;

}

void m2m_ftp_clear_DataBuffer(DataBuffer *p_data)
{
	m2m_os_mem_free(p_data->p_buffer);
	p_data->p_buffer = NULL;
	p_data->size= 0;
}



/********************************************/

/* Functions */
INT32 M2M_DataReceive(INT32 socket, INT32 size, INT32 file);
INT32 M2M_DataSend(INT32 socket, INT32 size, INT32 file);
void clearCompleted(void);
void setCompleted(void);

void initTransferLock(void);
void waitTransferLock(void);
void unlockTransferLock(void);

/* Global buffer */
DataBuffer data;

/* Global Dynamic Pool Size (in bytes) */
UINT32 DynPoolSize;

FTP_CLIENT_ERROR m2m_ftp_connection_create(M2M_SOCKET_BSD_SOCKET *p_socket, const String p_serverName, const int serverPort)
{

	INT32 nonblocking;
	struct M2M_SOCKET_BSD_SOCKADDR_IN serverSockAddr;
	UINT32 serverAddr;
	INT32 sockState;
	INT32 count = 0;

	memset(&serverSockAddr, 0, sizeof(struct M2M_SOCKET_BSD_SOCKADDR_IN));

	if (! p_socket || ! p_serverName){
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_connection_create(): input strings not initialized\n");
		return M2M_FTP_UNALLOCATED_BUFF_ERROR;
	}


	/* determine server IP from server name (or dotted IP as a string) */
	if ((serverAddr = m2m_socket_bsd_get_host_by_name(p_serverName)) == 0) {
		/* Address was not an URL, try the dotted IP */
		if ((serverAddr = m2m_socket_bsd_inet_addr(p_serverName)) == 0) {
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_connection_create(). Error: %d", m2m_socket_errno());
			return M2M_FTP_GET_IP_ERROR;  //we failed to get the IP address. abort.
		}
	}

	serverSockAddr.sin_family = M2M_SOCKET_BSD_PF_INET;
	serverSockAddr.sin_port = m2m_socket_bsd_htons(serverPort);
	serverSockAddr.sin_addr.s_addr = serverAddr;

	/**
	 * The address was an URL or an IP address, so now
	 * serverAddr contains the ipv4 address in integer format.
	 */

	#ifndef NODEBUG
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_connection_create(): serverName='%s', serverAddr='%s', port: %d\n", p_serverName, m2m_socket_bsd_addr_str(serverSockAddr.sin_addr.s_addr), serverPort);
	#endif

	/* Create socket */

	if( M2M_SOCKET_BSD_INVALID_SOCKET ==
			(*p_socket = m2m_socket_bsd_socket( serverSockAddr.sin_family,
												M2M_SOCKET_BSD_SOCK_STREAM,
												M2M_SOCKET_BSD_IPPROTO_TCP))) {
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_service_create(): create socket\r\n");
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_service_create(). Error: %d", m2m_socket_errno());
		return M2M_FTP_SOCK_CREATE_ERROR;
	}




#if BLOCKING
	// connect on socket (blocking mode)
	if (M2M_SOCKET_BSD_INVALID_SOCKET ==
			m2m_socket_bsd_connect(*p_socket,
					(M2M_SOCKET_BSD_SOCKADDR *)&serverSockAddr, sizeof(struct M2M_SOCKET_BSD_SOCKADDR_IN))) {
		LOG_ERROR("m2m_ftp_connection_create(): connect socket. Error: %d\r\n", m2m_socket_errno());
		m2m_socket_bsd_close(p_socket);
		return M2M_FTP_SOCK_CREATE_ERROR;
	}
	return true;
#else

#ifndef NODEBUG
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_connection_create(): Setting socket as non blocking...\r\n");
#endif
	nonblocking = 1;
	if (m2m_socket_bsd_ioctl(*p_socket, M2M_SOCKET_BSD_FIONBIO, &nonblocking) != 0)	{
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_connection_create(): Error setting socket in non blocking mode!\r\n");
	   /* Handle the ioctl error. */
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_connection_create(). Error: %d", m2m_socket_errno());
	   return M2M_FTP_SOCK_UNBLOCKING_ERROR;
	}
#ifndef NODEBUG
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_connection_create(): Done. Connecting...\r\n");
#endif

	m2m_socket_bsd_connect(*p_socket, (M2M_SOCKET_BSD_SOCKADDR *)&serverSockAddr,
										sizeof(struct M2M_SOCKET_BSD_SOCKADDR_IN));

	while (count < FTP_CONNECTION_TIMEOUT )
	{
		sockState = m2m_socket_bsd_socket_state(*p_socket);

		/* available from FW 12.00.007-B03 */
		if (sockState == M2M_SOCKET_STATE_CONNECTED) {
#ifndef NODEBUG
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_connection_create(): Socket connected.\r\n");
#endif
			//LOG_INFO("\n\rSocket connected");
			return M2M_FTP_SUCCESS;
		}

		count += 1;
		m2m_os_sleep_ms(1000);
	}

	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_connection_create(): connecting the socket.\n");
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_connection_create(). Error: %d", m2m_socket_errno());
	return M2M_FTP_SOCK_CONNECT_ERROR;

#endif

}

void m2m_ftp_connection_destroy(M2M_SOCKET_BSD_SOCKET *p_socket)
{
	m2m_socket_bsd_close(*p_socket);
	while (true) {
		if (m2m_socket_bsd_socket_state(*p_socket) == M2M_SOCKET_STATE_CLOSED ) {
			break;
		}
		m2m_os_sleep_ms(500);
	}
	return;

}


FTP_CLIENT_ERROR m2m_ftp_session_create(const M2M_SOCKET_BSD_SOCKET socket, const String p_user, const String p_pass)
{
	Message msgOut, msgIn;
	char expString[128];

	/* init vars */
	m2m_ftp_message_clear(&msgOut);
	m2m_ftp_message_clear(&msgIn);
	DynPoolSize = 64*1024;

	/* Change the dynamic memory pool size to 64KB */
	m2m_os_mem_pool(DynPoolSize);


	if (! p_user || ! p_pass) {
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_session_create(): input strings not initialized\n");
		return M2M_FTP_UNALLOCATED_BUFF_ERROR;
	}


	/* Expected server response: greeting message */
	if (!m2m_ftp_recv(socket, &msgIn,60000,M2M_FTP_SERVER_RESP_READY)) {
		if (!m2m_ftp_message_hasCode(&msgIn, M2M_FTP_CODES_SERVER_READY)) {
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_session_create(): Missing greeting message.\n");
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Message: %s %s\n", (&msgIn)->m_verb, (&msgIn)->m_param );
			return M2M_FTP_GREETING_MISSING;
		}
	}
#ifndef NODEBUG
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Received greeting code.\n");
#endif

	/* Client sends USER username */
	m2m_ftp_message_setType(&msgOut, M2M_FTP_CMDS_USERNAME);
	m2m_ftp_message_clear(&msgIn);

	/* Copy the username into the output message */
	strcpy(msgOut.m_param, p_user);
#ifndef NODEBUG
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"username: %s\n", msgOut.m_param);
#endif

	/* Expected server response: '331 Password required for <username>' */
	if (!m2m_ftp_query(socket, &msgOut, &msgIn, 60000, M2M_FTP_SERVER_RESP_USER_OK(p_user))) {
		/* Expected message was not received, check at least for return code */
		if (!m2m_ftp_message_hasCode(&msgIn, M2M_FTP_CODES_USER_OK_WAIT_PASS)) {
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_session_create(): username rejected.\n");
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Message: %s %s\n", (&msgIn)->m_verb, (&msgIn)->m_param );
			return M2M_FTP_USERNAME_ERROR;
		}

	}
#ifndef NODEBUG
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Username Accepted. Asking for password..\n");
#endif

	/* Client sends PASS password */
	m2m_ftp_message_setType(&msgOut, M2M_FTP_CMDS_PASSWORD);
	m2m_ftp_message_clear(&msgIn);

	/* Copy the password into the output message */
	strcpy(msgOut.m_param, p_pass);
#ifndef NODEBUG
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"password: %s\n", msgOut.m_param);
#endif

	/* Expected server response: '230 User <username> logged in.' */
	if (!m2m_ftp_query(socket, &msgOut, &msgIn, 60000, M2M_FTP_SERVER_RESP_USER_LOGGED_IN(p_user))) {
		if (!m2m_ftp_message_hasCode(&msgIn, M2M_FTP_CODES_USER_LOGGED_IN)) {
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_session_create(): password rejected.\n");
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Message: %s %s\n", (&msgIn)->m_verb, (&msgIn)->m_param );
			return M2M_FTP_PASSWORD_ERROR;
		}
	}
// session now established
	#ifndef NODEBUG
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"session_create(): success\n");
	#endif

	return M2M_FTP_SUCCESS;
}



FTP_CLIENT_ERROR m2m_ftp_session_close(const M2M_SOCKET_BSD_SOCKET socket)
{
#ifndef NODEBUG
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_session_destroy(): closing session.");
#endif
	char expString[128];

	Message msgOut, msgIn;

	m2m_ftp_message_clear(&msgOut);
	m2m_ftp_message_clear(&msgIn);

	m2m_ftp_message_setType(&msgOut, M2M_FTP_CMDS_SESSION_END);

	if (!m2m_ftp_query(socket, &msgOut, &msgIn, 60000, M2M_FTP_SERVER_RESP_GOODBYE) ||
			!m2m_ftp_message_hasCode(&msgIn, M2M_FTP_CODES_SERVER_GOODBYE))	{
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_session_close(): error closing connection.\n");
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Message: %s %s\n", (&msgIn)->m_verb, (&msgIn)->m_param );
		return M2M_FTP_SESSION_CLOSE_ERROR;
	}

	return M2M_FTP_SUCCESS;
}



FTP_CLIENT_ERROR m2m_ftp_get_server_features(const M2M_SOCKET_BSD_SOCKET socket, Message *p_features)
{
	Message msgOut, msgIn;

	m2m_ftp_message_clear(&msgOut);
	m2m_ftp_message_clear(&msgIn);


	m2m_ftp_message_setType(&msgOut, M2M_FTP_CMDS_FEATURES);

	/* Expected server response: '211 End' */
	if (!m2m_ftp_query(socket, &msgOut, &msgIn, 20000, M2M_FTP_SERVER_RESP_FEAT)) {
		/* Expected message was not received, check at least for return code */
		if (!m2m_ftp_message_hasCode(&msgIn, M2M_FTP_CODES_SERVER_FEAT)) {
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_get_server_features(): unexpected response.\n");
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Message: %s %s\n", (&msgIn)->m_verb, (&msgIn)->m_param );
			return M2M_FTP_FEATURES_ERROR;
		}

	}

	//TODO parse the output in an array of messages

	#ifndef NODEBUG
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Received features list..\n%s", (&msgIn)->m_param );
	#endif
	return M2M_FTP_SUCCESS;

}


FTP_CLIENT_ERROR m2m_ftp_get_Present_Working_Directory(const M2M_SOCKET_BSD_SOCKET socket, String p_currentFolder)
{
	Message msgOut, msgIn;
	char *p_startingString, *p_respString;

	int i, len, quotesCount = 0;

	if (!p_currentFolder) {
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_get_Present_Working_Directory(): output string not initialized\n");
		return M2M_FTP_UNALLOCATED_BUFF_ERROR;
	}

	m2m_ftp_message_clear(&msgOut);
	m2m_ftp_message_clear(&msgIn);

	m2m_ftp_message_setType(&msgOut, M2M_FTP_CMDS_PWD);

	/* Expected server response: '257 "<PWD>" is the current directory' */
	if (!m2m_ftp_query(socket, &msgOut, &msgIn, 20000, M2M_FTP_SERVER_RESP_PWD)) {
		/* Expected message was not received, check at least for return code */
		if (!m2m_ftp_message_hasCode(&msgIn, M2M_FTP_CODES_SERVER_PWD)) {
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_get_Present_Working_Directory(): unexpected response.\n");
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Message: %s %s\n", (&msgIn)->m_verb, (&msgIn)->m_param );
			return M2M_FTP_UNEXPECTED_RESPONSE;
		}
	}
	p_startingString = (&msgIn)->m_param;
	len = strlen(p_startingString);

	for (i=0; i< len; i++) {
		if ((char) (*(p_startingString + i)) == '"' ) {
			switch(quotesCount)	{
			case 0:
#if INCLUDE_QUOTE
				p_respString = p_startingString + i;		//include the " at the beginning
#else
				p_respString = p_startingString + i + 1;  //exclude the " at the beginning
#endif
				break;
			case 1:
#if INCLUDE_QUOTE
				*(p_startingString + i + 1) = '\0'; //set the character next to the 2nd " as end of string
#else
				*(p_startingString + i ) = '\0'; //set the 2nd " as end of string (exclude the " at the end)
#endif
				break;

			default:
				break;
			}
			quotesCount++;
		}
	}

	memset(p_currentFolder, 0, sizeof(p_currentFolder));
	strcpy(p_currentFolder, p_respString);
	return M2M_FTP_SUCCESS;
}


FTP_CLIENT_ERROR m2m_ftp_Change_Working_Directory(const M2M_SOCKET_BSD_SOCKET socket, String p_path)
{

	Message msgOut, msgIn;

	if (!p_path) {
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_Change_Working_Directory(): input string not initialized\n");
		return M2M_FTP_UNALLOCATED_BUFF_ERROR;
	}


	m2m_ftp_message_clear(&msgOut);
	m2m_ftp_message_clear(&msgIn);

	/* Copy the path into the output message */
	m2m_ftp_message_setType(&msgOut, M2M_FTP_CMDS_CWD);
	strcpy(msgOut.m_param, p_path);
	//LOG_VERB("m2m_ftp_CWD(): path: %s\n", msgOut.m_param);


	/* Expected server response: '250 CWD command successful' */
	if (!m2m_ftp_query(socket, &msgOut, &msgIn, 20000, M2M_FTP_SERVER_RESP_CWD)) {
		/* Expected message was not received, check at least for return code */
		if (!m2m_ftp_message_hasCode(&msgIn, M2M_FTP_CODES_SERVER_CWD)) {
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_Change_Working_Directory(): unexpected response.\n");
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Message: %s %s\n", (&msgIn)->m_verb, (&msgIn)->m_param );
			return M2M_FTP_UNEXPECTED_RESPONSE;
		}
	}
	return M2M_FTP_SUCCESS;
}

FTP_CLIENT_ERROR m2m_ftp_change_to_parent_directory(const M2M_SOCKET_BSD_SOCKET socket)
{
	Message msgOut, msgIn;

	m2m_ftp_message_clear(&msgOut);
	m2m_ftp_message_clear(&msgIn);

	/* Copy the path into the output message */
	m2m_ftp_message_setType(&msgOut, M2M_FTP_CMDS_CD_PARENT_DIR);


	/* Expected server response: '250 CWD command successful' */
	if (!m2m_ftp_query(socket, &msgOut, &msgIn, 20000, M2M_FTP_SERVER_RESP_CDUP)) {
		/* Expected message was not received, check at least for return code */
		if (!m2m_ftp_message_hasCode(&msgIn, M2M_FTP_CODES_SERVER_CDUP)) {
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_change_to_parent_directory(): unexpected response.\n");
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Message: %s %s\n", (&msgIn)->m_verb, (&msgIn)->m_param );
			return M2M_FTP_UNEXPECTED_RESPONSE;
		}
	}
	return M2M_FTP_SUCCESS;
}





FTP_CLIENT_ERROR m2m_ftp_set_ASCII_type(const M2M_SOCKET_BSD_SOCKET socket)
{
	Message msgOut, msgIn;

	if (get_last_type() == ASCII) { /*Type is already ASCII, return. */
		return M2M_FTP_SUCCESS;
	}


	m2m_ftp_message_clear(&msgOut);
	m2m_ftp_message_clear(&msgIn);

	/* Copy the Type into the output message */
	m2m_ftp_message_setType(&msgOut, M2M_FTP_CMDS_TYPE);
	strcpy(msgOut.m_param, "A");


	/* Expected server response: '200 Type set to I' */
	if (!m2m_ftp_query(socket, &msgOut, &msgIn, 20000, M2M_FTP_SERVER_RESP_TYPE_A))	{
		/* Expected message was not received, check at least for return code */
		if (!m2m_ftp_message_hasCode(&msgIn, M2M_FTP_CODES_SERVER_TYPE)) {
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_set_ASCII_type(): unexpected response.\n");
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Message: %s %s\n", (&msgIn)->m_verb, (&msgIn)->m_param );
			return M2M_FTP_UNEXPECTED_RESPONSE;
		}
	}
#ifndef NODEBUG
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"ASCII type set.\n");
#endif
	set_last_type(ASCII);
	return M2M_FTP_SUCCESS;
}

FTP_CLIENT_ERROR m2m_ftp_set_Image_type(const M2M_SOCKET_BSD_SOCKET socket)
{
	Message msgOut, msgIn;

	if (get_last_type() == IMAGE) { /*Type is already Image, return. */
		return M2M_FTP_SUCCESS;
	}

	m2m_ftp_message_clear(&msgOut);
	m2m_ftp_message_clear(&msgIn);

	/* Copy the Type into the output message */
	m2m_ftp_message_setType(&msgOut, M2M_FTP_CMDS_TYPE);
	strcpy(msgOut.m_param, "I");


	/* Expected server response: '200 Type set to I' */
	if (!m2m_ftp_query(socket, &msgOut, &msgIn, 20000, M2M_FTP_SERVER_RESP_TYPE_I))	{
		/* Expected message was not received, check at least for return code */
		if (!m2m_ftp_message_hasCode(&msgIn, M2M_FTP_CODES_SERVER_TYPE)) {
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_set_Image_type(): unexpected response.\n");
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Message: %s %s\n", (&msgIn)->m_verb, (&msgIn)->m_param );
			return M2M_FTP_UNEXPECTED_RESPONSE;
		}
	}

#ifndef NODEBUG
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Binary type set.\n");
#endif
	set_last_type(IMAGE);
	return M2M_FTP_SUCCESS;
}



FTP_CLIENT_ERROR m2m_ftp_execute_command(const M2M_SOCKET_BSD_SOCKET socket, Message *p_msgIn,
		String p_command, String p_parameters, String p_expected_response)
{
	Message msgOut;

	char p_code_buf[5];
	int expected_code;

	if (!p_command || !p_expected_response)	{
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_ask_remote_file(): input buffers not initialized\n");
		return M2M_FTP_UNALLOCATED_BUFF_ERROR;
	}

	/* Get the numeric code from expected string */
	memset(p_code_buf,0,sizeof(p_code_buf));
	/* TODO: check for p_expected_response to be at least 4 bytes?*/
	strncpy(p_code_buf,p_expected_response,4);
	expected_code = atoi(p_code_buf);


	/* Clear buffers */
	m2m_ftp_message_clear(&msgOut);
	m2m_ftp_message_clear(p_msgIn);

	/* Copy the command and parameters (if any) into the output message */
	m2m_ftp_message_setType(&msgOut, p_command);

	if (p_parameters)
		strcpy(msgOut.m_param, p_parameters);


	/* Check for expected server response. */
	if (!m2m_ftp_query(socket, &msgOut, p_msgIn, 20000, p_expected_response)) {
		/* Expected message was not received, check at least for return code */
		if (!m2m_ftp_message_hasCode(p_msgIn, expected_code)) {
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_ask_remote_file(): unexpected response.\n");
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Message: %s %s\n", (p_msgIn)->m_verb, (p_msgIn)->m_param );
			return M2M_FTP_UNEXPECTED_RESPONSE;
		}
	}
	return M2M_FTP_SUCCESS;

}

FTP_CLIENT_ERROR m2m_ftp_get_remote_file_size(const M2M_SOCKET_BSD_SOCKET socket, String p_filename, UINT32 *p_size)
{
	Message msgOut, msgIn;
	FTP_CLIENT_ERROR ret;

	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"in remote_file_function_getting_file_size if_case_akhi.\n");
	if (!p_filename || !p_size) {
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_get_remote_file_size(): pointers not initialized\n");
		return M2M_FTP_UNALLOCATED_BUFF_ERROR;
	}


	m2m_ftp_message_clear(&msgOut);
	m2m_ftp_message_clear(&msgIn);

	/* Copy the filename into the output message */
	m2m_ftp_message_setType(&msgOut, M2M_FTP_CMDS_REM_FILE_SIZE);
	strcpy(msgOut.m_param, p_filename);

	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"in remote_file_function_getting_file_size_akhi.\n");
	/* Set IMAGE type */
	if((ret = m2m_ftp_set_Image_type(socket)) != M2M_FTP_SUCCESS ) {
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE, "m2m_ftp_get_remote_file_size(): cannot set Binary type. %d\n", ret);
		return ret;
	}
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_get_remote_file_size(): Type set to Binary (I)\r\n");

	/* Expected server response: '200 Type set to I' */
	if (!m2m_ftp_query(socket, &msgOut, &msgIn, 20000, M2M_FTP_SERVER_RESP_FILE_STAT)) {
		/* Expected message was not received, check at least for return code */
		if (!m2m_ftp_message_hasCode(&msgIn, M2M_FTP_CODES_SERVER_FILE_STAT)) {
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_get_remote_file_size(): unexpected response.\n");
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Message: %s %s\n", (&msgIn)->m_verb, (&msgIn)->m_param );
			return M2M_FTP_UNEXPECTED_RESPONSE;
		}
	}
	*p_size = atoi((&msgIn)->m_param);

	return M2M_FTP_SUCCESS;
}


FTP_CLIENT_ERROR m2m_ftp_set_passive_mode(const M2M_SOCKET_BSD_SOCKET socket, UINT32* p_ipAddr, UINT32* p_port)
{
	Message msgOut, msgIn;
	char *p_startString, *p_endString;
	int i, len;
	char temp[26];
	char ip_addr_str[24];
	char comma_count=0;
	int  tmp_port = 0;

	if (!p_ipAddr || !p_port) {
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_set_passive_mode(): output buffer not initialized\n");
		return M2M_FTP_UNALLOCATED_BUFF_ERROR;
	}

	m2m_ftp_message_clear(&msgOut);
	m2m_ftp_message_clear(&msgIn);

	m2m_ftp_message_setType(&msgOut, M2M_FTP_CMDS_PASSIVE);

	/* Expected server response: '227 Entering Passive Mode (<ip and port>)' */
	if (!m2m_ftp_query(socket, &msgOut, &msgIn, 20000, M2M_FTP_SERVER_RESP_PASV)) {
		/* Expected message was not received, check at least for return code */
		if (!m2m_ftp_message_hasCode(&msgIn, M2M_FTP_CODES_SERVER_PASV)) {
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_set_passive_mode(): unexpected response.\n");
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Message: %s %s\n", (&msgIn)->m_verb, (&msgIn)->m_param );
			return M2M_FTP_UNEXPECTED_RESPONSE;
		}
	}

	memset(temp,0,sizeof(temp));

	if ((p_startString = strstr((&msgIn)->m_param,"(")) &&
			(p_endString = strstr((&msgIn)->m_param,")"))) {
		//LOG_VERB("parsing the string...\n");

		len = p_endString - (p_startString + 1);
		strncpy(temp, p_startString + 1, len);
		temp[len] = '\0';

		//LOG_VERB("len: %d\n",len);

		for (i=0; i < len; i++)	{
			if (temp[i] == ',') {
				comma_count++;
				if (comma_count < 4) {
					temp[i] = '.';
				} else if(comma_count == 4) { 			/* End of IP address */
					temp[i] = '\0';
					p_startString = &(temp[i+1]); 		/* From here starts the port string */
					memset(ip_addr_str,0,sizeof(ip_addr_str));
					strcpy(ip_addr_str,temp);
				} else if(comma_count == 5)	{ 			/* Parse the port... */
					temp[i] = '\0'; 					/* Delete the comma */
					tmp_port += atoi(p_startString) * 256;
					p_startString = &(temp[i+1]); 		/* From here starts the port string */
					tmp_port += atoi(p_startString);
				}
			}
		}

	} else {
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_set_passive_mode(): Error parsing the response string.\n");
		return M2M_FTP_RESPONSE_PARSING_ERROR;
	}


	*p_ipAddr = m2m_socket_bsd_inet_addr(ip_addr_str);
	*p_port = tmp_port;


	return M2M_FTP_SUCCESS;
}

FTP_CLIENT_ERROR m2m_ftp_request_remote_file(const M2M_SOCKET_BSD_SOCKET socket, String p_filename)
{
	Message msgOut, msgIn;
	char *p_startString, *p_endString;
	UINT32 tmp_size;

	if (!p_filename )	{
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_request_remote_file(): pointers not initialized\n");
		return M2M_FTP_UNALLOCATED_BUFF_ERROR;
	}


	m2m_ftp_message_clear(&msgOut);
	m2m_ftp_message_clear(&msgIn);

	/* Copy the filename into the output message */
	m2m_ftp_message_setType(&msgOut, M2M_FTP_CMDS_RETRIEVE_FILE);
	strcpy(msgOut.m_param, p_filename);


	/* Expected server response: '150 Opening [...]' */
	if (!m2m_ftp_query(socket, &msgOut, &msgIn, 20000, M2M_FTP_SERVER_RESP_RETR)) {
		/* Expected message was not received, check at least for return code */
		if (!m2m_ftp_message_hasCode(&msgIn, M2M_FTP_CODES_SERVER_RECV)) {
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_request_remote_file(): unexpected response.\n");
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Message: %s %s\n", (&msgIn)->m_verb, (&msgIn)->m_param );
			return M2M_FTP_UNEXPECTED_RESPONSE;
		}
	}
	/* check for 226 response before the transfer completion*/
	if ( strstr((&msgIn)->m_param, M2M_FTP_SERVER_RESP_TRANSF_COMPL_INT)) {
		set_received_transfer_complete(1);
	}


	return M2M_FTP_SUCCESS;
}

FTP_CLIENT_ERROR m2m_ftp_download_remote_file(const M2M_SOCKET_BSD_SOCKET socket, String p_localFilename, String p_remoteFilename, UINT32 *p_file_size ,UINT32 p_size)
{
	M2M_SOCKET_BSD_SOCKET data_socket;

	UINT32 dataAddress, dataPort;
	Message msgIn;
//	UINT32 file_size ;//=163868;//23456768;

	INT32 taskId;

	FTP_CLIENT_ERROR ret;


	if(!p_localFilename || !p_remoteFilename)
	{
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_download_remote_file(): input buffer not initialized\n");
		return M2M_FTP_UNALLOCATED_BUFF_ERROR;
	}

	set_received_transfer_complete(0);

	m2m_ftp_message_clear(&msgIn);

	/* Clear the complete download status to false*/
	clearCompleted();

	/* Set passive mode and retrieve data socket IP address and port */
	if((ret = m2m_ftp_set_Image_type(socket)) != M2M_FTP_SUCCESS ) {
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE, "m2m_ftp_download_remote_file(): cannot set Binary type. %d\n", ret);
		*p_file_size = 0;
		return ret;
	}
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_download_remote_file(): Type set to Binary (I)\r\n");
	/* Set passive mode and retrieve data socket IP address and port */
	if((ret = m2m_ftp_set_passive_mode(socket, &dataAddress, &dataPort)) != M2M_FTP_SUCCESS ) {
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE, "m2m_ftp_download_remote_file(): set Passive mode failed. %d\n", ret);
		*p_file_size = 0;
		return ret;
	}
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_download_remote_file(): Data IP:PORT %s:%d\n", m2m_socket_bsd_addr_str(dataAddress), dataPort);


	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Initializing IPC semaphore.\r\n");
	initTransferLock();


	/* Open the data socket and pass it to the data task */
	if ((ret = m2m_ftp_connection_create(&data_socket, m2m_socket_bsd_addr_str(dataAddress), dataPort))
			!= M2M_FTP_SUCCESS ) {
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_download_remote_file(): Connection to %s:%s failed. %d\n",
										m2m_socket_bsd_addr_str(dataAddress), dataPort, ret);
		*p_file_size = 0;
		return ret;
	}

	if ((ret = m2m_ftp_request_remote_file(socket, p_remoteFilename)) != M2M_FTP_SUCCESS) {
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE, "m2m_ftp_download_remote_file(): ask remote file failed. %d\n", ret);
		*p_file_size = 0;
		return RSP_FTP_REMOTE_FILE_NOT_FOUND;
	}

	/*getting the remote file size...*/
//	if (M2M_FTP_SUCCESS != m2m_ftp_get_remote_file_size(socket, p_remoteFilename, &p_size)) {
//		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"FTP client: Get remote file size failed.\n");
//		*p_file_size = 0;
//		return 9;
//	}

	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_download_remote_file(): file size: %d\n", p_size);



	/* Create the file download task */
// Changed task creation to function

	M2M_DataReceive(data_socket, p_size, (INT32)p_localFilename);

	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Waiting for transfer task completion (semaphore is locked).\r\n");
	/* wait for the download Task to complete */

	waitTransferLock();
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Transfer task completed (semaphore unlocked). Getting server response...\r\n");

	if (getTransferStatus() < 0) {
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_download_remote_file(): An error occurred while receiving file data.\n");
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Error: %d\n", getTransferStatus());
		*p_file_size = 0;
		return M2M_FTP_GET_FILE_DOWNLOAD_STATUS_ERROR; //use m2m_ftp_get_last_download_status() to retrieve the last download error status
	}

	m2m_ftp_message_clear(&msgIn);

#ifndef NODEBUG
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"received transfer complete status: %d.\r\n", get_received_transfer_complete());
#endif

	if (get_received_transfer_complete() == 0) { /*the 226 response message was not received, check explicitly*/

		if (!m2m_ftp_recv(socket, &msgIn, 60000, M2M_FTP_SERVER_RESP_TRANSF_COMPL))	{
			if (!m2m_ftp_message_hasCode(&msgIn, M2M_FTP_CODES_SERVER_TRSF_CMPL)) {
				dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_download_remote_file(): cannot receive the trasfer complete message\n");
				dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Message: %s %s\n", (&msgIn)->m_verb, (&msgIn)->m_param );
				*p_file_size = 0;
				return M2M_FTP_GET_FILE_TRANSFER_COMPLETE_MISSING;
				/* TODO we need to somehow close this FTP request before we can send any other command.... */
			}
		}
	}
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Transfer Completed\n");
	setCompleted();


	*p_file_size = p_size;


	return M2M_FTP_SUCCESS;
}




FTP_CLIENT_ERROR m2m_ftp_store_remote_file(const M2M_SOCKET_BSD_SOCKET socket, String p_filename)
{
	Message msgOut, msgIn;
	char *p_startString, *p_endString;
	UINT32 tmp_size;

	if (!p_filename)	{
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_store_remote_file(): pointer not initialized\n");
		return M2M_FTP_UNALLOCATED_BUFF_ERROR;
	}

	m2m_ftp_message_clear(&msgOut);
	m2m_ftp_message_clear(&msgIn);

	/* Copy the filename into the output message */
	m2m_ftp_message_setType(&msgOut, M2M_FTP_CMDS_STORE_FILE);
	strcpy(msgOut.m_param, p_filename);


	/* Expected server response: '150 Opening [...]' */
	if (!m2m_ftp_query(socket, &msgOut, &msgIn, 20000, M2M_FTP_SERVER_RESP_STOR)) {
		/* Expected message was not received, check at least for return code */
		if (!m2m_ftp_message_hasCode(&msgIn, M2M_FTP_CODES_SERVER_STOR)) {
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_store_remote_file(): unexpected response.\n");
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Message: %s %s\n", (&msgIn)->m_verb, (&msgIn)->m_param );
			return M2M_FTP_UNEXPECTED_RESPONSE;
		}
	}
	return M2M_FTP_SUCCESS;
}



FTP_CLIENT_ERROR m2m_ftp_upload_local_file(const M2M_SOCKET_BSD_SOCKET socket, String p_localFilename, String p_remoteFilename)
{
	M2M_SOCKET_BSD_SOCKET data_socket;

	UINT32 dataAddress, dataPort;
	Message msgIn;

	UINT32 file_size;
	UINT32 remote_file_size;
	INT32 taskId;

	FTP_CLIENT_ERROR ret;

	if(!p_localFilename || !p_remoteFilename)
	{
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_upload_local_file(): input strings not initialized\n");
		return M2M_FTP_UNALLOCATED_BUFF_ERROR;
	}


	m2m_ftp_message_clear(&msgIn);

	/* Clear the complete upload status to false*/
	clearCompleted();

	file_size = m2m_fs_get_size(p_localFilename);

	if (file_size == M2M_FS_ERROR) {
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_upload_local_file(): Cannot obtain local file size. Error: %d", m2m_fs_last_error());
		return M2M_FTP_LOCAL_FILE_SIZE_ERROR;
	}



	if((ret = m2m_ftp_set_Image_type(socket)) != M2M_FTP_SUCCESS ) {
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE, "m2m_ftp_upload_local_file(): cannot set Binary type. %d\n", ret);
		return ret;
	}

	/* Set passive mode and retrieve data socket IP address and port */
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_upload_local_file(): Type set to Binary (I)\r\n");
	/* Set passive mode and retrieve data socket IP address and port */
	if((ret = m2m_ftp_set_passive_mode(socket, &dataAddress, &dataPort)) != M2M_FTP_SUCCESS ) {
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE, "m2m_ftp_upload_local_file(): set Passive mode failed. %d\n", ret);
		return ret;
	}
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_upload_local_file(): Data IP:PORT %s:%d\n", m2m_socket_bsd_addr_str(dataAddress), dataPort);


	/* Open the data socket and pass it to the data task */
	if ((ret = m2m_ftp_connection_create(&data_socket, m2m_socket_bsd_addr_str(dataAddress), dataPort))
			!= M2M_FTP_SUCCESS ) {
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_upload_local_file(): Connection to %s:%s failed. %d\n",
										m2m_socket_bsd_addr_str(dataAddress), dataPort, ret);
		return ret;
	}


	if ((ret = m2m_ftp_store_remote_file(socket, p_remoteFilename)) != M2M_FTP_SUCCESS) {
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_upload_local_file(): ask remote file failed. %d\n", ret);
		return ret;
	}


	/* Create the file upload task */
	taskId = m2m_os_create_task(M2M_OS_TASK_STACK_XL,8,M2M_OS_TASK_MBOX_M,M2M_DataSend);


	m2m_os_send_message_to_task(taskId, data_socket, file_size, (INT32)p_localFilename);

	m2m_ftp_message_clear(&msgIn);


	if (!m2m_ftp_recv(socket, &msgIn, 60000, M2M_FTP_SERVER_RESP_TRANSF_COMPL))	{
		if (!m2m_ftp_message_hasCode(&msgIn, M2M_FTP_CODES_SERVER_TRSF_CMPL)) {
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_upload_local_file(): cannot receive the trasfer complete message\n");
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Message: %s %s\n", (&msgIn)->m_verb, (&msgIn)->m_param );
			return M2M_FTP_GET_FILE_TRANSFER_COMPLETE_MISSING;
			/* TODO we need to somehow close this FTP request before we can send any other command.... */
		}
	}

	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Transfer Completed\n");
	setCompleted();

	/* wait for the upload Task to complete */
	waitTransferLock();

	if (getTransferStatus() < 0) {
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_upload_local_file(): An error occurred while sending file data.\n");
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Error: %d\n", getTransferStatus());
		return M2M_FTP_GET_FILE_UPLOAD_STATUS_ERROR; //use m2m_ftp_get_last_transfer_status() to retrieve the last download error status
	}


	if ((ret = m2m_ftp_get_remote_file_size(socket, p_remoteFilename, &remote_file_size)) != M2M_FTP_SUCCESS) {
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_upload_local_file(): ask remote file size failed. %d\n", ret);
			return ret;
	}

	if (file_size != remote_file_size) {
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_upload_local_file(): remote file size (%u) is different than local file size (%u)\n", remote_file_size, file_size);
		return M2M_FTP_REMOTE_FILE_SIZE_ERROR;
	}




	return M2M_FTP_SUCCESS;
}




FTP_CLIENT_ERROR m2m_ftp_getPasvData(const M2M_SOCKET_BSD_SOCKET socket, String p_command,
							String p_parameters, String p_expected, String *pp_response )
{
	M2M_SOCKET_BSD_SOCKET data_socket;

	UINT32 dataAddress, dataPort;
	Message msgIn;

	UINT32 data_size;
	INT32 taskId;

	FTP_CLIENT_ERROR ret;

	if(!p_command || !p_expected)
	{
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_getPasvData(): input strings not initialized\n");
		return M2M_FTP_UNALLOCATED_BUFF_ERROR;
	}


	m2m_ftp_message_clear(&msgIn);

	/* Clear the complete download status to false*/
	clearCompleted();


	/* Set passive mode and retrieve data socket IP address and port */
	if((ret = m2m_ftp_set_passive_mode(socket, &dataAddress, &dataPort)) != M2M_FTP_SUCCESS ) {
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE, "m2m_ftp_getPasvData(): set Passive mode failed. %d\n", ret);
		return ret;
	}
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_getPasvData(): Data IP:PORT %s:%d\n", m2m_socket_bsd_addr_str(dataAddress), dataPort);


	/* Open the data socket and pass it to the data task */
	if ((ret = m2m_ftp_connection_create(&data_socket, m2m_socket_bsd_addr_str(dataAddress), dataPort))
			!= M2M_FTP_SUCCESS ) {
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_getPasvData(): Connection to %s:%s failed. %d\n",
										m2m_socket_bsd_addr_str(dataAddress), dataPort, ret);
		return ret;
	}


	if((ret = m2m_ftp_execute_command(socket, &msgIn, p_command, p_parameters, p_expected)) != M2M_FTP_SUCCESS ) {
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_getPasvData(): command execution failed. %d\n", ret);
		return ret;
	}


	/* Create the file download task */
	taskId = m2m_os_create_task(M2M_OS_TASK_STACK_XL,8,M2M_OS_TASK_MBOX_M,M2M_DataReceive);


	m2m_os_send_message_to_task(taskId, data_socket, 0, 0);

	m2m_ftp_message_clear(&msgIn);


	if (!m2m_ftp_recv(socket, &msgIn, 60000, M2M_FTP_SERVER_RESP_TRANSF_COMPL))	{
		if (!m2m_ftp_message_hasCode(&msgIn, M2M_FTP_CODES_SERVER_TRSF_CMPL)) {
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_getPasvData(): cannot receive the trasfer complete message\n");
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Message: %s %s\n", (&msgIn)->m_verb, (&msgIn)->m_param );
			return M2M_FTP_GET_FILE_TRANSFER_COMPLETE_MISSING;
			/* TODO we need to somehow close this FTP request before we can send any other command.... */
		}
	}

	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Transfer Completed\n");
	setCompleted();

	/* wait for the download Task to complete */
	waitTransferLock();

	if (getTransferStatus() < 0) {
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_getPasvData(): An error occurred while receiving file data.\n");
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Error: %d\n", getTransferStatus());
		return M2M_FTP_GET_FILE_DOWNLOAD_STATUS_ERROR; //use m2m_ftp_get_last_transfer_status() to retrieve the last download error status
	}

	data_size = strlen(data.p_buffer);

#ifndef NODEBUG
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Free mem: %d bytes; trying to allocate %d bytes\r\n", m2m_os_get_mem_info(NULL), data_size);
#endif

	if (data_size == 0) {
#ifndef NODEBUG
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Empty response received. Allocating 2 bytes for output buffer\r\n");
#endif
		/* an empty response was received. "Statically" allocate the output buffer, and put an empty string in it */
		*pp_response = (char*) m2m_os_mem_alloc(2 * sizeof(char));
		if (! *pp_response) {
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_getPasvData(): An error occurred while allocating output data buffer.\n");
			m2m_ftp_clear_DataBuffer(&data);
			return M2M_FTP_DYNAMIC_ALLOCATION_ERROR;
		}


		strcpy(*pp_response,"");

#ifndef NODEBUG
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Output buffer created.\r\n");
#endif

	} else {  /* data_size > 0 */

#ifndef NODEBUG
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Response received. Allocating output buffer\r\n");
#endif
		/* Allocate the output buffer and return the server response to the caller*/
		*pp_response = (char*) m2m_os_mem_alloc(strlen(data.p_buffer) * sizeof(char));
		if (! *pp_response) {
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_getPasvData(): An error occurred while allocating output data buffer.\n");
			m2m_ftp_clear_DataBuffer(&data);
			return M2M_FTP_DYNAMIC_ALLOCATION_ERROR;
		}

		strcpy(*pp_response,data.p_buffer);
	}
	/* Clear the data buffer */
	m2m_ftp_clear_DataBuffer(&data);

	return M2M_FTP_SUCCESS;
}



FTP_CLIENT_ERROR m2m_ftp_get_directory_List(const M2M_SOCKET_BSD_SOCKET socket, String p_path, String *pp_response)
{
	FTP_CLIENT_ERROR resp;
	resp = m2m_ftp_set_ASCII_type(socket);
	if (M2M_FTP_SUCCESS != resp) {
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_get_directory_List(): cannot set ASCII type.\r\n");
		return resp;
	}
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_get_directory_List(): Type set to ASCII (A)\r\n");

	resp = m2m_ftp_getPasvData(socket, M2M_FTP_CMDS_LIST, p_path, M2M_FTP_SERVER_RESP_LIST, pp_response );
	if (M2M_FTP_SUCCESS != resp) {
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_get_directory_List(): cannot get directory list\r\n");
		return resp;
	}
	return M2M_FTP_SUCCESS;
}


FTP_CLIENT_ERROR m2m_ftp_get_directory_info(const M2M_SOCKET_BSD_SOCKET socket, String p_path, String *pp_response)
{
	FTP_CLIENT_ERROR resp;
	resp = m2m_ftp_set_ASCII_type(socket);
	if (M2M_FTP_SUCCESS != resp) {
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_get_directory_info(): cannot set ASCII type.\r\n");
		return resp;
	}
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_get_directory_info(): Type set to ASCII (A)\r\n");

	resp = m2m_ftp_getPasvData(socket, M2M_FTP_CMDS_REMOTE_DIR_LIST, p_path, M2M_FTP_SERVER_RESP_MLSD, pp_response );
	if (M2M_FTP_SUCCESS != resp) {
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_get_directory_info(): cannot get directory list\r\n");
		return resp;
	}
	return M2M_FTP_SUCCESS;
}

FTP_CLIENT_ERROR m2m_ftp_get_remote_info(const M2M_SOCKET_BSD_SOCKET socket, String p_path, String *pp_response)
{

	Message msgOut, msgIn;
	char *p_startString, *p_endString;
	char *tmpBuf;
	UINT32 len = 0;


	m2m_ftp_message_clear(&msgOut);
	m2m_ftp_message_clear(&msgIn);

	/* Copy the path into the output message */
	m2m_ftp_message_setType(&msgOut, M2M_FTP_CMDS_REMOTE_INFO);

	/* If path is passed, will be sent to the server. Otherwise, PWD will be used*/
	if (p_path) {
		strcpy(msgOut.m_param, p_path);
	}


	/* Expected server response: '250 End of list' */
	if (!m2m_ftp_query(socket, &msgOut, &msgIn, 20000, M2M_FTP_SERVER_RESP_MLST)) {
		/* Expected message was not received, check at least for return code */
		if (!m2m_ftp_message_hasCode(&msgIn, M2M_FTP_CODES_SERVER_MLST)) {
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_get_remote_info(): unexpected response.\n");
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Message: %s %s\n", (&msgIn)->m_verb, (&msgIn)->m_param );
			return M2M_FTP_UNEXPECTED_RESPONSE;
		}
	}

	*pp_response = (char*) m2m_os_mem_alloc(strlen((&msgIn)->m_param) * sizeof(char));

	if (! *pp_response) {
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_get_remote_info(): An error occurred while allocating output data buffer.\n");
		return M2M_FTP_DYNAMIC_ALLOCATION_ERROR;
	}

	if (p_path) {
		tmpBuf = (char*) m2m_os_mem_alloc((strlen(p_path) + 22 ) * sizeof(char));

		if (! tmpBuf) {
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_get_remote_info(): An error occurred while allocating internal data buffer.\n");
			return M2M_FTP_DYNAMIC_ALLOCATION_ERROR;
		}

		memset(tmpBuf,0,sizeof(tmpBuf));
		sprintf(tmpBuf, "Start of list for %s\r\n", p_path);
	}
	else {
		tmpBuf = (char*) m2m_os_mem_alloc( 20  * sizeof(char));

		if (! tmpBuf) {
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_get_remote_info(): An error occurred while allocating internal data buffer.\n");
			return M2M_FTP_DYNAMIC_ALLOCATION_ERROR;
		}

		memset(tmpBuf,0,sizeof(tmpBuf));
		sprintf(tmpBuf, "Start of list for");
	}

	memset(*pp_response,0,strlen((&msgIn)->m_param));



	if ((p_startString = strstr((&msgIn)->m_param, tmpBuf))){
		/* no path was given, try to parse the string anyway*/
		if (! p_path) {
			/* search for the first EoL. Using p_endString as a support variable */
			if ((p_endString = strstr(p_startString, "\r\n"))) {
				p_startString = p_endString + 2; /* The starting string was found, strip it */
			}
			p_endString = NULL;
		}
		else {
			p_startString += strlen(tmpBuf);	/* The starting string was found, strip it */
		}

		if ((p_endString = strstr(p_startString, M2M_FTP_SERVER_RESP_MLST)) ) { /* the ending string was found, strip it as well */
        	len = p_endString - p_startString - 2;  /*strip also <CRLF> at the end*/
            strncpy(*pp_response, p_startString, len);
        }
        else { /* Only the starting string was found*/
        	strcpy(*pp_response, p_startString );
        }
	}
	else { /* Unable to find the strings, return the complete buffer */
		strcpy(*pp_response, (&msgIn)->m_param);
    }

	/* Release the internal buffer */
	m2m_os_mem_free(tmpBuf);

	return M2M_FTP_SUCCESS;
}




FTP_CLIENT_ERROR m2m_ftp_create_remote_directory(const M2M_SOCKET_BSD_SOCKET socket, String p_path)
{
	Message msgOut, msgIn;


	if(!p_path)
	{
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_create_remote_directory(): input string not initialized\n");
		return M2M_FTP_UNALLOCATED_BUFF_ERROR;
	}


	m2m_ftp_message_clear(&msgOut);
	m2m_ftp_message_clear(&msgIn);

	/* Copy the Type into the output message */
	m2m_ftp_message_setType(&msgOut, M2M_FTP_CMDS_MAKE_DIR);
	strcpy(msgOut.m_param, p_path);


	if (!m2m_ftp_query(socket, &msgOut, &msgIn, 20000, M2M_FTP_SERVER_RESP_MKD))	{
		/* Expected message was not received, check at least for return code */
		if (!m2m_ftp_message_hasCode(&msgIn, M2M_FTP_CODES_SERVER_MKD)) {
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_create_remote_directory(): unexpected response.\n");
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Message: %s %s\n", (&msgIn)->m_verb, (&msgIn)->m_param );
			return M2M_FTP_UNEXPECTED_RESPONSE;
		}
	}
	return M2M_FTP_SUCCESS;
}

FTP_CLIENT_ERROR m2m_ftp_delete_remote_directory(const M2M_SOCKET_BSD_SOCKET socket, String p_path)
{
	Message msgOut, msgIn;


	if(!p_path)
	{
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_delete_remote_directory(): input string not initialized\n");
		return M2M_FTP_UNALLOCATED_BUFF_ERROR;
	}

	m2m_ftp_message_clear(&msgOut);
	m2m_ftp_message_clear(&msgIn);

	/* Copy the Type into the output message */
	m2m_ftp_message_setType(&msgOut, M2M_FTP_CMDS_DELETE_DIR);
	strcpy(msgOut.m_param, p_path);


	if (!m2m_ftp_query(socket, &msgOut, &msgIn, 20000, M2M_FTP_SERVER_RESP_RMD))	{
		/* Expected message was not received, check at least for return code */
		if (!m2m_ftp_message_hasCode(&msgIn, M2M_FTP_CODES_SERVER_RMD)) {
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_delete_remote_directory(): unexpected response.\n");
					dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Message: %s %s\n", (&msgIn)->m_verb, (&msgIn)->m_param );
			return M2M_FTP_UNEXPECTED_RESPONSE;
		}
	}
	return M2M_FTP_SUCCESS;
}


FTP_CLIENT_ERROR m2m_ftp_delete_remote_file(const M2M_SOCKET_BSD_SOCKET socket, String p_filePath)
{
	Message msgOut, msgIn;

	if(!p_filePath)
	{
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_delete_remote_file(): input string not initialized\n");
		return M2M_FTP_UNALLOCATED_BUFF_ERROR;
	}

	m2m_ftp_message_clear(&msgOut);
	m2m_ftp_message_clear(&msgIn);

	/* Copy the Type into the output message */
	m2m_ftp_message_setType(&msgOut, M2M_FTP_CMDS_DELETE_FILE);
	strcpy(msgOut.m_param, p_filePath);


	if (!m2m_ftp_query(socket, &msgOut, &msgIn, 20000, M2M_FTP_SERVER_RESP_DELE_COMPLETE))	{
		/* Expected message was not received, check at least for return code */
		if (!m2m_ftp_message_hasCode(&msgIn, M2M_FTP_CODES_SERVER_DELE_OK)) {
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_delete_remote_file(): 'From' unexpected response.\n");
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Message: %s %s\n", (&msgIn)->m_verb, (&msgIn)->m_param );
			return M2M_FTP_UNEXPECTED_RESPONSE;
		}
	}

	return M2M_FTP_SUCCESS;
}






FTP_CLIENT_ERROR m2m_ftp_remote_rename(const M2M_SOCKET_BSD_SOCKET socket, String p_fromPath, String p_toPath)
{
	Message msgOut, msgIn;

	if(!p_fromPath || !p_toPath)
	{
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_remote_rename(): input strings not initialized\n");
		return M2M_FTP_UNALLOCATED_BUFF_ERROR;
	}

	m2m_ftp_message_clear(&msgOut);
	m2m_ftp_message_clear(&msgIn);

	/* Copy the Type into the output message */
	m2m_ftp_message_setType(&msgOut, M2M_FTP_CMDS_RENAME_FROM);
	strcpy(msgOut.m_param, p_fromPath);


	if (!m2m_ftp_query(socket, &msgOut, &msgIn, 20000, M2M_FTP_SERVER_WAIT_DEST_NAME))	{
		/* Expected message was not received, check at least for return code */
		if (!m2m_ftp_message_hasCode(&msgIn, M2M_FTP_CODES_SERVER_WAIT_DEST_NAME)) {
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_remote_rename(): 'From' unexpected response.\n");
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Message: %s %s\n", (&msgIn)->m_verb, (&msgIn)->m_param );
			return M2M_FTP_UNEXPECTED_RESPONSE;
		}
	}

	m2m_ftp_message_clear(&msgOut);
	m2m_ftp_message_clear(&msgIn);
	/* Copy the Type into the output message */
	m2m_ftp_message_setType(&msgOut, M2M_FTP_CMDS_RENAME_TO);
	strcpy(msgOut.m_param, p_toPath);

	if (!m2m_ftp_query(socket, &msgOut, &msgIn, 20000, M2M_FTP_SERVER_RENAME_COMPLETE))	{
		/* Expected message was not received, check at least for return code */
		if (!m2m_ftp_message_hasCode(&msgIn, M2M_FTP_CODES_SERVER_RENAME_OK)) {
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_ftp_remote_rename(): 'to' unexpected response.\n");
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Message: %s %s\n", (&msgIn)->m_verb, (&msgIn)->m_param );
			return M2M_FTP_UNEXPECTED_RESPONSE;
		}
	}

	return M2M_FTP_SUCCESS;
}




/*PASSIVE MODE Callback*/

#define DEFAULT_FILE_CHUNK 1024
#define DEFAULT_DATA_CHUNK 256

#define VOID_COUNT_TIMEOUT 10

static bool s_transferCompleted = false;
static M2M_T_OS_LOCK s_transferLock = NULL;
static TRANSFER_STATUS s_transferStatus;

char FileBuffer[10*1024];

extern UINT32 DynPoolSize;
extern DataBuffer data;



void clearCompleted(void)
{
	s_transferCompleted = false;
}

/* transfer is completed server side (a 226 message was received) */
void setCompleted(void)
{
	s_transferCompleted = true;
}

bool isCompleted(void)
{
	return s_transferCompleted;
}

//set the client transfer status (to check for transfer errors)
void setTransferStatus(TRANSFER_STATUS status)
{
	s_transferStatus = status;
}

TRANSFER_STATUS getTransferStatus(void)
{
	return s_transferStatus;
}




void initTransferLock(void)
{
	if (NULL == s_transferLock)
	{
		s_transferLock = m2m_os_lock_init(M2M_OS_LOCK_IPC);
	}
}

void waitTransferLock(void)
{
	m2m_os_lock_lock(s_transferLock);
}

void unlockTransferLock(void){
	m2m_os_lock_unlock(s_transferLock);
}


/* =================================================================================================
 *
 * DESCRIPTION:     Handles events sent to data task
 *
 * PARAMETERS:     	socket: addition info
 *                  size:   file size if a file is receiving (0 to not expect the size, for example for generic data receive, not file data)
 *                  file:   file Name (to store the data locally)
 *
 * RETURNS:         None.
 *
 * PRE-CONDITIONS:  None.
 *
 * POST-CONDITIONS: None.
 *
 * ============================================================================================== */

INT32 M2M_DataReceive(INT32 socket, INT32 size, INT32 file)
{
	char *p_fileName =(char*) file;
	int i,n;
	int bufferindex = 0;

	int written = 0;
	int chunk = DEFAULT_FILE_CHUNK;

	data.size = 0;
	data.p_buffer = NULL;
	char tmpBuffer[DEFAULT_DATA_CHUNK * 2];
	char *p_backup;

	UINT32 count = 0;
	UINT32 void_count = 0;

	M2M_SOCKET_BSD_SOCKET data_socket = (M2M_SOCKET_BSD_SOCKET) socket;

	UINT32 total = (UINT32) size;

	UINT32 int_timeout = 500, int_delay = 800;


	UINT32 globalTimeout = 300000;


	char *p_localFileName= p_fileName;
	M2M_T_FS_HANDLE localFileHandle = NULL;




	setTransferStatus(START);


	/* File Download */
	if (size > 0) {
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Requested to download remote file into local %s, size: %d\n", p_fileName, size);
		//Create the file on local file-system
		if (M2M_API_RESULT_SUCCESS != m2m_fs_create(p_localFileName)) {
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"M2M_DataReceive(): cannot create file. Error: %d",  m2m_fs_last_error());
			setTransferStatus(CANNOT_CREATE);
			m2m_ftp_connection_destroy(&data_socket);
			unlockTransferLock();
			return false;
		}

		//Open the file in append-mode
		localFileHandle = m2m_fs_open(p_localFileName, M2M_FS_OPEN_APPEND);
		if (NULL == localFileHandle) {
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"M2M_DataReceive(): cannot open the local file. Error: %d",  m2m_fs_last_error());
			setTransferStatus(CANNOT_OPEN);
			m2m_ftp_connection_destroy(&data_socket);
			unlockTransferLock();
			return false;
		}



		while (!isCompleted() || total > 0)
		{

#ifndef NODEBUG
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"receiving data...\n");
#endif

			if (chunk > sizeof(FileBuffer) - bufferindex) {
				chunk = sizeof(FileBuffer) - bufferindex;
			}
			else {
				chunk = DEFAULT_FILE_CHUNK;
			}

			/* 2 seconds timeout */
#ifndef NODEBUG
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"reading from data socket...\r\n");
#endif
			if ((n = m2m_socket_bsd_recv_timeout( data_socket, FileBuffer + bufferindex, chunk, 0, int_timeout)) < 0) {

				dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"M2M_DataReceive(): errno: %d",  m2m_socket_errno());
				setTransferStatus(RECV_ERROR);
				m2m_ftp_connection_destroy(&data_socket);
				unlockTransferLock();

				return false;
			}

			/* Some data was received, set the status as "receiving" (thus increasing the main socket timeout) */

			if (n > 0) {
#ifndef NODEBUG
				dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"M2M_DataReceive(): received %d bytes\r\n", n);

#endif
				setTransferStatus(RECEIVING);
				globalTimeout += 2*(int_timeout + int_delay);
				total -= n;
				bufferindex += n;
				void_count = 0;
			} else {
#ifndef NODEBUG
				dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"0 bytes received\r\n");
#endif
				void_count++;
#ifndef NODEBUG
				dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"This is the #%d time in a row that nothing was received!\r\n", void_count);
#endif
				setTransferStatus(START);
				if (void_count > VOID_COUNT_TIMEOUT) { /* Nothing was received for VOID_COUNT_TIMEOUT times. Exit the loop and close the data socket.  */
#ifndef NODEBUG
					dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Nothing was received for %d cycles. \
							Supposing the data reception is complete. Closing the receiving task..\n", VOID_COUNT_TIMEOUT);
#endif
					break;

				} else {
#ifndef NODEBUG
					dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Wait for 800ms..\r\n");
#endif
					m2m_os_sleep_ms(int_delay); //wait 4 seconds
				}
			}



			#ifndef NODEBUG
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"M2M_DataReceive(): bytes recv=%d, remaining=%d\n", n, total);
				//LOG_VERB("M2M_DataReceive(): buffer='%s'\n", FILEBUFF);
			#endif

			/* buffer is full, or we received the last chunk */
			if (bufferindex >= sizeof(FileBuffer) || total == 0) {
				/* FLUSH into file.. */
#ifndef NODEBUG
				dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Buffer full, flushing it into file..\n");
#endif
				/* File append */
				written = m2m_fs_write(localFileHandle, FileBuffer, bufferindex);
				if (written < bufferindex) {
					dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"M2M_DataReceive(): Write error: %d",  m2m_fs_last_error());
					setTransferStatus(WRITE_ERROR);
					m2m_fs_close(localFileHandle);
					m2m_ftp_connection_destroy(&data_socket);
					unlockTransferLock();
					return false;
				}
#ifndef NODEBUG
				dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Buffer written into file.\r\n");
#endif
				/* clear variables */
				written = 0;
				bufferindex = 0;
				memset(FileBuffer,0,sizeof(FileBuffer));
			}
			/* Completed reception*/
			if (total == 0)
			{
#ifndef NODEBUG
				dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"All bytes were received.\r\n");
#endif
				break;
			}

			/* Check if timeout expired */
			count += (int_timeout + int_delay);
			if (count > globalTimeout) {
				dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"M2M_DataReceive(): Reached timeout.\n");
				break;
			}

		}


		m2m_fs_close(localFileHandle);

		m2m_os_sleep_ms(2000);

		/* Check local file size */
//		if (m2m_fs_get_size(p_localFileName) < size) {
//			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"M2M_DataReceive(): Local file size is less then expected\n");
//			setTransferStatus(SIZE_ERROR);
//			m2m_fs_close(localFileHandle);
//			m2m_ftp_connection_destroy(&data_socket);
//			unlockTransferLock();
//			return false;
//		}

/* ============================================================================================================ */

	} else { 	/* Not a file operation. For example, directory list operation. we don't know the data size */

		total = 0;
		data.size = DEFAULT_DATA_CHUNK * 4;

		/* clear the receiving buffer */
		memset(tmpBuffer,0,sizeof(tmpBuffer));

		data.p_buffer = (char*) m2m_os_mem_alloc((data.size + 1)* sizeof(char));

		if (!data.p_buffer) {
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"M2M_DataReceive(): Cannot allocate buffer!\n");
			setTransferStatus(ALLOC_ERROR);
			m2m_ftp_connection_destroy(&data_socket);
			return false;
		}
		memset(data.p_buffer,0,data.size);
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Requested to receive data from server\n");
		chunk = DEFAULT_DATA_CHUNK;

		/* Start the receiving loop */
		while (!isCompleted())
		{

#ifndef NODEBUG
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"receiving data...\n");
#endif

			chunk = DEFAULT_DATA_CHUNK;
			n = 0;
			/* 0.5 seconds timeout */
			if ((n = m2m_socket_bsd_recv_timeout( data_socket, tmpBuffer, chunk, 0, int_timeout)) < 0) {

				dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"M2M_DataReceive(): errno: %d",  m2m_socket_errno());
				setTransferStatus(RECV_ERROR);
				m2m_ftp_connection_destroy(&data_socket);
				unlockTransferLock();

				return false;
			}

			/* Some data was received, set the status as "receiving" (thus increasing the main socket timeout) */
			if (n > 0) {

				setTransferStatus(RECEIVING);
				globalTimeout += 2*(int_timeout + int_delay);
				total += n;

				if (total >= data.size) {

#ifndef NODEBUG
					dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"total > data.size, reallocating the buffer...\n");
#endif


					/* Check if there is enough available bytes on dynamic memory pool space
					 * (2 times the data.size, one for p_backup, one for data.p_buffer) */
#ifndef NODEBUG
					dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Checking free dynamic memory...\n");
#endif
					if ((data.size + 1)*2 > m2m_os_get_mem_info(NULL)) {

#ifndef NODEBUG
						dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Space is not enough, increasing dynamic memory pool size...\n");
#endif
						DynPoolSize += (data.size + 1); /* Add to the pool size the data.size */

						if (m2m_os_mem_pool(DynPoolSize) < 0) { 	/* Increase the memory pool with the new size*/
							/* failed to increase the pool size. No more room in the memory pool?? */
							dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"M2M_DataReceive(): Cannot resize the dynamic memory pool to accommodate the data!\n");
							m2m_os_mem_free(p_backup);
							p_backup = NULL;
							setTransferStatus(MEM_POOL_ERROR);
							m2m_ftp_connection_destroy(&data_socket);
							//m2m_ftp_clear_DataBuffer(&data);
							return false;
						}

					}
					p_backup = (char*) m2m_os_mem_alloc((data.size + 1) * sizeof(char));

					if (!p_backup) {
						dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"M2M_DataReceive(): Cannot allocate backup buffer!\n");
						p_backup = NULL;
						setTransferStatus(ALLOC_ERROR);
						m2m_ftp_connection_destroy(&data_socket);
						//m2m_ftp_clear_DataBuffer(&data);
						return false;
					}

					memset(p_backup,0,data.size + 1);
#ifndef NODEBUG
					dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Copying the data in the backup buffer.\n");
#endif
					/* make a copy of the current data*/
					strcpy(p_backup,data.p_buffer);

					/* increment the buffer size */
					data.size += DEFAULT_DATA_CHUNK * 4;

#ifndef NODEBUG
					dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Reallocating the buffer with the new size.\n");
#endif
					/* realloc the buffer with the new size */
					//m2m_os_mem_pool(64*1024);

					data.p_buffer = (char*) m2m_os_mem_realloc(data.p_buffer, data.size * sizeof(char));

					if (!data.p_buffer) {
						dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"M2M_DataReceive(): Cannot reallocate buffer!\n");

						/* Clear buffers */
						m2m_os_mem_free(p_backup);
						p_backup = NULL;
						setTransferStatus(REALLOC_ERROR);
						m2m_ftp_connection_destroy(&data_socket);
						//m2m_ftp_clear_DataBuffer(&data);
						return false;
					}
					memset(data.p_buffer,0,data.size);

#ifndef NODEBUG
					dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Copying back the data in the buffer.\n");
#endif
					/* Copy again the data in the reallocated buffer */
					strcpy(data.p_buffer, p_backup);

					/* free the backup buffer */
					m2m_os_mem_free(p_backup);
					p_backup = NULL;

				}


#ifndef NODEBUG
				dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Adding the new string into the buffer.\n");
#endif
				/* Add the new content to the buffer*/
				strcat(data.p_buffer, tmpBuffer);



			}
			else {
				void_count++;
				setTransferStatus(START);
				if (void_count > VOID_COUNT_TIMEOUT) { /* Nothing was received for VOID_COUNT_TIMEOUT times. Exit the loop and close the data socket.  */
#ifndef NODEBUG
					dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Nothing was received for %d cycles. \
							Supposing the data reception is complete. Closing the receiving task..\n", VOID_COUNT_TIMEOUT);
#endif
					break;

				} else {
					m2m_os_sleep_ms(int_delay); //wait 4 seconds
				}
			}

			/* clear the receiving buffer */
			memset(tmpBuffer,0,sizeof(tmpBuffer));

			#ifndef NODEBUG
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"M2M_DataReceive(): received bytes=%d, total=%d\n", n, total);
				//LOG_VERB("M2M_DataReceive(): buffer='%s'\n", FILEBUFF);
			#endif

			/* Check if timeout expired */
			count += (int_timeout + int_delay);
			if (count > globalTimeout) {
				dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"M2M_DataReceive(): Reached timeout.\n");
				break;
			}

		}

	} /* end of else*/

#ifndef NODEBUG
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Closing data socket..\n");
#endif
	m2m_ftp_connection_destroy(&data_socket);

	//signal to the calling task that download was successfully completed.
	setTransferStatus(SUCCESS);
	unlockTransferLock();

	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Data transfer task completed\n");

	//LOG_VERB("Buffer size: %d\n", strlen(data.p_buffer));
	//LOG_VERB("Data:\n%s\n", data.p_buffer);
	return 0;
}






INT32 M2M_DataSend(INT32 socket, INT32 size, INT32 file)
{
	char *p_fileName =(char*) file;
	int i,n;
	int bufferindex = 0;

	int readBytes = 0;
	int chunk = DEFAULT_FILE_CHUNK;

	UINT32 void_count = 0, count = 0;
	UINT32 sendBufSize = 0;

	M2M_SOCKET_BSD_SOCKET data_socket = (M2M_SOCKET_BSD_SOCKET) socket;

	UINT32 fileSize = (UINT32) size;
	UINT32 total = 0;

	UINT32 int_timeout = 500, int_delay = 1500;


	UINT32 globalTimeout = 60000;


	char *p_localFileName= p_fileName;
	M2M_T_FS_HANDLE localFileHandle = NULL;
  INT32 socket_error; // 5Barz
  int   tx_timeout;   // 5Barz


	initTransferLock();
	setTransferStatus(START);


	/* File Upload */
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Requested to Upload %s, size: %d\n", p_fileName, fileSize);


	//Open the file in read-mode
	localFileHandle = m2m_fs_open(p_localFileName, M2M_FS_OPEN_READ);
	if (NULL == localFileHandle) {
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"M2M_DataSend(): cannot open the local file. Error: %d",  m2m_fs_last_error());
		setTransferStatus(CANNOT_OPEN);
		m2m_ftp_connection_destroy(&data_socket);
		unlockTransferLock();
		return false;
	}

	total = fileSize;


	while (!isCompleted() || total > 0)
	{

#ifndef NODEBUG
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Sending data...\n");
#endif


		/* Check if the data remaining on the file is less than a single default chunk. In this case, resize the chunk. */
		if (chunk > total) {
			chunk = total;
		}
		else {
			chunk = DEFAULT_FILE_CHUNK;
		}

		/* clear variables */
		readBytes = 0;
		memset(FileBuffer,0,sizeof(FileBuffer));

		/* Read data from local file */

		readBytes = m2m_fs_read(localFileHandle, FileBuffer, chunk);
		if (readBytes < chunk) {
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"M2M_DataSend(): Read error: %d",  m2m_fs_last_error());
			setTransferStatus(READ_ERROR);
			m2m_fs_close(localFileHandle);
			m2m_ftp_connection_destroy(&data_socket);
			unlockTransferLock();
			return false;
		}

		sendBufSize = m2m_socket_bsd_send_buf_size(data_socket);
#ifndef NODEBUG
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Send buffer size: %u\n", sendBufSize);
#endif
//		while (sendBufSize <= chunk * 3) {
		while (sendBufSize <= chunk ) {  // 5Barz
#ifndef NODEBUG
//			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Send buffer is almost full! Waiting...\r\n");
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Send buffer not ready yet, Waiting...\r\n"); //5Barz
#endif
			m2m_os_sleep_ms(int_delay);
			sendBufSize = m2m_socket_bsd_send_buf_size(data_socket);
		}

// 5Barz: deleted the code below and replace with our own    
//		if ((n = m2m_socket_bsd_send( data_socket, FileBuffer, chunk, 0 )) < 0) {
//			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"M2M_DataSend(): errno: %d\n",  m2m_socket_errno());			
//      setTransferStatus(SEND_ERROR);
//			m2m_ftp_connection_destroy(&data_socket);
//			unlockTransferLock();
//			return false;
//		}
    
    // 5Barz: New TX code:
    tx_timeout = 100;
    while( tx_timeout-- )
    {  
      n = m2m_socket_bsd_send( data_socket, FileBuffer, chunk, 0 );
     
	  	if( n < 0)
      {
        socket_error = m2m_socket_errno();
        if( socket_error == M2M_SOCKET_BSD_EWOULDBLOCK && tx_timeout > 0 )
        {
          dwPrintDebug( M2M_CLOUD_LOG_LOG, M2M_FALSE, "M2M_DataSend(): Waiting for TX buffer to clear..." );
          m2m_os_sleep_ms(100); // small delay for TX buffer to clear, then loop again
        }
        else  
        {  
          dwPrintDebug( M2M_CLOUD_LOG_LOG, M2M_FALSE, "M2M_DataSend(): errno: %d\n", socket_error );
          setTransferStatus(SEND_ERROR);
			    m2m_ftp_connection_destroy(&data_socket);
			    unlockTransferLock();
			    return false;
        }
		  }
      else break;
    }


		/* Some data was sent, set the status as "sending" (thus increasing the main socket timeout) */


		if (n > 0) {
			setTransferStatus(SENDING);
			globalTimeout += 2*(int_timeout + int_delay);
			total -= n; /* decrease the data counter */
			if (n < chunk) {
				/* TODO if less bytes than requested were sent, do something... */
			}
		}
		else {
			void_count++;
			setTransferStatus(START);
			if (void_count > VOID_COUNT_TIMEOUT) { /* Nothing was sent for VOID_COUNT_TIMEOUT times. Exit the loop and close the data socket.  */
#ifndef NODEBUG
				dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Nothing was sent for %d cycles. \
						Supposing there was some problem on the socket. Closing the receiving task..\n", VOID_COUNT_TIMEOUT);
#endif
				break;

			} else {

				m2m_os_sleep_ms(int_delay); //wait 4 seconds
			}

		}



		#ifndef NODEBUG
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"M2M_DataSend(): bytes sent=%d, remaining=%d\n", n, total);
			//LOG_VERB("M2M_DataReceive(): buffer='%s'\n", FILEBUFF);
		#endif


		/* Completed send*/
		if (total == 0)
		{
			break;
		}

		/* Check if timeout expired */
		count += (int_timeout + int_delay);
		if (count > globalTimeout) {
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"M2M_DataSend(): Reached timeout.\n");
			break;
		}

	}


	m2m_fs_close(localFileHandle);

	m2m_os_sleep_ms(1000);

#ifndef NODEBUG
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Closing data socket..\n");
#endif
	m2m_ftp_connection_destroy(&data_socket);

	//signal to the calling task that the transfer was successfully completed.
	setTransferStatus(SUCCESS);
	unlockTransferLock();

	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Data transfer task completed\n");

	return 0;
}











