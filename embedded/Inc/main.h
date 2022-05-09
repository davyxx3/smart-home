/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l0xx_hal.h"

  /* Private includes ----------------------------------------------------------*/
  /* USER CODE BEGIN Includes */

  /* USER CODE END Includes */

  /* Exported types ------------------------------------------------------------*/
  /* USER CODE BEGIN ET */

  /* USER CODE END ET */

  /* Exported constants --------------------------------------------------------*/
  /* USER CODE BEGIN EC */

  extern ADC_HandleTypeDef hadc;

  extern RTC_HandleTypeDef hrtc;

  extern SPI_HandleTypeDef hspi2;

  extern UART_HandleTypeDef huart1;
  extern UART_HandleTypeDef huart2;
  extern UART_HandleTypeDef huart4;
  extern UART_HandleTypeDef huart5;
  extern DMA_HandleTypeDef hdma_usart1_rx;
  extern DMA_HandleTypeDef hdma_usart2_rx;
  extern DMA_HandleTypeDef hdma_usart2_tx;
  extern DMA_HandleTypeDef hdma_usart4_rx;
  extern DMA_HandleTypeDef hdma_usart4_tx;
  extern DMA_HandleTypeDef hdma_usart5_rx;
  /* USER CODE END EC */

  /* Exported macro ------------------------------------------------------------*/
  /* USER CODE BEGIN EM */

  /* USER CODE END EM */

  /* Exported functions prototypes ---------------------------------------------*/
  void Error_Handler(void);

  /* USER CODE BEGIN EFP */
  void ConsoleLog(const char *str);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define AIN_BAT_Pin GPIO_PIN_5
#define AIN_BAT_GPIO_Port GPIOA
#define VSEN_Pin GPIO_PIN_10
#define VSEN_GPIO_Port GPIOB
#define SPI2_NCS_Pin GPIO_PIN_12
#define SPI2_NCS_GPIO_Port GPIOB
#define UART1_NRE_Pin GPIO_PIN_11
#define UART1_NRE_GPIO_Port GPIOA
#define PWR1_EN_Pin GPIO_PIN_15
#define PWR1_EN_GPIO_Port GPIOA
#define UART5_NRE_Pin GPIO_PIN_6
#define UART5_NRE_GPIO_Port GPIOB
#define PWR2_EN_Pin GPIO_PIN_7
#define PWR2_EN_GPIO_Port GPIOB
#define BRESET_Pin GPIO_PIN_8
#define BRESET_GPIO_Port GPIOB
  /* USER CODE BEGIN Private defines */

  /* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
