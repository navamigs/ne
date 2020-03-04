/*************************************************************************
                    5BARz International Incorporated. 

                 (C) Copyright 2017. All Rights Reserved.

Filename: client.c

Description: This file contains functions to process MQTT client actions.

*************************************************************************/
/*=======================================================================
                            INCLUDE FILES
=========================================================================*/


#include"client.h"
#include "m2m_cloud_defines.h"
#include "paho_task.h"
#include "MQTTConnect.h"
#include <sys/time.h>
#include "cloud_user_method.h"
#include "m2m_cloud_api.h"
#include "m2m_socket_api.h"
#include "m2m_type.h"
#include "m2m_timer_api.h"
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include "sms.h"
#include "utilities.h"

/* global variables ********************************************************* */
M2M_T_OS_LOCK     PDPLock;
M2M_CLOUD_GLOBALS globals;

INT32 getNextPacketId(Client *client);
/////////////////////////////////////////////////////////////////////////////
// Function: cycle()
//
// Parameters: pointer to a struct Client
//
// Return: response_t -> response enum type
//
// Description: This function checks if the device got any data from RMS
//
/////////////////////////////////////////////////////////////////////////////

response_t cycle(Client* client)
{

   MQTTString topicName;
   MQTTString message;
   MQTTMessage msg;
   msg_queue_t    t1_msg;
   unsigned short packet_type ;
   int            len = 0;
   response_t rc = RSP_SUCCESS;
   packet_type= MQTTPacket_read(client->readbuf, client->readbuf_size, transport_getdata);

    switch (packet_type)
    {
        case CONNACK:
        	break;
        case PUBACK:
        	break;
        case SUBACK:
        	break;
        case PUBLISH:
        {
            if (MQTTDeserialize_publish((unsigned char*)&msg.dup, (int*)&msg.qos, (unsigned char*)&msg.retained, (unsigned short*)&msg.id, &topicName,
            	(unsigned char**)&msg.payload, (int*)&msg.payloadlen, client->readbuf, client->readbuf_size) != 1)

            dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"DATA CAME BY MQTT___msg.payloadlen:%d",msg.payloadlen);

            dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"DATA CAME BY MQTT ___ msg.payload:%s",msg.payload);
            
            t1_msg.msg = (char *)m2m_os_mem_alloc( SMS_MAX_LENGTH ); // Msg length has been increased to receive of FTP msg.

            if( t1_msg.msg && msg.payloadlen <= SMS_MAX_LENGTH )
            {
				  t1_msg.cmd = MSQ_QUEUE_CMD_MQTT_MSG;
				  t1_msg.payload1 = msg.payloadlen;
				  memset( t1_msg.msg, 0, sizeof(t1_msg.msg));
				  strncpy( t1_msg.msg, msg.payload, msg.payloadlen );
				  PushToMsgQueue( t1_msg );
				  rc = packet_type;
            }
            else
            {
			  // Jinoj: need code here to message the MQTT broker of an
              // internal error, and cannot process the original message.
              // send error code: RSP_GENERAL_ERROR back to MQTT broker
              // Rsp: Any junk message may end up here for which we don't know the message ID correctly. 
			  // Should we send message to RMS and confuse it or just drop the message the way we do in case of SMS?
            }
            return rc;

            if (msg.qos != QOS0)
            {
                if (msg.qos == QOS1)
                    len = MQTTSerialize_ack(client->buf, client->buf_size, PUBACK, 0, msg.id);
                else if (msg.qos == QOS2)
                    len = MQTTSerialize_ack(client->buf, client->buf_size, PUBREC, 0, msg.id);
                if (len <= 0)
                    rc = RSP_MQTT_FAILURE;
                   else
                      rc = sendPacket(len);
                if (rc == RSP_MQTT_FAILURE)
                	 return rc;
            }
            break;
        }
        case PUBREC:
        {
            unsigned short mypacketid;
            unsigned char dup, type;
            if (MQTTDeserialize_ack(&type, &dup, &mypacketid,client->readbuf, client->readbuf_size) != 1)
                rc = RSP_MQTT_FAILURE;
            else if ((len = MQTTSerialize_ack(client->buf, client->buf_size, PUBREL, 0, mypacketid)) <= 0)
                rc = RSP_MQTT_FAILURE;
            else if ((rc = sendPacket(len)) != RSP_SUCCESS) // send the PUBREL packet
                rc = RSP_MQTT_FAILURE;
            if (rc == RSP_MQTT_FAILURE)
            	 return rc;
            break;
        }
        case PUBCOMP:
            break;
        case PINGRESP:
        {
            break;
        }
        default:
        {
        	rc = RSP_MQTT_FAILURE;
        	break;
        }
    }
    if (rc == RSP_SUCCESS)rc = packet_type;
    return rc;
}


/////////////////////////////////////////////////////////////////////////////
// Function: MQTT_Subscribe()
//
// Parameters: pointer to a struct Client, topic to be subscribed and the
//             QOS level
//
// Return: response_t -> response enum type
//
// Description: This function sends the subscribe packet to the broker 
// and waits for the SUBACK
//
/////////////////////////////////////////////////////////////////////////////

response_t MQTT_Subscribe(Client* client, char* topicFilter, enum QoS qos )
{
	response_t rc = RSP_MQTT_FAILURE;
    int len = 0;
    MQTTString topic = MQTTString_initializer;
    topic.cstring = (char *)topicFilter;

    m2m_timer_start(client->png_timer, 100);

    len = MQTTSerialize_subscribe(client->buf, client->buf_size, 0, getNextPacketId(client), 1, &topic, (int*)&qos);
    if (len <= 0)
    	return rc;
    if ((rc = sendPacket(len)) != RSP_SUCCESS) // send the subscribe packet
    	return rc;
    if (waitfor(client, SUBACK) == SUBACK)      // wait for suback
    {
        int count = 0, grantedQoS = -1;
        unsigned short mypacketid;
        if (MQTTDeserialize_suback(&mypacketid, 1, &count, &grantedQoS, client->readbuf, client->readbuf_size) == 1)

        if (grantedQoS != 0)
        {
        	dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"granted qos != 0, %d", grantedQoS);
        	return rc;
        }
    }
    else
        rc = RSP_MQTT_FAILURE;
    return rc;
}

INT32 getNextPacketId(Client *client)
{
  return client->next_packetid = (client->next_packetid == MAX_PACKET_ID) ? 1 : client->next_packetid + 1;
}

/////////////////////////////////////////////////////////////////////////////
// Function: waitfor()
//
// Parameters: pointer to a struct Client ,packet_type
//
// Return: response_t -> response enum type
//
// Description: This function waits for the return response from the RMS server
//
/////////////////////////////////////////////////////////////////////////////

response_t waitfor(Client* client, int packet_type)
{
	response_t rc = RSP_MQTT_FAILURE;
    waitTimeout=0;
    m2m_timer_start(client->waiting,6000); // it will wait for 6 sec
    while(1)
	{
		if (waitTimeout == 1)
		{
			dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_FALSE, "Unable to connect to the broker Timeout!.");
			m2m_timer_stop( client->waiting );/*stop the timer */
			return M2M_CLOUD_TIMEOUT;
		}
		else
		{
			if ((rc = cycle(client)) == packet_type)return rc;
	        dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE," NOT GOT REQUIRED PACKET: %d",rc);
		}
	}
    return rc;
}


/////////////////////////////////////////////////////////////////////////////
// Function: MQTT_Connect()
//
// Parameters: pointer to a struct Client ,pointer to a struct Network and the connection options.
//
// Return: response_t -> response enum type
//
// Description: This function connects to the broker as per the connect options and waits for the CONNACK packet.
//
/////////////////////////////////////////////////////////////////////////////


response_t MQTT_Connect(Client* client, MQTTPacket_connectData* options)
{

	response_t rc = RSP_MQTT_FAILURE;
    int len = 0;

	if ((len = MQTTSerialize_connect(client->buf, client->buf_size, options)) <= 0) return rc;  // send the connect packet

	if ((rc = sendPacket(len)) != RSP_SUCCESS) return rc;

	NE_debug("CONNECTING");

	/* wait for connack */
	if (waitfor(client, CONNACK) == CONNACK)
	{
		unsigned char sessionPresent, connack_rc;

		if (MQTTDeserialize_connack(&sessionPresent, &connack_rc, client->readbuf, client->readbuf_size) == 1 || connack_rc == 0)
		{
			rc = connack_rc;
		}
		else
		{
		  dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Unable to connect, return code %d", connack_rc);
		  rc = RSP_MQTT_FAILURE;
		}
	}
	else
	{
		 rc = RSP_MQTT_FAILURE;
	}
    return rc;
}

/////////////////////////////////////////////////////////////////////////////
// Function: sendPacket()
//
// Return: response_t -> response enum type
//
// Description: This function send the MQTT packet through the socket connected.
//
/////////////////////////////////////////////////////////////////////////////
response_t sendPacket(int len)
{
	response_t rsp;
	INT32 bytes_sent,res;
	res = m2m_socket_bsd_send_buf_size(network.socketFD);
	if (res > sizeof(client.buf))
	{
	  bytes_sent = m2m_socket_bsd_send(network.socketFD,client.buf,len,0);
	  m2m_os_sleep_ms(1000);
	  if(bytes_sent<0) rsp=RSP_MQTT_FAILURE;
	  else rsp= RSP_SUCCESS;
	}
	return rsp;
}
/////////////////////////////////////////////////////////////////////////////
// Function: MQTT_ping()
//
// Parameters: pointer to a struct Client.
//
// Return: response_t -> response enum type
//
// Description: This function pings the broker to keep the connection alive for every 30 seconds
//
/////////////////////////////////////////////////////////////////////////////

response_t MQTT_ping(Client* client)
{
	response_t rsp = RSP_MQTT_FAILURE,len;
    if ((len = MQTTSerialize_pingreq(client->buf, client->buf_size)) <= 0)
    	return rsp; // send the connect packet
    if ((rsp = sendPacket(len)) != RSP_SUCCESS)
    	return rsp;
	return rsp;
}


/////////////////////////////////////////////////////////////////////////////
// Function: transport_getdata()
//
// Parameters: none
//
// Return: int -> response enum type
//
// Description: This function is a receiver function which gets the data from the socket connected with the broker.
//
/////////////////////////////////////////////////////////////////////////////

INT32 transport_getdata(unsigned char* buf, int buf_size)
{
	UINT32 len;
	int rc = 0;
	rc = m2m_socket_bsd_recv_data_size(network.socketFD, &len);
	if (len >= 0)
	{
		rc= m2m_socket_bsd_recv(network.socketFD,buf, buf_size, 0);
		if (rc ==-1) return rc;
		else return rc;
	}
	else return -1;
}

/////////////////////////////////////////////////////////////////////////////
// Function: MQTTClient()
//
// Parameters: pointer to a struct Client.
//
// Return: none
//
// Description: This function initializes the MQTT client.
//
/////////////////////////////////////////////////////////////////////////////
void MQTTClient( Client* client, Network* network, unsigned int command_timeout_ms,
                 unsigned char* buf, size_t buf_size, unsigned char* readbuf, 
                 size_t readbuf_size )
{

	client->ipstack = network;
	client->command_timeout_ms = command_timeout_ms;
	client->buf = buf;
	client->buf_size = buf_size;
	client->readbuf = readbuf;
	client->readbuf_size = readbuf_size;
}

/////////////////////////////////////////////////////////////////////////////
// Function: MQTTPublish()
//
// Parameters: pointer to a struct Client, topic name and message
//
//  Return: response_t -> response enum type
//
// Description: This function Publishes the data from the device to RMS and waits for ack as per the QOS
//
/////////////////////////////////////////////////////////////////////////////

response_t MQTTPublish(Client* client, const char* topicName, MQTTMessage* message)
{
	response_t rc = RSP_MQTT_FAILURE;
    MQTTString topic = MQTTString_initializer;
    topic.cstring = (char *)topicName;
    int len = 0;

    if (message->qos == QOS1 || message->qos == QOS2)
        message->id = getNextPacketId(client);

    len = MQTTSerialize_publish(client->buf, client->buf_size, 0, message->qos, message->retained, message->id,
              topic, (unsigned char*)message->payload, message->payloadlen);
    if (len <= 0) return rc;

    if ((rc = sendPacket( len)) != RSP_SUCCESS)return rc; // send the subscribe packet

    if (message->qos == QOS1)
    {
        if (waitfor(client, PUBACK) == PUBACK)
        {
            unsigned short mypacketid;
            unsigned char dup, type;
            if (MQTTDeserialize_ack(&type, &dup, &mypacketid, client->readbuf,client->readbuf_size) != 1)
            rc = RSP_MQTT_FAILURE;
        }
        else
            rc = RSP_MQTT_FAILURE;
    }
    else if (message->qos == QOS2)
    {
        if (waitfor(client, PUBCOMP) == PUBCOMP)
        {
            unsigned short mypacketid;
            unsigned char dup, type;
            if (MQTTDeserialize_ack(&type, &dup, &mypacketid, client->readbuf, client->readbuf_size) != 1)
            rc = RSP_MQTT_FAILURE;
        }
        else
            rc = RSP_MQTT_FAILURE;
    }
    return rc;
}

/////////////////////////////////////////////////////////////////////////////
// Function: mqtt_publish_response()
//
// Parameters: none
//
//  Return: response_t -> response enum type
//
// Description: This function Publishes the data from the device to RMS.
//
/////////////////////////////////////////////////////////////////////////////
response_t mqtt_publish_response(char *buf, int len)
{
	int res;
	response_t rsp;
    MQTTMessage Packet;
    Packet.id = 1;
    Packet.qos = 0;
    Packet.payload = buf;
    Packet.payloadlen = len;
	res = MQTTPublish(&client,TOPIC_PUBLISH, &Packet);
	if( res == 0 )
	{
	 dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"Data published:%s",buf );
	 rsp = RSP_SUCCESS ;
    }
	else rsp =RSP_MQTT_PUBLISH_FAIL ;

    return rsp;
 }
/////////////////////////////////////////////////////////////////////////////
// Function: MQTTDisconnect()
//
// Parameters: pointer to a struct Client.
//
//  Return: response_t -> response enum type
//
// Description: This function disconnects from the RMS
//
/////////////////////////////////////////////////////////////////////////////

response_t MQTTDisconnect(Client* client)
{
    int rc = RSP_MQTT_FAILURE;
    dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE," Disconnecting MQTT ......");
    int len = MQTTSerialize_disconnect(client->buf, client->buf_size);
    // send the disconnect packet
    if (len > 0)
    {
	 rc = sendPacket( len);
	 if(rc == RSP_SUCCESS)
	 globals.MQTT_state = MQTT_DISCONNECTED;
    }
    return rc;
}

/////////////////////////////////////////////////////////////////////////////
// Function: MQTT_Connect()
//
// Parameters: pointer to a struct Client ,pointer to a struct Network and the connection options.
//
// Return: int -> response enum type
//
// Description: This function connects to the broker as per the connect options and waits for the CONNACK packet.
//
/////////////////////////////////////////////////////////////////////////////

INT32 ConnectNetwork(Network* network, const char* p_serverName,const int serverPort)
{

	UINT32 serverAddr;
	INT32 count = 0,sockState,nonblocking;
	int i,res,PDP_Active;

	struct M2M_SOCKET_BSD_SOCKADDR_IN serverSockAddr;

	memset(&serverSockAddr, 0, sizeof(struct M2M_SOCKET_BSD_SOCKADDR_IN));

	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_socket_connection_create():p_serverName:%s",p_serverName);

	if (!network || ! p_serverName){
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_socket_connection_create(): input strings not initialized");
		return ERR_BADHANDLE;
	}


	/* determine server IP from server name (or dotted IP as a string) */
	if ((serverAddr = m2m_socket_bsd_get_host_by_name(p_serverName)) == 0) {
		/* Address was not an URL, try the dotted IP */
		if ((serverAddr = m2m_socket_bsd_inet_addr(p_serverName)) == 0) {
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_socket_connection_create(). Error: %d", m2m_socket_errno());
			return ERR_SOCK_GET_IP_ERROR;  //we failed to get the IP address. abort.
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
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"m2m_connection_create(): serverName='%s', serverAddr='%s', port: %d", p_serverName, m2m_socket_bsd_addr_str(serverSockAddr.sin_addr.s_addr), serverPort);
	#endif

	/* Create socket */

	if( M2M_SOCKET_BSD_INVALID_SOCKET ==
			(network->socketFD = m2m_socket_bsd_socket( serverSockAddr.sin_family,
												M2M_SOCKET_BSD_SOCK_STREAM,
												M2M_SOCKET_BSD_IPPROTO_TCP)))
	{
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE," create socket");
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE," Error: %d", m2m_socket_errno());
		return ERR_SOCK_SOCKET_FAILED;
	}
	dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE,"n->my_socket Connect network %d ", (network->socketFD ));




	#if BLOCKING
	// connect on socket (blocking mode)
	if (M2M_SOCKET_BSD_INVALID_SOCKET ==
			m2m_socket_bsd_connect(*p_socket,
					(M2M_SOCKET_BSD_SOCKADDR *)&serverSockAddr, sizeof(struct M2M_SOCKET_BSD_SOCKADDR_IN))) {
		LOG_ERROR("m2m_socket_connection_create(): connect socket. Error: %d", m2m_socket_errno());
		m2m_socket_bsd_close(p_socket);
		return M2M_FTP_SOCK_CREATE_ERROR;
	}
	return true;
	#else

	#ifndef NODEBUG
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"Setting socket as non blocking...");
	#endif
	nonblocking = 1;
	if (m2m_socket_bsd_ioctl(network->socketFD, M2M_SOCKET_BSD_FIONBIO, &nonblocking) != 0)
	{
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE," Error setting socket in non blocking mode!");
	   /* Handle the ioctl error. */
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE," Error: %d", m2m_socket_errno());
	   return ERR_SOCK_UNBLOCKING_ERROR;
	}
	#ifndef NODEBUG
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE," Done setting in non blocking and started Connecting...");
	#endif
	if( m2m_pdp_get_status() == M2M_PDP_STATE_ACTIVE )
	  {
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"PDP is active");
	if (m2m_socket_bsd_connect(network->socketFD, (M2M_SOCKET_BSD_SOCKADDR *)&serverSockAddr,
										sizeof(struct M2M_SOCKET_BSD_SOCKADDR_IN))!= 0)
	{
	while (count < ERR_SOCK_TIMEOUT )
	{
		dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE," Connecting Socket_count<60:%d",count);
		sockState = m2m_socket_bsd_socket_state(network->socketFD);

		/* available from FW 12.00.007-B03 */
		if (sockState == M2M_SOCKET_STATE_CONNECTED) {
	#ifndef NODEBUG
			dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE," Socket connected.");
	#endif
			//LOG_INFO("Socket connected");
			return SOCK_CONNECTED;
		}
		count += 1;
		m2m_os_sleep_ms(1000);
	}
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE,"connecting the socket.");
	dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE," Error: %d", m2m_socket_errno());
	// retry of connecting
	return ERR_SOCK_CONNECT_FAILED;
	}
	else return SOCK_CONNECTED;
	#endif
	}
	else return (ERR_SOCK_CONNECT_FAILED);
}
