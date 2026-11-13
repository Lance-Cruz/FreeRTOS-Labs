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


static void temperatureReadTask(void * pvParameters);
static void displayTask(void * pvParameters);
QueueHandle_t temperatureQueue = NULL;


// _write function used for printf
int _write(int file, char *ptr, int len) {
	HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
	return len;
}


void userApp() {
	printf("Starting application\r\n\n");

	xTaskCreate(temperatureReadTask, "Temperature Task", 200, NULL, 1, NULL);
	xTaskCreate(displayTask, "Display Task", 200, NULL, 2, NULL);
	temperatureQueue = xQueueCreate(5, sizeof(uint8_t *));
	vQueueAddToRegistry(temperatureQueue, "Temperature Queue");
	vTaskStartScheduler();

	while(1) {
	}
}

void temperatureReadTask(void * pvParameters) {
	uint8_t tempBuffer[10], x, *tempPtr;
	printf("Starting Temperature task\r\n\n");
	while(1) {
		//fill the array with 10 temperatures between 15 and 30 degrees
		//using a for loop
		//print 10 temperature values
		printf("\nTemperature Sensors: ");
		for(x = 0; x < 10; x++) {
			tempBuffer[x] = 15 + rand()%16;
			printf("%dC ", tempBuffer[x]);
		}
		tempPtr = tempBuffer;
		printf("\r\nSending array reference to queue\r\n");
		xQueueSend(temperatureQueue, &tempPtr, 0);
		vTaskDelay(pdMS_TO_TICKS(2000));
	}
}

void displayTask(void * pvParameters) {
	uint8_t x, *rxPtr;
	printf("Starting Display task\r\n\n");
	while(1) {
		if(xQueueReceive(temperatureQueue, &rxPtr, portMAX_DELAY) == pdPASS) {
			printf("Temperature values received from queue\r\n");
			// print 10 temperature values
			for(x = 0; x < 10; x++) {
				printf("Sensor %d: %dC\r\n", x, *(rxPtr+x));
			}
		}
	}
}


