/*
 *
 * modbus_master.h
 *
 *
 *
 * */
#ifndef MODBUS_MASTER_H
#define MODBUS_MASTER_H
#include "stdint.h"
#include "stm32l0xx_hal.h"

/******Macro define begin******/
#define MODBUS_TXBUF_SIZE	300
#define MODBUS_RXBUF_SIZE	300


/******Macro define end******/
/******Type define begin******/
typedef enum{
	ReadCoilStatus				=0x01,
	ReadInputStatus				=0x02,
	ReadHoldRegister			=0x03,
	ReadInputRegister			=0x04,
	WriteSingleCoilStatu		=0x05,
	WriteSingleHoldRegister		=0x06,
	WriteMuiltyCoilStatus		=0x0F,
	WriteMuiltyHoldRegister		=0x10,

	OperationError				=0x80
}ModbusFunCodeTypeDef;

typedef enum{
	Empty,
	Requested,
	Replied,
	Error,
	OverTime
}ModbusStateTypeDef;

typedef struct __ModbusRequestTypeDef{
	uint8_t 				slaveId;
	ModbusFunCodeTypeDef 	funCode;
	uint16_t 				baseAddr;
	uint8_t 				*dataPtr;
	uint16_t 				dataLength;
}ModbusRequestTypeDef;

typedef struct __ModbusChannelTypeDef{
	ModbusStateTypeDef 		state;
	uint16_t 				overTimeMs;
	uint16_t 				passedTimeMs;
	uint16_t 				retryTime;
	uint16_t 				retryCounter;
	uint8_t 				rxOverFlow;
	UART_HandleTypeDef 		*huart;
	ModbusRequestTypeDef 	lastRequest;
	int16_t 				txLength;
	int16_t 				rxLength;
	uint8_t 				txBuf[MODBUS_TXBUF_SIZE];
	uint8_t 				rxBuf[MODBUS_RXBUF_SIZE];
}ModbusChannelTypeDef;
/******Type define end******/

/******Function declaration begin******/
int16_t Modbus_SendRequest(ModbusChannelTypeDef *channel,
		uint8_t slave_id,
		ModbusFunCodeTypeDef fun_code,
		uint16_t address,
		uint16_t *data_ptr,
		uint16_t len);
int16_t Modbus_DecodeFrame(ModbusChannelTypeDef *channel);
void Modbus_ReceiveBytes(ModbusChannelTypeDef *channel,
		uint8_t *data,int16_t len);

/******Function declaration end******/

#endif
