/*
 * stm32f103xx_spi_driver.c
 *
 *  Created on: Jul 22, 2026
 *      Author: sande
 */

#include "stm32f103xx_spi_driver.h"

static void spi_txe_interrupt_handle(SPI_Handle_t *pSPIHandle);
static void spi_rxne_interrupt_handle(SPI_Handle_t *pSPIHandle);
static void spi_ovr_err_interrupt_handle(SPI_Handle_t *pSPIHandle);

/*****************************************************************
 * @fn                  -  SPI_PeriClockControl
 *
 * @brief               -  Enables or disables peripheral clock for the given SPI peripheral
 *
 * @param[in]           -  base address of the SPI peripheral
 * @param[in]           -  ENABLE or DISABLE macros
 *
 * @return              -  none
 */
void SPI_PeriClockControl(SPI_RegDef_t *pSPIx, uint8_t EnorDi)
{
	if (EnorDi == ENABLE)
	{
		if (pSPIx == SPI1)
		{
			SPI1_PCLK_EN();
		}
		else if (pSPIx == SPI2)
		{
			SPI2_PCLK_EN();
		}
		else if (pSPIx == SPI3)
		{
			SPI3_PCLK_EN();
		}
	}
	else
	{
		if (pSPIx == SPI1)
		{
			SPI1_PCLK_DI();
		}
		else if (pSPIx == SPI2)
		{
			SPI2_PCLK_DI();
		}
		else if (pSPIx == SPI3)
		{
			SPI3_PCLK_DI();
		}
	}
}

/*****************************************************************
 * @fn                  -  SPI_Init
 *
 * @brief               -  Initializes the SPIx peripheral according to SPIConfig
 *
 * @param[in]           -  pointer to SPI handle structure
 *
 * @return              -  none
 */
void SPI_Init(SPI_Handle_t *pSPIHandle)
{
	/* Enable peripheral clock */
	SPI_PeriClockControl(pSPIHandle->pSPIx, ENABLE);

	uint32_t tempreg = 0;

	/* 1. Configure device mode (Master/Slave) */
	tempreg |= ((uint32_t)pSPIHandle->SPIConfig.SPI_DeviceMode << SPI_CR1_MSTR);

	/* 2. Configure bus configuration */
	if (pSPIHandle->SPIConfig.SPI_BusConfig == SPI_BUS_CONFIG_FD)
	{
		tempreg &= ~(1U << SPI_CR1_BIDIMODE);
	}
	else if (pSPIHandle->SPIConfig.SPI_BusConfig == SPI_BUS_CONFIG_HD)
	{
		tempreg |= (1U << SPI_CR1_BIDIMODE);
	}
	else if (pSPIHandle->SPIConfig.SPI_BusConfig == SPI_BUS_CONFIG_SIMPLEX_RXONLY)
	{
		tempreg &= ~(1U << SPI_CR1_BIDIMODE);
		tempreg |= (1U << SPI_CR1_RXONLY);
	}

	/* 3. Configure SPI serial clock speed (baud rate prescaler) */
	tempreg |= ((uint32_t)pSPIHandle->SPIConfig.SPI_SclkSpeed << SPI_CR1_BR);

	/* 4. Configure Data Frame Format (8-bit / 16-bit) */
	tempreg |= ((uint32_t)pSPIHandle->SPIConfig.SPI_DFF << SPI_CR1_DFF);

	/* 5. Configure CPOL */
	tempreg |= ((uint32_t)pSPIHandle->SPIConfig.SPI_CPOL << SPI_CR1_CPOL);

	/* 6. Configure CPHA */
	tempreg |= ((uint32_t)pSPIHandle->SPIConfig.SPI_CPHA << SPI_CR1_CPHA);

	/* 7. Configure Software Slave Management (SSM) */
	tempreg |= ((uint32_t)pSPIHandle->SPIConfig.SPI_SSM << SPI_CR1_SSM);

	pSPIHandle->pSPIx->CR1 = tempreg;
}

/*****************************************************************
 * @fn                  -  SPI_DeInit
 *
 * @brief               -  Resets all registers of the given SPI peripheral
 *
 * @param[in]           -  base address of the SPI peripheral
 *
 * @return              -  none
 */
void SPI_DeInit(SPI_RegDef_t *pSPIx)
{
	if (pSPIx == SPI1)
	{
		SPI1_REG_RESET();
	}
	else if (pSPIx == SPI2)
	{
		SPI2_REG_RESET();
	}
	else if (pSPIx == SPI3)
	{
		SPI3_REG_RESET();
	}
}

/*****************************************************************
 * @fn                  -  SPI_GetFlagStatus
 *
 * @brief               -  Returns FLAG_SET if the specified SR flag is set
 *
 * @param[in]           -  base address of the SPI peripheral
 * @param[in]           -  SPI flag mask
 *
 * @return              -  FLAG_SET or FLAG_RESET
 */
uint8_t SPI_GetFlagStatus(SPI_RegDef_t *pSPIx, uint32_t FlagName)
{
	if (pSPIx->SR & FlagName)
	{
		return FLAG_SET;
	}
	return FLAG_RESET;
}

/*****************************************************************
 * @fn                  -  SPI_SendData
 *
 * @brief               -  Blocking (polling-based) SPI data transmission
 *
 * @param[in]           -  base address of the SPI peripheral
 * @param[in]           -  pointer to transmit buffer
 * @param[in]           -  number of bytes to transmit
 *
 * @return              -  none
 */
void SPI_SendData(SPI_RegDef_t *pSPIx, uint8_t *pTxBuffer, uint32_t Len)
{
	while (Len > 0U)
	{
		/* 1. Wait until TXE is set */
		while (SPI_GetFlagStatus(pSPIx, SPI_TXE_FLAG) == FLAG_RESET)
			;

		/* 2. Check the DFF bit in CR1 */
		if (pSPIx->CR1 & (1U << SPI_CR1_DFF))
		{
			/* 16-bit DFF */
			pSPIx->DR = *((uint16_t *)pTxBuffer);
			pTxBuffer += 2;
			Len = (Len >= 2U) ? (Len - 2U) : 0U;
		}
		else
		{
			/* 8-bit DFF */
			pSPIx->DR = *pTxBuffer;
			pTxBuffer++;
			Len--;
		}
	}
}

/*****************************************************************
 * @fn                  -  SPI_ReceiveData
 *
 * @brief               -  Blocking (polling-based) SPI data reception
 *
 * @param[in]           -  base address of the SPI peripheral
 * @param[in]           -  pointer to receive buffer
 * @param[in]           -  number of bytes to receive
 *
 * @return              -  none
 */
void SPI_ReceiveData(SPI_RegDef_t *pSPIx, uint8_t *pRxBuffer, uint32_t Len)
{
	while (Len > 0U)
	{
		/* 1. Wait until RXNE is set */
		while (SPI_GetFlagStatus(pSPIx, SPI_RXNE_FLAG) == FLAG_RESET)
			;

		/* 2. Check the DFF bit in CR1 */
		if (pSPIx->CR1 & (1U << SPI_CR1_DFF))
		{
			/* 16-bit DFF */
			*((uint16_t *)pRxBuffer) = (uint16_t)pSPIx->DR;
			pRxBuffer += 2;
			Len = (Len >= 2U) ? (Len - 2U) : 0U;
		}
		else
		{
			/* 8-bit DFF */
			*pRxBuffer = (uint8_t)pSPIx->DR;
			pRxBuffer++;
			Len--;
		}
	}
}

/*****************************************************************
 * @fn                  -  SPI_IRQInterruptConfig
 *
 * @brief               -  Enables or disables the given IRQ number in NVIC
 *
 * @param[in]           -  IRQ number
 * @param[in]           -  ENABLE or DISABLE macros
 *
 * @return              -  none
 */
void SPI_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi)
{
	if (EnorDi == ENABLE)
	{
		if (IRQNumber <= 31U)
		{
			*NVIC_ISER0 |= (1U << IRQNumber);
		}
		else if (IRQNumber > 31U && IRQNumber < 64U)
		{
			*NVIC_ISER1 |= (1U << (IRQNumber % 32U));
		}
		else if (IRQNumber >= 64U && IRQNumber < 96U)
		{
			*NVIC_ISER2 |= (1U << (IRQNumber % 64U));
		}
	}
	else
	{
		if (IRQNumber <= 31U)
		{
			*NVIC_ICER0 |= (1U << IRQNumber);
		}
		else if (IRQNumber > 31U && IRQNumber < 64U)
		{
			*NVIC_ICER1 |= (1U << (IRQNumber % 32U));
		}
		else if (IRQNumber >= 64U && IRQNumber < 96U)
		{
			*NVIC_ICER2 |= (1U << (IRQNumber % 64U));
		}
	}
}

/*****************************************************************
 * @fn                  -  SPI_IRQPriorityConfig
 *
 * @brief               -  Configures NVIC priority for the given IRQ number
 *
 * @param[in]           -  IRQ number
 * @param[in]           -  IRQ priority (0..15)
 *
 * @return              -  none
 */
void SPI_IRQPriorityConfig(uint8_t IRQNumber, uint8_t IRQPriority)
{
	uint8_t iprx = IRQNumber / 4U;
	uint8_t iprx_section = IRQNumber % 4U;
	uint8_t shift_amount = (uint8_t)((8U * iprx_section) + (8U - NO_PR_BITS_IMPLEMENTED));

	*(NVIC_PR_BASE_ADDR + iprx) &= ~(0xFU << shift_amount);
	*(NVIC_PR_BASE_ADDR + iprx) |= ((uint32_t)IRQPriority << shift_amount);
}

/*****************************************************************
 * @fn                  -  SPI_IRQHandling
 *
 * @brief               -  Dispatches TXE, RXNE, and OVR SPI interrupts
 *
 * @param[in]           -  pointer to SPI handle structure
 *
 * @return              -  none
 */
void SPI_IRQHandling(SPI_Handle_t *pHandle)
{
	uint32_t temp1, temp2;

	/* Check for TXE */
	temp1 = pHandle->pSPIx->SR & (1U << SPI_SR_TXE);
	temp2 = pHandle->pSPIx->CR2 & (1U << SPI_CR2_TXEIE);
	if (temp1 && temp2)
	{
		spi_txe_interrupt_handle(pHandle);
	}

	/* Check for RXNE */
	temp1 = pHandle->pSPIx->SR & (1U << SPI_SR_RXNE);
	temp2 = pHandle->pSPIx->CR2 & (1U << SPI_CR2_RXNEIE);
	if (temp1 && temp2)
	{
		spi_rxne_interrupt_handle(pHandle);
	}

	/* Check for Overrun error */
	temp1 = pHandle->pSPIx->SR & (1U << SPI_SR_OVR);
	temp2 = pHandle->pSPIx->CR2 & (1U << SPI_CR2_ERRIE);
	if (temp1 && temp2)
	{
		spi_ovr_err_interrupt_handle(pHandle);
	}
}

static void spi_txe_interrupt_handle(SPI_Handle_t *pSPIHandle)
{
	if (pSPIHandle->pSPIx->CR1 & (1U << SPI_CR1_DFF))
	{
		/* 16-bit DFF */
		pSPIHandle->pSPIx->DR = *((uint16_t *)pSPIHandle->pTxBuffer);
		pSPIHandle->pTxBuffer += 2;
		pSPIHandle->TxLen = (pSPIHandle->TxLen >= 2U) ? (pSPIHandle->TxLen - 2U) : 0U;
	}
	else
	{
		/* 8-bit DFF */
		pSPIHandle->pSPIx->DR = *(pSPIHandle->pTxBuffer);
		pSPIHandle->pTxBuffer++;
		pSPIHandle->TxLen--;
	}

	if (pSPIHandle->TxLen == 0U)
	{
		SPI_CloseTransmission(pSPIHandle);
		SPI_ApplicationEventCallback(pSPIHandle, SPI_EVENT_TX_CMPLT);
	}
}

static void spi_rxne_interrupt_handle(SPI_Handle_t *pSPIHandle)
{
	if (pSPIHandle->pSPIx->CR1 & (1U << SPI_CR1_DFF))
	{
		/* 16-bit DFF */
		*((uint16_t *)pSPIHandle->pRxBuffer) = (uint16_t)pSPIHandle->pSPIx->DR;
		pSPIHandle->pRxBuffer += 2;
		pSPIHandle->RxLen = (pSPIHandle->RxLen >= 2U) ? (pSPIHandle->RxLen - 2U) : 0U;
	}
	else
	{
		/* 8-bit DFF */
		*(pSPIHandle->pRxBuffer) = (uint8_t)pSPIHandle->pSPIx->DR;
		pSPIHandle->pRxBuffer++;
		pSPIHandle->RxLen--;
	}

	if (pSPIHandle->RxLen == 0U)
	{
		SPI_CloseReception(pSPIHandle);
		SPI_ApplicationEventCallback(pSPIHandle, SPI_EVENT_RX_CMPLT);
	}
}

static void spi_ovr_err_interrupt_handle(SPI_Handle_t *pSPIHandle)
{
	if (pSPIHandle->TxState != SPI_BUSY_IN_TX)
	{
		SPI_ClearOVRFlag(pSPIHandle->pSPIx);
	}

	SPI_ApplicationEventCallback(pSPIHandle, SPI_EVENT_OVR_ERR);
}

/*****************************************************************
 * @fn                  -  SPI_SendDataIT
 *
 * @brief               -  Non-blocking (interrupt-driven) SPI transmission
 *
 * @param[in]           -  pointer to SPI handle structure
 * @param[in]           -  pointer to transmit buffer
 * @param[in]           -  number of bytes to transmit
 *
 * @return              -  previous TX state
 */
uint8_t SPI_SendDataIT(SPI_Handle_t *pSPIHandle, uint8_t *pTxBuffer, uint32_t Len)
{
	uint8_t state = pSPIHandle->TxState;

	if (state != SPI_BUSY_IN_TX)
	{
		pSPIHandle->pTxBuffer = pTxBuffer;
		pSPIHandle->TxLen = Len;
		pSPIHandle->TxState = SPI_BUSY_IN_TX;

		pSPIHandle->pSPIx->CR2 |= (1U << SPI_CR2_TXEIE);
	}

	return state;
}

/*****************************************************************
 * @fn                  -  SPI_ReceiveDataIT
 *
 * @brief               -  Non-blocking (interrupt-driven) SPI reception
 *
 * @param[in]           -  pointer to SPI handle structure
 * @param[in]           -  pointer to receive buffer
 * @param[in]           -  number of bytes to receive
 *
 * @return              -  previous RX state
 */
uint8_t SPI_ReceiveDataIT(SPI_Handle_t *pSPIHandle, uint8_t *pRxBuffer, uint32_t Len)
{
	uint8_t state = pSPIHandle->RxState;

	if (state != SPI_BUSY_IN_RX)
	{
		pSPIHandle->pRxBuffer = pRxBuffer;
		pSPIHandle->RxLen = Len;
		pSPIHandle->RxState = SPI_BUSY_IN_RX;

		pSPIHandle->pSPIx->CR2 |= (1U << SPI_CR2_RXNEIE);
	}

	return state;
}

/*****************************************************************
 * @fn                  -  SPI_PeripheralControl
 *
 * @brief               -  Enables or disables the SPI peripheral (CR1 SPE bit)
 */
void SPI_PeripheralControl(SPI_RegDef_t *pSPIx, uint8_t EnorDi)
{
	if (EnorDi == ENABLE)
	{
		pSPIx->CR1 |= (1U << SPI_CR1_SPE);
	}
	else
	{
		pSPIx->CR1 &= ~(1U << SPI_CR1_SPE);
	}
}

/*****************************************************************
 * @fn                  -  SPI_SSIConfig
 *
 * @brief               -  Sets or clears internal slave select (CR1 SSI bit)
 */
void SPI_SSIConfig(SPI_RegDef_t *pSPIx, uint8_t EnorDi)
{
	if (EnorDi == ENABLE)
	{
		pSPIx->CR1 |= (1U << SPI_CR1_SSI);
	}
	else
	{
		pSPIx->CR1 &= ~(1U << SPI_CR1_SSI);
	}
}

/*****************************************************************
 * @fn                  -  SPI_SSOEConfig
 *
 * @brief               -  Enables or disables SS output in master mode (CR2 SSOE bit)
 */
void SPI_SSOEConfig(SPI_RegDef_t *pSPIx, uint8_t EnorDi)
{
	if (EnorDi == ENABLE)
	{
		pSPIx->CR2 |= (1U << SPI_CR2_SSOE);
	}
	else
	{
		pSPIx->CR2 &= ~(1U << SPI_CR2_SSOE);
	}
}

void SPI_ClearOVRFlag(SPI_RegDef_t *pSPIx)
{
	volatile uint32_t temp;
	temp = pSPIx->DR;
	temp = pSPIx->SR;
	(void)temp;
}

void SPI_CloseTransmission(SPI_Handle_t *pSPIHandle)
{
	pSPIHandle->pSPIx->CR2 &= ~(1U << SPI_CR2_TXEIE);
	pSPIHandle->pTxBuffer = NULL;
	pSPIHandle->TxLen = 0U;
	pSPIHandle->TxState = SPI_READY;
}

void SPI_CloseReception(SPI_Handle_t *pSPIHandle)
{
	pSPIHandle->pSPIx->CR2 &= ~(1U << SPI_CR2_RXNEIE);
	pSPIHandle->pRxBuffer = NULL;
	pSPIHandle->RxLen = 0U;
	pSPIHandle->RxState = SPI_READY;
}

__weak void SPI_ApplicationEventCallback(SPI_Handle_t *pSPIHandle, uint8_t AppEv)
{
	(void)pSPIHandle;
	(void)AppEv;
}