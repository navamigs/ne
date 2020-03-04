/*************************************************************************
                    5BARz International Incorporated.

                 (C) Copyright 2017. All Rights Reserved.

Filename: MQTTSubscribe.h

Description: This header file contains interfaces for functions to process
MQTT client subscribe and unsubscribe actions from/to the server.

*************************************************************************/
/*******************************************************************************
 * Copyright (c) 2014 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *    Xiang Rong - 442039 Add makefile to Embedded C client
 *******************************************************************************/
#ifndef HDR_MQTTSUBSCRIBE_H_
#define HDR_MQTTSUBSCRIBE_H_

#ifdef __cplusplus
 extern "C" {
#endif

#if !defined(DLLImport)
  #define DLLImport 
#endif
#if !defined(DLLExport)
  #define DLLExport
#endif

DLLExport int MQTTSerialize_subscribe(unsigned char* buf, int buflen, unsigned char dup, unsigned short packetid,
		int count, MQTTString topicFilters[], int requestedQoSs[]);

DLLExport int MQTTDeserialize_subscribe(unsigned char* dup, unsigned short* packetid,
		int maxcount, int* count, MQTTString topicFilters[], int requestedQoSs[], unsigned char* buf, int len);

DLLExport int MQTTSerialize_suback(unsigned char* buf, int buflen, unsigned short packetid, int count, int* grantedQoSs);

DLLExport int MQTTDeserialize_suback(unsigned short* packetid, int maxcount, int* count, int grantedQoSs[], unsigned char* buf, int len);

DLLExport int MQTTSerialize_unsubscribe(unsigned char* buf, int buflen, unsigned char dup, unsigned short packetid,
		int count, MQTTString topicFilters[]);

DLLExport int MQTTDeserialize_unsubscribe(unsigned char* dup, unsigned short* packetid, int max_count, int* count, MQTTString topicFilters[],
		unsigned char* buf, int len);

DLLExport int MQTTSerialize_unsuback(unsigned char* buf, int buflen, unsigned short packetid);

DLLExport int MQTTDeserialize_unsuback(unsigned short* packetid, unsigned char* buf, int len);

#ifdef __cplusplus
}
#endif

#endif /* MQTTSUBSCRIBE_H_ */
