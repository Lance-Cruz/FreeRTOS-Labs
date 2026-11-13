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
#include "queue.h"

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


static void buttonTask(void * pvParameters);
static void displayTask(void * pvParameters);
QueueHandle_t buttonQueue = NULL;


// _write function used for printf
int _write(int file, char *ptr, int len) {
	HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
	return len;
}


void userApp() {
	printf("Starting application\r\n\n");

	xTaskCreate(buttonTask, "Button Task", 200, NULL, 1, NULL);
	xTaskCreate(displayTask, "Display Task", 200, NULL, 2, NULL);
	buttonQueue = xQueueCreate(5, sizeof(uint32_t));
	vQueueAddToRegistry(buttonQueue, "Button Queue");
	vTaskStartScheduler();

	while(1) {
	}
}

void buttonTask(void * pvParameters) {
	uint32_t pressTime;
	printf("Starting Button task\r\n\n");
	while(1) {
		if(HAL_GPIO_ReadPin(BUTTON_EXTI13_GPIO_Port, BUTTON_EXTI13_Pin) == 0) {
			pressTime = xTaskGetTickCount()*1000/configTICK_RATE_HZ;
			printf("Switch pressed, sending %lu to queue\r\n\n", pressTime);
			xQueueSend(buttonQueue, &pressTime, 0);
		}
		vTaskDelay(pdMS_TO_TICKS(100));
	}
}

void displayTask(void * pvParameters) {
	uint32_t rxTime;
	printf("Starting Display task\r\n\n");
	while(1) {
		if(xQueueReceive(buttonQueue, &rxTime, portMAX_DELAY) == pdPASS);
		printf("Data received in queue, switch pressed at %lums\r\n\n", rxTime);
	}
}


