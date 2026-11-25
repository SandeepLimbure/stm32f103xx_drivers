#include <string.h>
#include "stm32f103xx.h"

#define MY_ADDR 0x61U
#define SLAVE_ADDR 0x68U

I2C_Handle_t I2C1Handle;
uint8_t some_data[] = "We are testing I2C master Tx\n";

static void delay(void)
{
    for (uint32_t i = 0; i < 250000; i++)
        ;
}

/*
 * PB6 -> I2C1_SCL
 * PB7 -> I2C1_SDA
 */
static void I2C1_GPIOInits(void)
{
    GPIO_Handle_t I2CPins;

    GPIO_PeriClockControl(GPIOB, ENABLE);

    I2CPins.pGPIOx = GPIOB;
    I2CPins.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
    I2CPins.GPIO_PinConfig.GPIO_PinOutputType = GPIO_OP_TYPE_OD;
    I2CPins.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_NO_PULL;
    I2CPins.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_50MHZ;

    /* SCL */
    I2CPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_6;
    GPIO_Init(&I2CPins);

    /* SDA */
    I2CPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_7;
    GPIO_Init(&I2CPins);
}

static void I2C1_Inits(void)
{
    memset(&I2C1Handle, 0, sizeof(I2C1Handle));

    I2C1Handle.pI2Cx = I2C1;
    I2C1Handle.I2C_Config.I2C_AckControl = I2C_ACK_ENABLE;
    I2C1Handle.I2C_Config.I2C_DeviceAddress = MY_ADDR;
    I2C1Handle.I2C_Config.I2C_FMDutyCycle = I2C_FM_DUTY_2;
    I2C1Handle.I2C_Config.I2C_SCLSpeed = I2C_SCL_SPEED_SM;

    I2C_Init(&I2C1Handle);
}

static void GPIO_ButtonInit(void)
{
    GPIO_Handle_t GPIOBtn;

    GPIO_PeriClockControl(GPIOC, ENABLE);

    GPIOBtn.pGPIOx = GPIOC;
    GPIOBtn.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_13;
    GPIOBtn.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_IN;
    GPIOBtn.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_10MHZ;
    GPIOBtn.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_NO_PULL;

    GPIO_Init(&GPIOBtn);
}

int main(void)
{
    GPIO_ButtonInit();
    I2C1_GPIOInits();
    I2C1_Inits();

    while (1)
    {
        /* Nucleo-F103RB user button (PC13) is active-low */
        while (GPIO_ReadFromInputPin(GPIOC, GPIO_PIN_NO_13) != GPIO_PIN_RESET)
            ;

        delay();

        I2C_MasterSendData(&I2C1Handle, some_data, (uint32_t)strlen((char *)some_data), SLAVE_ADDR, I2C_DISABLE_SR);
    }
}