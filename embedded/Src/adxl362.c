/*
 * adxl362.c
 *
 *  Created on: 2019年8月21日
 *      Author: Sociya
 *  @brief:For ADXL362 synchronization
 */

#include "adxl362.h"
#include "main.h"
#include "stdio.h"
#include "string.h"

#define ADXL362_SPI_PORT	(hspi2)
#define ADXL362_DEBUG		1

typedef struct __ADXL362RegStaTypeDef{
	uint8_t err_user_regs:1;
	uint8_t awake:1;
	uint8_t inact:1;
	uint8_t act:1;
	uint8_t fifo_overrun:1;
	uint8_t fifo_watermark:1;
	uint8_t fifo_ready:1;
	uint8_t data_ready:1;
}ADXL362RegStaTypeDef;
typedef union __ADXL362RegStaUTypeDef{
	ADXL362RegStaTypeDef bit;
	uint8_t all;
}ADXL362RegStaUTypeDef;

typedef struct __ADXL362RegActInactCtlTypeDef{
	uint8_t res:2;
	uint8_t linkloop:2;
	uint8_t inact_ref:1;
	uint8_t inact_en:1;
	uint8_t act_ref:1;
	uint8_t act_en:1;
}ADXL362RegActInactCtlTypeDef;

typedef union __ADXL362RegActInactCtlUTypeDef{
	ADXL362RegActInactCtlTypeDef bit;
	uint8_t all;
}ADXL362RegActInactCtlUTypeDef;

typedef struct __ADXL362RegFifoCtlTypeDef{
	uint8_t unused:4;
	uint8_t ah:1;
	uint8_t fifo_temp:1;
	uint8_t fifo_mode:2;
}ADXL362RegFifoCtlTypeDef;
typedef union __ADXL362RegFifoCtlTypeUDef{
	ADXL362RegFifoCtlTypeDef bit;
	uint8_t all;
}ADXL362RegFifoCtlTypeUDef;
typedef struct __ADXL362RegIntmapTypeDef{
	uint8_t int_low:1;
	uint8_t awake:1;
	uint8_t inact:1;
	uint8_t act:1;
	uint8_t fifo_overrun:1;
	uint8_t fifo_watermark:1;
	uint8_t fifo_ready:1;
	uint8_t data_ready:1;
}ADXL362RegIntmapTypeDef;
typedef union __ADXL362RegIntmapUTypeDef{
	ADXL362RegIntmapTypeDef bit;
	uint8_t all;
}ADXL362RegIntmapUTypeDef;
typedef struct __ADXL362RegFilterCtlTypeDef{
	uint8_t range:2;
	uint8_t res:1;
	uint8_t half_bw:1;
	uint8_t ext_sample:1;
	uint8_t odr:3;
}ADXL362RegFilterCtlTypeDef;

typedef union __ADXL362RegFilterCtlUTypeDef{
	ADXL362RegFilterCtlTypeDef bit;
	uint8_t all;
}ADXL362RegFilterCtlUTypeDef;
typedef struct __ADXL362RegPowerCtlTypeDef{
	uint8_t res:1;
	uint8_t ext_clk:1;
	uint8_t low_noise:2;
	uint8_t wakeup:1;
	uint8_t autosleep:1;
	uint8_t measure:2;
}ADXL362RegPowerCtlTypeDef;

typedef union __ADXL362RegPowerCtlUTypeDef{
	ADXL362RegPowerCtlTypeDef bit;
	uint8_t all;
}ADXL362RegPowerCtlUTypeDef;
typedef struct __ADXL362RegTypeDef{
	uint8_t	devid_ad;
	uint8_t	devid_mst;
	uint8_t partid;
	uint8_t revid;
	uint8_t xdata;
	uint8_t ydata;
	uint8_t zdata;
	ADXL362RegStaUTypeDef status;
	uint8_t fifo_entries_l;
	uint8_t resvd0:6;
	uint8_t fifo_entries_h:2;
	uint8_t xdata_l;//May use continued uint16_t data?
	uint8_t resvd1:4;
	uint8_t xdata_h:4;
	uint8_t ydata_l;
	uint8_t resvd2:4;
	uint8_t ydata_h:4;
	uint8_t zdata_l;
	uint8_t resvd3:4;
	uint8_t zdata_h;
	uint8_t temp_l;
	uint8_t resvd4:4;
	uint8_t temp_h;
	uint16_t resvd5;
	uint8_t soft_reset;
	uint8_t thresh_act_l;
	uint8_t resvd6:5;
	uint8_t thresh_act_h:3;
	uint8_t time_act;
	uint8_t thresh_inact_l;
	uint8_t resvd7:5;
	uint8_t thresh_inact_h:3;
	uint8_t time_inact_l;
	uint8_t time_incat_h;
	ADXL362RegActInactCtlUTypeDef act_inact_ctl;
	ADXL362RegFifoCtlTypeUDef fifo_control;
	uint8_t fifo_samples;
	ADXL362RegIntmapUTypeDef intmap1;
	ADXL362RegIntmapUTypeDef intmap2;
	ADXL362RegFilterCtlUTypeDef filter_ctl;
	ADXL362RegPowerCtlUTypeDef power_ctl;
	uint8_t resvd8:7;
	uint8_t st:1;
}ADXL362RegTypeDef;

typedef union __ADXL362RegUTypeDef{
	ADXL362RegTypeDef bit;
	uint8_t byte[0x2F];
}ADXL362RegTypeUDef;


ADXL362RegTypeUDef g_ADXL362Reg;

void Init_ADXL362(){
	uint8_t _writeBuf[3],_readBuf[3];
	_writeBuf[0]=0x0B;
	_writeBuf[1]=0x2D;
	_writeBuf[2]=0x00;

	_readBuf[0]=0x00;
	_readBuf[1]=0x00;
	_readBuf[2]=0x00;

	//Reset device
	HAL_GPIO_WritePin(VSEN_GPIO_Port, VSEN_Pin, GPIO_PIN_RESET);
	HAL_Delay(100);
	HAL_GPIO_WritePin(VSEN_GPIO_Port, VSEN_Pin, GPIO_PIN_SET);
	HAL_Delay(100);
	//Read Power control register
	HAL_GPIO_WritePin(SPI2_NCS_GPIO_Port,SPI2_NCS_Pin,GPIO_PIN_RESET);
	HAL_Delay(1);
	HAL_SPI_TransmitReceive(&ADXL362_SPI_PORT,_writeBuf,_readBuf,3,1000);
	HAL_Delay(1);
	HAL_GPIO_WritePin(SPI2_NCS_GPIO_Port,SPI2_NCS_Pin,GPIO_PIN_SET);
	//Write power control register (turn on)
	_writeBuf[0]=0x0A;
	_writeBuf[2]=_readBuf[2]|0x02;
	HAL_Delay(100);
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_12,GPIO_PIN_RESET);
	HAL_Delay(1);
	HAL_SPI_TransmitReceive(&hspi2,_writeBuf,_readBuf,3,1000);
	HAL_Delay(1);
	HAL_GPIO_WritePin(SPI2_NCS_GPIO_Port,SPI2_NCS_Pin,GPIO_PIN_SET);
}
int16_t ADXL362_ReadByte(uint8_t *dest,uint8_t addr){
  uint8_t _writeBuf[3],_readBuf[3];
  HAL_StatusTypeDef _transmitSta;

  _writeBuf[0]=0x0B;
  _writeBuf[1]=addr;
  _writeBuf[2]=0x00;
  memset(_readBuf,0,3);

//  HAL_GPIO_WritePin(GPIOB,GPIO_PIN_12,GPIO_PIN_RESET);

  _transmitSta=HAL_SPI_TransmitReceive(&hspi2,_writeBuf,_readBuf,3,1000);
  if(_transmitSta!=HAL_OK){
	  *dest=_readBuf[2];
	  return 1;
  }else{
	  if(_transmitSta==HAL_BUSY){

	  }else if(_transmitSta==HAL_ERROR){

	  }else if(_transmitSta==HAL_TIMEOUT){

	  }
	  return -1;
  }

//  HAL_GPIO_WritePin(GPIOB,GPIO_PIN_12,GPIO_PIN_SET);
  return 0;
}
int16_t ADXL362_WriteByte(uint8_t addr, uint8_t val){
	uint8_t _writeBuf[3],_readBuf[3];
	HAL_StatusTypeDef _transmitSta;
	_writeBuf[0]=0x0A;
	_writeBuf[1]=addr;
	_writeBuf[2]=val;

	memset(_readBuf,0,3);

//	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_12,GPIO_PIN_RESET);
	_transmitSta=HAL_SPI_TransmitReceive(&hspi2,_writeBuf,_readBuf,3,1000);
	if(_transmitSta!=HAL_OK){

		  return 1;
	  }else{
		  if(_transmitSta==HAL_BUSY){

		  }else if(_transmitSta==HAL_ERROR){

		  }else if(_transmitSta==HAL_TIMEOUT){

		  }
		  return -1;
	  }

//	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_12,GPIO_PIN_SET);
}
int16_t Act_ADXL362_ReadData(){
	HAL_StatusTypeDef _sta;
	uint8_t _writeBuf[0x2F+2]={0x0B,0x00};
	uint8_t _readBuf[0x2F+2];
	char _logChar[128];
	memset(_readBuf,0,0x2F+2);

	HAL_GPIO_WritePin(SPI2_NCS_GPIO_Port,SPI2_NCS_Pin,GPIO_PIN_RESET);
	HAL_Delay(1);
	_sta=HAL_SPI_TransmitReceive(&hspi2,_writeBuf,_readBuf,0x30,1000);
	HAL_Delay(1);
	HAL_GPIO_WritePin(SPI2_NCS_GPIO_Port,SPI2_NCS_Pin,GPIO_PIN_SET);
	if(_sta==HAL_OK){
		memcpy(g_ADXL362Reg.byte,_readBuf+2,0x2F);
	}
	sprintf(_logChar,
			"X:%5d  Y:%5d  Z:%5d",
			ADXL362_GetGravity(0),
			ADXL362_GetGravity(1),
			ADXL362_GetGravity(2));
	Log(_logChar);
	return 0;
}
int16_t ADXL362_GetGravity(uint16_t channel){
	int16_t _result;
	switch(channel){
	case 0:
		_result=(g_ADXL362Reg.byte[0x0F]<<8)+g_ADXL362Reg.byte[0x0E];
		break;
	case 1:
		_result=(g_ADXL362Reg.byte[0x11]<<8)+g_ADXL362Reg.byte[0x10];
		break;
	case 2:
		_result=(g_ADXL362Reg.byte[0x13]<<8)+g_ADXL362Reg.byte[0x12];
		break;
	default:
		_result=0;
		break;
	}
	return _result;
}
