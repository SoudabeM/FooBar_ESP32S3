#FooBar_ESP32S3  
##SimulationBranch  

This branch contains the simulation, which is run in Wokwi online.  
You can simulate the project on the ESP32S3 board at:  
[Wokwi Simulation](https://wokwi.com/projects/426385527943128065)  

The current version is the final version with:  
	Separate classes (`SerialHandler` and `FooBarCounter`)  
	ESP-IDF compatible code included in this commit: SHA-1: d9a1776024c0caf978492d5c5942871c9158d81a

This version is now fully compatible with Arduino:  
	Uses `Serial.printf()` instead of `ESP_LOGI(TAG, "...")`  
	Reads UART input with `uart_read_bytes()`  
	Calls `ESP.restart()` instead of `esp_restart()`  
	Uses `setup()` instead of `app_main()`  
	Initializes UART via `Serial.begin(115200)` instead of `uart_driver_install()`  