/*
 * adxl362.h
 *
 *  Created on: 2019年8月21日
 *      Author: Sociya
 */
#ifndef ADXL362_H
#define ADXL362_H

#include "stm32l0xx_hal.h"


void Init_ADXL362();
int16_t Act_ADXL362_ReadData();
int16_t ADXL362_GetGravity(uint16_t channel);
#endif
