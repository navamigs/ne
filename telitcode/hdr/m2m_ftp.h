/**
 * Fabio Pintus <fabio.pintus@telit.com>
 *
 * FTP client common services interface.
**/

#ifndef FTP_H
#define FTP_H

#if defined(__cplusplus) /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif

	/*UTILS*/

	#include <stdio.h>
	#include <string.h>
	#include <stdlib.h>
    #include <stdbool.h>

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





/* Comment to print additional messages*/
//	#define NODEBUG

/*Messages*/

	#ifndef HDR_M2M_FTP_MESSAGES_H_
	#define HDR_M2M_FTP_MESSAGES_H_

/* FTP COMMANDS */

	#define M2M_FTP_CMDS_USERNAME				"USER"	/* param = username */
	#define M2M_FTP_CMDS_PASSWORD				"PASS"	/* param = password */

	#define M2M_FTP_CMDS_FEATURES				"FEAT"
	#define M2M_FTP_CMDS_PWD					"PWD"
	#define M2M_FTP_CMDS_CWD					"CWD"
	#define M2M_FTP_CMDS_TYPE					"TYPE"	/* A: ASCII, I: image(binary data) */
	#define M2M_FTP_CMDS_PASSIVE				"PASV"
	#define M2M_FTP_CMDS_LIST					"LIST"	/* param = [remote-directory] */
	#define M2M_FTP_CMDS_REM_FILE_SIZE 			"SIZE" 	/* param = remote_filename. returns remote filesize (in bytes) as a decimal number */
	#define M2M_FTP_CMDS_RETRIEVE_FILE			"RETR" 	/* param = remote_filename */
	#define	M2M_FTP_CMDS_STORE_FILE				"STOR" 	/* param = remote_filename */
	#define M2M_FTP_CMDS_DELETE_FILE			"DELE"	/* param = remote_filename */

	#define M2M_FTP_CMDS_MAKE_DIR				"MKD"	/* param = remote_directory */
	#define M2M_FTP_CMDS_DELETE_DIR				"RMD"	/* param = remote_directory */
	#define M2M_FTP_CMDS_CD_PARENT_DIR			"CDUP"

	#define M2M_FTP_CMDS_RENAME_FROM			"RNFR"  /* param = from_filename . Must be followed by RNTO to specify the new file name */
	#define M2M_FTP_CMDS_RENAME_TO				"RNTO"	/* param = to_filename*/

	#define M2M_FTP_CMDS_REMOTE_DIR_LIST		"MLSD"	/* param = [remote-directory]*/
	#define M2M_FTP_CMDS_REMOTE_INFO		  	"MLST"	/* param = remote-file-path*/



	#define M2M_FTP_CMDS_SESSION_END			"QUIT"



	/////////////////////////////////////////////

	/*TODO*/

/*
MODE mode-character

Sets the transfer mode to one of:

    S - Stream
    B - Block
    C - Compressed

The default mode is Stream.
*/
	#define M2M_FTP_CMDS_MACHINE_LIST_DIRECTORY "MODE"	/* param = mode*/



	#define M2M_FTP_CMDS_ABORT				"ABOR"
	/////////////////////////////////////////////




/* FTP SERVER RESPONSE CODES */
	/*--- SHARED CODES ---------------------------*/
	#define M2M_FTP_CODES_SERVER_OPEN_DATA_CONN	150
	#define M2M_FTP_CODES_SERVER_FILE_ACTION_OK	250
	#define M2M_FTP_CODES_SERVER_PATH_ACTION_OK	257
	/*--------------------------------------------*/

	#define M2M_FTP_CODES_SERVER_RECV 			M2M_FTP_CODES_SERVER_OPEN_DATA_CONN
	#define M2M_FTP_CODES_SERVER_STOR 			M2M_FTP_CODES_SERVER_OPEN_DATA_CONN
	#define M2M_FTP_CODES_SERVER_TYPE 			200
	#define M2M_FTP_CODES_SERVER_FEAT 			211
	#define M2M_FTP_CODES_SERVER_FILE_STAT		213
	#define M2M_FTP_CODES_SERVER_READY 			220
	#define M2M_FTP_CODES_SERVER_GOODBYE 		221
	#define M2M_FTP_CODES_SERVER_TRSF_CMPL 		226
	#define M2M_FTP_CODES_SERVER_PASV	 		227

	#define M2M_FTP_CODES_USER_LOGGED_IN 		230


	#define M2M_FTP_CODES_SERVER_CWD			M2M_FTP_CODES_SERVER_FILE_ACTION_OK
	#define M2M_FTP_CODES_SERVER_RMD			M2M_FTP_CODES_SERVER_FILE_ACTION_OK
	#define M2M_FTP_CODES_SERVER_RENAME_OK		M2M_FTP_CODES_SERVER_FILE_ACTION_OK
	#define M2M_FTP_CODES_SERVER_CDUP			M2M_FTP_CODES_SERVER_FILE_ACTION_OK
	#define M2M_FTP_CODES_SERVER_DELE_OK		M2M_FTP_CODES_SERVER_FILE_ACTION_OK
	#define M2M_FTP_CODES_SERVER_MLST			M2M_FTP_CODES_SERVER_FILE_ACTION_OK

	#define M2M_FTP_CODES_SERVER_PWD			M2M_FTP_CODES_SERVER_PATH_ACTION_OK
	#define M2M_FTP_CODES_SERVER_MKD			M2M_FTP_CODES_SERVER_PATH_ACTION_OK

	#define M2M_FTP_CODES_USER_OK_WAIT_PASS		331
	#define M2M_FTP_CODES_SERVER_WAIT_DEST_NAME	350

	#define M2M_FTP_CODES_SYNTAX_ERROR_SERIES	500

	#define M2M_FTP_SERVER_CODE_AS_STRING(code) \
	({ \
		char temp[6]; \
		sprintf(temp, "%d " , code); \
		temp; \
	})


/* FTP SERVER RESPONSE MESSAGES */
	/*--- SHARED RESPONSES------------------------*/
	#define M2M_FTP_SERVER_RESP_DATA_OPEN		"150 Opening "
	#define M2M_FTP_SERVER_RESP_PATH_ACTION_OK	"257 \""
	/*--------------------------------------------*/

	#define M2M_FTP_SERVER_RESP_RETR			M2M_FTP_SERVER_RESP_DATA_OPEN
	#define M2M_FTP_SERVER_RESP_STOR			M2M_FTP_SERVER_RESP_DATA_OPEN
	#define M2M_FTP_SERVER_RESP_LIST			M2M_FTP_SERVER_RESP_DATA_OPEN
	#define M2M_FTP_SERVER_RESP_MLSD			M2M_FTP_SERVER_RESP_DATA_OPEN
	#define M2M_FTP_SERVER_RESP_MLST			"250 End of list"

	#define M2M_FTP_SERVER_RESP_TYPE_A			"200 Type set to A"
	#define M2M_FTP_SERVER_RESP_TYPE_I			"200 Type set to I"
	#define M2M_FTP_SERVER_RESP_FEAT 			"211 End"
	#define M2M_FTP_SERVER_RESP_FILE_STAT		"213 "
	#define M2M_FTP_SERVER_RESP_READY 			"220 FTP Server ready."
	#define M2M_FTP_SERVER_RESP_GOODBYE 		"221 Goodbye."
	#define M2M_FTP_SERVER_RESP_TRANSF_COMPL    "226 Transfer complete"
	#define M2M_FTP_SERVER_RESP_TRANSF_COMPL_INT "226 "
	#define M2M_FTP_SERVER_RESP_PASV			"227 Entering Passive Mode ("
	#define M2M_FTP_SERVER_RESP_CWD 			"250 CWD command successful"
	#define M2M_FTP_SERVER_RESP_RMD 			"250 RMD command successful"
	#define M2M_FTP_SERVER_RENAME_COMPLETE		"250 Rename successful"
	#define M2M_FTP_SERVER_RESP_CDUP			"250 CDUP command successful"
	#define M2M_FTP_SERVER_RESP_DELE_COMPLETE	"250 DELE command successful"
	#define M2M_FTP_SERVER_RESP_PWD				M2M_FTP_SERVER_RESP_PATH_ACTION_OK
	#define M2M_FTP_SERVER_RESP_MKD				M2M_FTP_SERVER_RESP_PATH_ACTION_OK
	#define M2M_FTP_SERVER_WAIT_DEST_NAME		"350 File or directory exists, ready for destination name"


	#define M2M_FTP_SERVER_RESP_USER_OK(user) \
	({ \
		char temp[128]; \
		sprintf(temp, "331 Password required for %s" , user); \
		temp; \
	})

	#define M2M_FTP_SERVER_RESP_USER_LOGGED_IN(user) \
	({ \
		char temp[128]; \
		sprintf(temp, "230 User %s logged in." , user); \
		temp; \
	})






/* Enumerations*/

	typedef enum
	{
		UNSET,
		ASCII,
		IMAGE
	} DATA_TYPE;

	typedef enum
	{
		SENDING				= 3,
		RECEIVING			= 2,
		SUCCESS				= 1,
		START 				= 0,
		CANNOT_CREATE		= -200,
		CANNOT_OPEN			= -201,
		RECV_ERROR			= -202,
		WRITE_ERROR			= -203,
		SEND_ERROR			= -204,
		READ_ERROR			= -205,
		SIZE_ERROR			= -206,
		MEM_POOL_ERROR		= -207,
		ALLOC_ERROR			= -208,
		REALLOC_ERROR		= -209,
	} TRANSFER_STATUS;

	typedef enum
	{
		//Connection errors
		M2M_FTP_SUCCESS = 1,
		M2M_FTP_GET_IP_ERROR = -1,
		M2M_FTP_SOCK_CREATE_ERROR = -2,
		M2M_FTP_SOCK_UNBLOCKING_ERROR = -3,
		M2M_FTP_SOCK_CONNECT_ERROR = -4,

		//Services errors
		M2M_FTP_GREETING_MISSING = -5,
		M2M_FTP_USERNAME_ERROR = -6,
		M2M_FTP_PASSWORD_ERROR = -7,
		M2M_FTP_SESSION_CLOSE_ERROR = -8,
		M2M_FTP_FEATURES_ERROR = -9,
		M2M_FTP_UNALLOCATED_BUFF_ERROR = -10,
		M2M_FTP_UNEXPECTED_RESPONSE = -11,
		M2M_FTP_RESPONSE_PARSING_ERROR = -12,

#if 0
		M2M_FTP_PWD_UNEXPECTED_RESPONSE = -11,
		M2M_FTP_CWD_UNEXPECTED_RESPONSE = -12,
		M2M_FTP_ASCII_UNEXPECTED_RESPONSE = -13,
		M2M_FTP_SIZE_UNEXPECTED_RESPONSE = -14,
		M2M_FTP_PASV_UNEXPECTED_RESPONSE = -15,
		M2M_FTP_PASV_RESPONSE_PARSING_ERROR = -16,
		M2M_FTP_ASK_REMOTE_FILE_UNEXPECTED_RESPONSE = -17,
		M2M_FTP_ASK_REMOTE_FILE_SIZE_PARSING_ERROR = -18,
#endif
		M2M_FTP_REMOTE_FILE_SIZE_ERROR = -15,
		M2M_FTP_LOCAL_FILE_SIZE_ERROR = -16,
		M2M_FTP_GET_DATA_TRANSFER_COMPLETE_MISSING = -17,
		M2M_FTP_GET_DATA_DOWNLOAD_STATUS_ERROR = -18,
		M2M_FTP_GET_FILE_TRANSFER_COMPLETE_MISSING = -19,
		M2M_FTP_GET_FILE_DOWNLOAD_STATUS_ERROR = -20,
		M2M_FTP_GET_FILE_UPLOAD_STATUS_ERROR = -21,
		M2M_FTP_DYNAMIC_ALLOCATION_ERROR = -22

	} FTP_CLIENT_ERROR;


	/* sizes in bytes */
	#define M2M_FTP_MESSAGE_SIZE 	1024
	#define M2M_FTP_VERB_SIZE			4
	#define M2M_FTP_PARAMETER_SIZE	( M2M_FTP_MESSAGE_SIZE - M2M_FTP_VERB_SIZE )

	/* typedefs */

//	typedef enum {
//		false,
//		true
//	} Boolean;

	typedef char* String;


	/* data structures */
	typedef char FTP_VERB[M2M_FTP_VERB_SIZE+1];

	typedef struct {
		FTP_VERB m_verb;							/* the message type/command */
		char m_param[M2M_FTP_PARAMETER_SIZE+1]; 	/* the message content */
	} Message;

	/* Passive mode data buffer definition */
	typedef struct {
		char *p_buffer;
		UINT32 size;
	} DataBuffer;

	DATA_TYPE get_last_type(void);
	void set_last_type(DATA_TYPE type);


/* AppZone Wrappers*/
	/*emulates the calloc utility with AppZone's APIs*/
	void * m2m_os_mem_calloc(UINT32 nitems, UINT32 size);

	/***
	 * This function should be invoked to receive the data on the specified socket. It uses a non-blocking socket to receive
	 * data from a socket with a timeout.
	 * The parameters are the same as m2m_socket_bsd_recv(), adding
	 * @param 	timeout 	Timeout of the receive in milliseconds (granularity: 100 ms).
	 * 						The function will always return after timeout expiration
	 */
	INT32 m2m_socket_bsd_recv_timeout(M2M_SOCKET_BSD_SOCKET socket, void *p_buf, INT32 len, INT32 flags, UINT32 timeout);


/* Utils */
	/**
	 * Returns the type of the Message.
	 * @pre	p_msg != NULL
	 */
	#define m2m_ftp_message_getType(p_msg) ( (p_msg)->m_verb )

	/**
	 * Changes the type of the Message.
	 * @pre	p_msg != NULL
	 * @pre	arg is null terminated
	 */
	#define m2m_ftp_message_setType(p_msg, arg) ( strcpy((p_msg)->m_verb, arg) )

	/**
	 * Returns the value of the Message.
	 * @pre	p_msg != NULL
	 */
	#define m2m_ftp_message_getValue(p_msg) ( (p_msg)->m_param )

	/**
	 * Changes the value of the Message.
	 * @pre	p_msg != NULL
	 * @pre	arg is null terminated
	 */
	#define m2m_ftp_message_setValue(p_msg, arg) ( strcpy((p_msg)->m_param, arg) )

	/**
	 * Checks if type of the Message is equal to the given type.
	 * @pre	p_msg != NULL
	 * @pre	value is null terminated
	 */
	#define m2m_ftp_message_hasType(p_msg, type) ( strcmp((p_msg)->m_verb, type) == 0 )

	#define m2m_ftp_message_hasCode(p_msg, type) ( (atoi((p_msg)->m_verb)) == type )

	/**
	 * Checks if value of the Message is equal to the given value.
	 * @pre	p_msg != NULL
	 * @pre	value is null terminated
	 */
	#define m2m_ftp_message_hasValue(p_msg, value) ( strcmp((p_msg)->m_param, value) == 0 )

	/**
	 * Fills the Message member values with zeros.
	 * @pre	p_msg != NULL
	 */
	#define m2m_ftp_message_clear(p_msg) ( memset(p_msg, 0, sizeof(Message)) )


/* services */

	/**
	 * Serializes a Message object for network transport.
	 * @param	ap_msg		Object to serialze
	 * @param	a_result	Storage (minimum size = SIFTP_MESSAGE_SIZE) where the resulting serialized string will be placed
	 */
	bool m2m_ftp_serialize(const Message *p_msg, String p_result);

	/**
	 * Deserializes a Message object from network transport.
	 * @param	a_str		String to deserialize
	 * @param	ap_result	Storage where resulting deserialized object will be placed
	 */
	bool m2m_ftp_deserialize(const String p_str, Message *p_result);

	/**
	 * Sends data to a FTP server.
	 * @param	socket		Socket descriptor.
	 * @param	p_query		Query message.
	 */
	bool m2m_ftp_send(const M2M_SOCKET_BSD_SOCKET socket, const Message *p_query);

	/**
	 * Waits for data from a FTP server.
	 * @param	socket		Socket descriptor.
	 * @param	p_response	Output buffer for response message.
	 * @param	timeout		Timeout of the response query in milliseconds (granularity: 100ms)
	 * @param	p_expected	Expected server response. Leave NULL to discard the check (exit only on timeout).
	 */
	bool m2m_ftp_recv(const M2M_SOCKET_BSD_SOCKET socket, Message *p_response, UINT32 timeout, String p_expected);


	/**
	 * Performs a send & receive operation.
	 * @param	socket		Socket descriptor.
	 * @param	p_query		Query message.
	 * @param	p_response	Storage for response message.
	 * @param	timeout		timeout of the response query in milliseconds (granularity: 100ms)
	 * @param	p_expected	Expected server response. Leave NULL to discard the check.
	 */
	bool m2m_ftp_query(const M2M_SOCKET_BSD_SOCKET socket, const Message *p_query, Message *p_response, UINT32 timeout, String p_expected);


	/**
	 * Returns the last occurred download status (set by a data retrieving task)
	 */
	TRANSFER_STATUS m2m_ftp_get_last_download_status(void);

	/**
	 * Performs a read on the socket to clear pending data in case of previous failure. (2 seconds timeout)
	 * @param	a_socket	Socket descriptor on which to send.
	 */
	bool m2m_ftp_clear_buffer(const M2M_SOCKET_BSD_SOCKET socket);


	/**
	 * Clears the DataBuffer structure used to retrieve data from the FTP server (it also frees the allocated memory)
	 * @param	p_data		pointer to the DataBuffer variable
	 */
	void m2m_ftp_clear_DataBuffer(DataBuffer *p_data);



	#endif


	/* constants */

 	/* timeout in seconds */
	#define FTP_CONNECTION_TIMEOUT 60

	/* when returning the PWD string, should the quotation marks be included? */
	#define INCLUDE_QUOTE 0

	/* APIs */
	
	/**
	 * Establishes a network connection with the specified server.
	 * @param	p_socket		Pointer to the socket descriptor.
	 * @param	p_serverName	Canonical name or IP address of remote host (as a string).
	 * @param	serverPort		Port number in decimal of remote host.
	 * @return  An error code among
	 * 			- M2M_FTP_UNALLOCATED_BUFF_ERROR	any of the input pointers is NULL
	 * 			- M2M_FTP_GET_IP_ERROR				error getting the server IP from DNS
	 * 			- M2M_FTP_SOCK_CREATE_ERROR			error creating the connection socket
	 * 			- M2M_FTP_SOCK_UNBLOCKING_ERROR		error setting the socket as non-blocking
	 * 			- M2M_FTP_SOCK_CONNECT_ERROR		error performing the actual connection
	 * 			- M2M_FTP_SUCCESS					in case of success.
	 */
	FTP_CLIENT_ERROR m2m_ftp_connection_create(M2M_SOCKET_BSD_SOCKET *p_socket, const String p_serverName, const int serverPort);

	/**
	 * Closes the network connection.
	 * @param	p_socket		Pointer to the socket descriptor.
	*/
	void m2m_ftp_connection_destroy(M2M_SOCKET_BSD_SOCKET *p_socket);

	/**
	 * Establishes an FTP session.
	 * @param	socket	Socket descriptor.
	 * @param	p_user	User name.
	 * @param	p_pass	Password.
	 * @return  An error code among
	 * 			- M2M_FTP_UNALLOCATED_BUFF_ERROR	any of the input pointers is NULL
	 * 			- M2M_FTP_GREETING_MISSING			the greeting message from server was missing
	 * 			- M2M_FTP_USERNAME_ERROR			error receiving the response after sending the Username
	 * 			- M2M_FTP_PASSWORD_ERROR			error receiving the response after sending the password
	 * 			- M2M_FTP_SUCCESS					in case of successful login.
	 */
	FTP_CLIENT_ERROR m2m_ftp_session_create(const M2M_SOCKET_BSD_SOCKET socket, const String p_user, const String p_pass);

	/**
	 * Closes an established FTP session.
	 * @param	socket	Socket descriptor.
	 * @return  An error code among
	 * 			- M2M_FTP_SESSION_CLOSE_ERROR		error receiving the goodbye response from the server
	 * 			- M2M_FTP_SUCCESS					in case of successful login.
	 */
	FTP_CLIENT_ERROR m2m_ftp_session_close(const M2M_SOCKET_BSD_SOCKET socket);



	FTP_CLIENT_ERROR m2m_ftp_get_server_features(const M2M_SOCKET_BSD_SOCKET socket, Message *p_features);


	/**
	 * Asks the FTP server for the remote Present Working Directory.
	 * @param	socket				Socket descriptor.
	 * @param	p_currentFolder		Output Buffer containing the server response.
	 * @return  An error code among
	 * 			- M2M_FTP_UNALLOCATED_BUFF_ERROR	any of the input pointers is NULL
	 * 			- M2M_FTP_UNEXPECTED_RESPONSE		a response was received, but not the expected one
	 * 			- M2M_FTP_SUCCESS					in case of success.
	 */
	FTP_CLIENT_ERROR m2m_ftp_get_Present_Working_Directory(const M2M_SOCKET_BSD_SOCKET socket, String p_currentFolder);

	/**
	 * Performs a change working directory operation.
	 * @param	socket	Socket descriptor.
	 * @param	p_path	New remote path. It must be absolute regarding "/".
	 * 					The m2m_ftp_get_Present_Working_Directory() output can be used to build the path from a relative one.
	 * @return  An error code among
	 * 			- M2M_FTP_UNALLOCATED_BUFF_ERROR	any of the input pointers is NULL
	 * 			- M2M_FTP_UNEXPECTED_RESPONSE		a response was received, but not the expected one
	 * 			- M2M_FTP_SUCCESS					in case of success.
	 */
	FTP_CLIENT_ERROR m2m_ftp_Change_Working_Directory(const M2M_SOCKET_BSD_SOCKET socket, String p_path);


	/**
	 * Asks the FTP server to change the working directory to the parent one.
	 * @param	socket	Socket descriptor.

	 * @return  An error code among
	 * 			- M2M_FTP_UNEXPECTED_RESPONSE		a response was received, but not the expected one
	 * 			- M2M_FTP_SUCCESS					in case of success.
	 */
	FTP_CLIENT_ERROR m2m_ftp_change_to_parent_directory(const M2M_SOCKET_BSD_SOCKET socket);

	/**
		 *	Asks the server to set the data Type to Image (Binary)
		 *	@param socket 	Socket descriptor
		 *	@return  An error code among
		 * 			- M2M_FTP_UNEXPECTED_RESPONSE	a response was received, but not the expected one
		 * 			- M2M_FTP_SUCCESS				in case of success.
		 */
	FTP_CLIENT_ERROR m2m_ftp_set_Image_type(const M2M_SOCKET_BSD_SOCKET socket);

	/**
	 *	Asks the server to set the data Type to ASCII
	 *	@param socket 	Socket descriptor
	 *	@return  An error code among
	 * 			- M2M_FTP_UNEXPECTED_RESPONSE		a response was received, but not the expected one
	 * 			- M2M_FTP_SUCCESS					in case of success.
	 */
	FTP_CLIENT_ERROR m2m_ftp_set_ASCII_type(const M2M_SOCKET_BSD_SOCKET socket);


	/**
	 *	Asks the FTP server to execute a generic command. Use it for commands that give an immediate response.
	 *	Commands that require a second connection (as passive mode) cannot be executed directly with this API.
	 *	See m2m_ftp_getPasvData().
	 *	@param	socket		 		Socket descriptor.
	 * 	@param	p_msgIn		 		The container of the server response as a Message structure.
	 * 	@param	p_command    		The command to be executed ( e.g. PWD)
	 * 	@param  p_parameters 		Optional parameters of the command
	 * 	@param  p_expected_response	The expected status response of the server, as a string.
	 * 	@return  An error code among
	 * 			- M2M_FTP_UNALLOCATED_BUFF_ERROR		any of p_command or p_expected_response is NULL
	 * 			- M2M_FTP_UNEXPECTED_RESPONSE			a response was received, but not the expected one
	 * 			- M2M_FTP_SUCCESS						in case of success.
	 */
	FTP_CLIENT_ERROR m2m_ftp_execute_command(const M2M_SOCKET_BSD_SOCKET socket, Message *p_msgIn,
			String p_command, String p_parameters, String p_expected_response);


	/**
	 * Asks the FTP server for the size (in Bytes) of a remote file.
	 * @param	socket		Socket descriptor.
	 * @param	p_filename	The remote file path. Can be both relative to PWD or absolute.
	 * @param	p_size		Pointer to the integer value that will be filled with the file size in Bytes.
	 * @return  An error code among
	 * 			- M2M_FTP_UNALLOCATED_BUFF_ERROR	any of the input pointers is NULL
	 * 			- M2M_FTP_UNEXPECTED_RESPONSE		a response was received, but not the expected one
	 * 			- M2M_FTP_SUCCESS					in case of success.
	 */
	FTP_CLIENT_ERROR m2m_ftp_get_remote_file_size(const M2M_SOCKET_BSD_SOCKET socket, String p_filename, UINT32 *p_size);


	/**
	 * Asks the FTP server to start a passive mode connection. In case of success,
	 * the input variables will contain the server socket for the passive mode data transfer.
	 * @param	socket		Socket descriptor.
	 * @param	p_ipAddr	Pointer to the integer that will contain the IP address where the server will be listening to.
	 * @param	p_port		Pointer to the integer that will contain the Port number where the server will be listening to.
	 * @return  An error code among
	 * 			- M2M_FTP_UNALLOCATED_BUFF_ERROR		any of the input pointers is NULL
	 * 			- M2M_FTP_UNEXPECTED_RESPONSE			a response was received, but not the expected one
	 * 			- M2M_FTP_RESPONSE_PARSING_ERROR 		there was an error parsing the server response
	 * 			- M2M_FTP_SUCCESS						in case of success.
	 */
	FTP_CLIENT_ERROR m2m_ftp_set_passive_mode(const M2M_SOCKET_BSD_SOCKET socket, UINT32* p_ipAddr, UINT32 *p_port);


	/**
	 * Asks the FTP server for a remote file connection.
	 * After this request, a passive mode connection must be
	 * initiated and data will be received on the secondary socket.
	 * @param	socket		Socket descriptor.
	 * @param	p_ipAddr	String containing the remote file path (absolute path).
	 * @param	p_port		Pointer to the integer that will contain file size in bytes.
	 * @return  An error code among
	 * 			- M2M_FTP_UNALLOCATED_BUFF_ERROR		any of the input pointers is NULL
	 * 			- M2M_FTP_UNEXPECTED_RESPONSE			a response was received, but not the expected one
	 * 			- M2M_FTP_RESPONSE_PARSING_ERROR		there was an error parsing the server response.
	 * 			- M2M_FTP_SUCCESS						in case of success.
	 */
	FTP_CLIENT_ERROR m2m_ftp_request_remote_file(const M2M_SOCKET_BSD_SOCKET socket, String p_filename);

	/**
	 * Writes a file locally, downloading it from the FTP server (any existing file at the given local path will be overwritten).
	 * @param	socket 						Socket descriptor of the FTP active session
	 * @param	p_localFilename				absolute path of the local file that will be created
	 * @param	p_remoteFilename			Path of the remote file to download. Can be both relative to PWD or absolute.
	 * @param 	p_file_size					Pointer to the remote file size that will be retrieved by the function and returned in case of success.
	 * @return  An error code among
	 * 			- M2M_FTP_UNALLOCATED_BUFF_ERROR				any of the input pointers is NULL
	 * 			- M2M_FTP_GET_FILE_TRANSFER_COMPLETE_MISSING	the "transfer complete" message was not received
	 * 			- M2M_FTP_GET_FILE_DOWNLOAD_STATUS_ERROR		there was an error during the Passive mode data downloading.
																Refer to DOWNLOAD_STATUS enum.
	 * 			- M2M_FTP_SUCCESS								in case of success.
	 */
	FTP_CLIENT_ERROR m2m_ftp_download_remote_file(const M2M_SOCKET_BSD_SOCKET socket, String p_localFilename, String p_remoteFilename, UINT32 *p_file_size ,UINT32 p_size);



	/**
	 * Asks the FTP server to create a new a remote file. The file content will be sent through a PASV data socket
	 * @param	socket 			Socket descriptor of the FTP active session
	 * @param 	p_fromPath		Path of the remote file to be created. Can be both relative to PWD or absolute.
	 *
	 * @return  An error code among
	 * 			- M2M_FTP_UNALLOCATED_BUFF_ERROR				any of the input pointers is NULL
	 * 			- M2M_FTP_UNEXPECTED_RESPONSE					a response was received, but not the expected one
	 * 			- M2M_FTP_SUCCESS								in case of success.
	 */
	FTP_CLIENT_ERROR m2m_ftp_store_remote_file(const M2M_SOCKET_BSD_SOCKET socket, String p_filename);


	/**
	 * Performs the upload of a local file to the FTP server. (Any existing remote file will be overwritten)
	 * @param	socket 				Socket descriptor of the FTP active session
	 * @param 	p_remoteFilename	Path of the remote file to upload. Can be both relative to PWD or absolute.
	 * @param 	p_localFilename		Absolute path of the source local file.
	 *
	 * @return  An error code among
	 * 			- M2M_FTP_UNALLOCATED_BUFF_ERROR				any of the input pointers is NULL
	 * 			- M2M_FTP_UNEXPECTED_RESPONSE					a response was received, but not the expected one
	 * 			- M2M_FTP_SUCCESS								in case of success.
	 */
	FTP_CLIENT_ERROR m2m_ftp_upload_local_file(const M2M_SOCKET_BSD_SOCKET socket, String p_localFilename, String p_remoteFilename);




	/**
	 * Asks the FTP server for data through a passive connection.
	 * @param	socket 			Socket descriptor of the FTP active session
	 * @param 	p_command		The FTP command (e.g. LIST, MLSD..)
	 * @param	p_parameters 	The optional command parameters (e.g. the absolute folder path)
	 * @param	p_expected		The expected server response, as a string.
	 * @param	pp_response		Double pointer to a buffer that will be filled with the server response.
	 * 			The dynamic allocation of pp_response is performed by the function. ** PLEASE NOTE: Deallocation is up to the user. **
	 * @return  An error code among
	 * 			- M2M_FTP_UNALLOCATED_BUFF_ERROR				any of the input pointers is NULL
	 * 			- M2M_FTP_GET_DATA_TRANSFER_COMPLETE_MISSING	the "transfer complete" message was not received
	 * 			- M2M_FTP_GET_DATA_DOWNLOAD_STATUS_ERROR		there was an error during the Passive mode data downloading.
																Refer to DOWNLOAD_STATUS enum.
	 * 			- M2M_FTP_SUCCESS								in case of success.
	 */
	FTP_CLIENT_ERROR m2m_ftp_getPasvData(const M2M_SOCKET_BSD_SOCKET socket, String p_command,
								String p_parameters, String p_expected, String *pp_response);


	/**
	 * Asks the directory LIST to the FTP server. Data will be returned on the pp_response output buffer.
	 * @param	socket 		Socket descriptor of the FTP active session
	 * @param 	p_path		the absolute path for the LIST command. Leave NULL to ask for current working directory
	 * @param	pp_response	pointer to a string that will be filled with the server response.
	 * 			The dynamic allocation of pp_response is performed by the function. ** PLEASE NOTE: Deallocation is up to the user. **
	 * @return  An error code among
	 * 			- M2M_FTP_UNALLOCATED_BUFF_ERROR				any of the input pointers is NULL
	 * 			- M2M_FTP_UNEXPECTED_RESPONSE					a response was received, but not the expected one
	 * 			- M2M_FTP_GET_DATA_TRANSFER_COMPLETE_MISSING	the "transfer complete" message was not received
	 * 			- M2M_FTP_GET_DATA_DOWNLOAD_STATUS_ERROR		there was an error during the Passive mode data downloading.
																Refer to DOWNLOAD_STATUS enum.
	 * 			- M2M_FTP_SUCCESS								in case of success.
	 */
	FTP_CLIENT_ERROR m2m_ftp_get_directory_List(const M2M_SOCKET_BSD_SOCKET socket, String p_path, String *pp_response);


	FTP_CLIENT_ERROR m2m_ftp_get_directory_info(const M2M_SOCKET_BSD_SOCKET socket, String p_path, String *pp_response);


	FTP_CLIENT_ERROR m2m_ftp_get_remote_info(const M2M_SOCKET_BSD_SOCKET socket, String p_path, String *pp_response);




	/**
		 * Asks the FTP server to create a new remote folder
		 * @param	socket 			Socket descriptor of the FTP active session
		 * @param 	p_path			The new folder path. Can be both relative to PWD or absolute.
		 * @return  An error code among
		 * 			- M2M_FTP_UNALLOCATED_BUFF_ERROR				any of the input pointers is NULL
		 * 			- M2M_FTP_UNEXPECTED_RESPONSE					a response was received, but not the expected one
		 * 			- M2M_FTP_SUCCESS								in case of success.
		 */
	FTP_CLIENT_ERROR m2m_ftp_create_remote_directory(const M2M_SOCKET_BSD_SOCKET socket, String p_path);

	/**
	 * Asks the FTP server to delete a new remote folder
	 * @param	socket 			Socket descriptor of the FTP active session
	 * @param 	p_path			The folder path to be deleted. Can be both relative to PWD or absolute.
	 * @return  An error code among
	 * 			- M2M_FTP_UNALLOCATED_BUFF_ERROR				any of the input pointers is NULL
	 * 			- M2M_FTP_UNEXPECTED_RESPONSE					a response was received, but not the expected one
	 * 			- M2M_FTP_SUCCESS								in case of success.
	 */
	FTP_CLIENT_ERROR m2m_ftp_delete_remote_directory(const M2M_SOCKET_BSD_SOCKET socket, String p_path);


	/**
	 * Asks the FTP server to delete a remote filer.
	 * @param	socket 			Socket descriptor of the FTP active session
	 * @param 	p_filePath		The path of the file to delete. Can be both relative to PWD or absolute.
	 *
	 * @return  An error code among
	 * 			- M2M_FTP_UNALLOCATED_BUFF_ERROR				any of the input pointers is NULL
	 * 			- M2M_FTP_UNEXPECTED_RESPONSE					a response was received, but not the expected one
	 * 			- M2M_FTP_SUCCESS								in case of success.
	 */
	FTP_CLIENT_ERROR m2m_ftp_delete_remote_file(const M2M_SOCKET_BSD_SOCKET socket, String p_filePath);


	/**
	 * Asks the FTP server to rename a remote file or folder. Can also be used to move  a file or a folder to a different path
	 * @param	socket 			Socket descriptor of the FTP active session
	 * @param 	p_fromPath		The source file or folder path. Can be both relative to PWD or absolute.
	 * @param 	p_fromPath		The destination file or folder path. Can be both relative to PWD or absolute.
	 *
	 * @return  An error code among
	 * 			- M2M_FTP_UNALLOCATED_BUFF_ERROR				any of the input pointers is NULL
	 * 			- M2M_FTP_UNEXPECTED_RESPONSE					a response was received, but not the expected one
	 * 			- M2M_FTP_SUCCESS								in case of success.
	 */
	FTP_CLIENT_ERROR m2m_ftp_remote_rename(const M2M_SOCKET_BSD_SOCKET socket, String p_fromPath, String p_toPath);


	int waitRegistration(UINT32 timeout);
  
#ifdef __cplusplus
}
#endif  
  
#endif

