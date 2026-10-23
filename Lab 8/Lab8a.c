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
TaskHandle_t timerTaskHandle = NULL;
uint8_t ch;

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
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	if(huart->Instance == USART1 ) {
		switch(ch) {
		case 'r':
			vTaskNotifyGiveFromISR(timerTaskHandle, &xHigherPriorityTaskWoken);
			break;
		default:
			printf("Invalid character, press 'r'.\r\n");
			break;
		}
		HAL_UART_Receive_IT(&huart1, &ch, 1); //IMPORTANT
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}
}

void userApp() {
	printf("Starting application\r\n\n");

	xTaskCreate(timerTask, "Timer Task", 200, NULL, 2, &timerTaskHandle);
	timerSemaphore = xSemaphoreCreateBinary();
	HAL_TIM_Base_Start_IT(&htim6);
	HAL_UART_Receive_IT(&huart1, &ch, 1);
	vTaskStartScheduler();

	while(1) {
	}
}

void timerTask(void * pvParameters) {
	uint32_t TNV;
	uint8_t sec = 0;
	printf("Starting timer task \r\n\n");
	printf("Timer: %02d\r\n", sec);
	while(1) {
		if(xSemaphoreTake(timerSemaphore, pdMS_TO_TICKS(50)) == pdTRUE) {
			sec++;
			printf("Timer: %02d\r\n", sec);
		}

		TNV = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(50));
		if(TNV != 0){
			HAL_TIM_Base_Stop_IT(&htim6);
			sec = 0;
			TIM6->CNT = 0;
			HAL_TIM_Base_Start_IT(&htim6);
			printf("Resetting timer to: %02d\r\n", sec);
		}
	}
}


