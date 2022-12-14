/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */

#ifndef _API_LOGGING_H_
#define _API_LOGGING_H_

#include "api_qmc_common.h"
#include "stdbool.h"
#include "FreeRTOS.h"
#include "flash_recorder.h"

#define LOG_ENCRYPTED_RECORD_MAX_SIZE (64U)

#define DATALOGGER_EVENTBIT_DQUEUE_QUEUE      (1 << 0)
#define DATALOGGER_EVENTBIT_FIRST_STATUS_QUEUE (1 << 2)

/*******************************************************************************
 * Definitions => Enumerations
 ******************************************************************************/

/*!
 * @brief Lists all available log data formats for the data : log_recorddata_t field of log_record_t.
 *
 * Implementation hint: When defining new formats, extend log_recorddata_t as well.
 */
typedef enum _log_record_type_id
{
    kLOG_DefaultData        = 0x01U, /*!< Identifier for log_recorddata_default_t. */
	kLOG_FaultDataWithID	= 0x02U, /*!< Identifier for log_recorddata_fault_with_id_t. */
	kLOG_FaultDataWithoutID = 0x03U, /*!< Identifier for log_recorddata_fault_without_id_t. */
} log_record_type_id_t;

/*!
 * @brief List of log record sources
 */
typedef enum _log_source_id
{
    LOG_SRC_Unspecified      = 0x00U, /*!< Unspecified source */
    LOG_SRC_Webservice       = 0x01U, /*!< Log written by the Web Service */
    LOG_SRC_FaultHandling    = 0x02U, /*!< Log written by the Fault Handling */
    LOG_SRC_CloudService     = 0x03U, /*!< Log written by the Cloud Service */
    LOG_SRC_LocalService     = 0x04U, /*!< Log written by the Local Service */
    LOG_SRC_AnomalyDetection = 0x05U, /*!< Log written by the Anomaly Detection */
    LOG_SRC_MotorControl     = 0x06U, /*!< Log written by the Motor Control e.g. via DataHub task */
	LOG_SRC_SecureWatchdog   = 0x07U, /*!< Log written by the Secure Watchdog */
} log_source_id_t;

/*!
 * @brief List of log record categories
 */
typedef enum _log_category_id
{
    LOG_CAT_General        = 0x00U, /*!< General category; covers log entries that don't fit any of the other categories */
    LOG_CAT_Fault          = 0x01U, /*!< Motor and system fault events */
    LOG_CAT_Authentication = 0x02U, /*!< Authentication events, e.g. login attempts */
    LOG_CAT_Connectivity   = 0x03U, /*!< Connectivity events e.g. connection established/lost, synchronization state change, etc. */
} log_category_id_t;

/*!
 * @brief List of log event codes
 */
typedef enum _log_event_code
{
	/* Fault Handling*/
	LOG_EVENT_AfeDbCommunicationError		= 0x00U,
	LOG_EVENT_AfePsbCommunicationError		= 0x01U,
	LOG_EVENT_DBTempSensCommunicationError	= 0x02U,
	LOG_EVENT_DbOverTemperature				= 0x03U,
	LOG_EVENT_EmergencyStop					= 0x04U,
	LOG_EVENT_FaultBufferOverflow			= 0x05U,
	LOG_EVENT_FaultQueueOverflow			= 0x06U,
	LOG_EVENT_GD3000_Desaturation			= 0x07U,
	LOG_EVENT_GD3000_LowVLS					= 0x08U,
	LOG_EVENT_GD3000_OverCurrent			= 0x09U,
	LOG_EVENT_GD3000_OverTemperature		= 0x0AU,
	LOG_EVENT_GD3000_PhaseError				= 0x0BU,
	LOG_EVENT_GD3000_Reset					= 0x0CU,
	LOG_EVENT_InvalidFaultSource			= 0x0DU,
	LOG_EVENT_McuOverTemperature			= 0x0EU,
	LOG_EVENT_NoFault						= 0x0FU,
	LOG_EVENT_NoFaultBS						= 0x10U,
	LOG_EVENT_NoFaultMC						= 0x11U,
	LOG_EVENT_OverCurrent					= 0x12U,
	LOG_EVENT_OverDcBusVoltage				= 0x13U,
	LOG_EVENT_OverLoad						= 0x14U,
	LOG_EVENT_OverSpeed						= 0x15U,
	LOG_EVENT_PmicOverTemperature			= 0x16U,
	LOG_EVENT_PmicUnderVoltage1				= 0x17U,
	LOG_EVENT_PmicUnderVoltage2				= 0x18U,
	LOG_EVENT_PmicUnderVoltage3				= 0x19U,
	LOG_EVENT_PmicUnderVoltage4				= 0x1AU,
	LOG_EVENT_PsbOverTemperature1			= 0x1BU,
	LOG_EVENT_PsbOverTemperature2			= 0x1CU,
	LOG_EVENT_RotorBlocked					= 0x1DU,
	LOG_EVENT_UnderDcBusVoltage				= 0x1EU,

	/* Local Service */
	LOG_EVENT_Button1Pressed				= 0x1FU,
	LOG_EVENT_Button2Pressed				= 0x20U,
	LOG_EVENT_Button3Pressed				= 0x21U,
	LOG_EVENT_Button4Pressed				= 0x22U,
	LOG_EVENT_EmergencyButtonPressed		= 0x23U,
	LOG_EVENT_LidOpenButton					= 0x24U,
	LOG_EVENT_LidOpenSd						= 0x25U,
	LOG_EVENT_TamperingButton				= 0x26U,
	LOG_EVENT_TamperingSd					= 0x27U,

	/* Secure Watchdog */
	LOG_EVENT_ResetRequest					= 0x28U,
	LOG_EVENT_ResetWatchdog					= 0x29U,

	/* Webservice / Authentication */
	LOG_EVENT_AccountResumed				= 0x2AU,
	LOG_EVENT_AccountSuspended				= 0x2BU,
	LOG_EVENT_LoginFailure					= 0x2CU,
	LOG_EVENT_SessionTimeout				= 0x2DU,
	LOG_EVENT_TerminateSession				= 0x2EU,
	LOG_EVENT_UserLogin						= 0x2FU,
	LOG_EVENT_UserLogout					= 0x30U,

	/* Motor control / DataHub */
	LOG_EVENT_QueueingCommandFailedInternal = 0x31U,
	LOG_EVENT_QueueingCommandFailedTSN      = 0x32U,
	LOG_EVENT_QueueingCommandFailedQueue    = 0x33U,

	/* General */
	LOG_EVENT_InvalidArgument				= 0x34U,

} log_event_code_t;



/*******************************************************************************
 * Definitions => Structures
 ******************************************************************************/

extern recorder_t g_LogRecorder;

/*!
 * @brief Structure that defines a general (default) log entry.
 */
typedef struct _log_recorddata_default
{
    log_source_id_t   source;
    log_category_id_t category;
    log_event_code_t  eventCode;
    uint16_t          user;
} log_recorddata_default_t;

/*!
 * @brief Structure that defines a motor or PSB specific fault log entry.
 */
typedef struct _log_recorddata_fault_with_id_t
{
    log_source_id_t   source;
    log_category_id_t category;
    log_event_code_t  eventCode;
    uint8_t			  motorId;
} log_recorddata_fault_with_id_t;

/*!
 * @brief Structure that defines a fault handling application error log entry.
 */
typedef struct _log_recorddata_fault_without_id_t
{
    log_source_id_t   source;
    log_category_id_t category;
    log_event_code_t  eventCode;
} log_recorddata_fault_without_id_t;

 /*!
 * @brief Union that groups all available log data formats.
 *
 * Implementation hint: When defining new formats, extend log_record_type_id_t as well.
 */
typedef union _log_recorddata
{
    log_recorddata_default_t defaultData;
    log_recorddata_fault_with_id_t faultDataWithID;
    log_recorddata_fault_without_id_t faultDataWithoutID;
} log_recorddata_t;

/*!
 * @brief A plain log record
 */
typedef struct _log_record
{
    record_head_t     rhead;
    uint32_t          type;    /*!< Log data format used for the data field */
    log_recorddata_t  data;    /*!< Additional data. Interpretation according to the type field. */
} log_record_t;

/*!
 * @brief An encrypted log record
 */
typedef struct _log_encrypted_record
{
    uint8_t data[LOG_ENCRYPTED_RECORD_MAX_SIZE]; /*!< Encrypted and signed log data */
    size_t  length;                              /*!< Length of the data */
} log_encrypted_record_t;

typedef struct {
	StaticQueue_t Queue;
	QueueHandle_t QueueHandle;
	qmc_msg_queue_handle_t MsgQueueHandle;
	uint8_t       QueueBuffer[DATALOGGER_DYNAMIC_RCV_QUEUE_DEPTH * sizeof( log_encrypted_record_t)];
} log_static_queue_t;

/*******************************************************************************
 * API
 ******************************************************************************/

/*!
 * @brief Put one LogRecord into the message queue and notify the Logging Service task. If the hasPriority flag is set the log record is put at the beginning of the queue instead of its end.
 *
 * @param[in] entry Pointer to the entry to be queued
 * @param[in] hasPriority If true, the entry is queue with priority.
 */
qmc_status_t LOG_QueueLogEntry(const log_record_t* entry, bool hasPriority);

/*!
 * @brief Request a new message queue handle to receive logging messages. Operation may return kStatus_QMC_ErrMem if queue cannot be created or all statically created queues are in use.
 *
 * @param[out] handle Address of the pointer to write the retrieved handle to
 */
qmc_status_t LOG_GetNewLoggingQueueHandle(qmc_msg_queue_handle_t** handle);

/*!
 * @brief Hand back a message queue handle obtained by LOG_GetNewLoggingQueueHandle(handle : qmc_msg_queue_handle_t*) : qmc_status_t.
 *
 * @param[in] handle Pointer to the handle that is no longer in use
 */
qmc_status_t LOG_ReturnLoggingQueueHandle(const qmc_msg_queue_handle_t* handle);

/*!
 * @brief Get one log_encrypted_record_t element from the previously registered queue, if available.
 *
 * @param[in]  handle Handle of the queue to receive the log entry from
 * @param[in]  timeout Timeout in milliseconds
 * @param[out] entry Pointer to write the retrieved log entry to
 * @return kStatus_QMC_Ok = A log entry was successfully retrieved; kStatus_QMC_ErrArgInvalid = Invalid input parameters, e.g. NULL pointer; kStatus_QMC_Timeout = No log entry was available until the timeout expired
 */
qmc_status_t LOG_DequeueEncryptedLogEntry(const qmc_msg_queue_handle_t* handle, uint32_t timeout, log_encrypted_record_t* entry);

/*!
 * @brief Get the log_record_t with the given ID.
 *
 * @param[in]  id ID of the record to be retrieved
 * @param[out] record Pointer to write the retrieved log record to
 * @return kStatus_QMC_Ok = The log entry was successfully retrieved and stored at the given location; kStatus_QMC_ErrArgInvalid = An invalid log ID or a NULL pointer was passed
 */
qmc_status_t LOG_GetLogRecord(uint32_t id, log_record_t* record);

/*!
 * @brief Get the log record with the given ID and encrypt it for the external log reader.
 *
 * @param[in]  id ID of the record to be retrieved
 * @param[out] record Pointer to write the encrypted log record to
 * @return kStatus_QMC_Ok = The log entry was successfully retrieved and stored at the given location; kStatus_QMC_ErrArgInvalid = An invalid log ID or a NULL pointer was passed
 */
qmc_status_t LOG_GetLogRecordEncrypted(uint32_t id, log_encrypted_record_t* record);

/*!
 * @brief Returns the ID of the latest log entry.
 */
uint32_t LOG_GetLastLogId();

/*!
 * @brief Initialization of Flash Recorder. Need to be executed before the first usage.
 */
qmc_status_t LOG_InitDatalogger();

/*!
 * @brief Format (erase) space of Flash Recorder.
 */
qmc_status_t LOG_FormatDatalogger();


#endif /* _API_LOGGING_H_ */
