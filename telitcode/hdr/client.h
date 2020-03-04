/*************************************************************************
                    5BARz International Incorporated. 

                 (C) Copyright 2017. All Rights Reserved.

Filename: client.h

Description: This header file contains interfaces for functions to process 
MQTT client actions.

*************************************************************************/
#ifndef HDR_CLIENT_H_
#define HDR_CLIENT_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "cloud_user_method.h"
#include "MQTTConnect.h"
#include "m2m_socket_api.h"
#include "m2m_type.h"

/////////////////////////////////////////////
#define ERR_SOCK_TIMEOUT            30
#define SOCK_CONNECTED              200
/////////////////////////////////////////////
#define ERR_PARMS                   -201
#define ERR_SOCK_SELECT_FAILED      -202
#define ERR_SOCK_OTHER              -204
#define ERR_BADHANDLE               -205
#define ERR_SOCK_SEND_FAILED        -206
#define ERR_SOCK_CONNECT_FAILED     -207
#define ERR_SOCK_BAD_TIMEOUT        -208
#define ERR_SOCK_RECV_FAILED        -209
#define ERR_SOCK_SHUTDOWN           -210
#define ERR_SOCK_SOCKET_FAILED      -211
#define ERR_SOCK_SETSOCKOPT_FAILED  -212
#define ERR_SOCK_HOSTNOTFOUND       -213
#define ERR_SOCK_FNCTL_FAILED       -214
#define ERR_SOCK_GET_IP_ERROR       -215
#define ERR_SOCK_UNBLOCKING_ERROR   -216

#define MAX_PACKET_ID               65535
#define MAX_MESSAGE_HANDLERS        5
enum QoS { QOS0, QOS1, QOS2 };
typedef struct MQTTMessage MQTTMessage;

typedef struct MessageData MessageData;

struct MQTTMessage
{
  enum QoS qos;
  char retained;
  char dup;
  unsigned short id;
  void *payload;
  size_t payloadlen;
};

struct MessageData
{
  MQTTMessage* message;
  MQTTString* topicName;
};

typedef struct Network Network;


struct Network
{
	INT32 socketFD;
};

int waitTimeout;

typedef struct Client Client;

struct Client {
  unsigned int next_packetid;
  unsigned int command_timeout_ms;
  size_t buf_size, readbuf_size;
  unsigned char *buf;
  unsigned char *readbuf;
  unsigned int keepAliveInterval;
  Network* ipstack;
  INT32 socketFD;
  M2M_T_TIMER_HANDLE cycle_timer;
  M2M_T_TIMER_HANDLE png_timer;
  M2M_T_TIMER_HANDLE waiting;
};

#define DefaultClient {0, 0, 0, 0, NULL, NULL, 0, 0, 0}


/////////////////////////////////////////////////////////////////////////////
// Function: MQTTPublish()
//
// Parameters: pointer to a struct Client, topic name and message
//
// Return: int -> response enum type
//
// Description: This function Publishes the data from the device to RMS and waits for ack as per the QOS0
//
/////////////////////////////////////////////////////////////////////////////
response_t MQTTPublish(Client* client, const char* topicName, MQTTMessage* message);


/////////////////////////////////////////////////////////////////////////////
// Function: mqtt_publish_response()
//
// Parameters: none
//
// Return: int -> response enum type
//
// Description: This function Publishes the data from the device to RMS.
//
/////////////////////////////////////////////////////////////////////////////
response_t mqtt_publish_response(char *buf, int len);


/////////////////////////////////////////////////////////////////////////////
// Function: MQTT_Subscribe()
//
// Parameters: pointer to a struct Client, topic to be subscribed and the QOS level
//
// Return: int -> response enum type
//
// Description: This function sends the subscribe packet to the broker and waits for the SUBACK
//
/////////////////////////////////////////////////////////////////////////////
response_t MQTT_Subscribe(Client* client, char* topicFilter, enum QoS qos );


/////////////////////////////////////////////////////////////////////////////
// Function: MQTT_ping()
//
// Parameters: pointer to a struct Client.
//
// Return: int -> response enum type
//
// Description: This function pings the broker to keep the connection alive for every 30 seconds
//
/////////////////////////////////////////////////////////////////////////////
response_t MQTT_ping(Client* client);


/////////////////////////////////////////////////////////////////////////////
// Function: MQTTDisconnect()
//
// Parameters: pointer to a struct Client.
//
// Return: int -> response enum type
//
// Description: This function disconnects from the RMS
//
/////////////////////////////////////////////////////////////////////////////
response_t MQTTDisconnect(Client* client);


/////////////////////////////////////////////////////////////////////////////
// Function: MQTT_Connect()
//
// Parameters: pointer to a struct Network and connection options.
//
// Return: int -> response enum type
//
// Description: This function connects to the broker as per the connect options and waits for the CONNACK packet.
//
/////////////////////////////////////////////////////////////////////////////
INT32 ConnectNetwork(Network* network, const char* p_serverName,const int serverPort);


/////////////////////////////////////////////////////////////////////////////
// Function: MQTTClient()
//
// Parameters: pointer to a struct Client.
//
// Return: int -> response enum type
//
// Description: This function initializes the MQTT client.
//
/////////////////////////////////////////////////////////////////////////////
void 	MQTTClient(Client*, Network*, unsigned int, unsigned char*, size_t, unsigned char*, size_t);


/////////////////////////////////////////////////////////////////////////////
// Function: MQTT_Connect()
//
// Parameters: pointer to a struct Client and the connection options.
//
// Return: int -> response enum type
//
// Description: This function connects to the broker as per the connect options and waits for the CONNACK packet.
//
/////////////////////////////////////////////////////////////////////////////
response_t MQTT_Connect(Client* client, MQTTPacket_connectData* options);


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
INT32  	transport_getdata(unsigned char* readbuf, int readbuf_size);


/////////////////////////////////////////////////////////////////////////////
// Function: sendPacket()
//
// Return: int -> response enum type
//
// Description: This function send the MQTT packet through the socket connected.
//
/////////////////////////////////////////////////////////////////////////////
response_t sendPacket(int len);


/////////////////////////////////////////////////////////////////////////////
// Function: waitfor()
//
// Parameters: pointer to a struct Client ,packet_type
//
// Return: int -> response enum type
//
// Description: This function waits for the return response from the RMS server
//
/////////////////////////////////////////////////////////////////////////////
response_t waitfor(Client* client, int packet_type);


/////////////////////////////////////////////////////////////////////////////
// Function: cycle()
//
// Parameters: pointer to a struct Client
//
// Return: int -> response enum type
//
// Description: This function checks if the device got data from RMS
//
/////////////////////////////////////////////////////////////////////////////
response_t cycle(Client* client);


#ifdef __cplusplus
}
#endif

#endif /* HDR_CLIENT_H_ */
