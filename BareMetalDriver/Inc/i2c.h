#ifndef I2C_H_
#define I2C_H_

#include <stdint.h>
#include "stm32f411xx.h"
#include "rcc_driver.h"


/*
 *@I2C_ReadWrite
 */
#define I2C_WRITE 0
#define I2C_READ 1

/*
 * Application state
 */
#define I2C_READY 0
#define I2C_BUSY_IN_TX 1
#define I2C_BUSY_IN_RX 2

/*
 * I2C Application Event
*/
#define I2C_EVENT_TX_CMPLT 0
#define I2C_EVENT_RX_CMPLT 1
#define I2C_EVENT_STOP 2
#define I2C_ERROR_BERR  3
#define I2C_ERROR_ARLO  4
#define I2C_ERROR_AF    5
#define I2C_ERROR_OVR   6
#define I2C_ERROR_TIMEOUT 7
#define I2C_EVENT_DATA_REQ 8
#define I2C_EVENT_DATA_RCV 9

/*
 *@I2C_SckSpeed
 */
#define I2C_SCL_SPEED_SM 100000
#define I2C_SCL_SPEED_FM4K 400000
#define I2C_SCL_SPEED_FM2K 200000

/*
 *@I2C_AckControl
 */
#define I2C_SCK_ACK_ENABLE 1
#define I2C_SCK_ACK_DISABLE 0

/*
 *@I2C_FMDutyCycle
 */
#define I2C_FM_DUTY_2 0
#define I2C_FM_DUTY_16_9 1

/*
 * Clock Status
*/
#define ENABLE 1
#define DISABLE 0

/*
 * SR value
 */
#define I2C_DISABLE_SR 0
#define I2C_ENABLE_SR 1

/*
 * I2C related status flags
 */
#define I2C_FLAG_SB (1<<I2C_SR1_SB)
#define I2C_FLAG_ADDR (1<<I2C_SR1_ADDR)
#define I2C_FLAG_BTF (1<<I2C_SR1_BTF)
#define I2C_FLAG_ADD10 (1<<I2C_SR1_ADD10)
#define I2C_FLAG_STOPF (1<<I2C_SR1_STOPF)
#define I2C_FLAG_RXNE (1<<I2C_SR1_RXNE)
#define I2C_FLAG_TXE (1<<I2C_SR1_TXE)
#define I2C_FLAG_BERR (1<<I2C_SR1_BERR)
#define I2C_FLAG_ARLO (1<<I2C_SR1_ARLO)
#define I2C_FLAG_AF (1<<I2C_SR1_AF)
#define I2C_FLAG_OVR (1<<I2C_SR1_OVR)
#define I2C_FLAG_PECERR (1<<I2C_SR1_PECERR)
#define I2C_FLAG_TIMEOUT (1<<I2C_SR1_TIMEOUT)
#define I2C_FLAG_SMBALERT (1<<I2C_SR1_SMBALERT)

 /**********************************************************************************
 *  						config structure of i2c
 * *****************************************************************************/
typedef struct {
    uint32_t I2C_SckSpeed; /* */
    uint32_t I2C_FMDutyCycle;
    uint32_t I2C_DeviceAddress;
    uint32_t I2C_AckControl;
}I2C_Config_t;

 /**********************************************************************************
 *  						handle structure of i2c
 * *****************************************************************************/
typedef struct {
    I2C_RegDef_t *pI2Cx;        /*this hold the base address of I2Cx peripheral*/
    I2C_Config_t I2C_Config;
    uint8_t* pTxBuffer;          /* Tx buffer Address*/
    uint8_t* pRxBuffer;          /* Rx buffer Address*/
    uint32_t TxLen;             /* Tx buffer length*/
    uint32_t RxLen;             /* Rx buffer length*/
    uint8_t TxRxState;            /* state*/
    uint8_t DevAddress;         /* Device/slave address*/
    uint32_t RxSize;
    uint8_t sr;                 /* store repeat start value*/
}I2C_Handle_t;




 /**********************************************************************************
 *  								API of i2c
 * *****************************************************************************/

/*
 * Clock Setup
 */
void I2C_PeriClockControl(I2C_RegDef_t *pI2Cx, uint8_t EnorDi);

/*
 * Init and Deinit
 */
void I2C_Init(I2C_Handle_t *pI2CHandle);
void I2C_DeInit(I2C_Handle_t *pI2CHandle);

/*
 * I2C send ànd receive
*/
void I2C_MasterSendData(I2C_Handle_t *pI2CHandle, uint8_t *pTxBuffer, uint32_t len, uint8_t SlaveAddress);
void I2C_MasterReceiveData(I2C_Handle_t *pI2CHandle, uint8_t *pRxBuffer, uint32_t len, uint8_t SlaveAddress);

uint8_t I2C_MasterSendDataIT(I2C_Handle_t *pI2CHandle, uint8_t *pTxBuffer, uint32_t len, uint8_t SlaveAddress, uint8_t sr);
uint8_t I2C_MasterReceiveDataIT(I2C_Handle_t *pI2CHandle, uint8_t *pRxBuffer, uint32_t len, uint8_t SlaveAddress, uint8_t sr);


void I2C_SlaveSendData(I2C_RegDef_t *pI2Cx, uint32_t data);
uint32_t I2C_SlaveReceiveData(I2C_RegDef_t *pI2Cx);
/*
 * IRQ Configuration and ISR handling
 */
void I2C_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi);
void I2C_IRQPriorityConfig(uint8_t IRQNumber, uint32_t IRQPriority);

void I2C_EV_IRQHandling(I2C_Handle_t *pI2CHandle);
void I2C_ER_IRQHandling(I2C_Handle_t *pI2CHandle);

/*
 * other Peripheral control API
*/


/*
 * Application callbacks
*/
__attribute((weak)) void I2C_ApplicationEventCallback(I2C_Handle_t *pHandle, uint8_t AppEv);
//weak to allow application writer to override the default weak implementation


#endif /* I2C_H_ */
