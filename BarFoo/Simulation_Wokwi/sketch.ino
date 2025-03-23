#include <Arduino.h>
#include <queue>

QueueHandle_t numberQueue;
SemaphoreHandle_t countCompleteSemaphore;
int currentNumber = 0;
bool processing = false;

bool isPrime(int num) {
    if (num < 2) return false;
    for (int i = 2; i * i <= num; i++) {
        if (num % i == 0) return false;
    }
    return true;
}

void fooTask(void *param) {
    while (true) {
        if (processing && (currentNumber % 2 == 0 || currentNumber == 0)) {
            Serial.printf("Foo %d", currentNumber);
            if (isPrime(currentNumber) && currentNumber > 1) {
                Serial.printf(" Prime");
            }
            Serial.printf("\n");
            currentNumber--;
            vTaskDelay(pdMS_TO_TICKS(1000));
            xSemaphoreGive(countCompleteSemaphore);
        }
        vTaskDelay(10);
    }
}

void barTask(void *param) {
    while (true) {
        if (processing && currentNumber % 2 != 0 && currentNumber >=0) {
            Serial.printf("Bar %d", currentNumber);
            if (isPrime(currentNumber)) {
                Serial.printf(" Prime");
            }
            Serial.printf("\n");
            currentNumber--;
            vTaskDelay(pdMS_TO_TICKS(1000));
            xSemaphoreGive(countCompleteSemaphore);
        }
        vTaskDelay(10);
    }
}

void serialTask(void *param) {
    char buffer[20];
    while (true) {
        if (Serial.available() > 0) {
            int len = Serial.readBytesUntil('\n', buffer, sizeof(buffer) - 1);
            buffer[len] = '\0';
            int receivedNum = atoi(buffer);

            if (receivedNum == 0) {
                Serial.println("Restarting ESP32...");
                ESP.restart();
            }

            if (uxQueueMessagesWaiting(numberQueue) < 8) {
                xQueueSend(numberQueue, &receivedNum, portMAX_DELAY);
                Serial.printf("Received %d\n", receivedNum);
            } else {
                Serial.println("Buffer is full");
            }
        }
        vTaskDelay(100);
    }
}

void countTask(void *param) {
    int num;
    while (true) {
        if (xQueueReceive(numberQueue, &num, portMAX_DELAY)) {
            currentNumber = num;
            processing = true;
            while (currentNumber >= 0) {
                xSemaphoreTake(countCompleteSemaphore, portMAX_DELAY);
            }
            processing = false;
        }
    }
}

void setup() {
    Serial.begin(115200);
    numberQueue = xQueueCreate(8, sizeof(int));
    countCompleteSemaphore = xSemaphoreCreateBinary();

    xTaskCreatePinnedToCore(fooTask, "FooTask", 2048, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(barTask, "BarTask", 2048, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(serialTask, "SerialTask", 2048, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(countTask, "CountTask", 2048, NULL, 2, NULL, 1);
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}