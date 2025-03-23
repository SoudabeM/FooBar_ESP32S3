#FooBar_ESP32S3  
##SimulationBranch  

This branch contains the simulation, which is run in Wokwi online.  
You can simulate the project on the ESP32S3 board at:  
[Wokwi Simulation](https://wokwi.com/projects/426161465077400577)  

The current version is the final version with:  
	Separate classes (`SerialHandler` and `FooBarCounter`)  
	ESP-IDF compatible code included in this commit: SHA-1: caf622bcdc03e94bedadb9f8a560bb262a16d5d7

This version is now fully compatible with Arduino:  
	Uses `Serial.printf()` instead of `ESP_LOGI(TAG, "...")`  
	Reads UART input with `uart_read_bytes()`  
	Calls `ESP.restart()` instead of `esp_restart()`  
	Uses `setup()` instead of `app_main()`  
	Initializes UART via `Serial.begin(115200)` instead of `uart_driver_install()`  