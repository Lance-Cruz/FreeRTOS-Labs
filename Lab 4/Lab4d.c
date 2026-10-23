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


static void SW_Task(void * pvParameters);
static void LED_Task(void * pvParameters);
static void Timer_Task(void * pvParameters);
SemaphoreHandle_t buttonSemaphore = NULL;
SemaphoreHandle_t timerSemaphore = NULL;

// _write function used for printf
int _write(int file, char *ptr, int len) {
	HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
	return len;
}


void userApp() {
	printf("Starting application\r\n\n");

	xTaskCreate(SW_Task, "Switch Task", 200, NULL, 2, NULL);
	xTaskCreate(LED_Task, "LED Task", 200, NULL, 4, NULL);
	xTaskCreate(Timer_Task, "Timer Task", 200, NULL, 3, NULL);
	buttonSemaphore = xSemaphoreCreateBinary();
	timerSemaphore = xSemaphoreCreateBinary();
	vQueueAddToRegistry(buttonSemaphore, "Button Semaphore");
	vQueueAddToRegistry(timerSemaphore, "Timer Semaphore");
	vTaskStartScheduler();

	while(1) {
	}
}

void SW_Task(void * pvParameters) {
	printf("Starting switch task\r\n");
	while(1) {
		// check for a switch
		if(HAL_GPIO_ReadPin(BUTTON_EXTI13_GPIO_Port, BUTTON_EXTI13_Pin) == 0) {
			//give semaphore
			xSemaphoreGive(buttonSemaphore);
			printf("Switch pressed, giving semaphore...\r\n");
			while(HAL_GPIO_ReadPin(BUTTON_EXTI13_GPIO_Port, BUTTON_EXTI13_Pin) == 0);
		}
		vTaskDelay(pdMS_TO_TICKS(100));
	}
}

void LED_Task(void * pvParameters) {
	uint16_t timerCount = 0;
	printf("Starting LED task \r\n");
	printf("Time: %03d seconds\r\n", timerCount);
	while(1) {
		//printf("LED task waiting for semaphore\r\n");

		if(xSemaphoreTake(buttonSemaphore, 0) == pdTRUE) {
			printf("Semaphore taken, toggling LED...\r\n");
			HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
		}

		if(xSemaphoreTake(timerSemaphore, 0) == pdTRUE) {
			printf("Time: %03d seconds\r\n", ++timerCount);
		}
		vTaskDelay(pdMS_TO_TICKS(100)); // Other method would be to delay the xSemaphoreTake for 10ms and comment this out
	}
}

void Timer_Task(void * pvParameters) {
	printf("Starting timer task \r\n");
	while(1) {
		vTaskDelay(pdMS_TO_TICKS(1000));
		xSemaphoreGive(timerSemaphore);
	}
}
