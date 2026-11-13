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


static void buttonTask(void * pvParameters);
static void displayTask(void * pvParameters);
QueueHandle_t dataQueue = NULL;
uint8_t ch;

typedef struct {
	uint8_t dataSource;
	uint32_t dataValue;
} dataStruct;

#define buttonSource 1
#define temperatureSource 2
#define humiditySource 3


// _write function used for printf
int _write(int file, char *ptr, int len) {
	HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
	return len;
}

//UART interrupt handler
void HAL_UART_RxCpltCallback (UART_HandleTypeDef * huart) {
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	dataStruct sensorStruct;
	if(huart->Instance == USART1 ) {
		switch(ch) {
		case 't':
			sensorStruct.dataSource = temperatureSource;
			sensorStruct.dataValue = rand()%31;
			printf("Character t received, sending temperature to queue\r\n");
			xQueueSendFromISR(dataQueue, &sensorStruct, &xHigherPriorityTaskWoken);
			portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
			break;
		case 'h':
			sensorStruct.dataSource = humiditySource;
			sensorStruct.dataValue = 50 + rand()%51;
			printf("Character h received, sending humidity to queue\r\n");
			xQueueSendFromISR(dataQueue, &sensorStruct, &xHigherPriorityTaskWoken);
			portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
			break;
		default:
			printf("Invalid character, press 't' or 'h'.\r\n");
		}
		HAL_UART_Receive_IT(&huart1, &ch, 1); //IMPORTANT
	}
}

void userApp() {
	printf("Starting application\r\n\n");

	xTaskCreate(buttonTask, "Button Task", 200, NULL, 1, NULL);
	xTaskCreate(displayTask, "Display Task", 200, NULL, 2, NULL);
	dataQueue = xQueueCreate(5, sizeof(dataStruct));
	vQueueAddToRegistry(dataQueue, "Data Queue");
	HAL_UART_Receive_IT(&huart1, &ch, 1);
	vTaskStartScheduler();

	while(1) {
	}
}

void buttonTask(void * pvParameters) {
	dataStruct buttonStruct;
	printf("Starting Button task\r\n\n");
	while(1) {
		if(HAL_GPIO_ReadPin(BUTTON_EXTI13_GPIO_Port, BUTTON_EXTI13_Pin) == 0) {
			buttonStruct.dataSource = buttonSource;
			buttonStruct.dataValue = xTaskGetTickCount()*1000/configTICK_RATE_HZ;
			printf("Switch pressed, sending %lu to queue\r\n", buttonStruct.dataValue);
			xQueueSend(dataQueue, &buttonStruct, 0);
		}
		vTaskDelay(pdMS_TO_TICKS(100));
	}
}

void displayTask(void * pvParameters) {
	dataStruct rxStruct;
	printf("Starting Display task\r\n\n");
	while(1) {
		if(xQueueReceive(dataQueue, &rxStruct, portMAX_DELAY) == pdPASS) {
			printf("Data received in queue\r\n");
			switch(rxStruct.dataSource) {
			case buttonSource:
				printf("Switch pressed at %lums\r\n\n", rxStruct.dataValue);
				break;

			case temperatureSource:
				printf("Temperature Received: %luC\r\n\n", rxStruct.dataValue);
				break;

			case humiditySource:
				printf("Humidity Received: %lu%%\r\n\n", rxStruct.dataValue);
				break;
			}
		}
	}
}


