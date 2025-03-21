/**
 * @file afsms.c
 * @brief Implementation of the Fire Sensing and Mitigation System using FreeRTOS and ESP-NOW.
 *
 * This system reads environmental data, calculates the Fire Weather Index (FWI),
 * displays status on an LCD, and sends alerts wirelessly when motion is detected.
 * 
 * @author Bardia Nasrulai and Ken Do
 * @date 3/20/2025
 */

 #include "afsms.h"

 // ============================= Input Pins =============================
 #define POT_DMC 14      ///< Duff Moisture Code potentiometer
 #define POT_DC 17       ///< Drought Code potentiometer
 #define POT_WIND 6      ///< Wind potentiometer
 #define SDA 8           ///< I2C SDA pin
 #define SCL 9           ///< I2C SCL pin
 #define PIR_PIN 7       ///< Motion sensor input pin
 
 // ============================= Output Pins =============================
 #define NORMAL 1        ///< LED for Normal fire risk (Green)
 #define MODERATE 2      ///< LED for Moderate fire risk (Blue)
 #define CRITICAL 42     ///< LED for High fire risk (Yellow)
 #define DANGEROUS 41    ///< LED for Very High fire risk (Red)
 
 // ============================= Devices =============================
 LiquidCrystal_I2C lcd(0x27, 16, 2);               ///< 16x2 I2C LCD
 Adafruit_AM2320 am2320 = Adafruit_AM2320();      ///< AM2320 Temperature & Humidity sensor
 
 // ============================= Handles =============================
 TaskHandle_t Data_Read_Handle = NULL;             ///< Data reading task handle
 TaskHandle_t FWI_Calc_Handle = NULL;              ///< FWI calculation task handle
 TaskHandle_t Motion_Sensor_Handle = NULL;         ///< Motion sensor task handle
 TimerHandle_t LED_Timer_Handle;                   ///< Software timer handle
 QueueHandle_t dataQueue;                          ///< Sensor data queue
 QueueHandle_t FWIQueue;                           ///< FWI value queue
 
 // ============================= ESP-NOW =============================
 volatile bool motionDetected = false;             ///< Flag set on motion detection
 uint8_t broadcastAddress[] = {0x24, 0xEC, 0x4A, 0x0E, 0xBC, 0x5C}; ///< MAC of peer
 
 // ============================= Interrupt Handler =============================
 /**
  * @brief PIR sensor interrupt handler
  */
 void IRAM_ATTR timerInterrupt() {
   motionDetected = true;
 }
 
 // ============================= ESP-NOW Callback =============================
 /**
  * @brief Callback after sending data via ESP-NOW.
  * @param mac_addr MAC address of recipient.
  * @param status Send status result.
  */
 void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
   Serial.print("Motion Data Send Status: ");
   Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Failed");
 }
 
 // ============================= Setup Function =============================
 /**
  * @brief Initialization logic for the system.
  */
 void setup() {
   Serial.begin(115200);
   Serial.println("Advanced Fire Sensing and Mitigation System!");
 
   // Pin setup
   pinMode(POT_DMC, INPUT);
   pinMode(POT_DC, INPUT);
   pinMode(POT_WIND, INPUT);
   pinMode(PIR_PIN, INPUT);
   pinMode(NORMAL, OUTPUT);
   pinMode(MODERATE, OUTPUT);
   pinMode(CRITICAL, OUTPUT);
   pinMode(DANGEROUS, OUTPUT);
 
   Wire.begin();
   WiFi.mode(WIFI_STA);
 
   attachInterrupt(digitalPinToInterrupt(PIR_PIN), &timerInterrupt, FALLING);
 
   if (esp_now_init() != ESP_OK) return;
   esp_now_register_send_cb(OnDataSent);
 
   esp_now_peer_info_t peerInfo = {};
   memcpy(peerInfo.peer_addr, broadcastAddress, 6);
   peerInfo.channel = 0;
   peerInfo.encrypt = false;
   if (esp_now_add_peer(&peerInfo) != ESP_OK) return;
 
   lcd.init();
   lcd.backlight();
 
   am2320.begin();
 
   dataQueue = xQueueCreate(5, sizeof(float) * 5);
   FWIQueue = xQueueCreate(1, sizeof(float));
 
   xTaskCreate(Data_Read, "Data Read", 4096, NULL, 1, &Data_Read_Handle);
   xTaskCreate(FWI_Calc, "FWI", 4096, NULL, 1, &FWI_Calc_Handle);
   xTaskCreate(Motion_Sensor, "Motion Sensor", 4096, NULL, 1, &Motion_Sensor_Handle);
 
   LED_Timer_Handle = xTimerCreate("LED Blinking", pdMS_TO_TICKS(500), pdTRUE, NULL, Blink_LED);
   xTimerStart(LED_Timer_Handle, 0);
 }
 
 // ============================= Tasks =============================
 
 /**
  * @brief Reads temperature, humidity, wind, DMC, and DC from sensors and pushes to queue.
  * @param pvParameter FreeRTOS parameter (unused).
  */
 void Data_Read(void *pvParameter) {
   TickType_t xLastWakeTime = xTaskGetTickCount();
   const TickType_t xFrequency = pdMS_TO_TICKS(20);
 
   int dmc, dc, potWind;
   float data[5];
 
   while (1) {
     data[0] = am2320.readTemperature();
     data[1] = am2320.readHumidity();
     potWind = analogRead(POT_WIND);
     data[2] = ((float)potWind / 4095.0) * 50.0;
     dmc = analogRead(POT_DMC);
     data[3] = ((float)dmc / 4095.0) * 80.0;
     dc = analogRead(POT_DC);
     data[4] = ((float)dc / 4095.0) * 500.0;
 
     xQueueSend(dataQueue, data, portMAX_DELAY);
     vTaskDelayUntil(&xLastWakeTime, xFrequency);
   }
 }
 
 /**
  * @brief Calculates Fire Weather Index (FWI) from sensor data and sends to queue.
  * @param pvParameter FreeRTOS parameter (unused).
  */
 void FWI_Calc(void *pvParameter) {
   TickType_t xLastWakeTime = xTaskGetTickCount();
   const TickType_t xFrequency = pdMS_TO_TICKS(15.625);
 
   float receivedData[5];
   float m, FFMC, ISI, BUI, FWI;
 
   while (1) {
     if (xQueueReceive(dataQueue, receivedData, portMAX_DELAY) == pdTRUE) {
       m = 147.2 * (101 - receivedData[1]) / (59.5 + receivedData[0]);
       FFMC = 59.5 * (250 - m) / (147.2 + m);
       ISI = 0.208 * receivedData[2] * exp(0.05039 * FFMC);
       BUI = (0.8 * receivedData[3] * receivedData[4]) / (receivedData[3] + 0.4 * receivedData[4]);
       FWI = exp(BUI / 50.0) * ISI;
 
       Serial.printf("Temp: %.2f\nHumidity: %.2f\nWind: %.2f\nDMC: %.2f\nDC: %.2f\nFWI: %.2f\n",
         receivedData[0], receivedData[1], receivedData[2], receivedData[3], receivedData[4], FWI);
 
       if (uxQueueSpacesAvailable(FWIQueue) > 0) {
         xQueueSend(FWIQueue, &FWI, portMAX_DELAY);
       }
     }
     vTaskDelayUntil(&xLastWakeTime, xFrequency);
   }
 }
 
 /**
  * @brief Timer callback to blink corresponding LED based on FWI value.
  * @param LED_Timer_Handle Timer handle
  */
 void Blink_LED(TimerHandle_t LED_Timer_Handle) {
   float FWI;
   if (xQueueReceive(FWIQueue, &FWI, portMAX_DELAY) == pdTRUE) {
     digitalWrite(NORMAL, (FWI < 5) ? !digitalRead(NORMAL) : LOW);
     digitalWrite(MODERATE, (FWI >= 5 && FWI < 15) ? !digitalRead(MODERATE) : LOW);
     digitalWrite(CRITICAL, (FWI >= 15 && FWI < 30) ? !digitalRead(CRITICAL) : LOW);
     digitalWrite(DANGEROUS, (FWI >= 30) ? !digitalRead(DANGEROUS) : LOW);
   }
 }
 
 /**
  * @brief Sends motion-triggered data over ESP-NOW when motion is detected.
  * @param pvParameter FreeRTOS parameter (unused).
  */
 void Motion_Sensor(void *pvParameter) {
   float FWI;
   while (1) {
     uint8_t message = 0;
     if (motionDetected) {
       if (xQueuePeek(FWIQueue, &FWI, 0) == pdTRUE) {
         xQueueReceive(FWIQueue, &FWI, portMAX_DELAY);
         motionDetected = false;
         message = FWI;
         esp_now_send(broadcastAddress, &message, sizeof(message));
       }
     }
     vTaskDelay(pdMS_TO_TICKS(500));
   }
 }
 
 void loop() {}
 