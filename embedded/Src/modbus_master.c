/*
 * modbus_master.c
 *
 *  Created on: Aug 22, 2019
 *      Author: Sociya
 */
#include "modbus_master.h"
#include "main.h"
#include "string.h"


ModbusChannelTypeDef g_ModbusChannel1;
ModbusChannelTypeDef g_ModbusChannel2;

int16_t Modbus_TransmitData(ModbusChannelTypeDef *channel){
	//Check UART port type
	if(channel->huart->hdmatx!=NULL){
		if(HAL_OK!=HAL_UART_Transmit_DMA(channel->huart,
				channel->txBuf, channel->txLength)){
			return -1;
		}
	}else{
		if(HAL_OK!=HAL_UART_Transmit(channel->huart,
				channel->txBuf, channel->txLength,1000)){
			return -1;
		}
	}
	return 0;
}
void Modbus_ResetChannel(ModbusChannelTypeDef *channel){
	channel->passedTimeMs=0;
	channel->retryCounter=0;
	channel->rxLength=0;
	channel->txLength=0;
	channel->rxOverFlow=0;
	channel->state=Empty;
}

void Modbus_InitChannel(ModbusChannelTypeDef *channel,
		UART_HandleTypeDef *huart,
		uint16_t overTimeMs,
		uint16_t retryTime){
	channel->huart=huart;
	channel->overTimeMs=overTimeMs;
	channel->retryTime=retryTime;
	Modbus_ResetChannel(channel);
}

//请求帧
int16_t Modbus_MakeRequestFrame(uint8_t *dest,
		ModbusRequestTypeDef *request){
	int16_t _byteCnter=0;
	if(request->dataLength>127)return -1;

	//Head
	*(dest)=request->slaveId;
	*(dest+1)=request->funCode;
	*(dest+2)=((request->baseAddr)>>8)&0xFF;
	*(dest+3)=(request->baseAddr)&0xFF;
	_byteCnter=4;

	//Body
	if((request->funCode==WriteMuiltyCoilStatus)||
			(request->funCode==WriteMuiltyHoldRegister)){
		*(dest+_byteCnter)=(request->dataLength>>8)&0xFF;
		*(dest+_byteCnter+1)=(request->dataLength)&0xFF;
		*(dest+_byteCnter+2)=(request->dataLength)*2;
		memcpy(dest+_byteCnter+3,request->dataPtr,request->dataLength*2);
		_byteCnter += 3+(request->dataLength)*2;
	}
	if((request->funCode=WriteSingleCoilStatu)||
			(request->funCode==WriteSingleHoldRegister)){
		*(dest+_byteCnter)=((*(request->dataPtr))>>8)&0xFF;
		*(dest+_byteCnter+1)=(*(request->dataPtr))&0xFF;
		_byteCnter += 2;
	}
	if((request->funCode==ReadCoilStatus)||
			(request->funCode==ReadInputStatus)||
			(request->funCode==ReadInputRegister)||
			(request->funCode==ReadHoldRegister)){
		*(dest+_byteCnter)=((request->dataLength)>>8)&0xFF;
		*(dest+_byteCnter+1)=(request->dataLength)&0xFF;
		_byteCnter +=2;
	}
	//Calculate CRC

	return _byteCnter;
}
int16_t Modbus_GenerateRequest(ModbusChannelTypeDef *channel,
		ModbusRequestTypeDef *request){

	channel->txLength=Modbus_MakeRequestFrame(channel->txBuf,request);
	if(channel->txLength<0)return channel->txLength;
	//Transmit data
	if(0!=Modbus_TransmitData(channel))return -1;

	channel->lastRequest=*request;
	channel->txLength=0;
	channel->state=Requested;
	return 0;
}


int16_t Modbus_DecodeFrame(ModbusChannelTypeDef *channel){
	uint8_t _slaveid,_byteCnter;
	uint8_t _addr;
	ModbusFunCodeTypeDef _funcCode;
	if(channel->rxLength<3)return -1;
	_slaveid=*(channel->rxBuf);
	_funcCode=*(channel->rxBuf+1);
	_byteCnter=*(channel->rxBuf+2);

	if(_slaveid!=channel->lastRequest.slaveId)return -1;
	if(_funcCode!=channel->lastRequest.funCode)return-1;

	if((_funcCode==ReadHoldRegister)||
			(_funcCode==ReadCoilStatus)||
			(_funcCode==ReadInputStatus)||
			(_funcCode==ReadInputRegister)){
		//Check CRC here
		if(channel->rxLength<(_byteCnter+5))return -2;

		if((_funcCode==ReadHoldRegister)||
				(_funcCode==ReadInputRegister)){
			memcpy(channel->lastRequest.dataPtr,channel->rxBuf+3,_byteCnter);
		}else{
			//Check coil bit offset
		}

	}
	if((_funcCode==WriteSingleCoilStatu)||
			(_funcCode==WriteSingleHoldRegister)||
			(_funcCode==WriteMuiltyCoilStatus)||
			(_funcCode==WriteMuiltyHoldRegister)){
		if(channel->rxLength<(6))return -2;
		_addr=((uint16_t)(*(channel->rxBuf+2))<<8)
				|((uint16_t)(*(channel->rxBuf+3))&0xFF);
		if(_addr!=channel->lastRequest.baseAddr)return -3;
	}
	channel->rxLength=0;
	channel->state=Replied;
	return _byteCnter;
}

void Modbus_ReceiveBytes(ModbusChannelTypeDef *channel,
		uint8_t *data,int16_t len){
	int16_t _rxLen=channel->rxLength+len;
	if(_rxLen>MODBUS_RXBUF_SIZE){
		len-=_rxLen-MODBUS_RXBUF_SIZE;
		channel->rxOverFlow=1;
	}
	memcpy(channel->rxBuf+channel->rxLength,
			data,len);
	channel->rxOverFlow=0;
}
