/*
 * stm32f103xx_i2c_driver.c
 *
 *  Created on: Aug 6, 2026
 *      Author: sande
 */

#include "stm32f103xx_i2c_driver.h"

#define HSI_CLK_HZ 8000000U
#define HSE_CLK_HZ 8000000U

static const uint16_t AHB_PreScaler[8] = {2, 4, 8, 16, 64, 128, 256, 512};
static const uint8_t APB1_PreScaler[4] = {2, 4, 8, 16};

static void I2C_GenerateStartCondition(I2C_RegDef_t *pI2Cx);
static void I2C_ExecuteAddressPhaseWrite(I2C_RegDef_t *pI2Cx, uint8_t SlaveAddr);
static void I2C_ExecuteAddressPhaseRead(I2C_RegDef_t *pI2Cx, uint8_t SlaveAddr);
static void I2C_ClearADDRFlag(I2C_Handle_t *pI2CHandle);
static void I2C_MasterHandleTXEInterrupt(I2C_Handle_t *pI2CHandle);
static void I2C_MasterHandleRXNEInterrupt(I2C_Handle_t *pI2CHandle);
static uint32_t RCC_GetPCLK1Value(void);

static uint32_t RCC_GetPCLK1Value(void)
{
	uint32_t pclk1, SystemClk = HSI_CLK_HZ;
	uint8_t clksrc, temp, ahbp, apb1p;

	clksrc = ((RCC->CFGR >> 2) & 0x3U);

	if (clksrc == 0)
	{
		SystemClk = HSI_CLK_HZ;
	}
	else if (clksrc == 1)
	{
		SystemClk = HSE_CLK_HZ;
	}
	else if (clksrc == 2)
	{
		uint32_t pllmul = ((RCC->CFGR >> 18) & 0xFU) + 2U;
		if (pllmul > 16U)
		{
			pllmul = 16U;
		}

		uint32_t pllsrc;
		if (RCC->CFGR & (1U << 16))
		{
			pllsrc = (RCC->CFGR & (1U << 17)) ? (HSE_CLK_HZ / 2U) : HSE_CLK_HZ;
		}
		else
		{
			pllsrc = HSI_CLK_HZ / 2U;
		}
		SystemClk = pllsrc * pllmul;
	}

	/* AHB prescaler */
	temp = ((RCC->CFGR >> 4) & 0xFU);
	if (temp < 8)
	{
		ahbp = 1;
	}
	else
	{
		ahbp = AHB_PreScaler[temp - 8];
	}

	/* APB1 prescaler */
	temp = ((RCC->CFGR >> 8) & 0x7U);
	if (temp < 4)
	{
		apb1p = 1;
	}
	else
	{
		apb1p = APB1_PreScaler[temp - 4];
	}

	pclk1 = (SystemClk / ahbp) / apb1p;

	return pclk1;
}

static void I2C_GenerateStartCondition(I2C_RegDef_t *pI2Cx)
{
	pI2Cx->CR1 |= (1U << I2C_CR1_START);
}

static void I2C_ExecuteAddressPhaseWrite(I2C_RegDef_t *pI2Cx, uint8_t SlaveAddr)
{
	SlaveAddr = (uint8_t)(SlaveAddr << 1);
	SlaveAddr &= ~(1U);
	pI2Cx->DR = SlaveAddr;
}

static void I2C_ExecuteAddressPhaseRead(I2C_RegDef_t *pI2Cx, uint8_t SlaveAddr)
{
	SlaveAddr = (uint8_t)(SlaveAddr << 1);
	SlaveAddr |= 1U;
	pI2Cx->DR = SlaveAddr;
}

static void I2C_ClearADDRFlag(I2C_Handle_t *pI2CHandle)
{
	uint32_t dummy_read;

	/* Check for device mode (Master vs Slave) */
	if (pI2CHandle->pI2Cx->SR2 & (1U << I2C_SR2_MSL))
	{
		if (pI2CHandle->TxRxState == I2C_BUSY_IN_RX)
		{
			if (pI2CHandle->RxSize == 1)
			{
				I2C_ManageAcking(pI2CHandle->pI2Cx, I2C_ACK_DISABLE);
			}
		}
	}

	dummy_read = pI2CHandle->pI2Cx->SR1;
	dummy_read = pI2CHandle->pI2Cx->SR2;
	(void)dummy_read;
}

void I2C_GenerateStopCondition(I2C_RegDef_t *pI2Cx)
{
	pI2Cx->CR1 |= (1U << I2C_CR1_STOP);
}

void I2C_SlaveEnableDisableCallbackEvents(I2C_RegDef_t *pI2Cx, uint8_t EnorDi)
{
	if (EnorDi == ENABLE)
	{
		pI2Cx->CR2 |= (1U << I2C_CR2_ITEVTEN);
		pI2Cx->CR2 |= (1U << I2C_CR2_ITBUFEN);
		pI2Cx->CR2 |= (1U << I2C_CR2_ITERREN);
	}
	else
	{
		pI2Cx->CR2 &= ~(1U << I2C_CR2_ITEVTEN);
		pI2Cx->CR2 &= ~(1U << I2C_CR2_ITBUFEN);
		pI2Cx->CR2 &= ~(1U << I2C_CR2_ITERREN);
	}
}

/*****************************************************************
 * @fn                  -  I2C_PeriClockControl
 *
 * @brief               -  Enables or disables peripheral clock for the given I2C peripheral
 *
 * @param[in]           -  base address of the I2C peripheral
 * @param[in]           -  ENABLE or DISABLE macros
 *
 * @return              -  none
 */
void I2C_PeriClockControl(I2C_RegDef_t *pI2Cx, uint8_t EnorDi)
{
	if (EnorDi == ENABLE)
	{
		if (pI2Cx == I2C1)
		{
			I2C1_PCLK_EN();
		}
		else if (pI2Cx == I2C2)
		{
			I2C2_PCLK_EN();
		}
	}
	else
	{
		if (pI2Cx == I2C1)
		{
			I2C1_PCLK_DI();
		}
		else if (pI2Cx == I2C2)
		{
			I2C2_PCLK_DI();
		}
	}
}

/*****************************************************************
 * @fn                  -  I2C_PeripheralControl
 *
 * @brief               -  Enables or disables the I2C peripheral (CR1 PE bit)
 *
 * @param[in]           -  base address of the I2C peripheral
 * @param[in]           -  ENABLE or DISABLE macros
 *
 * @return              -  none
 */
void I2C_PeripheralControl(I2C_RegDef_t *pI2Cx, uint8_t EnorDi)
{
	if (EnorDi == ENABLE)
	{
		pI2Cx->CR1 |= (1U << I2C_CR1_PE);
	}
	else
	{
		pI2Cx->CR1 &= ~(1U << I2C_CR1_PE);
	}
}

/*****************************************************************
 * @fn                  -  I2C_Init
 *
 * @brief               -  Initializes the I2Cx peripheral timing, address, and ACK settings
 *
 * @param[in]           -  pointer to I2C handle structure
 *
 * @return              -  none
 */
void I2C_Init(I2C_Handle_t *pI2CHandle)
{
	uint32_t tempreg = 0;
	uint32_t pclk1 = RCC_GetPCLK1Value();

	/* Enable the clock for the I2Cx peripheral */
	I2C_PeriClockControl(pI2CHandle->pI2Cx, ENABLE);

	/* Ensure peripheral is disabled before configuring CCR and TRISE */
	I2C_PeripheralControl(pI2CHandle->pI2Cx, DISABLE);

	/* 1. Configure the FREQ field of CR2 */
	tempreg = (pclk1 / 1000000U) & 0x3FU;
	pI2CHandle->pI2Cx->CR2 = tempreg;

	/* 2. Program the device own 7-bit address (RM0008 requires bit 14 kept at 1) */
	tempreg = 0;
	tempreg |= (pI2CHandle->I2C_Config.I2C_DeviceAddress << 1);
	tempreg |= (1U << 14);
	pI2CHandle->pI2Cx->OAR1 = tempreg;

	/* 3. CCR calculation */
	uint16_t ccr_value = 0;
	tempreg = 0;

	if (pI2CHandle->I2C_Config.I2C_SCLSpeed <= I2C_SCL_SPEED_SM)
	{
		/* Standard mode */
		ccr_value = (uint16_t)(pclk1 / (2U * pI2CHandle->I2C_Config.I2C_SCLSpeed));
		if (ccr_value < 4U)
		{
			ccr_value = 4U;
		}
		tempreg |= (ccr_value & 0x0FFFU);
	}
	else
	{
		/* Fast mode */
		tempreg |= (1U << I2C_CCR_FS);
		tempreg |= (pI2CHandle->I2C_Config.I2C_FMDutyCycle << I2C_CCR_DUTY);

		if (pI2CHandle->I2C_Config.I2C_FMDutyCycle == I2C_FM_DUTY_2)
		{
			ccr_value = (uint16_t)(pclk1 / (3U * pI2CHandle->I2C_Config.I2C_SCLSpeed));
		}
		else
		{
			ccr_value = (uint16_t)(pclk1 / (25U * pI2CHandle->I2C_Config.I2C_SCLSpeed));
		}

		if (ccr_value < 1U)
		{
			ccr_value = 1U;
		}
		tempreg |= (ccr_value & 0x0FFFU);
	}
	pI2CHandle->pI2Cx->CCR = tempreg;

	/* 4. TRISE configuration */
	if (pI2CHandle->I2C_Config.I2C_SCLSpeed <= I2C_SCL_SPEED_SM)
	{
		tempreg = (pclk1 / 1000000U) + 1U;
	}
	else
	{
		tempreg = ((pclk1 / 1000000U) * 300U) / 1000U + 1U;
	}
	pI2CHandle->pI2Cx->TRISE = (tempreg & 0x3FU);

	/* 5. Enable peripheral, then configure ACK control bit (ACK only sticks when PE = 1) */
	I2C_PeripheralControl(pI2CHandle->pI2Cx, ENABLE);
	I2C_ManageAcking(pI2CHandle->pI2Cx, (uint8_t)pI2CHandle->I2C_Config.I2C_AckControl);
}

/*****************************************************************
 * @fn                  -  I2C_DeInit
 *
 * @brief               -  Resets all registers of the given I2C peripheral
 *
 * @param[in]           -  base address of the I2C peripheral
 *
 * @return              -  none
 */
void I2C_DeInit(I2C_RegDef_t *pI2Cx)
{
	if (pI2Cx == I2C1)
	{
#ifdef I2C1_REG_RESET
		I2C1_REG_RESET();
#else
		RCC->APB1RSTR |= (1U << 21);
		RCC->APB1RSTR &= ~(1U << 21);
#endif
	}
	else if (pI2Cx == I2C2)
	{
#ifdef I2C2_REG_RESET
		I2C2_REG_RESET();
#else
		RCC->APB1RSTR |= (1U << 22);
		RCC->APB1RSTR &= ~(1U << 22);
#endif
	}
}

/*****************************************************************
 * @fn                  -  I2C_GetFlagStatus
 *
 * @brief               -  Returns SET if the specified SR1 flag is set, otherwise RESET
 *
 * @param[in]           -  base address of the I2C peripheral
 * @param[in]           -  I2C_FLAG_* mask
 *
 * @return              -  FLAG_SET or FLAG_RESET
 */
uint8_t I2C_GetFlagStatus(I2C_RegDef_t *pI2Cx, uint32_t FlagName)
{
	if (pI2Cx->SR1 & FlagName)
	{
		return FLAG_SET;
	}
	return FLAG_RESET;
}

/*****************************************************************
 * @fn                  -  I2C_ManageAcking
 *
 * @brief               -  Enables or disables hardware ACK generation
 *
 * @param[in]           -  base address of the I2C peripheral
 * @param[in]           -  I2C_ACK_ENABLE or I2C_ACK_DISABLE
 *
 * @return              -  none
 */
void I2C_ManageAcking(I2C_RegDef_t *pI2Cx, uint8_t EnorDi)
{
	if (EnorDi == I2C_ACK_ENABLE)
	{
		pI2Cx->CR1 |= (1U << I2C_CR1_ACK);
	}
	else
	{
		pI2Cx->CR1 &= ~(1U << I2C_CR1_ACK);
	}
}

/*****************************************************************
 * @fn                  -  I2C_MasterSendData
 *
 * @brief               -  Blocking master transmit over I2C
 *
 * @param[in]           -  pointer to I2C handle
 * @param[in]           -  pointer to TX buffer
 * @param[in]           -  number of bytes to transmit
 * @param[in]           -  7-bit slave address
 * @param[in]           -  I2C_DISABLE_SR or I2C_ENABLE_SR
 *
 * @return              -  none
 */
void I2C_MasterSendData(I2C_Handle_t *pI2CHandle, uint8_t *pTxbuffer, uint32_t Len, uint8_t SlaveAddr, uint8_t Sr)
{
	/* 1. Generate START condition */
	I2C_GenerateStartCondition(pI2CHandle->pI2Cx);

	/* 2. Confirm START generation by checking SB flag in SR1 */
	while (!I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_SB))
		;

	/* 3. Send slave address with R/W bit = 0 (Write) */
	I2C_ExecuteAddressPhaseWrite(pI2CHandle->pI2Cx, SlaveAddr);

	/* 4. Confirm address phase completion by checking ADDR flag in SR1 */
	while (!I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_ADDR))
		;

	/* 5. Clear ADDR flag */
	I2C_ClearADDRFlag(pI2CHandle);

	/* 6. Send data until Len becomes 0 */
	uint32_t sent = 0;
	while (Len > 0)
	{
		while (!I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_TXE))
			;
		pI2CHandle->pI2Cx->DR = *pTxbuffer;
		pTxbuffer++;
		Len--;
		sent = 1;
	}

	/* 7. Wait until TXE=1 and BTF=1 before generating STOP */
	if (sent)
	{
		while (!I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_TXE))
			;
		while (!I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_BTF))
			;
	}

	/* 8. Generate STOP condition if repeated start is disabled */
	if (Sr == I2C_DISABLE_SR)
	{
		I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
	}
}

/*****************************************************************
 * @fn                  -  I2C_MasterReceiveData
 *
 * @brief               -  Blocking master receive over I2C
 *
 * @param[in]           -  pointer to I2C handle
 * @param[in]           -  pointer to RX buffer
 * @param[in]           -  number of bytes to receive
 * @param[in]           -  7-bit slave address
 * @param[in]           -  I2C_DISABLE_SR or I2C_ENABLE_SR
 *
 * @return              -  none
 */
void I2C_MasterReceiveData(I2C_Handle_t *pI2CHandle, uint8_t *pRxBuffer, uint8_t Len, uint8_t SlaveAddr, uint8_t Sr)
{
	if (Len == 0)
	{
		return;
	}

	/* 1. Generate START condition */
	I2C_GenerateStartCondition(pI2CHandle->pI2Cx);

	/* 2. Wait for SB flag */
	while (!I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_SB))
		;

	/* 3. Send slave address with R/W bit = 1 (Read) */
	I2C_ExecuteAddressPhaseRead(pI2CHandle->pI2Cx, SlaveAddr);

	/* 4. Wait for ADDR flag */
	while (!I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_ADDR))
		;

	/* Procedure to read only 1 byte from slave */
	if (Len == 1)
	{
		I2C_ManageAcking(pI2CHandle->pI2Cx, I2C_ACK_DISABLE);
		I2C_ClearADDRFlag(pI2CHandle);

		while (!I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_RXNE))
			;

		if (Sr == I2C_DISABLE_SR)
		{
			I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
		}

		*pRxBuffer = (uint8_t)pI2CHandle->pI2Cx->DR;
	}

	/* Procedure to read more than 1 byte from slave */
	if (Len > 1)
	{
		I2C_ClearADDRFlag(pI2CHandle);

		for (uint32_t i = Len; i > 0; i--)
		{
			while (!I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_RXNE))
				;

			if (i == 2)
			{
				I2C_ManageAcking(pI2CHandle->pI2Cx, I2C_ACK_DISABLE);

				if (Sr == I2C_DISABLE_SR)
				{
					I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
				}
			}

			*pRxBuffer = (uint8_t)pI2CHandle->pI2Cx->DR;
			pRxBuffer++;
		}
	}

	/* Re-enable ACKing if configured */
	if (pI2CHandle->I2C_Config.I2C_AckControl == I2C_ACK_ENABLE)
	{
		I2C_ManageAcking(pI2CHandle->pI2Cx, I2C_ACK_ENABLE);
	}
}

/*****************************************************************
 * @fn                  -  I2C_MasterSendDataIT
 *
 * @brief               -  Non-blocking (interrupt-driven) master transmit
 *
 * @return              -  previous bus state (I2C_READY / I2C_BUSY_IN_*)
 */
uint8_t I2C_MasterSendDataIT(I2C_Handle_t *pI2CHandle, uint8_t *pTxBuffer, uint32_t Len, uint8_t SlaveAddr, uint8_t Sr)
{
	uint8_t busystate = pI2CHandle->TxRxState;

	if ((busystate != I2C_BUSY_IN_TX) && (busystate != I2C_BUSY_IN_RX))
	{
		pI2CHandle->pTxBuffer = pTxBuffer;
		pI2CHandle->TxLen = Len;
		pI2CHandle->TxRxState = I2C_BUSY_IN_TX;
		pI2CHandle->DevAddr = SlaveAddr;
		pI2CHandle->Sr = Sr;

		I2C_GenerateStartCondition(pI2CHandle->pI2Cx);

		pI2CHandle->pI2Cx->CR2 |= (1U << I2C_CR2_ITBUFEN);
		pI2CHandle->pI2Cx->CR2 |= (1U << I2C_CR2_ITEVTEN);
		pI2CHandle->pI2Cx->CR2 |= (1U << I2C_CR2_ITERREN);
	}

	return busystate;
}

/*****************************************************************
 * @fn                  -  I2C_MasterReceiveDataIT
 *
 * @brief               -  Non-blocking (interrupt-driven) master receive
 *
 * @return              -  previous bus state (I2C_READY / I2C_BUSY_IN_*)
 */
uint8_t I2C_MasterReceiveDataIT(I2C_Handle_t *pI2CHandle, uint8_t *pRxBuffer, uint8_t Len, uint8_t SlaveAddr, uint8_t Sr)
{
	uint8_t busystate = pI2CHandle->TxRxState;

	if ((busystate != I2C_BUSY_IN_TX) && (busystate != I2C_BUSY_IN_RX))
	{
		pI2CHandle->pRxBuffer = pRxBuffer;
		pI2CHandle->RxLen = Len;
		pI2CHandle->TxRxState = I2C_BUSY_IN_RX;
		pI2CHandle->RxSize = Len;
		pI2CHandle->DevAddr = SlaveAddr;
		pI2CHandle->Sr = Sr;

		I2C_GenerateStartCondition(pI2CHandle->pI2Cx);

		pI2CHandle->pI2Cx->CR2 |= (1U << I2C_CR2_ITBUFEN);
		pI2CHandle->pI2Cx->CR2 |= (1U << I2C_CR2_ITEVTEN);
		pI2CHandle->pI2Cx->CR2 |= (1U << I2C_CR2_ITERREN);
	}

	return busystate;
}

void I2C_CloseReceiveData(I2C_Handle_t *pI2CHandle)
{
	pI2CHandle->pI2Cx->CR2 &= ~(1U << I2C_CR2_ITBUFEN);
	pI2CHandle->pI2Cx->CR2 &= ~(1U << I2C_CR2_ITEVTEN);

	pI2CHandle->TxRxState = I2C_READY;
	pI2CHandle->pRxBuffer = NULL;
	pI2CHandle->RxLen = 0;
	pI2CHandle->RxSize = 0;

	if (pI2CHandle->I2C_Config.I2C_AckControl == I2C_ACK_ENABLE)
	{
		I2C_ManageAcking(pI2CHandle->pI2Cx, I2C_ACK_ENABLE);
	}
}

void I2C_CloseSendData(I2C_Handle_t *pI2CHandle)
{
	pI2CHandle->pI2Cx->CR2 &= ~(1U << I2C_CR2_ITBUFEN);
	pI2CHandle->pI2Cx->CR2 &= ~(1U << I2C_CR2_ITEVTEN);

	pI2CHandle->TxRxState = I2C_READY;
	pI2CHandle->pTxBuffer = NULL;
	pI2CHandle->TxLen = 0;
}

void I2C_SlaveSendData(I2C_RegDef_t *pI2C, uint8_t data)
{
	pI2C->DR = data;
}

uint8_t I2C_SlaveReceiveData(I2C_RegDef_t *pI2C)
{
	return (uint8_t)pI2C->DR;
}

/*****************************************************************
 * @fn                  -  I2C_IRQInterruptConfig
 *
 * @brief               -  Enables or disables the given IRQ number in NVIC
 */
void I2C_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi)
{
	if (EnorDi == ENABLE)
	{
		if (IRQNumber <= 31)
		{
			*NVIC_ISER0 |= (1U << IRQNumber);
		}
		else if (IRQNumber > 31 && IRQNumber < 64)
		{
			*NVIC_ISER1 |= (1U << (IRQNumber % 32));
		}
		else if (IRQNumber >= 64 && IRQNumber < 96)
		{
			*NVIC_ISER2 |= (1U << (IRQNumber % 64));
		}
	}
	else
	{
		if (IRQNumber <= 31)
		{
			*NVIC_ICER0 |= (1U << IRQNumber);
		}
		else if (IRQNumber > 31 && IRQNumber < 64)
		{
			*NVIC_ICER1 |= (1U << (IRQNumber % 32));
		}
		else if (IRQNumber >= 64 && IRQNumber < 96)
		{
			*NVIC_ICER2 |= (1U << (IRQNumber % 64));
		}
	}
}

/*****************************************************************
 * @fn                  -  I2C_IRQPriorityConfig
 *
 * @brief               -  Sets NVIC priority for the given IRQ number
 */
void I2C_IRQPriorityConfig(uint8_t IRQNumber, uint8_t IRQPriority)
{
	uint8_t iprx = IRQNumber / 4;
	uint8_t iprx_section = IRQNumber % 4;
	uint8_t shift_amount = (uint8_t)((8 * iprx_section) + (8 - NO_PR_BITS_IMPLEMENTED));

	*(NVIC_PR_BASE_ADDR + iprx) &= ~(0xFU << shift_amount);
	*(NVIC_PR_BASE_ADDR + iprx) |= ((uint32_t)IRQPriority << shift_amount);
}

static void I2C_MasterHandleTXEInterrupt(I2C_Handle_t *pI2CHandle)
{
	if (pI2CHandle->TxLen > 0)
	{
		pI2CHandle->pI2Cx->DR = *(pI2CHandle->pTxBuffer);
		pI2CHandle->TxLen--;
		pI2CHandle->pTxBuffer++;
	}
}

static void I2C_MasterHandleRXNEInterrupt(I2C_Handle_t *pI2CHandle)
{
	if (pI2CHandle->RxSize == 1)
	{
		*pI2CHandle->pRxBuffer = (uint8_t)pI2CHandle->pI2Cx->DR;
		pI2CHandle->RxLen--;
	}
	else if (pI2CHandle->RxSize > 1)
	{
		if (pI2CHandle->RxLen == 2)
		{
			I2C_ManageAcking(pI2CHandle->pI2Cx, I2C_ACK_DISABLE);
		}

		*pI2CHandle->pRxBuffer = (uint8_t)pI2CHandle->pI2Cx->DR;
		pI2CHandle->pRxBuffer++;
		pI2CHandle->RxLen--;
	}

	if (pI2CHandle->RxLen == 0)
	{
		if (pI2CHandle->Sr == I2C_DISABLE_SR)
		{
			I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
		}

		I2C_CloseReceiveData(pI2CHandle);
		I2C_ApplicationEventCallback(pI2CHandle, I2C_EV_RX_CMPLT);
	}
}

/*****************************************************************
 * @fn                  -  I2C_EV_IRQHandling
 *
 * @brief               -  Interrupt handler for I2C event interrupts (Master and Slave)
 */
void I2C_EV_IRQHandling(I2C_Handle_t *pI2CHandle)
{
	uint32_t temp1, temp2, temp3;

	temp1 = pI2CHandle->pI2Cx->CR2 & (1U << I2C_CR2_ITEVTEN);
	temp2 = pI2CHandle->pI2Cx->CR2 & (1U << I2C_CR2_ITBUFEN);

	/* 1. Handle SB event (Master mode only) */
	temp3 = pI2CHandle->pI2Cx->SR1 & (1U << I2C_SR1_SB);
	if (temp1 && temp3)
	{
		if (pI2CHandle->TxRxState == I2C_BUSY_IN_TX)
		{
			I2C_ExecuteAddressPhaseWrite(pI2CHandle->pI2Cx, pI2CHandle->DevAddr);
		}
		else if (pI2CHandle->TxRxState == I2C_BUSY_IN_RX)
		{
			I2C_ExecuteAddressPhaseRead(pI2CHandle->pI2Cx, pI2CHandle->DevAddr);
		}
	}

	/* 2. Handle ADDR event (Master: address sent; Slave: address matched) */
	temp3 = pI2CHandle->pI2Cx->SR1 & (1U << I2C_SR1_ADDR);
	if (temp1 && temp3)
	{
		I2C_ClearADDRFlag(pI2CHandle);
	}

	/* 3. Handle BTF (Byte Transfer Finished) event */
	temp3 = pI2CHandle->pI2Cx->SR1 & (1U << I2C_SR1_BTF);
	if (temp1 && temp3)
	{
		if (pI2CHandle->TxRxState == I2C_BUSY_IN_TX)
		{
			if (pI2CHandle->pI2Cx->SR1 & (1U << I2C_SR1_TXE))
			{
				if (pI2CHandle->TxLen == 0)
				{
					if (pI2CHandle->Sr == I2C_DISABLE_SR)
					{
						I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
					}
					I2C_CloseSendData(pI2CHandle);
					I2C_ApplicationEventCallback(pI2CHandle, I2C_EV_TX_CMPLT);
				}
			}
		}
	}

	/* 4. Handle STOPF event (Slave mode only) */
	temp3 = pI2CHandle->pI2Cx->SR1 & (1U << I2C_SR1_STOPF);
	if (temp1 && temp3)
	{
		/* Clear STOPF by reading SR1 (done above) followed by writing CR1 */
		pI2CHandle->pI2Cx->CR1 |= 0x0000U;
		I2C_ApplicationEventCallback(pI2CHandle, I2C_EV_STOP);
	}

	/* 5. Handle TXE event */
	temp3 = pI2CHandle->pI2Cx->SR1 & (1U << I2C_SR1_TXE);
	if (temp1 && temp2 && temp3)
	{
		if (pI2CHandle->pI2Cx->SR2 & (1U << I2C_SR2_MSL))
		{
			if (pI2CHandle->TxRxState == I2C_BUSY_IN_TX)
			{
				I2C_MasterHandleTXEInterrupt(pI2CHandle);
			}
		}
		else
		{
			if (pI2CHandle->pI2Cx->SR2 & (1U << I2C_SR2_TRA))
			{
				I2C_ApplicationEventCallback(pI2CHandle, I2C_EV_DATA_REQ);
			}
		}
	}

	/* 6. Handle RXNE event */
	temp3 = pI2CHandle->pI2Cx->SR1 & (1U << I2C_SR1_RXNE);
	if (temp1 && temp2 && temp3)
	{
		if (pI2CHandle->pI2Cx->SR2 & (1U << I2C_SR2_MSL))
		{
			if (pI2CHandle->TxRxState == I2C_BUSY_IN_RX)
			{
				I2C_MasterHandleRXNEInterrupt(pI2CHandle);
			}
		}
		else
		{
			if (!(pI2CHandle->pI2Cx->SR2 & (1U << I2C_SR2_TRA)))
			{
				I2C_ApplicationEventCallback(pI2CHandle, I2C_EV_DATA_RCV);
			}
		}
	}
}

/*****************************************************************
 * @fn                  -  I2C_ER_IRQHandling
 *
 * @brief               -  Interrupt handler for I2C error interrupts
 */
void I2C_ER_IRQHandling(I2C_Handle_t *pI2CHandle)
{
	uint32_t temp1, temp2;

	temp2 = (pI2CHandle->pI2Cx->CR2) & (1U << I2C_CR2_ITERREN);

	/* Bus error */
	temp1 = (pI2CHandle->pI2Cx->SR1) & (1U << I2C_SR1_BERR);
	if (temp1 && temp2)
	{
		pI2CHandle->pI2Cx->SR1 &= ~(1U << I2C_SR1_BERR);
		I2C_ApplicationEventCallback(pI2CHandle, I2C_ERROR_BERR);
	}

	/* Arbitration lost error */
	temp1 = (pI2CHandle->pI2Cx->SR1) & (1U << I2C_SR1_ARLO);
	if (temp1 && temp2)
	{
		pI2CHandle->pI2Cx->SR1 &= ~(1U << I2C_SR1_ARLO);
		I2C_ApplicationEventCallback(pI2CHandle, I2C_ERROR_ARLO);
	}

	/* ACK failure error */
	temp1 = (pI2CHandle->pI2Cx->SR1) & (1U << I2C_SR1_AF);
	if (temp1 && temp2)
	{
		pI2CHandle->pI2Cx->SR1 &= ~(1U << I2C_SR1_AF);
		I2C_ApplicationEventCallback(pI2CHandle, I2C_ERROR_AF);
	}

	/* Overrun/underrun error */
	temp1 = (pI2CHandle->pI2Cx->SR1) & (1U << I2C_SR1_OVR);
	if (temp1 && temp2)
	{
		pI2CHandle->pI2Cx->SR1 &= ~(1U << I2C_SR1_OVR);
		I2C_ApplicationEventCallback(pI2CHandle, I2C_ERROR_OVR);
	}

	/* Time out error */
	temp1 = (pI2CHandle->pI2Cx->SR1) & (1U << I2C_SR1_TIMEOUT);
	if (temp1 && temp2)
	{
		pI2CHandle->pI2Cx->SR1 &= ~(1U << I2C_SR1_TIMEOUT);
		I2C_ApplicationEventCallback(pI2CHandle, I2C_ERROR_TIMEOUT);
	}
}

__weak void I2C_ApplicationEventCallback(I2C_Handle_t *pI2CHandle, uint8_t AppEv)
{
	(void)pI2CHandle;
	(void)AppEv;
}