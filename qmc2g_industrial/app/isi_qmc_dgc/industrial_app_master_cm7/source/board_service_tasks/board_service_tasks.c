/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */

#include <math.h>
#include "board_service_tasks.h"
#include "api_fault.h"
#include "api_rpc.h"
#include "api_board.h"
#include "fsl_tempsensor.h"
#include "nafe1x388.h"
#include "nafe_hal_flexio.h"
#include "helper_flexio_spi.h"
#include "mlib_types.h"
#include "qmc_features_config.h"

#include "fsl_debug_console.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define DELAY_MS				300u		/* Temperature measurement frequency */
#define CHANNEL_AMT				2u			/* Amount of channels used in AFE */
#define CONT_SAMPLE_AMT			1u			/* Amount of readings per channel */
#define WAKEUPS_BEFORE_TEMPS	5u			/* Iterations of the infinite loop before temperatures are measured */

#define R_25 					47000u		/* Thermistor resistance at 25 °C */
#define BETA 					4101u		/* Thermistor beta */
#define T_0 					273.15		/* 0 °C in Kelvin */
#define T_25 					(T_0 + 25u)	/* 25 °C in Kelvin */
#define V_REF					3.3			/* Referential voltage */

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

#if (MC_HAS_AFE_ANY_MOTOR != 0)
/*!
 * @brief Calculates temperature from voltage measured by the AFE
 *
 * @param[in] V The voltage value.
 *
 * @return The calculated temperature.
 */
static double CalculateTemperatureFromVoltage(double V);
#endif /* #if (MC_HAS_AFE_ANY_MOTOR != 0) */

/*******************************************************************************
 * Globals
 ******************************************************************************/
/* FLEXIO SPI handle */
FLEXIO_SPI_Type gs_FlexioSPIHandle[4] = {
	{ /* SPI1 */
		.flexioBase = FLEXIO1,              /*!< FlexIO base pointer. */
		.SCKPinIndex = 15,                  /*!< Pin select for clock. */
		.CSnPinIndex = 16,                  /*!< Pin select for enable. */
		.SDIPinIndex = 17,                  /*!< Pin select for data input. */
		.SDOPinIndex = 18,                  /*!< Pin select for data output. */
		.shifterIndex = { 0, 1 },        	/*!< Shifter index used in FlexIO SPI. */
		.timerIndex = { 0, 1 }           	/*!< Timer index used in FlexIO SPI. */
	},
	{ /* SPI2 */
		.flexioBase = FLEXIO1,              /*!< FlexIO base pointer. */
		.SCKPinIndex = 19,                  /*!< Pin select for clock. */
		.CSnPinIndex = 20,                  /*!< Pin select for enable. */
		.SDIPinIndex = 21,                  /*!< Pin select for data input. */
		.SDOPinIndex = 22,                  /*!< Pin select for data output. */
		.shifterIndex = { 2, 3 },       	/*!< Shifter index used in FlexIO SPI. */
		.timerIndex = { 2, 3 }           	/*!< Timer index used in FlexIO SPI. */
	},
	{ /* SPI3 */
		.flexioBase = FLEXIO2,              /*!< FlexIO base pointer. */
		.SCKPinIndex = 04,                  /*!< Pin select for clock. */
		.CSnPinIndex = 05,                  /*!< Pin select for enable. */
		.SDIPinIndex = 26,                  /*!< Pin select for data input. */
		.SDOPinIndex = 27,                  /*!< Pin select for data output. */
		.shifterIndex = { 0, 1 },        	/*!< Shifter index used in FlexIO SPI. */
		.timerIndex = { 0, 1 }           	/*!< Timer index used in FlexIO SPI. */
	},
	{ /* SPI4 */
		.flexioBase = FLEXIO2,              /*!< FlexIO base pointer. */
		.SCKPinIndex = 28,                  /*!< Pin select for clock. */
		.CSnPinIndex = 29,                  /*!< Pin select for enable. */
		.SDIPinIndex = 30,                  /*!< Pin select for data input. */
		.SDOPinIndex = 31,                  /*!< Pin select for data output. */
		.shifterIndex = { 2, 3 },        	/*!< Shifter index used in FlexIO SPI. */
		.timerIndex = { 2, 3 }           	/*!< Timer index used in FlexIO SPI. */
	}
};

#if (MC_HAS_AFE_ANY_MOTOR != 0)
/* System config */
static NAFE_sysConfig_t gs_SysConfig = {
    .adcResolutionCode = kNafeAdcResolution_24bits,			/* 24 bit ADC resolution */
    .triggerMode = kNafeTrigger_spiCmd,						/* AFE readings triggered by SPI commands */
    .readyPinSeqMode = kNafeReadyPinSeqMode_onConversion,	/* Data readings right after a conversion */
};

/* Channel config array */
static NAFE_chnConfig_t gs_ChnConfig[CHANNEL_AMT] = {
    {
        .chnIndex = 0u,									/* Channel index */
        .inputSel = kNafeInputSel_hvsig,				/* Input type - HV signals: H = HV_AIP - HV_AIN */
        .hvAip = kNafeHvInputPos_ai3p,					/* TEMP_AFE */
        .hvAin = kNafeHvInputNeg_gnd,					/* GND */
        .gain = kNafeChnGain_0p8x,						/* Channel gain 0.8x */
        .dataRateCode = 4u,								/* Data rate code */
        .adcSinc = kNafeAdcSinc_sinc4,					/* Sinc type */
        .chDelayCode = 0u,								/* Programmable delay */
        .adcSettling = kNafeAdcSettling_singleCycle,	/* ADC settling type */
    },
    {
        .chnIndex = 1u,									/* Channel index */
        .inputSel = kNafeInputSel_hvsig,				/* Input type - HV signals: H = HV_AIP - HV_AIN */
        .hvAip = kNafeHvInputPos_gnd,					/* GND */
        .hvAin = kNafeHvInputNeg_ai3n,					/* TEMP_AFE1 */
        .gain = kNafeChnGain_0p8x,						/* Channel gain 0.8x */
        .dataRateCode = 4u,								/* Data rate code */
        .adcSinc = kNafeAdcSinc_sinc4,					/* Sinc type */
        .chDelayCode = 0u,								/* Programmable delay */
        .adcSettling = kNafeAdcSettling_singleCycle,	/* ADC settling type */
    }
};

/* Device handle array */
static NAFE_devHdl_t gs_DevHdl[4] = {
	{ /* SPI1 */
		.devAddr = 0u,								/* SPI device index */
		.sysConfig = &gs_SysConfig,					/* NAFE system config */
		.chConfig = gs_ChnConfig,					/* NAFE channel config */
		.currentSampleMode = kNafeSampleMode_none,	/* NAFE sample mode */
		.halHdl = &gs_FlexioSPIHandle[0],			/* FLEXIO SPI handle */
		.currentChnIndex = 0						/* Current channel index */
	},
	{ /* SPI2 */
		.devAddr = 0u,								/* SPI device index */
		.sysConfig = &gs_SysConfig,					/* NAFE system config */
		.chConfig = gs_ChnConfig,					/* NAFE channel config */
		.currentSampleMode = kNafeSampleMode_none,	/* NAFE sample mode */
		.halHdl = &gs_FlexioSPIHandle[1],			/* FLEXIO SPI handle */
		.currentChnIndex = 0						/* Current channel index */
	},
	{ /* SPI3 */
		.devAddr = 0u,								/* SPI device index */
		.sysConfig = &gs_SysConfig,					/* NAFE system config */
		.chConfig = gs_ChnConfig,					/* NAFE channel config */
		.currentSampleMode = kNafeSampleMode_none,	/* NAFE sample mode */
		.halHdl = &gs_FlexioSPIHandle[2],			/* FLEXIO SPI handle */
		.currentChnIndex = 0						/* Current channel index */
	},
	{ /* SPI4 */
		.devAddr = 0u,								/* SPI device index */
		.sysConfig = &gs_SysConfig,					/* NAFE system config */
		.chConfig = gs_ChnConfig,					/* NAFE channel config */
		.currentSampleMode = kNafeSampleMode_none,	/* NAFE sample mode */
		.halHdl = &gs_FlexioSPIHandle[3],			/* FLEXIO SPI handle */
		.currentChnIndex = 0						/* Current channel index */
	}
};

static double gs_Result = 0; /* Change to array of length [CH_AMT * CONT_SAMPLE_AMT] for SCCR/MCCR/MCMR */
static bool gs_AfePSBInitialized[4] = { false, false, false, false };

/* Transfer handle */
static NAFE_xferHdl_t gs_XferHdl = {
	.sampleMode = kNafeSampleMode_none,			/* NAFE sample mode */
	.pResult = &gs_Result,						/* Sample result */
	.contSampleAmt = CONT_SAMPLE_AMT,			/* Amount of readings per channel */
	.chnAmt = CHANNEL_AMT,						/* Amount of channels */
	.requestedChn = 0							/* Requested channel index */
};

#endif /* #if (MC_HAS_AFE_ANY_MOTOR != 0) */

static bool gs_communicationErrorReported = false;

GD3000_t g_sM1GD3000 = {0};		/* Global Motor 1 GD3000 handle */
GD3000_t g_sM2GD3000 = {0};		/* Global Motor 2 GD3000 handle */
GD3000_t g_sM3GD3000 = {0};		/* Global Motor 3 GD3000 handle */
GD3000_t g_sM4GD3000 = {0};		/* Global Motor 4 GD3000 handle */

GD3000_t *gs_sMGD3000Array[4] = {&g_sM1GD3000, &g_sM2GD3000, &g_sM3GD3000, &g_sM4GD3000};

double g_PSB_TEMP1_THRESHOLD = PSB_TEMP1_THRESHOLD;
double g_PSB_TEMP2_THRESHOLD = PSB_TEMP2_THRESHOLD;
float g_DB_TEMP_THRESHOLD = DB_TEMP_THRESHOLD;
float g_MCU_TEMP_THRESHOLD = MCU_TEMP_THRESHOLD;

#if (MC_HAS_AFE_ANY_MOTOR != 0)
double g_PSBTemps[8] = {0, 0, 0, 0, 0, 0, 0, 0};
#endif  /* #if (MC_HAS_AFE_ANY_MOTOR != 0) */

float g_MCUTemp = 0;
float g_DBTemp = 0;

TaskHandle_t g_board_service_task_handle;

/*******************************************************************************
 * Code
 ******************************************************************************/
/*!
 * @brief Initiates the Board Service Task.
 *
 * @return QMC status
 */
qmc_status_t BoardServiceInit()
{
	tmpsns_config_t config;
	flexio_spi_master_config_t masterConfig = { 0 };
	FLEXIO_SPI_MasterGetDefaultConfig(&masterConfig);
	masterConfig.baudRate_Bps = SPI_BAUDRATE;
	masterConfig.phase = kFLEXIO_SPI_ClockPhaseSecondEdge;

	NAFE_HAL_init(&gs_FlexioSPIHandle[0], &masterConfig, BOARD_BOOTCLOCKOVERDRIVERUN_FLEXIO1_CLK_ROOT);
	NAFE_HAL_init(&gs_FlexioSPIHandle[1], &masterConfig, BOARD_BOOTCLOCKOVERDRIVERUN_FLEXIO1_CLK_ROOT);
	NAFE_HAL_init(&gs_FlexioSPIHandle[2], &masterConfig, BOARD_BOOTCLOCKOVERDRIVERUN_FLEXIO2_CLK_ROOT);
	NAFE_HAL_init(&gs_FlexioSPIHandle[3], &masterConfig, BOARD_BOOTCLOCKOVERDRIVERUN_FLEXIO2_CLK_ROOT);

	RPC_SelectPowerStageBoardSpiDevice(kQMC_SpiMotorDriver);
	helper_FLEXIO_SPI_Set_TIMCTL(GD3000_ON_PSB, gs_FlexioSPIHandle);
	for (mc_motor_id_t motorId = kMC_Motor1; motorId < MC_MAX_MOTORS; motorId++)
	{
		GD3000_init(gs_sMGD3000Array[motorId], &gs_FlexioSPIHandle[motorId]);
	}

	TMPSNS_GetDefaultConfig(&config);
	TMPSNS_Init(TMPSNS, &config);
	TMPSNS_DisableInterrupt(TMPSNS, kTEMPSENSOR_LowTempInterruptStatusEnable);
	TMPSNS_DisableInterrupt(TMPSNS, kTEMPSENSOR_HighTempInterruptStatusEnable);
	TMPSNS_DisableInterrupt(TMPSNS, kTEMPSENSOR_PanicTempInterruptStatusEnable);
	DisableIRQ(TMPSNS_LOW_HIGH_IRQn);

	return kStatus_QMC_Ok;
}

/*!
 * @brief Runs in an infinite loop. Reads GD3000 status registers, Temperatures on PSB, DB and MCU
 *
 * @param pvParameters unused
 */
void BoardServiceTask(void *pvParameters)
{
#if (MC_HAS_AFE_ANY_MOTOR != 0)
	double PSBTemp1 = 0;
	double PSBTemp2 = 0;
	bool PSBOvertemperatureReported[4] = { false, false, false, false };
	qmc_status_t status = kStatus_QMC_Err;
#endif  /* #if (MC_HAS_AFE_ANY_MOTOR != 0) */

	float MCUTemp = 0;
	float DBTemp = 0;
	int wakeupCounter = 0;
	bool DBOvertemperatureReported = false;
	bool MCUOvertemperatureReported = false;
	bool communicationOK = true;
	TickType_t xLastWakeTime = xTaskGetTickCount();

#if (MC_HAS_AFE_ANY_MOTOR != 0)
    RPC_SelectPowerStageBoardSpiDevice(kQMC_SpiAfe);
	helper_FLEXIO_SPI_Set_TIMCTL(AFE_ON_PSB, gs_FlexioSPIHandle);
	for (mc_motor_id_t motorId = kMC_Motor1; motorId <= kMC_Motor4; motorId++)
	{
		if (!MC_PSBx_HAS_AFE(motorId))
			continue;
		gs_DevHdl[motorId].sysConfig->enabledChnMask = 0x0003;

		status = (qmc_status_t) NAFE_init(&gs_DevHdl[motorId], &gs_XferHdl);

		if (status == kStatus_QMC_Ok)
		{
			gs_AfePSBInitialized[motorId] = true;
		}
		else
		{
			fault_source_t src = kFAULT_AfePsbCommunicationError;
			src |= motorId;
			FAULT_RaiseFaultEvent(src);
			gs_communicationErrorReported = true;
		}
	}
	RPC_SelectPowerStageBoardSpiDevice(kQMC_SpiMotorDriver);
	helper_FLEXIO_SPI_Set_TIMCTL(GD3000_ON_PSB, gs_FlexioSPIHandle);
#endif  /* #if (MC_HAS_AFE_ANY_MOTOR != 0) */

	for (;;)
	{
		wakeupCounter = (wakeupCounter + 1) % (WAKEUPS_BEFORE_TEMPS + 1);

		/*=========================== Motor status polling ===========================*/
		for (mc_motor_id_t motorId = kMC_Motor1; motorId < MC_MAX_MOTORS; motorId++)
		{
			GD3000_getSR(gs_sMGD3000Array[motorId], &gs_FlexioSPIHandle[motorId]);

			if(gs_sMGD3000Array[motorId]->ui8ResetRequest)
			{
				GD3000_init(gs_sMGD3000Array[motorId], &gs_FlexioSPIHandle[motorId]);
				gs_sMGD3000Array[motorId]->ui8ResetRequest = 0;
			}
			else if(gs_sMGD3000Array[motorId]->sStatus.uStatus0.B.desaturation || \
					gs_sMGD3000Array[motorId]->sStatus.uStatus0.B.lowVls || \
					gs_sMGD3000Array[motorId]->sStatus.uStatus0.B.overCurrent || \
					gs_sMGD3000Array[motorId]->sStatus.uStatus0.B.overTemp || \
					gs_sMGD3000Array[motorId]->sStatus.uStatus0.B.framingErr || \
					gs_sMGD3000Array[motorId]->sStatus.uStatus0.B.phaseErr)
			{
				GD3000_clearFlags(&gs_FlexioSPIHandle[motorId]);
			}

		}

		if (wakeupCounter == WAKEUPS_BEFORE_TEMPS)
		{
#if (MC_HAS_AFE_ANY_MOTOR != 0)
			RPC_SelectPowerStageBoardSpiDevice(kQMC_SpiAfe);
			helper_FLEXIO_SPI_Set_TIMCTL(AFE_ON_PSB, gs_FlexioSPIHandle);
			int index_g_PSBTemps = 0;
			for (mc_motor_id_t motorId = kMC_Motor1; motorId <= kMC_Motor4; motorId++)
			{
				if (!MC_PSBx_HAS_AFE(motorId))
					continue;

				if (!gs_AfePSBInitialized[motorId])
				{
					status = (qmc_status_t) NAFE_init(&gs_DevHdl[motorId], &gs_XferHdl);

					if (status == kStatus_QMC_Ok)
					{
						gs_AfePSBInitialized[motorId] = true;
					}
					else
					{
						/* Already reported in the Board Service Init task. Skip this iteration. */
						communicationOK = false;
						continue;
					}
				}

				bool PSBTempOK = true;

				/* Read and check temperature measured from TEMP_AFE (PSB Temp 1) */
				gs_Result = 0;
				PSBTemp1 = 0;
				gs_XferHdl.requestedChn = 0;
				gs_XferHdl.sampleMode = kNafeSampleMode_scsrBlock;
				status = (qmc_status_t) NAFE_startSample(&gs_DevHdl[motorId], &gs_XferHdl);
				if (status != kStatus_QMC_Ok)
				{
					fault_source_t src = kFAULT_AfePsbCommunicationError;
					src |= motorId;
					FAULT_RaiseFaultEvent(src);
					communicationOK = false;
					gs_communicationErrorReported = true;
					gs_AfePSBInitialized[motorId] = false;
					continue;
				}

				PSBTemp1 = CalculateTemperatureFromVoltage(gs_Result);
				if (PSBTemp1 > g_PSB_TEMP1_THRESHOLD)
				{
					fault_source_t src = kMC_PsbOverTemperature1;
					src |= motorId;
					FAULT_RaiseFaultEvent(src);
					PSBOvertemperatureReported[motorId] = true;
					g_psbFaults[motorId] |= kMC_PsbOverTemperature1;
					PSBTempOK = false;
				}
				else
				{
					g_psbFaults[motorId] &= ~(kMC_PsbOverTemperature1);
				}
				g_PSBTemps[index_g_PSBTemps++] = PSBTemp1;


				/* Read and check temperature measured from TEMP_AFE1 (PSB Temp 2) */
				gs_Result = 0;
				PSBTemp2 = 0;
				gs_XferHdl.requestedChn = 1;
				gs_XferHdl.sampleMode = kNafeSampleMode_scsrBlock;
				status = (qmc_status_t) NAFE_startSample(&gs_DevHdl[motorId], &gs_XferHdl);
				if (status != kStatus_QMC_Ok)
				{
					fault_source_t src = kFAULT_AfePsbCommunicationError;
					src |= motorId;
					FAULT_RaiseFaultEvent(src);
					communicationOK = false;
					gs_communicationErrorReported = true;
					gs_AfePSBInitialized[motorId] = false;
					continue;
				}

				PSBTemp2 = CalculateTemperatureFromVoltage(gs_Result);
				if (PSBTemp2 > g_PSB_TEMP2_THRESHOLD)
				{
					fault_source_t src = kMC_PsbOverTemperature2;
					src |= motorId;
					FAULT_RaiseFaultEvent(src);
					PSBOvertemperatureReported[motorId] = true;
					g_psbFaults[motorId] |= kMC_PsbOverTemperature2;
					PSBTempOK = false;
				}
				else
				{
					g_psbFaults[motorId] &= ~(kMC_PsbOverTemperature2);
				}
				g_PSBTemps[index_g_PSBTemps++] = PSBTemp2;


				/* If both temperatures are OK and a fault was previously reported, send a Nofault */
				if (PSBTempOK)
				{
					if (PSBOvertemperatureReported[motorId])
					{
						fault_source_t src = kMC_NoFaultBS;
						src |= motorId;
						FAULT_RaiseFaultEvent(src);
						PSBOvertemperatureReported[motorId] = false;
					}
				}
			}
			RPC_SelectPowerStageBoardSpiDevice(kQMC_SpiMotorDriver);
			helper_FLEXIO_SPI_Set_TIMCTL(GD3000_ON_PSB, gs_FlexioSPIHandle);
#endif  /* #if (MC_HAS_AFE_ANY_MOTOR != 0) */

			/* Check the temperature measured by the sensor on the digital board */
			bool systemTempOK = true;
			DBTemp = 0;
			if (BOARD_GetDbTemperature(&DBTemp) == kStatus_QMC_Ok)
			{
				g_DBTemp = DBTemp;
				if (DBTemp > g_DB_TEMP_THRESHOLD)
				{
					fault_source_t src = kFAULT_DbOverTemperature;
					FAULT_RaiseFaultEvent(src);
					DBOvertemperatureReported = true;
					systemTempOK = false;
				}
			}
			else
			{
				fault_source_t src = kFAULT_DBTempSensCommunicationError;
				FAULT_RaiseFaultEvent(src);
				communicationOK = false;
				gs_communicationErrorReported = true;
			}

			/* Check the temperature of the MCU */
			MCUTemp = 0;
			TMPSNS_StartMeasure(TMPSNS);
			MCUTemp = TMPSNS_GetCurrentTemperature(TMPSNS);
			TMPSNS_StopMeasure(TMPSNS);

			g_MCUTemp = MCUTemp;

			if (MCUTemp > g_MCU_TEMP_THRESHOLD)
			{
				fault_source_t src = kFAULT_McuOverTemperature;
				FAULT_RaiseFaultEvent(src);
				MCUOvertemperatureReported = true;
				systemTempOK = false;
			}

			/* If the DB and MCU temperatures and all the communications were OK but had been reported as faulty previously, send a NoFault */
			if (systemTempOK && communicationOK && (DBOvertemperatureReported || MCUOvertemperatureReported || gs_communicationErrorReported))
			{
				fault_source_t src = kFAULT_NoFault;
				FAULT_RaiseFaultEvent(src);
				DBOvertemperatureReported = false;
				MCUOvertemperatureReported = false;
				gs_communicationErrorReported = false;
			}
		}

		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(DELAY_MS));
	}
}

#if (MC_HAS_AFE_ANY_MOTOR != 0)
/*!
 * @brief Calculates temperature from voltage measured by the AFE
 *
 * @param[in] V The voltage value.
 *
 * @return The calculated temperature.
 */
static double CalculateTemperatureFromVoltage(double V)
{
	double R = 0;
	double T_out = 0;

	R = R_25 * V / (V_REF - V);
	T_out = 1 / ((log(R / R_25) / BETA) + (1 / T_25));
	T_out = T_out - T_0;
	return T_out;
}
#endif /* #if (MC_HAS_AFE_ANY_MOTOR != 0) */
