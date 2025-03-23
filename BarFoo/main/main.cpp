#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_system.h"

#define UART_NUM UART_NUM_0
#define BUF_SIZE 1024
static const char *TAG = "FooBar";

QueueHandle_t numberQueue;
SemaphoreHandle_t countCompleteSemaphore;
int currentNumber = 0;
bool processing = false;

bool isPrime(int num) 
{
    if (num < 2) 
		return false;
    for (int i = 2; i * i <= num; i++) 
    {
        if (num % i == 0) return false;
    }
    return true;
}

void fooTask(void *param) 
{
    while (true) 
    {
        if (processing && (currentNumber % 2 == 0 || currentNumber == 0)) 
        {
            ESP_LOGI(TAG, "Foo %d%s", currentNumber, isPrime(currentNumber) && currentNumber > 1 ? " Prime" : "");
            currentNumber--;
            vTaskDelay(pdMS_TO_TICKS(1000));
            xSemaphoreGive(countCompleteSemaphore);
        }
        vTaskDelay(10);
    }
}

void barTask(void *param) 
{
    while (true) 
    {
        if (processing && currentNumber % 2 != 0 && currentNumber >= 0) 
        {
            ESP_LOGI(TAG, "Bar %d%s", currentNumber, isPrime(currentNumber) ? " Prime" : "");
            currentNumber--;
            vTaskDelay(pdMS_TO_TICKS(1000));
            xSemaphoreGive(countCompleteSemaphore);
        }
        vTaskDelay(10);
    }
}

void serialTask(void *param) 
{
    uint8_t data[BUF_SIZE];
    while (true) 
    {
        int len = uart_read_bytes(UART_NUM, data, BUF_SIZE - 1, pdMS_TO_TICKS(100));
        if (len > 0) 
        {
            data[len] = '\0';
            int receivedNum = atoi((char*)data);
            if (receivedNum == 0) 
            {
                ESP_LOGI(TAG, "Restarting ESP32...");
                esp_restart();
            }

            if (uxQueueMessagesWaiting(numberQueue) < 8) 
            {
                xQueueSend(numberQueue, &receivedNum, portMAX_DELAY);
                ESP_LOGI(TAG, "Received %d", receivedNum);
            } 
            else
            {
                ESP_LOGI(TAG, "Buffer is full");
            }
        }
    }
}

void countTask(void *param) 
{
    int num;
    while (true) 
    {
        if (xQueueReceive(numberQueue, &num, portMAX_DELAY)) 
        {
            currentNumber = num;
            processing = true;
            while (currentNumber >= 0) 
            {
                xSemaphoreTake(countCompleteSemaphore, portMAX_DELAY);
            }
            processing = false;
        }
    }
}

extern "C" void app_main() 
{
    numberQueue = xQueueCreate(8, sizeof(int));
    countCompleteSemaphore = xSemaphoreCreateBinary();
    uart_driver_install(UART_NUM, BUF_SIZE, 0, 0, NULL, 0);

    xTaskCreatePinnedToCore(fooTask, "FooTask", 2048, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(barTask, "BarTask", 2048, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(serialTask, "SerialTask", 2048, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(countTask, "CountTask", 2048, NULL, 2, NULL, 1);
}
