/*
 * userApp.c
 *
 *  Created on: Dec 8, 2023
 *      Author: Niall.OKeeffe@atu.ie
 */

#include "userApp.h"
#include <stdio.h>

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

void userApp() {
	printf("Starting application\r\n\n");

	xTaskCreate(timerTask, "Timer Task", 200, NULL, 2, NULL);
	timerSemaphore = xSemaphoreCreateBinary();
	HAL_TIM_Base_Start_IT(&htim6);
	vTaskStartScheduler();

	while(1) {
	}
}

void timerTask(void * pvParameters) {
	uint8_t sec = 0;
	printf("Starting timer task \r\n\n");
	printf("Timer: %02d\r\n", sec);
	while(1) {
		if(xSemaphoreTake(timerSemaphore, portMAX_DELAY) == pdTRUE) {
			sec++;
			printf("Timer: %02d\r\n", sec);
		}
	}
}


