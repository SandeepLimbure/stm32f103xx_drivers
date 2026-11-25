/*
 * stm32f103xx_gpio_driver.c
 *
 *  Created on: Jul 8, 2026
 *      Author: LIMBUSM
 */

#include "stm32f103xx_gpio_driver.h"

/*****************************************************************
 * @fn                  -  GPIO_PeriClockControl
 *
 * @brief               -  Enables or disables peripheral clock for the given GPIO port
 *
 * @param[in]           -  base address of the GPIO peripheral
 * @param[in]           -  ENABLE or DISABLE macros
 *
 * @return              -  none
 */
void GPIO_PeriClockControl(GPIO_RegDef_t *pGPIOx, uint8_t EnorDi)
{
    if (EnorDi == ENABLE)
    {
        if (pGPIOx == GPIOA)
        {
            GPIOA_PCLK_EN();
        }
        else if (pGPIOx == GPIOB)
        {
            GPIOB_PCLK_EN();
        }
        else if (pGPIOx == GPIOC)
        {
            GPIOC_PCLK_EN();
        }
        else if (pGPIOx == GPIOD)
        {
            GPIOD_PCLK_EN();
        }
        else if (pGPIOx == GPIOE)
        {
            GPIOE_PCLK_EN();
        }
        else if (pGPIOx == GPIOF)
        {
            GPIOF_PCLK_EN();
        }
        else if (pGPIOx == GPIOG)
        {
            GPIOG_PCLK_EN();
        }
    }
    else
    {
        if (pGPIOx == GPIOA)
        {
            GPIOA_PCLK_DI();
        }
        else if (pGPIOx == GPIOB)
        {
            GPIOB_PCLK_DI();
        }
        else if (pGPIOx == GPIOC)
        {
            GPIOC_PCLK_DI();
        }
        else if (pGPIOx == GPIOD)
        {
            GPIOD_PCLK_DI();
        }
        else if (pGPIOx == GPIOE)
        {
            GPIOE_PCLK_DI();
        }
        else if (pGPIOx == GPIOF)
        {
            GPIOF_PCLK_DI();
        }
        else if (pGPIOx == GPIOG)
        {
            GPIOG_PCLK_DI();
        }
    }
}

/*****************************************************************
 * @fn                  -  GPIO_Init
 *
 * @brief               -  Configures CRL/CRH, ODR (Pull-up/down), AFIO, and EXTI
 *                         for the given GPIO pin
 *
 * @param[in]           -  pointer to GPIO handle structure
 *
 * @return              -  none
 */
void GPIO_Init(GPIO_Handle_t *pGPIOHandle)
{
    uint8_t pinNumber = pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber;
    uint8_t bitOffset = (uint8_t)((pinNumber % 8U) * 4U);
    uint32_t cnf_mode_val = 0U;

    /* Enable peripheral clock */
    GPIO_PeriClockControl(pGPIOHandle->pGPIOx, ENABLE);

    /* 1. NON-INTERRUPT MODES (Input, Output, Alternate Function) */
    if (pGPIOHandle->GPIO_PinConfig.GPIO_PinMode <= GPIO_MODE_ALTFN)
    {
        if (pGPIOHandle->GPIO_PinConfig.GPIO_PinMode == GPIO_MODE_IN)
        {
            /* MODE[1:0] = 00 */
            if (pGPIOHandle->GPIO_PinConfig.GPIO_PinPuPdControl == GPIO_ANALOG_MODE)
            {
                cnf_mode_val = (0x0U << 2) | 0x0U; /* CNF = 00 (Analog) */
            }
            else if (pGPIOHandle->GPIO_PinConfig.GPIO_PinPuPdControl == GPIO_NO_PULL)
            {
                cnf_mode_val = (0x1U << 2) | 0x0U; /* CNF = 01 (Floating) */
            }
            else if (pGPIOHandle->GPIO_PinConfig.GPIO_PinPuPdControl == GPIO_PIN_PULL_UP)
            {
                cnf_mode_val = (0x2U << 2) | 0x0U; /* CNF = 10 (Input PU/PD) */
                pGPIOHandle->pGPIOx->ODR |= (1U << pinNumber);
            }
            else if (pGPIOHandle->GPIO_PinConfig.GPIO_PinPuPdControl == GPIO_PIN_PULL_DOWN)
            {
                cnf_mode_val = (0x2U << 2) | 0x0U; /* CNF = 10 (Input PU/PD) */
                pGPIOHandle->pGPIOx->ODR &= ~(1U << pinNumber);
            }
        }
        else if (pGPIOHandle->GPIO_PinConfig.GPIO_PinMode == GPIO_MODE_OUT)
        {
            uint32_t speed = (pGPIOHandle->GPIO_PinConfig.GPIO_PinSpeed & 0x3U);
            uint32_t cnfBits = (pGPIOHandle->GPIO_PinConfig.GPIO_PinOutputType == GPIO_OP_TYPE_OD) ? 0x1U : 0x0U;
            if (speed == 0U)
            {
                speed = GPIO_SPEED_2MHZ;
            }
            cnf_mode_val = (cnfBits << 2) | speed;
        }
        else if (pGPIOHandle->GPIO_PinConfig.GPIO_PinMode == GPIO_MODE_ALTFN)
        {
            uint32_t speed = (pGPIOHandle->GPIO_PinConfig.GPIO_PinSpeed & 0x3U);
            uint32_t cnfBits = (pGPIOHandle->GPIO_PinConfig.GPIO_PinOutputType == GPIO_OP_TYPE_OD) ? 0x3U : 0x2U;
            if (speed == 0U)
            {
                speed = GPIO_SPEED_2MHZ;
            }
            cnf_mode_val = (cnfBits << 2) | speed;
        }

        /* Apply configuration to CRL (Pins 0-7) or CRH (Pins 8-15) */
        if (pinNumber < 8U)
        {
            pGPIOHandle->pGPIOx->CRL &= ~(0xFU << bitOffset);
            pGPIOHandle->pGPIOx->CRL |= (cnf_mode_val << bitOffset);
        }
        else
        {
            pGPIOHandle->pGPIOx->CRH &= ~(0xFU << bitOffset);
            pGPIOHandle->pGPIOx->CRH |= (cnf_mode_val << bitOffset);
        }
    }
    /* 2. INTERRUPT MODES (GPIO_MODE_IT_FT, GPIO_MODE_IT_RT, GPIO_MODE_IT_RFT) */
    else
    {
        /* A. Configure pin as Input in CRL/CRH */
        if (pGPIOHandle->GPIO_PinConfig.GPIO_PinPuPdControl == GPIO_PIN_PULL_UP)
        {
            cnf_mode_val = (0x2U << 2) | 0x0U;
            pGPIOHandle->pGPIOx->ODR |= (1U << pinNumber);
        }
        else if (pGPIOHandle->GPIO_PinConfig.GPIO_PinPuPdControl == GPIO_PIN_PULL_DOWN)
        {
            cnf_mode_val = (0x2U << 2) | 0x0U;
            pGPIOHandle->pGPIOx->ODR &= ~(1U << pinNumber);
        }
        else
        {
            cnf_mode_val = (0x1U << 2) | 0x0U; /* Floating Input */
        }

        if (pinNumber < 8U)
        {
            pGPIOHandle->pGPIOx->CRL &= ~(0xFU << bitOffset);
            pGPIOHandle->pGPIOx->CRL |= (cnf_mode_val << bitOffset);
        }
        else
        {
            pGPIOHandle->pGPIOx->CRH &= ~(0xFU << bitOffset);
            pGPIOHandle->pGPIOx->CRH |= (cnf_mode_val << bitOffset);
        }

        /* B. Enable AFIO clock */
        AFIO_PCLK_EN();

        /* C. Route GPIO port to EXTI line in AFIO->EXTICR */
        uint8_t exticr_index = pinNumber / 4U;
        uint8_t exticr_shift = (uint8_t)((pinNumber % 4U) * 4U);
        uint32_t portCode = (uint32_t)GPIO_BASEADDR_TO_CODE(pGPIOHandle->pGPIOx);

        AFIO->EXTICR[exticr_index] &= ~(0xFU << exticr_shift);
        AFIO->EXTICR[exticr_index] |= (portCode << exticr_shift);

        /* D. Configure edge triggers in EXTI */
        if (pGPIOHandle->GPIO_PinConfig.GPIO_PinMode == GPIO_MODE_IT_FT)
        {
            EXTI->FTSR |= (1U << pinNumber);
            EXTI->RTSR &= ~(1U << pinNumber);
        }
        else if (pGPIOHandle->GPIO_PinConfig.GPIO_PinMode == GPIO_MODE_IT_RT)
        {
            EXTI->RTSR |= (1U << pinNumber);
            EXTI->FTSR &= ~(1U << pinNumber);
        }
        else if (pGPIOHandle->GPIO_PinConfig.GPIO_PinMode == GPIO_MODE_IT_RFT)
        {
            EXTI->FTSR |= (1U << pinNumber);
            EXTI->RTSR |= (1U << pinNumber);
        }

        /* E. Unmask EXTI interrupt line */
        EXTI->IMR |= (1U << pinNumber);
    }
}

/*****************************************************************
 * @fn                  -  GPIO_DeInit
 *
 * @brief               -  Resets all registers of the given GPIO port
 *
 * @param[in]           -  base address of the GPIO peripheral
 *
 * @return              -  none
 */
void GPIO_DeInit(GPIO_RegDef_t *pGPIOx)
{
    if (pGPIOx == GPIOA)
    {
        GPIOA_REG_RESET();
    }
    else if (pGPIOx == GPIOB)
    {
        GPIOB_REG_RESET();
    }
    else if (pGPIOx == GPIOC)
    {
        GPIOC_REG_RESET();
    }
    else if (pGPIOx == GPIOD)
    {
        GPIOD_REG_RESET();
    }
    else if (pGPIOx == GPIOE)
    {
        GPIOE_REG_RESET();
    }
}

/*****************************************************************
 * @fn                  -  GPIO_ReadFromInputPin
 *
 * @brief               -  Reads logic level (0 or 1) from a single GPIO pin
 *
 * @param[in]           -  base address of the GPIO peripheral
 * @param[in]           -  pin number (0..15)
 *
 * @return              -  0 or 1
 */
uint8_t GPIO_ReadFromInputPin(GPIO_RegDef_t *pGPIOx, uint8_t PinNumber)
{
    return (uint8_t)((pGPIOx->IDR >> PinNumber) & 0x01U);
}

/*****************************************************************
 * @fn                  -  GPIO_ReadFromInputPort
 *
 * @brief               -  Reads the 16-bit input data register of a GPIO port
 *
 * @param[in]           -  base address of the GPIO peripheral
 *
 * @return              -  16-bit port input state
 */
uint16_t GPIO_ReadFromInputPort(GPIO_RegDef_t *pGPIOx)
{
    return (uint16_t)pGPIOx->IDR;
}

/*****************************************************************
 * @fn                  -  GPIO_WriteToOutputPin
 *
 * @brief               -  Atomically sets or resets a single GPIO output pin
 *
 * @param[in]           -  base address of the GPIO peripheral
 * @param[in]           -  pin number (0..15)
 * @param[in]           -  GPIO_PIN_SET or GPIO_PIN_RESET
 *
 * @return              -  none
 */
void GPIO_WriteToOutputPin(GPIO_RegDef_t *pGPIOx, uint8_t PinNumber, uint8_t Value)
{
    if (Value == GPIO_PIN_SET)
    {
        pGPIOx->BSRR = (1U << PinNumber);
    }
    else
    {
        pGPIOx->BRR = (1U << PinNumber);
    }
}

/*****************************************************************
 * @fn                  -  GPIO_WriteToOutputPort
 *
 * @brief               -  Writes a 16-bit value to the GPIO port output data register
 *
 * @param[in]           -  base address of the GPIO peripheral
 * @param[in]           -  16-bit output value
 *
 * @return              -  none
 */
void GPIO_WriteToOutputPort(GPIO_RegDef_t *pGPIOx, uint16_t Value)
{
    pGPIOx->ODR = Value;
}

/*****************************************************************
 * @fn                  -  GPIO_ToggleOutputPin
 *
 * @brief               -  Toggles the output state of a single GPIO pin
 *
 * @param[in]           -  base address of the GPIO peripheral
 * @param[in]           -  pin number (0..15)
 *
 * @return              -  none
 */
void GPIO_ToggleOutputPin(GPIO_RegDef_t *pGPIOx, uint8_t PinNumber)
{
    pGPIOx->ODR ^= (1U << PinNumber);
}

/*****************************************************************
 * @fn                  -  GPIO_IRQInterruptConfig
 *
 * @brief               -  Enables or disables the given IRQ number in NVIC
 *
 * @param[in]           -  IRQ number
 * @param[in]           -  ENABLE or DISABLE macros
 *
 * @return              -  none
 */
void GPIO_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi)
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
 * @fn                  -  GPIO_IRQPriorityConfig
 *
 * @brief               -  Configures NVIC priority for the given IRQ number
 *
 * @param[in]           -  IRQ number
 * @param[in]           -  IRQ priority (0..15)
 *
 * @return              -  none
 */
void GPIO_IRQPriorityConfig(uint8_t IRQNumber, uint8_t IRQPriority)
{
    uint8_t iprx = IRQNumber / 4U;
    uint8_t iprx_section = IRQNumber % 4U;
    uint8_t shift_amount = (uint8_t)((8U * iprx_section) + (8U - NO_PR_BITS_IMPLEMENTED));

    *(NVIC_PR_BASE_ADDR + iprx) &= ~(0xFU << shift_amount);
    *(NVIC_PR_BASE_ADDR + iprx) |= ((uint32_t)IRQPriority << shift_amount);
}

/*****************************************************************
 * @fn                  -  GPIO_IRQHandling
 *
 * @brief               -  Clears the EXTI pending bit for the given pin number
 *
 * @param[in]           -  pin number (0..15)
 *
 * @return              -  none
 */
void GPIO_IRQHandling(uint8_t PinNumber)
{
    if (EXTI->PR & (1U << PinNumber))
    {
        EXTI->PR = (1U << PinNumber);
    }
}