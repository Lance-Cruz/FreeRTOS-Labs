/*
 * userApp.c
 *
 *  Created on: Dec 8, 2023
 *      Author: Niall.OKeeffe@atu.ie
 */

#include "userApp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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


static void displayTask(void * pvParameters);
QueueHandle_t dataQueue = NULL;
uint8_t ch;


// _write function used for printf
int _write(int file, char *ptr, int len) {
	HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
	return len;
}

//UART interrupt handler
void HAL_UART_RxCpltCallback (UART_HandleTypeDef * huart) {
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	static char buffer[50] = "0";
	static uint8_t x = 0;
	if(huart->Instance == USART1 ) {
		if(ch == '\r') {
			printf("String received: %s\r\n", buffer);
			printf("Sending string to queue\r\n\n");
			xQueueSendFromISR(dataQueue, buffer, &xHigherPriorityTaskWoken);
			portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
			x = 0;
			memset(buffer, 0, sizeof(buffer));
		}
		else {
			buffer[x++] = ch;
		}
	}
	HAL_UART_Receive_IT(&huart1, &ch, 1);
}

void userApp() {
	printf("Starting application\r\n\n");

	xTaskCreate(displayTask, "Display Task", 200, NULL, 2, NULL);
	dataQueue = xQueueCreate(5, 50*sizeof(char));
	vQueueAddToRegistry(dataQueue, "Data Queue");
	HAL_UART_Receive_IT(&huart1, &ch, 1);
	vTaskStartScheduler();

	while(1) {
	}
}

void displayTask(void * pvParameters) {
	char rxBuffer[50] = "0";
	char * ptr;
	uint8_t humidity, temperature;
	uint16_t pressure;
	printf("Starting Display task\r\n\n");
	while(1) {
		if(xQueueReceive(dataQueue, &rxBuffer, portMAX_DELAY) == pdPASS) {
			printf("String received: %s \r\n\n", rxBuffer);
			ptr = strstr(rxBuffer, "Humidity");
			if(ptr) {
				ptr = strchr(ptr, ':');
				if(ptr) {
					ptr++;
					humidity = atoi(ptr);
					printf("Humidity Sensor: %d%%\r\n\n", humidity);
				}
			}

			ptr = strstr(rxBuffer, "Temperature");
			if(ptr) {
				ptr = strchr(ptr, ':');
				if(ptr) {
					ptr++;
					temperature = atoi(ptr);
					printf("Temperature Sensor: %dC\r\n\n", temperature);
				}
			}

			ptr = strstr(rxBuffer, "Pressure");
			if(ptr) {
				ptr = strchr(ptr, ':');
				if(ptr) {
					ptr++;
					pressure = atoi(ptr);
					printf("Pressure Sensor: %dmBar\r\n\n", pressure);
				}
			}
		}
	}
}


