/*
 * userApp.c
 *
 *  Created on: Aug 28, 2025
 *      Author: Niall.OKeeffe@atu.ie
 */

#include "main.h"
#include "userApp.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

extern UART_HandleTypeDef huart1;
static void task1(void * pvParameters);
static void task2(void * pvParameters);

int _write(int file, char *ptr, int len) {
	HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
	return len;
}

void userApp() {
	printf("Starting user application...\r\n\n");
	xTaskCreate(task1, "Task 1", 200, NULL, 2, NULL);
	vTaskStartScheduler();
	while(1) {

	}
}

static void task1(void * pvParameters) {
	uint8_t i;
	printf("Starting task 1...\r\n");
	printf("Task1 Running...\r\n\n");
	while(1) {
		for (i = 0; i < 6; i++) {
			HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
			vTaskDelay(pdMS_TO_TICKS(500));
		}

		//Create task 2
		printf("Creating task 2...\r\n");
		xTaskCreate(task2, "Task 2", 200, NULL, 1, NULL);

		//Suspend task 1
		printf("Suspending task 1...\r\n\n");
		vTaskSuspend( NULL );
	}
}

static void task2(void * pvParameters) {
	printf("Starting task 2...\r\n");
	while(1) {
		HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
		printf("Task2 Running...\r\n");
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}
