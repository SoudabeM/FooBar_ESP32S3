#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#define BUF_SIZE 64
static const char *TAG = "FooBar";

class SerialHandler 
{
private:
    QueueHandle_t &numberQueue;

public:
    SerialHandler(QueueHandle_t &queue) : numberQueue(queue) {}

    void serialTask() 
    {
        char buffer[BUF_SIZE];
        while (true) 
        {
            if (Serial.available()) 
            {
                int len = Serial.readBytesUntil('\n', buffer, BUF_SIZE - 1);
                buffer[len] = '\0';
                int receivedNum = atoi(buffer);

                if (receivedNum == 0) 
                {
                    Serial.println("Restarting ESP32...");
                    ESP.restart();
                }

                if (uxQueueMessagesWaiting(numberQueue) < 8) 
                {
                    xQueueSend(numberQueue, &receivedNum, portMAX_DELAY);
                    Serial.printf("Received %d\n", receivedNum);
                } 
                else 
                {
                    Serial.println("Buffer is full");
                }
            }
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }
};

class FooBarCounter 
{
private:
    QueueHandle_t numberQueue;
    QueueHandle_t primeQueue;
    SemaphoreHandle_t countCompleteSemaphore;
    int currentNumber;
    bool processing;

public:
    FooBarCounter() : currentNumber(0), processing(false)
    {
        numberQueue = xQueueCreate(8, sizeof(int));
        primeQueue = xQueueCreate(8, sizeof(int));
        countCompleteSemaphore = xSemaphoreCreateBinary();
    }

    QueueHandle_t &getQueue() { return numberQueue; }
    QueueHandle_t &getPrimeQueue() { return primeQueue; }

    static bool isPrime(int num) 
    {
        if (num < 2) return false;
        for (int i = 2; i * i <= num; i++) 
        {
            if (num % i == 0) return false;
        }
        return true;
    }

    void fooTask() 
    {
        while (true) 
        {
            if (processing && (currentNumber % 2 == 0 || currentNumber == 0)) 
            {
                Serial.printf("Foo %d", currentNumber);
                xQueueSend(primeQueue, &currentNumber, portMAX_DELAY);
                currentNumber--;
                vTaskDelay(pdMS_TO_TICKS(1000));
                xSemaphoreGive(countCompleteSemaphore);
            }
            vTaskDelay(10);
        }
    }

    void barTask() 
    {
        while (true) 
        {
            if (processing && currentNumber % 2 != 0 && currentNumber >= 0) 
            {
                Serial.printf("Bar %d", currentNumber);
                xQueueSend(primeQueue, &currentNumber, portMAX_DELAY);
                currentNumber--;
                vTaskDelay(pdMS_TO_TICKS(1000));
                xSemaphoreGive(countCompleteSemaphore);
            }
            vTaskDelay(10);
        }
    }

    void primeTask() 
    {
        int num;
        while (true) 
        {
            if (xQueueReceive(primeQueue, &num, portMAX_DELAY)) 
            {
                if (isPrime(num)) 
                {
                    Serial.print(" Prime\n");
                }
                else
                {
                  Serial.print("\n");
                }
            }
        }
    }

    void countTask() 
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
};

FooBarCounter counter;
SerialHandler serialHandler(counter.getQueue());

void fooTaskWrapper(void *param) { counter.fooTask(); }
void barTaskWrapper(void *param) { counter.barTask(); }
void serialTaskWrapper(void *param) { serialHandler.serialTask(); }
void countTaskWrapper(void *param) { counter.countTask(); }
void primeTaskWrapper(void *param) { counter.primeTask(); }

void setup() 
{
    Serial.begin(115200);
    xTaskCreatePinnedToCore(fooTaskWrapper, "FooTask", 2048, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(barTaskWrapper, "BarTask", 2048, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(serialTaskWrapper, "SerialTask", 2048, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(countTaskWrapper, "CountTask", 2048, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(primeTaskWrapper, "PrimeTask", 2048, NULL, 1, NULL, 0);
}

void loop() 
{
    vTaskDelay(portMAX_DELAY);
}
