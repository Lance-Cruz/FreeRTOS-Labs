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
static void UART_Task(void * pvParameters);
SemaphoreHandle_t buttonSemaphore = NULL, UART_Semaphore = NULL;

// _write function used for printf
int _write(int file, char *ptr, int len) {
	HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
	return len;
}


void userApp() {
	printf("Starting application\r\n\n");

	xTaskCreate(countDownTask, "Count down task", 200, NULL, 3, NULL);
	xTaskCreate(buttonTask, "Button task", 200, NULL, 2, NULL);
	xTaskCreate(UART_Task, "UART task", 200, NULL, 1, NULL);
	buttonSemaphore = xSemaphoreCreateBinary();
	UART_Semaphore = xSemaphoreCreateBinary();
	vQueueAddToRegistry(buttonSemaphore, "Button Semaphore");
	vTaskStartScheduler();

	while(1) {
	}
}

void countDownTask(void * pvParameters) {
	uint16_t timerCount = 30, timerRunning = 1;
	printf("Starting count down task\r\n\n");
	printf("Time: %02d\r\n", timerCount);
	while(1) {
		vTaskDelay(pdMS_TO_TICKS(1000));
		if(timerCount > 0 && timerRunning == 1) {
			--timerCount;

			if(timerCount <= 10) {
				HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
			}
			if (timerCount == 0) {
				vTaskSuspend(NULL);
			}

			printf("Time: %02d\r\n", timerCount);
		}
		if(xSemaphoreTake(buttonSemaphore, 0) == pdTRUE) {
			printf("Semaphore taken, resetting time\r\n");
			timerCount = 30;
		}
		if(xSemaphoreTake(UART_Semaphore, 0) == pdTRUE){
			if (timerRunning == 1) {
				printf("Semaphore taken, pausing timer at %02d\r\n", timerCount);
				timerRunning = 0;
			} else {
				printf("Semaphore taken, resuming timer\r\n");
				timerRunning = 1;
			}

		}

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

void UART_Task(void * pvParameters) {
	printf("Starting UART task\r\n");
	while(1) {
		uint8_t ch;
		if(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE) != 0) {
			HAL_UART_Receive(&huart1, &ch, 1, 100);
			if(ch == 'p') {
				printf("Character 'p' received, giving semaphore\r\n");
				xSemaphoreGive(UART_Semaphore);
			}
			else {
				printf("Invalid option, enter p to pause or resume the timer\r\n\n");
			}
		}
	}
}


