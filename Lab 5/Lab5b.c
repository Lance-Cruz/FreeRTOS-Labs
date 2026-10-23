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


static void countDownTask(void * pvParameters);
static void buttonTask(void * pvParameters);
SemaphoreHandle_t buttonSemaphore = NULL;

// _write function used for printf
int _write(int file, char *ptr, int len) {
	HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
	return len;
}


void userApp() {
	printf("Starting application\r\n\n");

	xTaskCreate(countDownTask, "Count down task", 200, NULL, 2, NULL);
	xTaskCreate(buttonTask, "Button task", 200, NULL, 1, NULL);
	buttonSemaphore = xSemaphoreCreateBinary();
	vQueueAddToRegistry(buttonSemaphore, "Button Semaphore");
	vTaskStartScheduler();

	while(1) {
	}
}

void countDownTask(void * pvParameters) {
	uint16_t timerCount = 30;
	printf("Starting count down task\r\n\n");
	printf("Time: %02d\r\n", timerCount);
	while(1) {
		vTaskDelay(pdMS_TO_TICKS(1000));
		if(timerCount >= 0) {
			--timerCount;

			if(timerCount <= 10) {
				HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
			}
			if (timerCount == 0) {
				vTaskSuspend(NULL);
			}

		}
		if(xSemaphoreTake(buttonSemaphore, 0) == pdTRUE) {
			printf("Semaphore taken, resetting time\r\n");
			timerCount = 30;
		}
		printf("Time: %02d\r\n", timerCount);
	}
}

void buttonTask(void * pvParameters) {
	printf("Starting button task\r\n");
	while(1) {
		if(HAL_GPIO_ReadPin(BUTTON_EXTI13_GPIO_Port, BUTTON_EXTI13_Pin) == 0 ){
			xSemaphoreGive(buttonSemaphore);
			printf("Button pressed, giving semaphore\r\n");
			while(HAL_GPIO_ReadPin(BUTTON_EXTI13_GPIO_Port, BUTTON_EXTI13_Pin) == 0);
		}
		vTaskDelay(pdMS_TO_TICKS(100));
	}
}


