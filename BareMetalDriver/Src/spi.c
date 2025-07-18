#include "spi.h"
#include <stddef.h>


static void spi_txe_interrupt_handle(SPI_Handle_t *pHandle);
static void spi_rxne_interrupt_handle(SPI_Handle_t *pHandle);
static void spi_ovr_interrupt_handle(SPI_Handle_t *pHandle);
/*******************************************************************
 * @fn          SPI_PeripheralClockControl
 * @brief       Enable or disable the SPI peripheral clock
 * @param[in]   SPIx: SPI peripheral selected
 *              This parameter can be one of the following values:
 *              SPI1, SPI2, SPI3, SPI4
 * @param[in]   EnorDi: new state of the SPI peripheral clock
 *              This parameter can be: ENABLE or DISABLE
 * @return      -
 * @note        None
 *
*/
void SPI_PeriClockControl(SPI_RegDef_t* SPIx, uint8_t EnorDi){
    if(EnorDi == ENABLE){
        if(SPIx == SPI1){
            SPI1_CLK_ENABLE();
        } else if(SPIx == SPI2){
            SPI2_CLK_ENABLE();
        } else if(SPIx == SPI3){
            SPI3_CLK_ENABLE();
        }
    } else {
        if(SPIx == SPI1){
            SPI1_CLK_DISABLE();
        } else if(SPIx == SPI2){
            SPI2_CLK_DISABLE();
        } else if(SPIx == SPI3){
            SPI3_CLK_DISABLE();
        }
    }
    return;
}

/*******************************************************************
  * @fn          SPI_Init
  * @brief       Initialize the SPI peripheral
  * @param[in]   SPIx: SPI peripheral selected
  *              This parameter can be one of the following values:
  *              SPI1, SPI2, SPI3, SPI4
  * @return      None
  * @note        None
  *
 */
void SPI_Init(SPI_Handle_t *pSPIHandler){
    // config CPI CR1
    uint32_t tempreg = 0;

    //enable peripheral clock
    SPI_PeriClockControl(pSPIHandler->pSPIx, ENABLE);

    //1.config device mode
    tempreg |= pSPIHandler->SPI_Config.SPI_DeviceMode << SPI_CR1_MSTR;

    //2.config bus config
    if(pSPIHandler->SPI_Config.SPI_BusConfig == SPI_BUS_CONFIG_FD){
        //bidi mode should be cleared
        tempreg &= ~(1<<15);
    }else if(pSPIHandler->SPI_Config.SPI_BusConfig == SPI_BUS_CONFIG_SIMPLEX_RXONLY){
        //bidi mode should be cleared
        tempreg &= ~(1<<15);
        //RX_ONLY must be set
        tempreg |= (1<<10);
    }
    //3.config sclk speed
    tempreg |= (pSPIHandler->SPI_Config.SPI_SclkSpeed << SPI_CR1_BR);
    //4.config frame format
    tempreg |= pSPIHandler->SPI_Config.SPI_DFF << SPI_CR1_DFF;
    //5.config sclk polarity
    tempreg |= pSPIHandler->SPI_Config.SPI_CPOL << SPI_CR1_CPOL;
    //6.config sclk phase
    tempreg |= pSPIHandler->SPI_Config.SPI_CPHA << SPI_CR1_CPHA;
    //7.config software slave management
    pSPIHandler->pSPIx->CR1 = tempreg;
}

/*******************************************************************
  * @fn          SPI_DeInit
  * @brief       Deinitialize the SPI peripheral
  *              This function deinitialize the SPI peripheral registers to their default reset values.
  *              This function can be used to reset the SPI peripheral registers to their default reset values.
  * @param[in]   SPIx: SPI peripheral selected
  *              This parameter can be one of the following values:
  *              SPI1, SPI2, SPI3
  * @return      None
  * @note        None
  *
  * */
void SPI_DeInit(SPI_Handle_t *pSPIHandle){
    if (pSPIHandle->pSPIx == SPI1)
    {
        RCC->APB2RSTR |= (1 << 12);  // SPI1 reset
        RCC->APB2RSTR &= ~(1 << 12); // clear reset bit
    }
    else if (pSPIHandle->pSPIx == SPI2)
    {
        RCC->APB1RSTR |= (1 << 14);  // SPI2 reset
        RCC->APB1RSTR &= ~(1 << 14);
    }
    else if (pSPIHandle->pSPIx == SPI3)
    {
        RCC->APB1RSTR |= (1 << 15);  // SPI3 reset
        RCC->APB1RSTR &= ~(1 << 15);
    }
    SPI_PeriClockControl(pSPIHandle->pSPIx, DISABLE);
}

/*******************************************************************
  * @fn          SPI_GetFlagStatus
  * @brief       Checks whether the specified SPI flag is set or not.
  * @param[in]   SPIx: SPI peripheral selected
  *              This parameter can be one of the following values:
  *              SPI1, SPI2, SPI3
  * @param[in]   SPI_FLAG: specifies the flag to check.
  *              This parameter can be one of the following values:
  *              SPI_TXE_FLAG, SPI_RXNE_FLAG, SPI_BSY_FLAG
  * @return      The new state of SPI_FLAG (SET or RESET).
  * @note        None
  * */

uint8_t SPI_GetFlagStatus(SPI_RegDef_t *pSPIx, uint8_t SPI_FLAG){
    if(pSPIx->SR & SPI_FLAG){
        return FLAG_SET;
    }
    return FLAG_RESET;
}

/*******************************************************************
  * @fn          SPI_SSIConfig
  * @brief       Enable or disable the SSI mode
  * @param[in]   SPIx: SPI peripheral selected
  *              This parameter can be one of the following values:
  *              SPI1, SPI2, SPI3
  * @param[in]   EnorDi: new state of the SSI mode
  *              This parameter can be: ENABLE or DISABLE
  * @return      None
  * @note        None
  * */
void SPI_SSIConfig(SPI_RegDef_t *pSPIx, uint8_t EnorDi){
    if(EnorDi == ENABLE){
        pSPIx->CR1 |= (1<<SPI_CR1_SSI);
    } else {
        pSPIx->CR1 &= ~(1<<SPI_CR1_SSI);
    }
}
/*******************************************************************
  * @fn          SPI_SendData
  * @brief       Send a Data through the SPI peripheral
  * @param[in]   SPIx: SPI peripheral selected
  *              This parameter can be one of the following values:
  *              SPI1, SPI2, SPI3
  * @param[in]   pTxBuffer: pointer to data buffer
  * @param[in]   len: length of data buffer
  * @return      None
  * @note        None
  * */
void SPI_SendData(SPI_RegDef_t *pSPIx, uint8_t *pTxBuffer, uint32_t len){
    while(len>0){
        //1. wait until TXE(Transmit buffer empty) is set
        while(SPI_GetFlagStatus(pSPIx, SPI_TXE_FLAG) == FLAG_RESET);

        //2. check DFF
        if(pSPIx->CR1 & (1<<SPI_CR1_DFF)){
            //16bit data
            //1. load data into DR
            pSPIx->DR = (*pTxBuffer & 0xFFFF);//&0xff to up 16bit
            len--;
            len--;
            (uint16_t*)pTxBuffer++;
        } else {
            //8bit data
            pSPIx->DR = (*pTxBuffer);
            len--;
            pTxBuffer++;
        }

    }
}

/*******************************************************************
  * @fn          SPI_ReceiveData
  * @brief       Receive a Data through the SPI peripheral
  * @param[in]   SPIx: SPI peripheral selected
  *              This parameter can be one of the following values:
  *              SPI1, SPI2, SPI3
  * @param[in]   pRxBuffer: pointer to data buffer
  * @return      None
  * @note        None
  * */
void SPI_ReceiveData(SPI_RegDef_t *pSPIx, uint8_t *pRxBuffer, uint32_t len){
    while(len>0){
        //1. wait until TXE(Transmit buffer empty) is set
        while(SPI_GetFlagStatus(pSPIx, SPI_RXNE_FLAG) == FLAG_RESET);

        //2. check DFF
        if(pSPIx->CR1 & (1<<SPI_CR1_DFF)){
            //16bit data
            //1. load data into DR
            *((uint16_t*)pRxBuffer) = pSPIx->DR;//&0xff to up 16bit
            len--;
            len--;
            (uint16_t*)pRxBuffer++;
        } else {
            //8bit data
            (*pRxBuffer) = pSPIx->DR;
            len--;
            pRxBuffer++;
        }

    }
}

/*******************************************************************
 * @fn          SPI_SendDataIT
 * @brief       Send a Data through the SPI peripheral
 * @param[in]   SPIx: SPI peripheral selected
 *              This parameter can be one of the following values:
 *              SPI1, SPI2, SPI3
 * @param[in]   pTxBuffer: pointer to data buffer
 * @param[in]   len: length of data buffer
 * @return      None
 * @note        None
 * */
uint8_t SPI_SendDataIT(SPI_Handle_t *pSPIHandler, uint8_t *pTxBuffer, uint32_t len){
    uint8_t state = pSPIHandler->TxState;
    if(state!=SPI_BUSY_IN_TX){
        //1. Save TX buffer address and length of data to be sent in global variable
        pSPIHandler->pTxBuffer = pTxBuffer;
        pSPIHandler->TxLen = len;
        //2. mark spi state as busy in transmission so that no other code can take over the same SPI bus until transmission is complete
        pSPIHandler->TxState = SPI_BUSY_IN_TX;
        //3. Enable TXEIE control bit in SPI_CR2 register to get interupt when TXE flag is set
        pSPIHandler->pSPIx->CR2 |= (1<<SPI_CR2_TXEIE);
        //4. Transmit data will be handled in ISR code
    }
    return state;
}

/*******************************************************************
 * @fn          SPI_ReceiveDataIT
 * @brief       Receive a Data through the SPI peripheral
 * @param[in]   SPIx: SPI peripheral selected
 *              This parameter can be one of the following values:
 *              SPI1, SPI2, SPI3
 * @param[in]   pRxBuffer: pointer to data buffer
 * @return      None
 * @note        None
 * */

uint8_t SPI_ReceiveDataIT(SPI_Handle_t *pSPIHandler, uint8_t *pRxBuffer, uint32_t len){
    uint8_t state = pSPIHandler->RxState;
    if(state!=SPI_BUSY_IN_RX){
        //1. Save RX buffer address and length of data to be sent in global variable
        pSPIHandler->pRxBuffer = pRxBuffer;
        pSPIHandler->RxLen = len;
        //2. mark spi state as busy in transmission so that no other code can take over the same SPI bus until transmission is complete
        pSPIHandler->RxState = SPI_BUSY_IN_RX;
        //3. Enable RXEIE control bit in SPI_CR2 register to get interupt when RXE flag is set
        pSPIHandler->pSPIx->CR2 |= (1<<SPI_CR2_TXEIE);
        //4. Transmit data will be handled in ISR code
    }
    return state;
}

/*
 * IRQ Configuration and ISR handling
 */
void SPI_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi){

}
void SPI_IRQPriorityConfig(uint8_t IRQNumber, uint32_t IRQPriority){

}
void SPI_IRQHandle(SPI_Handle_t *pHandle){
    uint8_t temp1, temp2;
    //first check if TXE  is set
    temp1 = pHandle->pSPIx->SR & (1<<SPI_SR_TXE);
    temp2 = pHandle->pSPIx->CR2 & (1<<SPI_CR2_TXEIE);
    if(temp1 && temp2){
        //hanlde TXE interrupt
        spi_txe_interrupt_handle(pHandle);
    }

    //check for RXE interrupt
    temp1 = pHandle->pSPIx->SR & (1<<SPI_SR_RXNE);
    temp2 = pHandle->pSPIx->CR2 & (1<<SPI_CR2_RXNEIE);
    if(temp1 && temp2){
        //hanlde RXnE interrupt
        spi_rxne_interrupt_handle(pHandle);
    }

    //check for ERR interrupt: (ignore: CRC, MODF, TI frame format ) onlye left overrun ( refer spi interupt table for more detail)
    //Overrun occurs when SPI receives new data but the old data has not been read out of the receive register (SPI->DR) in time.
    temp1 = pHandle->pSPIx->SR & (1<<SPI_SR_OVR);
    temp2 = pHandle->pSPIx->CR2 & (1<<SPI_CR2_ERRIE);
    if(temp1 && temp2){
        //hanlde ERR interrupt
        spi_ovr_interrupt_handle(pHandle);
    }

}

static void spi_txe_interrupt_handle(SPI_Handle_t *pHandle){
        //2. check DFF
        if(pHandle->pSPIx->CR1 & (1<<SPI_CR1_DFF)){
            //16bit data
            //1. load data into DR
            pHandle->pSPIx->DR = *((uint16_t*)pHandle->pTxBuffer);//&0xff to up 16bit
            pHandle->TxLen--;
            pHandle->TxLen--;
            (uint16_t*)pHandle->pTxBuffer++;
        } else {
            //8bit data
            pHandle->pSPIx->DR = *(pHandle->pTxBuffer);//&0xff to up 16bit
            pHandle->TxLen--;
            pHandle->pTxBuffer++;
        }
        if(!pHandle->TxLen){
            //TXE len = 0 => close SPI communication and inform to application
            //prevent interupt from being called again
            SPI_CloseTransmission(pHandle);
            SPI_ApplicationEventCallback(pHandle, SPI_EVENT_TX_CMPLT);
        }
}
static void spi_rxne_interrupt_handle(SPI_Handle_t *pHandle){
        //2. check DFF
        if(pHandle->pSPIx->CR1 & (1<<SPI_CR1_DFF)){
            //16bit data
            //1. load data into DR
            *((uint16_t*)pHandle->pRxBuffer) = pHandle->pSPIx->DR;//&0xff to up 16bit
            pHandle->RxLen--;
            pHandle->RxLen--;
            (uint16_t*)pHandle->pRxBuffer++;
        } else {
            //8bit data
            *(pHandle->pRxBuffer) = pHandle->pSPIx->DR;//&0xff to up 16bit
            pHandle->RxLen--;
            pHandle->pRxBuffer++;
        }
        if(!pHandle->RxLen){
            //RXE len = 0 => close SPI communication and inform to application
            //prevent interupt from being called again
            SPI_CloseReception(pHandle);
            SPI_ApplicationEventCallback(pHandle, SPI_EVENT_RX_CMPLT);
        }
}
static void spi_ovr_interrupt_handle(SPI_Handle_t *pHandle){
        uint8_t temp;
        //1. clear overrun flag
        if(pHandle->TxState == SPI_BUSY_IN_TX){
            temp = pHandle->pSPIx->SR ;
            temp = pHandle->pSPIx->DR;
        }
        (void)temp;//to ignore warning: unused variable
        //2. inform to Application
        SPI_ApplicationEventCallback(pHandle, SPI_EVENT_OVR_ERROR);
}

void SPI_ClearOVRFlag(SPI_RegDef_t *pSPIx){
        uint8_t temp;
        temp = pSPIx->SR ;
        temp = pSPIx->DR;
        (void)temp;
}
void SPI_CloseTransmission(SPI_Handle_t *pSPIHandle){
        pSPIHandle->pSPIx->CR2 &= ~(1<<SPI_CR2_TXEIE);
        pSPIHandle->pTxBuffer = NULL;
        pSPIHandle->TxLen = 0;
        pSPIHandle->TxState = SPI_READY;
}
void SPI_CloseReception(SPI_Handle_t *pSPIHandle){
            pSPIHandle->pSPIx->CR2 &= ~(1<<SPI_CR2_RXNEIE);
            pSPIHandle->pRxBuffer = NULL;
            pSPIHandle->RxLen = 0;
            pSPIHandle->RxState = SPI_READY;
}


