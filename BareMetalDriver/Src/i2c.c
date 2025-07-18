#include "i2c.h"
#include <stddef.h>

static void I2C_ManageAcking(I2C_RegDef_t* pI2Cx, uint8_t EnorDi);
 /*
 */
void I2C_PeriClockControl(I2C_RegDef_t *pI2Cx, uint8_t EnorDi){
    if(EnorDi == ENABLE){
        if(pI2Cx == I2C1)
            I2C1_CLK_ENABLE();
        else if(pI2Cx == I2C2)
            I2C2_CLK_ENABLE();
        else if(pI2Cx == I2C3)
            I2C3_CLK_ENABLE();
    }
    else{
        if(pI2Cx == I2C1)
            I2C1_CLK_DISABLE();
        else if(pI2Cx == I2C2)
            I2C2_CLK_DISABLE();
        else if(pI2Cx == I2C3)
            I2C3_CLK_DISABLE();
    }
}

/*
 *@Function I2C_Init
 *@brief Initialize the I2C peripheral
 *@param[in] I2Cx: I2C peripheral selected
 *              This parameter can be one of the following values:
 *              I2C1, I2C2, I2C3
 * @return None
 * @note None
 */
void I2C_Init(I2C_Handle_t *pI2CHandle){
    uint32_t tempreg = 0;
    uint8_t trise;
        //enable peripheral clock
    I2C_PeriClockControl(pI2CHandle->pI2Cx, ENABLE);
    //1.config mode (standard or fast)
    //2. config the speed of i2c_scl: using CR2 and CCR for config clock setting and other time like hold time and setup time
    tempreg = 0;
    tempreg |= RCC_GetPCLK1Value()/ 1000000U;
    pI2CHandle->pI2Cx->CR2 |= tempreg&0x3F;

    uint16_t ccr = 0;
    tempreg = 0;

    if(pI2CHandle->I2C_Config.I2C_SckSpeed < I2C_SCL_SPEED_SM){
        //mode is standard mode
        ccr = RCC_GetPCLK1Value()/(2*pI2CHandle->I2C_Config.I2C_SckSpeed);
        tempreg |= ccr&0xfff;
    } else {
        //mode is Fast mode
        tempreg |= (1<<15);//fast mode enable
        tempreg |= (pI2CHandle->I2C_Config.I2C_FMDutyCycle<<14);//duty cycle
        if(pI2CHandle->I2C_Config.I2C_FMDutyCycle == I2C_FM_DUTY_2){
            ccr = RCC_GetPCLK1Value()/(3*pI2CHandle->I2C_Config.I2C_SckSpeed);
        } else {
            ccr = RCC_GetPCLK1Value()/(2*pI2CHandle->I2C_Config.I2C_SckSpeed);
        }
        tempreg |= (ccr&0xFFF);
        pI2CHandle->pI2Cx->CCR = tempreg;
    }
    //3. config the device address
    tempreg = (pI2CHandle->I2C_Config.I2C_DeviceAddress<<1);
    //4. enable Acking
    tempreg = (pI2CHandle->I2C_Config.I2C_AckControl << 10);
    tempreg |= (1<<14);//requied in I2C_OAR1 register
    pI2CHandle->pI2Cx->CR1 |= tempreg;
    //5. config rise time
    if(pI2CHandle->I2C_Config.I2C_SckSpeed < I2C_SCL_SPEED_SM){
        //mode is standard mode
        trise = RCC_GetPCLK1Value()/1000000U  + 1;

    } else {
        //mode is Fast mode
        trise = RCC_GetPCLK1Value()*300/100000000U  + 1;//refer I2C manual for these specification
    }
    pI2CHandle->pI2Cx->TRISE = trise & 0x3F;
}
void I2C_DeInit(I2C_Handle_t *pI2CHandle){
    if (pI2CHandle->pI2Cx == I2C1)
    {
        RCC->APB1RSTR |= (1 << 21);  // Reset I2C1
        RCC->APB1RSTR &= ~(1 << 21); // Clear reset bit
    }
    else if (pI2CHandle->pI2Cx == I2C2)
    {
        RCC->APB1RSTR |= (1 << 22);  // Reset I2C2
        RCC->APB1RSTR &= ~(1 << 22);
    }
    else if (pI2CHandle->pI2Cx == I2C3)
    {
        RCC->APB1RSTR |= (1 << 23);
        RCC->APB1RSTR &= ~(1 << 23);
    }
    I2C_PeriClockControl(pI2CHandle->pI2Cx, DISABLE);
}

/*
 * I2C send ànd receive
*/

uint8_t getFlagStatus(I2C_RegDef_t *pI2Cx, uint32_t FlagName){
    if(pI2Cx->SR1 & FlagName){
        return FLAG_SET;
    }

    return FLAG_RESET;
}
void I2C_GenerateStartCondition(I2C_RegDef_t *pI2Cx){
    pI2Cx->CR1 |= (1<<I2C_CR1_START);//generate start condition
}

void I2C_ExecuteAddressPhase(I2C_RegDef_t *pI2Cx, uint8_t SlaveAddress, uint8_t Direction){
    SlaveAddress = SlaveAddress << 1;
    SlaveAddress &= Direction;//redudancy ^-^  transfer data = 7bit address + 1bit r/nw
    pI2Cx->DR = SlaveAddress;//send slave address
}

void I2C_ClearADDRFlag(I2C_Handle_t *pI2CHandle){
    //check device mode
    if(pI2CHandle->pI2Cx->CR2 & (1<<I2C_SR2_MSL)){
        //master mode
        if(pI2CHandle->TxRxState == I2C_BUSY_IN_RX){
            if(pI2CHandle->RxSize == 1){
                //fist disable ACK
                I2C_ManageAcking(pI2CHandle->pI2Cx, I2C_SCK_ACK_DISABLE);
                //clear ADDR flag
                //cleared by: read SR1 and then read SR2
                uint8_t dummy_read = pI2CHandle->pI2Cx->SR1;
                dummy_read = pI2CHandle->pI2Cx->SR2;//read sr1 and sr2 to clear ADDR flag
                (void)dummy_read;
            }
        }
        else {
                uint8_t dummy_read = pI2CHandle->pI2Cx->SR1;
                dummy_read = pI2CHandle->pI2Cx->SR2;//read sr1 and sr2 to clear ADDR flag
                (void)dummy_read;
        }
    }else {
        //slave mode
        uint8_t dummy_read = pI2CHandle->pI2Cx->SR1;
        dummy_read = pI2CHandle->pI2Cx->SR2;//read sr1 and sr2 to clear ADDR flag
        (void)dummy_read;
    }
}

void I2C_GenerateStopCondition(I2C_RegDef_t *pI2Cx){
    pI2Cx->CR1 |= (1<<I2C_CR1_STOP);//generate stop condition
}
void I2C_MasterSendData(I2C_Handle_t *pI2CHandle, uint8_t *pTxBuffer, uint32_t len, uint8_t SlaveAddress){
    //1. generate start condition
    I2C_GenerateStartCondition(pI2CHandle->pI2Cx);
    //2. confirm that start condition is generated successfully by checking the SB flag in the SB1 register
    //Note: until SB(start bit) is cleared by software(clear default value from 0 to 1), SCL will be stretch to low
    while(!getFlagStatus(pI2CHandle->pI2Cx, I2C_SR1_SB));
    //3. send slave address with write direction bit (total 8 bits)
    I2C_ExecuteAddressPhase(pI2CHandle->pI2Cx, SlaveAddress, I2C_WRITE);
    //4. confirm that address phase is completed by checking the ADDR flag in the SR1 register
    while(!getFlagStatus(pI2CHandle->pI2Cx, I2C_SR1_ADDR));
    //5. clear ADDR flag according to its software sequence
    //Note: Until ADDR is cleared by software(clear default value from 0 to 1), SCL will be stretch to low
    I2C_ClearADDRFlag(pI2CHandle);
    //6. send data until len become 0
    while(len>0){
        while(!getFlagStatus(pI2CHandle->pI2Cx, I2C_SR1_TXE));//wait till txe is set
        pI2CHandle->pI2Cx->DR = (*pTxBuffer & 0xFF);
        len--;
        pTxBuffer++;
    }
    //7. when len become 0, wait for TXE = 0 and BTF = 1(Byte transfer finished) before generating stop condition
    //Note: TXE = 1, BTF = 1 means that both DR and SR are empty and next transmission is possible
    //When BTF = 1 then SCL pulled to LOW
    while(!getFlagStatus(pI2CHandle->pI2Cx, I2C_SR1_TXE));
    while(!getFlagStatus(pI2CHandle->pI2Cx, I2C_SR1_BTF));
    //8. generate stop condition and master have to wait for completion of stop condition
    //Note: generation stop condition, automatically clears BTF flag
    I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
}


void I2C_ManageAcking(I2C_RegDef_t* pI2Cx, uint8_t EnorDi){
    if(EnorDi == I2C_SCK_ACK_ENABLE){
        pI2Cx->CR1 |= (1<<I2C_CR1_ACK);//enable Acking
    } else {
        pI2Cx->CR1 &= ~(1<<I2C_CR1_ACK);//disable Acking
    }
}
void I2C_MasterReceiveData(I2C_Handle_t *pI2CHandle, uint8_t *pRxBuffer, uint32_t len, uint8_t SlaveAddress){
    //1. generate start condition
    I2C_GenerateStartCondition(pI2CHandle->pI2Cx);
    //2. confirm that stop condition is generated successfully by checking the SB flag in the SB1 register
    while(!getFlagStatus(pI2CHandle->pI2Cx, I2C_SR1_SB));
    //3. send slave address with write direction bit (total 8 bits)
    I2C_ExecuteAddressPhase(pI2CHandle->pI2Cx, SlaveAddress, I2C_WRITE);
    //4. confirm that address phase is completed by checking the ADDR flag in the SR1 register
    while(!getFlagStatus(pI2CHandle->pI2Cx, I2C_SR1_ADDR));

    //5. read only 1 byte from slave
    if(len==1){
        //disable Acking
        I2C_ManageAcking(pI2CHandle->pI2Cx, I2C_SCK_ACK_DISABLE);
        //clear the addr flag
        I2C_ClearADDRFlag(pI2CHandle);
        //wait until RXNE become 1
        //automatically set when a byte data completely received into DR (Data Register) from slave.
        while(!getFlagStatus(pI2CHandle->pI2Cx, I2C_SR1_RXNE));
        //generate end condition
        I2C_GenerateStopCondition(pI2CHandle->pI2Cx);

        //read data into buffer
        *pRxBuffer = pI2CHandle->pI2Cx->DR;
        return;
    }

    //5. read when read many bytes from slave
    if(len>1){
        //clear ADDr Flag
        //read data until len = 0;
        for(int i=0; i<len; i++){
            //wait until RXNE becomes 1
            while(!getFlagStatus(pI2CHandle->pI2Cx, I2C_SR1_RXNE));
            //Note: không ACK byte cuối, byte cuối được NACK
            //ack: finish reading a byte, nack: finish reading transmission(many bytes)
            //Note: now finish receive The data  before the last one so the next setting is applied for the last
            //but if you want using i==len-1 then you have to reverse the order of the loop with wating RXNE setting statement(NOT ASSURE TO WORK)
            if(i==len-2){//if last 2 bytes remain
                //clear ack bit
                I2C_ManageAcking(pI2CHandle->pI2Cx, I2C_SCK_ACK_DISABLE);
                //generate stop condition
                I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
            }
            //read data from DR reg into buffer
            *pRxBuffer = pI2CHandle->pI2Cx->DR;
            //increment buffer address
            pRxBuffer++;
        }
    }
    //re-enable Acking
    if(pI2CHandle->I2C_Config.I2C_AckControl == I2C_SCK_ACK_ENABLE){
            I2C_ManageAcking(pI2CHandle->pI2Cx, I2C_SCK_ACK_ENABLE);
    }

}
void I2C_EnableITBUFEN(I2C_RegDef_t* pI2Cx){
    pI2Cx->CR2 |= (1<<I2C_CR2_ITBUFEN);
}
void I2C_EnableITEVTEN(I2C_RegDef_t* pI2Cx){
    pI2Cx->CR2 |= (1<<I2C_CR2_ITEVTEN);
}
void I2C_EnableITERREN(I2C_RegDef_t* pI2Cx){
    pI2Cx->CR2 |= (1<<I2C_CR2_ITERREN);
}

void I2C_DisableITBUFEN(I2C_RegDef_t* pI2Cx){
    pI2Cx->CR2 &= ~(1<<I2C_CR2_ITBUFEN);
}
void I2C_DisableITEVTEN(I2C_RegDef_t* pI2Cx){
    pI2Cx->CR2 &= ~(1<<I2C_CR2_ITEVTEN);
}
void I2C_DisableITERREN(I2C_RegDef_t* pI2Cx){
    pI2Cx->CR2 &= ~(1<<I2C_CR2_ITERREN);
}


uint8_t I2C_MasterSendDataIT(I2C_Handle_t *pI2CHandle, uint8_t *pTxBuffer, uint32_t len, uint8_t SlaveAddress, uint8_t Sr){
    uint8_t busyState = pI2CHandle->TxRxState;
    if(busyState == I2C_READY){
        pI2CHandle->pTxBuffer = pTxBuffer;
        pI2CHandle->TxLen = len;
        pI2CHandle->DevAddress = SlaveAddress;
        pI2CHandle->TxRxState = I2C_BUSY_IN_TX;
        pI2CHandle->sr = Sr;
        //start condition
        I2C_GenerateStartCondition(pI2CHandle->pI2Cx);
        //enable ITBUFEN contorl bit
        I2C_EnableITBUFEN(pI2CHandle->pI2Cx);
        //enable ITEVTEN control bit
        I2C_EnableITEVTEN(pI2CHandle->pI2Cx);
        //enable ITERREN control bit
        I2C_EnableITERREN(pI2CHandle->pI2Cx);
    }
    return busyState;

}
uint8_t I2C_MasterReceiveDataIT(I2C_Handle_t *pI2CHandle, uint8_t *pRxBuffer, uint32_t len, uint8_t SlaveAddress, uint8_t Sr){
    uint8_t busyState = pI2CHandle->TxRxState;
    if(busyState == I2C_READY){
        pI2CHandle->pRxBuffer = pRxBuffer;
        pI2CHandle->RxLen = len;
        pI2CHandle->DevAddress = SlaveAddress;
        pI2CHandle->TxRxState = I2C_BUSY_IN_RX;
        pI2CHandle->RxSize = len;//RxSize is used in ISR code to manage the data reception
        pI2CHandle->sr = Sr;
        //start condition
        I2C_GenerateStartCondition(pI2CHandle->pI2Cx);
        //enable ITBUFEN contorl bit
        I2C_EnableITBUFEN(pI2CHandle->pI2Cx);
        //enable ITEVTEN control bit
        I2C_EnableITEVTEN(pI2CHandle->pI2Cx);
        //enable ITERREN control bit
        I2C_EnableITERREN(pI2CHandle->pI2Cx);
    }
    return busyState;
}

void I2C_SlaveSendData(I2C_RegDef_t *pI2Cx, uint32_t data){
    pI2Cx->DR = data;
}
uint32_t I2C_SlaveReceiveData(I2C_RegDef_t *pI2Cx){
    return (uint32_t)pI2Cx->DR;
}
/*
 * IRQ Configuration and ISR handling
 */
//refer I2C interupt from manual
void I2C_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi);
void I2C_IRQPriorityConfig(uint8_t IRQNumber, uint32_t IRQPriority);

void I2C_CloseSendData(I2C_Handle_t *pI2CHandle){
        pI2CHandle->pTxBuffer = NULL;
        pI2CHandle->TxRxState = I2C_READY;
        pI2CHandle->TxLen = 0;

                //enable ITBUFEN contorl bit
        I2C_DisableITBUFEN(pI2CHandle->pI2Cx);
        //enable ITEVTEN control bit
        I2C_DisableITEVTEN(pI2CHandle->pI2Cx);

}

void I2C_CloseReceiveData(I2C_Handle_t *pI2CHandle){
        pI2CHandle->pRxBuffer = NULL;
        pI2CHandle->RxLen = 0;
        pI2CHandle->TxRxState = I2C_READY;
        pI2CHandle->RxSize = 0;//RxSize is used in ISR code to manage the data reception

        if(pI2CHandle->I2C_Config.I2C_AckControl == I2C_SCK_ACK_ENABLE){
            I2C_ManageAcking(pI2CHandle->pI2Cx, I2C_SCK_ACK_ENABLE);
        }

        //enable ITBUFEN contorl bit
        I2C_DisableITBUFEN(pI2CHandle->pI2Cx);
        //enable ITEVTEN control bit
        I2C_DisableITEVTEN(pI2CHandle->pI2Cx);

}
void I2C_MasterHandleTXEInterupt(I2C_Handle_t *pI2CHandle){
        if(pI2CHandle->TxLen > 0){
            //1. load data in DR
            pI2CHandle->pI2Cx->DR = *(pI2CHandle->pTxBuffer);
            //2.dereament in TXLen
            pI2CHandle->TxLen--;
            //3.Increment pTxBuffer
            pI2CHandle->pTxBuffer++;
        }
}
void I2C_MasterHandleRXNEInterupt(I2C_Handle_t *pI2CHandle){
            //data reception
        if(pI2CHandle->RxSize  == 1){
            //1. read data from DR
            *(pI2CHandle->pRxBuffer) = pI2CHandle->pI2Cx->DR;
            //2. dereament in RxLen
            pI2CHandle->RxLen--;
            //3. Increment pRxBuffer
            pI2CHandle->pRxBuffer++;
        } else if(pI2CHandle->RxSize  > 1){
            if(pI2CHandle->RxLen == 2){
                //clear ACK
                I2C_ManageAcking(pI2CHandle->pI2Cx, I2C_SCK_ACK_DISABLE);
            }
            //read DR
            *(pI2CHandle->pRxBuffer) = pI2CHandle->pI2Cx->DR;
            //2. dereament in RxLen
            pI2CHandle->RxLen--;
            //3. Increment pRxBuffer
            pI2CHandle->pRxBuffer++;
        }
        if(pI2CHandle->RxLen  == 0){
            //close reception and notify application
            //1. generate close condition
            if(pI2CHandle->sr == I2C_DISABLE_SR){
                I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
            }
            //2.close I2C reception
            I2C_CloseReceiveData(pI2CHandle);

            //3. notify application that reception is completed
            I2C_ApplicationEventCallback(pI2CHandle, I2C_EVENT_RX_CMPLT);
        }

}
void I2C_EV_IRQHandling(I2C_Handle_t *pI2CHandle){
    uint8_t temp1, temp2, temp3;
    temp1 = pI2CHandle->pI2Cx->CR2 & (1<<I2C_CR2_ITEVTEN);
    temp2 = pI2CHandle->pI2Cx->CR2 & (1<<I2C_CR2_ITERREN);

    //Interupt handling for both master and slave mode
    //1. handle Interupt generated by SB event
    //note SB bit is only applied in master mode
    temp3 = pI2CHandle->pI2Cx->SR1 & (1<<I2C_SR1_SB);
    if(temp1 && temp3){
        //SB flag is set
        //this block will not be executed in master mode because SB bit is always 0
        //let execute Address phase
        if(pI2CHandle->TxRxState == I2C_BUSY_IN_TX){
            I2C_ExecuteAddressPhase(pI2CHandle->pI2Cx, pI2CHandle->DevAddress, I2C_WRITE);
        } else if(pI2CHandle->TxRxState == I2C_BUSY_IN_RX){
            I2C_ExecuteAddressPhase(pI2CHandle->pI2Cx, pI2CHandle->DevAddress, I2C_READ);
        }
    }

    //2. handle Interrupt generated by ADDR event
    //Note: in master mode: Adddress is sent
    //        in slave modeL: Address matched with own address
    temp3 = pI2CHandle->pI2Cx->SR1 & (1<<I2C_SR1_ADDR);
    if(temp1 && temp3){
        I2C_ClearADDRFlag(pI2CHandle);
    }

    //3. handle interupt generated by BTF event
    temp3 = pI2CHandle->pI2Cx->SR1 & (1<<I2C_SR1_BTF);
    if(temp1 && temp3){
        //BTF flag is set
        if(pI2CHandle->TxRxState == I2C_BUSY_IN_TX){
            //make sure that TXE is also set
            if(pI2CHandle->pI2Cx->SR1 & (1<<I2C_SR1_TXE)){
                //Btx, TXE = 1
                if(pI2CHandle->TxLen == 0){
                    //1 generate Stop condition
                    if(pI2CHandle->sr == I2C_DISABLE_SR){
                        I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
                    }
                    //2. reset all element of pI2CHandle structure
                    I2C_CloseSendData(pI2CHandle);
                    //3. notify application that transmission is completed
                    I2C_ApplicationEventCallback(pI2CHandle, I2C_EVENT_TX_CMPLT);
                }
            }
        }else if(pI2CHandle->TxRxState == I2C_BUSY_IN_RX){
            ;
        }
    }

    //4. handle interrupt generated by STOPF event
    //Note: Stop detection flag is only applicable in slave mode, for master mode this flag will never be set
    temp3 = pI2CHandle->pI2Cx->SR1 & (1<<I2C_SR1_STOPF);
    if(temp1 && temp3){
        //STOPF flag is set
        //clear STOPF flag read SR1 then write to cr1
        pI2CHandle->pI2Cx->CR1 |= 0x0000;//read sr1 from temp3 and write to cr1 in this statement
        //this statement to trigger harware to clear STOPF flag and dont need have change value of cr1 so using |= 0x0000

        //notify application that stop condition is detected
        I2C_ApplicationEventCallback(pI2CHandle, I2C_EVENT_STOP);
    }

    //5. handle interrupt generated by TXE event
    temp3 = pI2CHandle->pI2Cx->SR1 & (1<<I2C_SR1_TXE);
    if(temp2 && temp2 && temp3){
        //check for device mode
        if(pI2CHandle->pI2Cx->SR2 & (1<<I2C_SR2_MSL)){
            //TXE flag is set
            //data transmisson
            if(pI2CHandle->TxRxState == I2C_BUSY_IN_TX){
                I2C_MasterHandleTXEInterupt(pI2CHandle);
            }
        }else{
            //slave mode
            //make sure slave in transmitter mode
            if(pI2CHandle->pI2Cx->SR2 & (1<<I2C_SR2_TRA)){
                I2C_ApplicationEventCallback(pI2CHandle, I2C_EVENT_DATA_REQ);
            }
        }
    }
    //6. handle interrupt generated by RXNE event
    temp3 = pI2CHandle->pI2Cx->SR1 & (1<<I2C_SR1_RXNE);
    if(temp2 && temp2 && temp3){
        //check device mode
        if(pI2CHandle->pI2Cx->SR2 & (1<<I2C_SR2_MSL)){
            //deivce is master
            //RXNE flag is set
            if(pI2CHandle->TxRxState == I2C_BUSY_IN_RX){
                I2C_MasterHandleRXNEInterupt(pI2CHandle);
            }
        } else {
            //slave mode
            //make sure slave in receiver mode
            if(!(pI2CHandle->pI2Cx->SR2 & (1<<I2C_SR2_TRA))){
                I2C_ApplicationEventCallback(pI2CHandle, I2C_EVENT_DATA_RCV);
            }
        }
    }
}
void I2C_ER_IRQHandling(I2C_Handle_t *pI2CHandle){
    uint32_t temp1,temp2;

    //Know the status of  ITERREN control bit in the CR2
	temp2 = (pI2CHandle->pI2Cx->CR2) & ( 1 << I2C_CR2_ITERREN);


/***********************Check for Bus error************************************/
	temp1 = (pI2CHandle->pI2Cx->SR1) & ( 1<< I2C_SR1_BERR);
	if(temp1  && temp2 )
	{
		//This is Bus error

		//Implement the code to clear the buss error flag
		pI2CHandle->pI2Cx->SR1 &= ~( 1 << I2C_SR1_BERR);

		//Implement the code to notify the application about the error
	   I2C_ApplicationEventCallback(pI2CHandle,I2C_ERROR_BERR);
	}

/***********************Check for arbitration lost error************************************/
	temp1 = (pI2CHandle->pI2Cx->SR1) & ( 1 << I2C_SR1_ARLO );
	if(temp1  && temp2)
	{
		//This is arbitration lost error

		//Implement the code to clear the arbitration lost error flag
		pI2CHandle->pI2Cx->SR1 &= ~( 1 << I2C_SR1_ARLO);
		//Implement the code to notify the application about the error
	   I2C_ApplicationEventCallback(pI2CHandle,I2C_ERROR_ARLO);
	}

/***********************Check for ACK failure  error************************************/

	temp1 = (pI2CHandle->pI2Cx->SR1) & ( 1 << I2C_SR1_AF);
	if(temp1  && temp2)
	{
		//This is ACK failure error

	    //Implement the code to clear the ACK failure error flag
	    pI2CHandle->pI2Cx->SR1 &= ~( 1 << I2C_SR1_AF);
		//Implement the code to notify the application about the error
        I2C_ApplicationEventCallback(pI2CHandle,I2C_ERROR_AF);
	}

/***********************Check for Overrun/underrun error************************************/
	temp1 = (pI2CHandle->pI2Cx->SR1) & ( 1 << I2C_SR1_OVR);
	if(temp1  && temp2)
	{
		//This is Overrun/underrun

	    //Implement the code to clear the Overrun/underrun error flag
        pI2CHandle->pI2Cx->SR1 &= ~( 1 << I2C_SR1_OVR);
		//Implement the code to notify the application about the error
        I2C_ApplicationEventCallback(pI2CHandle,I2C_ERROR_OVR);
	}

/***********************Check for Time out error************************************/
	temp1 = (pI2CHandle->pI2Cx->SR1) & ( 1 << I2C_SR1_TIMEOUT);
	if(temp1  && temp2)
	{
		//This is Time out error

	    //Implement the code to clear the Time out error flag
        pI2CHandle->pI2Cx->SR1 &= ~( 1 << I2C_SR1_TIMEOUT);

		//Implement the code to notify the application about the error
        I2C_ApplicationEventCallback(pI2CHandle,I2C_ERROR_TIMEOUT);
	}
}

/*
 * other Peripheral control API
*/


/*
 * Application callbacks
*/
__attribute((weak)) void I2C_ApplicationEventCallback(I2C_Handle_t *pHandle, uint8_t AppEv);

