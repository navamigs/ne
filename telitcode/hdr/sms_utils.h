/*
 * sms_utils.h
 *
 *  Created on: 16/set/2016
 *      Author: FabioPi
 */

#ifndef HDR_SMS_UTILS_H_
#define HDR_SMS_UTILS_H_

#ifdef __cplusplus
  extern "C" {
#endif

	#include "m2m_type.h"

	#define GSM338_REPLACE_CHAR 0xFF
	#define STRING_TERMINATOR 0x00
	#define BMASK			((UINT16)(0x00FF))
	#define DEFAULTMASK		0x7F
	#define ESCAPEEXT		0x1B /* escape extension of the 7 bit default alphabet */

	typedef UINT16 UNICODE_CHAR;

	typedef enum
	{
		CODING_8_BIT_CL0 = 0,  	/*8 bit coding, class 0 immediate display*/
		CODING_8_BIT_CL1, 		/*8 bit coding, class 1 ME (Mobile Equipment) specific*/
		CODING_8_BIT_CL2, 		/*8 bit coding, class 2 SIM specific*/
		CODING_8_BIT_CL3, 		/*8 bit coding, class 3 TE (Terminate Equipment) specific*/
		CODING_7_BIT			/*default 7 bit coding*/
	} CODING_SCHEME;

	UINT8* FromUnicodeTo0338Spec( UINT8* des, UNICODE_CHAR*  src, UINT16 len );
	UNICODE_CHAR* From0338ToUnicodeSpec(UNICODE_CHAR* des, UINT8* src, UINT16 len );
	CODING_SCHEME parseSchemeFromDCS(CHAR dcs);
	char* FromRAWToASCII(char* des, char *src,  UINT16 len );

#ifdef __cplusplus
  }
#endif

#endif /* HDR_SMS_UTILS_H_ */
