/*************************************************************************
                    5BARz International Incorporated.

                 (C) Copyright 2017. All Rights Reserved.

Filename: m2m_cloud_defines.h

Description: This header file contains definitions, structs, global 
variables, and interfaces to functions used throughout the TCA program

*************************************************************************/
#ifndef FEATURE_5BARZ
#define FEATURE_5BARZ

// SMS pre-fix used for test purposes only in India, not for commercial release.
#define SMS_PREFIX 

// Select One Cellalure Provider:
#define INDIA_VODAFONE   
//#define INDIA_AIRTEL
//#define USA_TMOBILE //debug: test version

#ifdef USA_TMOBILE
  #ifdef INDIA_VODAFONE
    #error only 1 carrier can be defined
  #endif
  #ifdef INDIA_AIRTEL
    #error only 1 carrier can be defined
  #endif
  #ifdef SMS_PREFIX
    #error Cannot compile SMS Pre-fix with T-mobile
  #endif
#endif

#ifdef INDIA_VODAFONE
  #ifdef USA_TMOBILE
    #error only 1 carrier can be defined
  #endif
  #ifdef INDIA_AIRTEL
    #error only 1 carrier can be defined
  #endif
#endif

#ifdef INDIA_AIRTEL
  #ifdef USA_TMOBILE
    #error only 1 carrier can be defined
  #endif
  #ifdef INDIA_VODAFONE
    #error only 1 carrier can be defined
  #endif
#endif

#endif  /* FEATURE_5BARZ */


#ifndef M2M_CLOUD_TYPEDEFS_H_
#define M2M_CLOUD_TYPEDEFS_H_
	#include "m2m_type.h"
	#include "m2m_clock_api.h"
	#include "m2m_cloud_types.h"
#ifdef FEATURE_5BARZ
    #include <stdbool.h>

    typedef enum 
    {
      RSP_SUCCESS                   = 0,  // Success
      RSP_NE_DISABLED               = 1,  // Can't perform while NE is disabled
      RSP_NE_COMM_ERROR             = 2,  // Communication Error on Serial Link
      RSP_WRONG_STATE               = 3,  // Can't perform in current state (either NE or Telit)
      RSP_PARAMETER_OUT_OF_RANGE    = 4,  // Input Parameter out of range
      RSP_NOT_SUPPORTED             = 5,  // Function not supported by this software version
      RSP_GENERAL_ERROR             = 6,  // General Error that does not apply to any other code
      RSP_OTA_MSG_FORMAT_ERROR      = 7,  // Parsing Error with JSON message
      RSP_SMS_WRONG_IMEI            = 8,  // IMEI in SMS does not match module's IMEI
      RSP_SMS_CRC_FAIL              = 9,  // SMS CRC calculation failed
      RSP_SMS_FORMAT_ERROR          = 10, // SMS format/syntax/length error
      RSP_SMS_DATA_PAYLOAD_ERROR    = 11, // The data payload is not formatted properly
      RSP_SMS_MO_FAILED             = 12, // Mobile Originated SMS failed
      RSP_NE_READ_FAILED            = 13, // NE could not read the item requested
      RSP_NE_WRITE_FAILED           = 14, // NE could not write the item requested
      RSP_NE_ALREADY_DONE           = 15, // NE could not perform this operation, as it is already done
      RSP_CFG_FILE_NOT_PRESENT      = 16, // TCA could not find the Config File in file system
      RSP_CFG_FILE_FORMAT_ERROR     = 17, // Invalid format in the Config File
      RSP_PDP_SESSION_FAILURE       = 18, // PDP session failed to connect, or no PDP available
      RSP_LOCATION_FIX_FAILURE      = 19, // Unable to return a location fix
      RSP_FTP_SERVER_CONNECT_FAIL   = 20, // FTP connection to server fail
      RSP_FTP_SOCKET_CONNECT_FAIL   = 21, // FTP Checking User/Pwd fail
      RSP_FTP_FILE_DOWNLOAD_FAIL    = 22, // FTP file download failure
      RSP_FTP_FILE_SIZE_UNMATCHED   = 23, // Ftp download file mismatch
      RSP_FTP_FILE_MD5_UNMATCHED    = 24, // FTP download checksum mismatch
      RSP_FTP_DOWNLOAD_IN_PROGRESS  = 25, // FTP download is in progress
      RSP_NO_CELL_COVERAGE          = 26, // No Cell Signal Available to perform action
      RSP_NO_12_VOLTS               = 27, // Lost 12v volt power, cannot perform task
      RSP_LOGGING_IN_PROCESS        = 28, // Logging in process (may not be able to perform requested task)
      RSP_DEVICE_NOT_ACTIVATED      = 29, // Device not activated unable to process.
      
      RSP_PDP_SESSION_SUCCESS       = 30, // PDP Session started
      RSP_FTP_SOCKET_SUCCESS        = 31, // FTP TCP socket established
      RSP_FTP_LOGIN_SUCCESS         = 32, // FTP Login successful      
      RSP_FTP_FILE_DLOAD_SUCCESS    = 33, // FTP File Download successful         
      RSP_FTP_FILE_UPLOAD_SUCCESS   = 34, // FTP File Upload successful
      RSP_FTP_REBOOT_AND_FINISH     = 35, // FTP TCA update successful, starting re-boot.
      RSP_FTP_FILE_UPLOAD_FAILED    = 36, // FTP Log File failed to upload
      RSP_FTP_REMOTE_FILE_NOT_FOUND = 37, // FTP File Requested not found in server
      RSP_FTP_NEW_CURR_VER_SAME     = 38, // Current version and new version are same
      
      RSP_FS_CANNOT_OPEN_FILE       = 40, // File System: cannot open the file
      RSP_FS_CANNOT_WRITE_FILE      = 41, // File System: cannot write to file
      RSP_FS_CANNOT_DELETE_FILE     = 42, // File System: cannot delete the file
      RSP_FS_CANNOT_FIND_FILE       = 43, // File System: file does not exist in file system
      
      RSP_MQTT_SERV_CONNECT_FAIL    = 44, // Server_connect fail user/port connection wrong
      RSP_MQTT_CONNECT_FAIL         = 45, // Unable to connect to MQTT server
      RSP_MQTT_SUBSCRIBE_FAIL       = 46, // Unable to subscribe to server.
      RSP_MQTT_PUBLISH_FAIL         = 47, // Unable to publish to server
      RSP_MQTT_SERV_CONNECT_SUCCESS = 48, // MQTT server connection success
      RSP_MQTT_CONNECT_SUCCESS      = 49, // MQTT connection success
      RSP_MQTT_SUBSCRIBE_SUCCESS    = 50, // MQTT subscribe success
      RSP_MQTT_BROKER_PING_FAIL     = 51, // MQTT disconnected after connecting to the broker.
      RSP_MQTT_IN_PROGRESS          = 52, // MQTT Request is in progress
      RSP_MQTT_FAILURE              = 53, // MQTT general failure

    } response_t;

    typedef enum
    {
      CONN_METHOD_MQTT_ONLY = 0,
      CONN_METHOD_SMS_ONLY  = 1,
      CONN_METHOD_MAX_VALUE = 2  // Always Last
      //CONN_METHOD_MQTT_SMS  = 2,  not supported yet
      //CONN_METHOD_SMS_MQTT  = 3,  not supported yet
      //CONN_METHOD_MQTT_TRIG = 4,  not supported yet
    } conn_meth_t;

    typedef enum
    {
      MSG_QUEUE_CMD_NONE          = 0,
      MSG_QUEUE_CMD_DISCONNECT    = 1,  // Disconnect MQTT & PPP
      MSG_QUEUE_CMD_MT_SMS        = 2,  // Process new MT SMS
      MSQ_QUEUE_CMD_PERF_MSG      = 3,  // Send out Performance Interval Message
      MSQ_QUEUE_CMD_BASIC_MSG1    = 4,  // Send out Boot-up Perf Basic Message 1
      MSQ_QUEUE_CMD_BASIC_MSG2    = 5,  // Send out Boot-up Perf Basic Message 2
	  MSQ_QUEUE_CMD_BASIC_MSG3    = 6,  // Send out Boot-up Perf Basic Message 3
      MSQ_QUEUE_CMD_12V_LOST      = 7,  // 12v power is lost
      MSQ_QUEUE_CMD_LOC_MSG       = 8,  // Send out location Interval Message
      MSQ_QUEUE_CMD_FTP_SMS       = 9,  // Send Unsolicited SMS update from the FTP task
      MSQ_QUEUE_CMD_REBOOT_MODULE = 10,  // Instruct the NE to re-boot the Module
      MSQ_QUEUE_CMD_FTP_UPLOAD    = 11, // FTP Upload of OTA Log file finished, payload indicates success or not
      MSQ_QUEUE_CMD_MQTT          = 12, // Mqtt Sending msg
      MSQ_QUEUE_CMD_MQTT_MSG      = 13, // Notify task 1 of a new MQTT message for sms parsing
      MSG_QUEUE_CMD_LOST_SERVICE  = 14, // Notify the NE that GSM service is lost
	  MSQ_QUEUE_POWERED_UP        = 15  // NE powered-up
    } msg_queue_cmd_t;
    
    typedef enum {
      INIT_SUCCESS,      // Initialization success
      INIT_FAIL_NE,      // No serial communication with NE
      INIT_FAIL_SIM,     // No SIM card detected
      INIT_FAIL_CONFIG,  // Config file failed to load
      INIT_FAIL_SOFTWARE // Internal Software failure
    } init_result_t;    

    typedef struct
    {
      char latitude[20];
      char longitude[20];
      int  accuracy;
    } dev_loc_t;
    
    typedef struct
    {
      msg_queue_cmd_t cmd;
      INT32           payload1;
      INT32           payload2;
      INT32           payload3;
      char            *msg;      
      //type          payloadX;  Add new Data payloads as needed
    } msg_queue_t;

    typedef struct
    {
      int         last;
      msg_queue_t queue[20];
    } msg_queue_struct_t;
#endif // FEATURE_5BARZ 

	typedef struct
	{
		int msgType;
		int msgID;
		int msgLen;
	} M2M_CLOUD_DWRING_STRUCT;


	typedef struct{
		int last;
		M2M_CLOUD_DWRING_STRUCT ringQueue[20];
	} M2M_CLOUD_DWRING_QUEUE;


	typedef struct {
	    double 			latitude;
	    double 			longitude;
	    float 			cog; //course over ground
	    float       altitude;
	    float 			speed;
	    unsigned short 	nsat;
	    short 			fix;
	    struct M2M_T_RTC_TIMEVAL time;

	} DW_GPSPositionData;



	typedef enum
	{
		M2M_CLOUD_LEAVE=20,
		M2M_CLOUD_AUTORETRIEVE
	} M2M_CLOUD_DWRING_ACTION;

	typedef enum
	{
		M2M_CLOUD_DWRING_TYPE,
		M2M_CLOUD_DWDATA_TYPE,
#ifndef FEATURE_5BARZ        
		M2M_CLOUD_DWRDATA_TYPE
#else //FEATURE_5BARZ        
		M2M_CLOUD_DWRDATA_TYPE,
    M2M_CLOUD_AT_CMD_CGSN_TYPE,
    M2M_CLOUD_AT_CMD_CGMM_TYPE,
    M2M_CLOUD_AT_CMD_CGMR_TYPE,
    M2M_CLOUD_AT_CMD_ADC_TYPE,
    M2M_CLOUD_AT_CMD_CNUM_TYPE,
    M2M_CLOUD_AT_CMD_MOB_TYPE,
    M2M_CLOUD_AT_CMD_LOC_TYPE,
#endif // FEATURE_5BARZ        
	} M2M_CLOUD_DWMSG_TYPE;



	/* ====DEFINES ==== */

	#define DW_RAW_BUF_LEN    1500

	#define DW_FIELD_LEN 	  512
	#define DW_FIELDS_NUM	  32

	#define M2M_CLOUD_AT_RESPONSE_TASK_PRIORITY 8
	#define M2M_CLOUD_PARSE_MESSAGE_TASK_PRIORITY 9
	#define M2M_CLOUD_METHODS_TASK_PRIORITY 10


#ifdef FEATURE_5BARZ
#define SMS_MAX_LENGTH       140 // Using 8-bit PDU encoding in the future
#define SMS_HDR_IMEI_LEN     15
#define SMS_HDR_CMD_LEN      4
#define SMS_HDR_MSG_ID_LEN   2
#define SMS_HDR_DATA_LEN_LEN 2
#define SMS_HDR_REPLY_ID_LEN 1
#define SMS_CRC_LEN          4 
#define SMS_MT_HDR_LEN       (SMS_HDR_IMEI_LEN + SMS_HDR_CMD_LEN + SMS_HDR_MSG_ID_LEN + SMS_HDR_DATA_LEN_LEN) // 23
#define SMS_MO_HDR_LEN       (SMS_MT_HDR_LEN + 1)                                                             // 24 
#define SMS_MT_MIN_LEN       (SMS_MT_HDR_LEN + SMS_CRC_LEN)                                                   // 27
#define SMS_MT_MAX_DATA_LEN  (SMS_MAX_LENGTH - SMS_MT_HDR_LEN - SMS_CRC_LEN)                                  // 113
#ifndef SMS_PREFIX  
#define SMS_MO_MAX_DATA_LEN  (SMS_MAX_LENGTH - SMS_MO_HDR_LEN - SMS_CRC_LEN)                                  // 112
#else   // Pre-fix On:
#define SMS_MO_MAX_DATA_LEN  (SMS_MAX_LENGTH - SMS_MO_HDR_LEN - SMS_CRC_LEN - SMS_PREFIX_LEN)                 // 109
#endif  
    
#define CFG_FILE_ITEM_LEN       100
#define USERID_PASSWD_LEN       50
  
#define ONE_SECOND              1000
#define TWENTY_SECONDS          20000
#define ONE_MINUTE              60000
#define IMEI_STR_LEN            16
#define FIRMWARE_STR_LEN        50
#define MODEL_STR_LEN           26
#define GENERAL_RETURN_STR_LEN  100
#define APN_STR_LEN             50
#define URL_STR_LEN             CFG_FILE_ITEM_LEN
#define SMS_ADDR_LEN            50
#define PHONE_NUM_LEN           50
#define RMS_ACT_LEN             2
#define MQTTPWD_STR_LEN         50
#define RESPONSE_LEN_MAX        SMS_MO_MAX_DATA_LEN
#define RESPONSE_ARRAY_LEN_MAX  (RESPONSE_LEN_MAX + 3) // 2 bytes for "" chars and 1 null terminate
#define HB_INTVL_LEN            20
#define PERF_INTVL_LEN          20
#define LOC_INTVL_LEN           20  
#define LOG_TIME_MSG_LEN        10
#define APP_VER_LEN             10
#define FTP_STATE_LEN           10
#define NE_VERSION_LEN          22
#define ADC_THRESHOLD_12V       392
#define FTP_MSGID_LEN           20
#define LOST_POWER_WAIT_TIME    10
// Extended Alarm codes from TCA to RMS server, we start at 100
// so we don't conflict with existing Alarm codeS from the NE(0-9).
#define NO_ALARM                0    
#define EXT_ALARM_CODE_NO_12V   100 // Lost 12v input power, TCA will be shutting down soon
#define EXT_ALARM_CODE_NO_COMM  101 // Lost communication with NE over serial link

    
	typedef enum {
    REG_STATE_SEARCHING,
    REG_STATE_CAMPING_HOME,
    REG_STATE_CAMPING_ROAM,
    REG_STATE_DENIED
	} REG_state_t;
    
  typedef enum {
    PPP_CELL_SERVICE  = 0,
    PPP_IDLE          = 1,
    PPP_CONNECTING    = 2,
    PPP_CONNECTED     = 3,
    PPP_DISCONNECTING = 4,
    PPP_DISCONNECTED  = 5,
    PPP_WAITING       = 6,
    PPP_CONNECT_FAIL  = 7,
    PPP_DISCONN_FAIL  = 8
  } PPP_state_t;
      
  typedef enum {
    MQTT_IDLE          = 0,
    MQTT_CONNECTING    = 1,
    MQTT_CONNECTED     = 2,
    MQTT_DISCONNECTING = 3,
    MQTT_DISCONNECTED  = 4,
    MQTT_WAITING       = 5,
    MQTT_CONNECT_SUCC  = 6,
    MQTT_CONNECT_FAIL  = 7,
    MQTT_DISCONN_FAIL  = 8
  } MQTT_state_t;
  
  typedef enum {
    PPP_CONNECT      = 0,
    PPP_DISCONNECT   = 1,
    MQTT_CONNECT 	 = 2,
    MQTT_DISCONNECT  = 3,
} mqtt_session_t;

  typedef enum {
    PWR_STATE_12V_ON,
    PWR_STATE_12V_OFF_WAIT,
    PWR_STATE_12V_OFF_SHUTDOWN
  } power_state_t;
  
        
    typedef struct{
      char        imei[IMEI_STR_LEN];                // Module's IMEI
      char        imei_quote[IMEI_STR_LEN+2];        // Module's IMEI with quotes
      char        model[MODEL_STR_LEN];              // Telit Model Designation
      char        firmware[FIRMWARE_STR_LEN];        // Firmware Version of the Telit module
      
      // Config File Items:
      char        RMS_Activate[RMS_ACT_LEN];         // RMS Activation Status (Y or N)
      conn_meth_t conn_method;                       // Connection method to the RMS server
      char        apn[APN_STR_LEN];                  // Access Point Name for the local Carrier
      char        sms_addr[SMS_ADDR_LEN];            // SMS Address for MO SMS or the SMS short code.
      char        mqttPWD[MQTTPWD_STR_LEN];          // MQTT password used to login
      char        mqttIP[URL_STR_LEN];               // URL or IP addr of the MQTT broker  www or xx.xx.xx.xx
      char        HB_intvl[HB_INTVL_LEN];            // Heart_beat interval parameter - MQTT
      char        perf_intvl[PERF_INTVL_LEN];        // Performance interval in minutes to send periodic performnce data
      char        Loc_intvl[LOC_INTVL_LEN];          // Location interval in minutes to send periodic location data
      char        TCA_ver_cfg[FIRMWARE_STR_LEN];     // TCA software version stored in the Config file.
      char        ftp_state_str[FTP_STATE_LEN];      // FTP status and msgID for TCA download
      char        log_ftp_ip[CFG_FILE_ITEM_LEN];     // IP addr of the FTP server used for log file upload
      char        log_ftp_path[CFG_FILE_ITEM_LEN];   // File Path on the FTP server used for log file upload  
      char        log_ftp_userID[USERID_PASSWD_LEN]; // UserID of the FTP server used for log file upload
      char        log_ftp_passWD[USERID_PASSWD_LEN]; // Password of the FTP server used for log file upload
      char        log_start_time[LOG_TIME_MSG_LEN];  // Start Time & Date for Logging - MMDDHH
      char        log_stop_time[LOG_TIME_MSG_LEN];   // Start Time & Date for Logging - MMDDHH
      char        sw_ftp_ip[CFG_FILE_ITEM_LEN];      // IP addr of the FTP server used for software downloads
      char        sw_ftp_path[CFG_FILE_ITEM_LEN];    // File Path on the FTP server used for software downloads  
      char        sw_ftp_userID[USERID_PASSWD_LEN];  // UserID of the FTP server used for software downloads
      char        sw_ftp_passWD[USERID_PASSWD_LEN];  // Password of the FTP server used for software downloads
      char        phone_num[PHONE_NUM_LEN];          // The Phone number of the module
      char        USSD_voda[PHONE_NUM_LEN];          // Phone number for Vodafone India
      char        USSD_air[PHONE_NUM_LEN];           // Phone number for AirTel India
      char        Operator[PHONE_NUM_LEN];           // Operator Name of the SIM inserted in device.
      char        ne_version[NE_VERSION_LEN];        // NE Software Version String
      bool        SIM_present;                       // True if SIM is present and readable
      bool        NE_debug_msg;                      // True turns ON debug messages tunneled to NE
      bool        cloud_enabled;                     // True if NE enabled (stored in the NE's flash)
      bool        lost_12_volts;                     // True if the 12v Alarm triggers
      bool        ftp_in_progress;                   // True if FTP session in progress
      bool        logging;                           // True if NE Logging in process
      bool        mqtt_bootup;                       // Variable to send bootup messages of MQTT only once.
      INT32       ftp_taskID;                        // Task ID for the FTP task
      INT32       mqtt_taskID;                       // Task ID for the MQTT task
      INT32       GSM_channel;                       // The GSM ARFCH of the module
      INT32       ppp_mqtt_retries;                  // Value of PPP/mqtt retries connection atempts
      UINT16      NE_boardID;                        // NE's Identification: Atlas or Titan
      UINT16      start_stop_delay;                  // Small delay on boot-up & power down for network sanity
      UINT8       ext_alarm_code;                    // Extended Alarm Code, Alarms generated on the Module only.
      UINT16      voltage;                           // 12v A/D reading
      UINT32      MQTT_start_time;                   // Starting time for the MQTT session.
      volatile    UINT32 sec_running;                // Seconds running since boot-up
      volatile    bool   rev_cmd_pkt;                // New reverse cmd_pkt received
      dev_loc_t   location;                          // Lat, Long information
	} module_info_t;
#endif // FEATURE_5BARZ 

	// -- GLOBAL VARIABLES STRUCT -- //
	typedef struct
	{
		M2M_CLOUD_AZ_LOGPORT	cloud_LogPortN;

		INT32 					M2M_CLOUD_AT_RESPONSE_TASK_ID;
		INT32 					M2M_CLOUD_PARSE_MESSAGE_TASK_ID;
		INT32 					M2M_CLOUD_METHODS_HANDLER_TASK_ID;

		M2M_CLOUD_DWSTATUS	 	cloudStatus;
		INT8					cloudRespState;
		M2M_CLOUD_BOOL 			dwIsInit;
		M2M_CLOUD_DBG_LEVEL		dwDebugLevel;
		M2M_USB_CH 				USB_LOG;

		M2M_CLOUD_DWRING_QUEUE	dw_RING_QUEUE;
		M2M_CLOUD_DWRING_ACTION DW_RING_ACTION;
		int						DW_LAST_MSG_ID;

		UINT32					timeout;

		char *CACertificatePath;

#ifdef FEATURE_5BARZ
        msg_queue_struct_t      msg_queue;
        REG_state_t             Reg_state;
        PPP_state_t             PPP_state;
        MQTT_state_t            MQTT_state;
        module_info_t           module;
        power_state_t           power_state;
#endif // FEATURE_5BARZ 

	} M2M_CLOUD_GLOBALS;


	static const char 		m2m_cloudDWConn[]    = "AT#DWCONN=1\r";
	static const char 		m2m_cloudDWDisconn[] = "AT#DWCONN=0\r";
	static const char		m2m_cloudDWStat[]    = "AT#DWSTATUS\r";
    
#ifdef FEATURE_5BARZ
// Strings & AT command:  
static const char       software_version[] = "TCA2.6E\0";     // Official Release Version String (no "_" due to Telit SMS bug)
static const char       at_cmd_CGSN[]      = "AT#CGSN\r";     // IMEI
static const char       at_cmd_CGMM[]      = "AT#CGMM\r";     // Model Designation
static const char       at_cmd_CGMR[]      = "AT#CGMR\r";     // Module firmware version
static const char       at_cmd_qss[]       = "AT#QSS?\r";     // Query SIM
static const char       at_cmd_adc[]       = "AT#ADC=1,2\r";  // A/D conversion
static const char       at_cmd_cnum[]      = "AT+CNUM\r";     // Get Phone number
static const char       at_cmd_loc[]       = "AT#AGPSSND\r";  // Telit Location services

static char             logFilePathName[]  = "/MOD/logfile.txt\0"; // Complete Path and Name of the OTA Log file

#ifdef SMS_PREFIX
static const char       sms_prefix[]       = "5B ";           // Special SMS Pre-fix string for tests purpose in India (MO SMS only)

#define SMS_PREFIX_LEN       3
#endif

// Board IDs:
#define ATLAS_BAND1          10
#define TITAN_BAND1_BAND8    12 

// Commands:    
#define CD_DISABLE_NE        0x0C00
#define CD_ENABLE_NE         0x0C01
#define CD_START_MQTT        0x0C06
#define CD_STOP_MQTT         0x0C07
#define CD_ACTIVATE_NE       0x0C08
#define CD_RESET_NE          0x0C09
#define CD_DELETE_LOG_FILE   0x0C0A
#define CD_FTP_LOG_FILE      0x0C0B
#define CD_STOP_LOGGING      0x0C0C
#define CD_DEACTIVATE_NE     0x0C0D
#define CD_PING_DEV          0x0C0E
#define CD_ENABLE_A_BAND     0x0C0F
#define CD_ENABLE_B_BAND     0x0C10

// Writes:
#define WR_RESET_MODULE      0x0D00
#define WR_UL_CHANNEL_A      0x0D02
#define WR_PERF_INTERVAL     0x0D03
#define WR_SMS_SHORT_CODE    0x0D04
#define WR_APN               0x0D06
#define WR_MQTT_URL          0x0D07
#define WR_FTP_TCA_FILE      0x0D0B
#define WR_CFG_FILE_ITEM     0x0D0C
#define WR_LOC_INTERVAL      0x0D0D
#define WR_NE_CAL_ITEM       0x0D0E
#define WR_LOG_START_STOP    0x0D10
#define WR_UL_CHANNEL_B      0x0D11
#define WR_FTP_CONN_INFO     0x0D12

// Reads:
#define RD_STATUS            0x0B00
#define RD_UL_DL_CHANNEL_A   0x0B01
#define RD_UL_DL_GAIN_A      0x0B02
#define RD_UL_DL_RSSI_A      0x0B03
#define RD_TEMPERATURE       0x0B04
#define RD_ALARM_STATE       0x0B05
#define RD_ACTIVITY_DETECT_A 0x0B06
#define RD_LED_STATUS        0x0B07
#define RD_MODULE_INFO       0x0B0A
#define RD_PHONE_NUMBER      0x0B0D
#define RD_LOCATION          0x0B0E
#define RD_CFG_FILE_ITEM     0x0B0F
#define RD_NE_CAL_ITEM       0x0B10
#define RD_UL_DL_TX_POWER_A  0x0B11
#define RD_LOG_FILE_SIZE     0x0B12
#define RD_LOG_START_STOP    0x0B13
#define RD_PERF_DATA         0x0B14
#define RD_UL_DL_CHANNEL_B   0x0B15
#define RD_UL_DL_GAIN_B      0x0B16
#define RD_UL_DL_RSSI_B      0x0B17
#define RD_ACTIVITY_DETECT_B 0x0B18
#define RD_UL_DL_TX_POWER_B  0x0B19

// Unsolicited:
#define NE_PERF_DATA         "0E00\0"
#define NE_CONF_PART1        "0E01\0"
#define NE_CONF_PART2        "0E02\0"
#define NE_CONF_PART3        "0E03\0"
#define NE_FTP_RSP           "0E04\0"
#define NE_FTP_VER           "0E05\0"
#define NE_LOC_DATA          "0E06\0"
#define NE_MQTT_STR          "0E07\0"
#define NE_POWERED_UP        "0E08\0"
// Alarms:    
#define NE_ALARM             0x0F00
#define NE_ALARM_STR         "0F00\0"

// LED Macros:
#define LED1_ON              m2m_hw_gpio_write(1,0)
#define LED1_OFF             m2m_hw_gpio_write(1,1)
#define LED2_ON              m2m_hw_gpio_write(2,0)
#define LED2_OFF             m2m_hw_gpio_write(2,1)

    
/////////////////////////////////////////////////////////////////////////////
// Function: PushToMsgQueue()
//
// Parameters: msg_queue_t
//
// Return: none
//
// Description: This function allows any other tasks or callback to 
// send a message to Task1 safety.
//
/////////////////////////////////////////////////////////////////////////////
void PushToMsgQueue( msg_queue_t msg );

#endif // FEATURE_5BARZ 


#endif /* M2M_CLOUD_TYPEDEFS_H_ */
