#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// I2C communication libraries
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ESP_NOW Wireless Protocol
#include <esp_now.h>
#include <WiFi.h>

// Temperature and humidity sensor library and initialization
#include <Adafruit_AM2320.h>

// ============================= Input Pins =============================
#define POT_DMC 14
#define POT_DC 17
#define POT_WIND 6
#define SDA 8
#define SCL 9
#define PIR_PIN 7

// ============================= Output Pins =============================
#define NORMAL 1
#define MODERATE 2
#define CRITICAL 42
#define DANGEROUS 41

// I2C communcation: Initialize 16x2 LCD (I2C address: 0x27, 16 columns, 2 rows)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Initialize AM2320 sensor
Adafruit_AM2320 am2320 = Adafruit_AM2320();


// ============================= Handles =============================
TaskHandle_t Data_Read_Handle = NULL;
TaskHandle_t FWI_Calc_Handle = NULL;
TaskHandle_t Motion_Sensor_Handle = NULL;
TimerHandle_t LED_Timer_Handle;
QueueHandle_t dataQueue;
QueueHandle_t FWIQueue;

// ============================= Prototypes =============================
void Data_Read(void *pvParameter);
void FWI_Calc(void *pvParameter);
void Motion_Sender(void *pvParameter);
void Blink_LED(TimerHandle_t);

// ============================= ESP_NOW Wireless Comm =============================

volatile bool motionDetected = false;
uint8_t broadcastAddress[] = {0x24, 0xEC, 0x4A, 0x0E, 0xBC, 0x5C};

void IRAM_ATTR timerInterrupt() {
  motionDetected = true;
}

// ESP-NOW Send Callback
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.print("Motion Data Send Status: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Failed");
}


// ============================= Setup =============================

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Advanced Fire Sensing and Mitigation System!");

  // Initialize pins
  pinMode(POT_DMC, INPUT);
  pinMode(POT_DC, INPUT);
  pinMode(POT_WIND, INPUT);
  pinMode(PIR_PIN, INPUT);

  pinMode(NORMAL, OUTPUT);
  pinMode(MODERATE, OUTPUT);
  pinMode(CRITICAL, OUTPUT);
  pinMode(DANGEROUS, OUTPUT);

  // Initialize I2C communication
  Wire.begin();

  WiFi.mode(WIFI_STA);

  attachInterrupt(digitalPinToInterrupt(PIR_PIN), &timerInterrupt, FALLING);

  if (esp_now_init() != ESP_OK) return; // Initialize ESP-NOW and check for success
  esp_now_register_send_cb(OnDataSent);

  esp_now_peer_info_t peerInfo;

  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);  
  peerInfo.channel = 0; // Set WiFi channel to 0 (default)
  peerInfo.encrypt = false; // Disable encryption
  if (esp_now_add_peer(&peerInfo) != ESP_OK) return; // Add peer and check for success

  
  // Start the LCD
  lcd.init();
  lcd.backlight(); // Turn on backlight

  // Start the sensor
  am2320.begin();

  // data[0] = temp data[1] = humid   data[2] = wind   data[3] = dmc   data[4] = dc
  dataQueue = xQueueCreate(5, sizeof(float) * 5);
  FWIQueue = xQueueCreate(1, sizeof(float));

  // Create tasks
  xTaskCreate(Data_Read, "Data Read", 4096, NULL, 1, &Data_Read_Handle);
  xTaskCreate(FWI_Calc, "FWI", 4096, NULL, 1, &FWI_Calc_Handle);
  xTaskCreate(Motion_Sensor, "Motion Sensor", 4096, NULL, 1, &Motion_Sensor_Handle);

  // Create a Software Timer
  LED_Timer_Handle = xTimerCreate(
    "LED Blinking",     // Timer name
    pdMS_TO_TICKS(500), // 0.5 second interval
    pdTRUE,             // TRUE means periodic, FALSE means one-time
    NULL,               // NO ID is needed
    Blink_LED           // The call back function defined
  );

  xTimerStart(LED_Timer_Handle, 0); // This will start the timer
}

// ============================= Tasks =============================

// Temperature and Humidity sensing
void Data_Read(void *pvParameter) {
  TickType_t xLastWakeTime = xTaskGetTickCount();  // Record start time
  const TickType_t xFrequency = pdMS_TO_TICKS(20); // 20ms period (50 Hz)

  int dmc;
  int dc;
  int potWind;
  float data[5];
  while (1) {  // Infinite loop to keep the task running
    data[0] = am2320.readTemperature();
    data[1] = am2320.readHumidity();
    potWind = analogRead(POT_WIND);
    data[2] = ((float)potWind / 4095.0) * 50.0;   // Scale value between 0-50 km/h
    dmc = analogRead(POT_DMC);
    data[3] = ((float)dmc / 4095.0) * 80.0; // Scale value between 0-80
    dc = analogRead(POT_DC);
    data[4] = ((float)dc / 4095.0) * 500.0; // Scale value between 0-500
    
    xQueueSend(dataQueue, data, portMAX_DELAY);
    
    vTaskDelayUntil(&xLastWakeTime, xFrequency); // Precisely maintain 50 Hz
  }
}

// Calculate FWI based on new sensor data.
void FWI_Calc(void *pvParameter) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(15.625); // 15.625ms period (64 Hz)

  float receivedData[5];
  float m; // moisture content
  float FFMC; // Fine Fuel Moisture Code
  float ISI; // Initial Spread Index
  float BUI; // Buildup Index
  float FWI; // Fire Weather Index
  
  while (1) {
    xLastWakeTime = xTaskGetTickCount();
    if (xQueueReceive(dataQueue, receivedData, portMAX_DELAY) == pdTRUE) {
      m = 147.2 * (101 - receivedData[1]) / (59.5 + receivedData[0]);
      FFMC = 59.5 * (250 - m) / (147.2 + m);
      ISI = 0.208 * receivedData[2] * exp(0.05039 * FFMC);
      BUI = (0.8 * receivedData[3] * receivedData[4]) / (receivedData[3] + 0.4 * receivedData[4]);
      FWI = exp(BUI / 50.0) * ISI;

      Serial.print("Temp: ");
      Serial.println(receivedData[0], 2);

      Serial.print("Humidity: ");
      Serial.println(receivedData[1], 2);

      Serial.print("Wind: ");
      Serial.println(receivedData[2], 2); 

      Serial.print("Duff Moisture Code: ");
      Serial.println(receivedData[3], 2);

      Serial.print("Drought Code: ");
      Serial.println(receivedData[4], 2);

      Serial.print("FWI: ");
      Serial.println(FWI, 2);

      // Ensure space is avaialble
      if (uxQueueSpacesAvailable(FWIQueue) > 0) {
        xQueueSend(FWIQueue, &FWI, portMAX_DELAY); // Send FWI data
      }

    }
    vTaskDelayUntil(&xLastWakeTime, xFrequency); // Precisely maintain 64 Hz
  }
}

void Blink_LED(TimerHandle_t LED_Timer_Handle) {
  // Green 0-5 low
  // Blue 5-15 moderate
  // Yellow 15-30 high
  // Red 30+ very high
  // Check FWI and toggle corresponding LED
  float FWI;
  if (xQueueReceive(FWIQueue, &FWI, portMAX_DELAY) == pdTRUE) {
    if (FWI < 5) {
      digitalWrite(NORMAL, !digitalRead(NORMAL));
      digitalWrite(MODERATE, LOW);
      digitalWrite(CRITICAL, LOW);
      digitalWrite(DANGEROUS, LOW);
    } else if (FWI < 15) {
      digitalWrite(NORMAL, LOW);
      digitalWrite(MODERATE, !digitalRead(MODERATE));
      digitalWrite(CRITICAL, LOW);
      digitalWrite(DANGEROUS, LOW);
    } else if (FWI < 30) {
      digitalWrite(NORMAL, LOW);
      digitalWrite(MODERATE, LOW);
      digitalWrite(CRITICAL, !digitalRead(CRITICAL));
      digitalWrite(DANGEROUS, LOW);
    } else {
      digitalWrite(NORMAL, LOW);
      digitalWrite(MODERATE, LOW);
      digitalWrite(CRITICAL, LOW);
      digitalWrite(DANGEROUS, !digitalRead(DANGEROUS));
    }
  }
}

// Motion Detection and ESP-NOW Send Task
void Motion_Sensor(void *pvParameter) {
  float FWI;
  while (1) {
    uint8_t message = 0;
    if (motionDetected) {  // Only send if FWI value available & motion detected
      if (xQueuePeek(FWIQueue, &FWI, 0) == pdTRUE) {
        xQueueReceive(FWIQueue, &FWI, portMAX_DELAY);
        motionDetected = false;
        message = FWI;
        esp_now_send(broadcastAddress, &message, sizeof(message));
      }
    }
    vTaskDelay(pdMS_TO_TICKS(500));  // Protect CPU
  }
}

void loop() {}