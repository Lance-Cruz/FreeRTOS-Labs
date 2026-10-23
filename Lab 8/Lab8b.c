/*
 * userApp.c
 *
 *  Created on: Dec 8, 2023
 *      Author: Niall.OKeeffe@atu.ie
 */

#include "userApp.h"
#include <stdio.h>
#include <stdlib.h>

//Add the relevant FreeRTOS header files here
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

//--------------------------------------------------------------
//used for real time stats, do not delete code from this section
extern TIM_HandleTypeDef htim7;
extern volatile unsigned long ulHighFrequencyTimerTicks;
void configureTimerForRunTimeStats(void)
{
    ulHighFrequencyTimerTicks = 0;
    HAL_TIM_Base_Start_IT(&htim7);
}

unsigned long getRunTimeCounterValue(void)
{
	return ulHighFrequencyTimerTicks;
}
//end of real time stats code
//----------------------------------------------------------------

extern UART_HandleTypeDef huart1;
extern TIM_HandleTypeDef htim6;

static void timerTask(void * pvParameters);
SemaphoreHandle_t timerSemaphore = NULL;
TaskHandle_t timerTaskHandle = NULL;
uint8_t buffer[9] = "0";

// _write function used for printf
int _write(int file, char *ptr, int len) {
	HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
	return len;
}

//TIM6 6 interrupts handler
void TIM6_Handler() {
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR(timerSemaphore, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

//Button interrupt
/*void HAL_GPIO_EXTI_Callback (uint16_t GPIO_Pin) {
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	if(GPIO_Pin == BUTTON_EXTI13_Pin) {
		//give semaphore
		xSemaphoreGiveFromISR(resetSemaphore, &xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}
}*/

//UART interrupt handler
void HAL_UART_RxCpltCallback (UART_HandleTypeDef * huart) {
	uint8_t hr, min, sec, errorFlag = 0;
	uint32_t seconds;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	if(huart->Instance == USART1 ) {
		printf("Received: %s\r\n", buffer);
		//Error checking
		for(uint8_t x =0; x<8; x++) {
			if(x== 2 || x ==5) {
				if (buffer[x] != ':' ){
					errorFlag = 1;
					break;
				}
			}
			else {
				if(buffer[x] < '0' || buffer[x] > '9') {
					errorFlag = 1;
					break;
				}
			}
		}

		if(errorFlag == 0) {
			hr = atoi((char *)buffer);
			min = atoi((char *)&buffer[3]);
			sec = atoi((char *)&buffer[6]);
			if(hr<24 && min<60 && sec<60) {
				seconds = hr*3600 + min*60 + sec;
				xTaskNotifyFromISR(timerTaskHandle, seconds, eSetValueWithOverwrite, &xHigherPriorityTaskWoken);
			}
			else {
				printf("\r\n\nInvalid entry, time value out of range\r\n\n");
			}
		}
		else {
			printf("\r\n\nInvalid entry, use format hh:mm:ss\r\n\n");
		}
		HAL_UART_Receive_IT(&huart1, buffer, 8); //IMPORTANT
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}
}

void userApp() {
	printf("Starting application\r\n\n");

	xTaskCreate(timerTask, "Timer Task", 200, NULL, 2, &timerTaskHandle);
	timerSemaphore = xSemaphoreCreateBinary();
	HAL_TIM_Base_Start_IT(&htim6);
	HAL_UART_Receive_IT(&huart1, buffer, 8);
	vTaskStartScheduler();

	while(1) {
	}
}

void timerTask(void * pvParameters) {
	uint32_t TNV;
	uint32_t sec = 0;
	printf("Starting timer task \r\n\n");
	printf("Time %02lu:%02lu:%02lu\r\n", sec/3600, (sec%3600)/60, sec%60);
	while(1) {
		if(xSemaphoreTake(timerSemaphore, pdMS_TO_TICKS(50)) == pdTRUE) {
			sec++;
			printf("Time %02lu:%02lu:%02lu\r\n", sec/3600, (sec%3600)/60, sec%60);
		}

		TNV = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(50));
		if(TNV != 0){
			sec = TNV;
			printf("Time change to %02lu:%02lu:%02lu\r\n", sec/3600, (sec%3600)/60, sec%60);
		}
	}
}


