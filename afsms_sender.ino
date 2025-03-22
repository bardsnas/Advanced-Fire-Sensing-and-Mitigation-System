// ======================= FreeRTOS & I2C Libraries ===========================
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

// ============================= Input Pins ===================================
#define POT_DMC 14   // Potentiometer simulating Duff Moisture Code (DMC)
#define POT_DC 17    // Potentiometer simulating Drought Code (DC)
#define POT_WIND 6   // Potentiometer simulating wind speed
#define SDA 8        // I2C SDA pin
#define SCL 9        // I2C SCL pin
#define PIR_PIN 7    // PIR motion sensor input pin

// ============================= Output Pins ==================================
#define NORMAL 1     // LED for Normal fire risk (Green)
#define MODERATE 2   // LED for Moderate fire risk (Blue)
#define CRITICAL 42  // LED for Critical fire risk (Yellow)
#define DANGEROUS 41 // LED for Dangerous fire risk (Red)

// ======================== I2C Device Initialization =========================
// Initialize 16x2 LCD (I2C address: 0x27, 16 columns, 2 rows)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Initialize AM2320 temperature and humidity sensor
Adafruit_AM2320 am2320 = Adafruit_AM2320();

// ============================= FreeRTOS Handles =============================
TaskHandle_t Data_Read_Handle = NULL;
TaskHandle_t FWI_Calc_Handle = NULL;
TaskHandle_t Motion_Sensor_Handle = NULL;
TimerHandle_t LED_Timer_Handle; // Timer handle for LED blinking
QueueHandle_t dataQueue;        // Queue for sensor data
QueueHandle_t FWIQueue;         // Queue for Fire Weather Index (FWI)

// =========================== Function Prototypes ============================
void Data_Read(void *pvParameter);      // Task to read sensor data
void FWI_Calc(void *pvParameter);       // Task to compute Fire Weather Index
void Motion_Sender(void *pvParameter);  // Task to handle motion detection and ESP-NOW sending
void Blink_LED(TimerHandle_t);          // Timer callback function for LED indication

// ========================== ESP_NOW Wireless Comm ===========================
volatile bool motionDetected = false; // Flag to indicate motion detection
uint8_t broadcastAddress[] = {0x24, 0xEC, 0x4A, 0x0E, 0xBC, 0x5C}; // MAC address of receiver

// Interrupt Service Routine for motion detection
void IRAM_ATTR timerInterrupt() {
  motionDetected = true;
}

// ESP-NOW Send Callback to confirm message transmission
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.print("Motion Data Send Status: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Failed");
}

// ================================== Setup ===================================
void setup() {
  Serial.begin(115200);
  Serial.println("Advanced Fire Sensing and Mitigation System!");

  // Initialize input pins
  pinMode(POT_DMC, INPUT);
  pinMode(POT_DC, INPUT);
  pinMode(POT_WIND, INPUT);
  pinMode(PIR_PIN, INPUT);

  // Initialize output pins
  pinMode(NORMAL, OUTPUT);
  pinMode(MODERATE, OUTPUT);
  pinMode(CRITICAL, OUTPUT);
  pinMode(DANGEROUS, OUTPUT);

  // Initialize I2C communication
  Wire.begin();
  WiFi.mode(WIFI_STA); // Set WiFi mode to Station

  // Attach interrupt for motion detection
  attachInterrupt(digitalPinToInterrupt(PIR_PIN), &timerInterrupt, FALLING);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) return; // Initialize ESP-NOW and check for success
  esp_now_register_send_cb(OnDataSent);

  // Add peer device for ESP-NOW communication
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);  
  peerInfo.channel = 0; // Set WiFi channel to 0 (default)
  peerInfo.encrypt = false; // Disable encryption
  if (esp_now_add_peer(&peerInfo) != ESP_OK) return; // Add peer and check for success

  
  // Start the LCD
  lcd.init();
  lcd.backlight(); // Turn on backlight

  // Start the temperature and humidity sensor
  am2320.begin();

  // Create queues for sensor and FWI data
  // data[0] = temp; data[1] = humid; data[2] = wind; data[3] = dmc; data[4] = dc
  dataQueue = xQueueCreate(5, sizeof(float) * 5);
  FWIQueue = xQueueCreate(1, sizeof(float));

  // Create FreeRTOS tasks
  xTaskCreate(Data_Read, "Data Read", 4096, NULL, 1, &Data_Read_Handle);
  xTaskCreate(FWI_Calc, "FWI", 4096, NULL, 1, &FWI_Calc_Handle);
  xTaskCreate(Motion_Sensor, "Motion Sensor", 4096, NULL, 1, &Motion_Sensor_Handle);

  // Create a Software Timer for LED blinking
  LED_Timer_Handle = xTimerCreate(
    "LED Blinking",     // Timer name
    pdMS_TO_TICKS(500), // 0.5 second interval
    pdTRUE,             // TRUE means periodic, FALSE means one-time
    NULL,               // NO ID is needed
    Blink_LED           // The call back function defined
  );

  xTimerStart(LED_Timer_Handle, 0); // This will start the timer
}

// ================================== Tasks =============================

// Task: Read temperature, humidity, and potentiometer values
void Data_Read(void *pvParameter) {
  TickType_t xLastWakeTime = xTaskGetTickCount();  // Record start time
  const TickType_t xFrequency = pdMS_TO_TICKS(20); // 20ms period (50 Hz)

  int dmc;
  int dc;
  int potWind;
  float data[5];
  while (1) {  // Infinite loop to keep the task running
    // Task temperature and humidity from AM2320 sensor
    data[0] = am2320.readTemperature();
    data[1] = am2320.readHumidity();

    // Read wind speed potentiometer and scale value between 0-50 km/h
    potWind = analogRead(POT_WIND);
    data[2] = ((float)potWind / 4095.0) * 50.0;
    
    // Read Duff Moisture Code potentiometer and scale value between 0-80
    dmc = analogRead(POT_DMC);
    data[3] = ((float)dmc / 4095.0) * 80.0;
    
    // Read Drought Code potentiometer and scale value between 0-500
    dc = analogRead(POT_DC);
    data[4] = ((float)dc / 4095.0) * 500.0;
    
    // Send sensor data to queue
    xQueueSend(dataQueue, data, portMAX_DELAY);
    
    // Maintain precise 50 Hz frequency
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

// Task: Compute Fire Weather Index (FWI) based on sensor data
void FWI_Calc(void *pvParameter) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(15.625); // 15.625ms period (64 Hz)

  float receivedData[5];
  float m;    // moisture content
  float FFMC; // Fine Fuel Moisture Code
  float ISI;  // Initial Spread Index
  float BUI;  // Buildup Index
  float FWI;  // Fire Weather Index
  
  while (1) {
    xLastWakeTime = xTaskGetTickCount();

    // Receive data from queue
    if (xQueueReceive(dataQueue, receivedData, portMAX_DELAY) == pdTRUE) { 
      // Compute moisture content
      m = 147.2 * (101 - receivedData[1]) / (59.5 + receivedData[0]);

      // Compute FFMC
      FFMC = 59.5 * (250 - m) / (147.2 + m);

      // Compute ISI
      ISI = 0.208 * receivedData[2] * exp(0.05039 * FFMC);

      // Compute BUI
      BUI = (0.8 * receivedData[3] * receivedData[4]) / (receivedData[3] + 0.4 * receivedData[4]);
      
      // Compute FWI
      FWI = exp(BUI / 50.0) * ISI;

      // Print sensor values and FWI to Serial Monitor
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

      // Send FWI data to queue if space is available
      if (uxQueueSpacesAvailable(FWIQueue) > 0) {
        xQueueSend(FWIQueue, &FWI, portMAX_DELAY); // Send FWI data
      }

    }

    // Maintain precise 64 Hz frequency
    vTaskDelayUntil(&xLastWakeTime, xFrequency); // Precisely maintain 64 Hz
  }
}

// Timer Callback Function: Blink LED based on FWI value
void Blink_LED(TimerHandle_t LED_Timer_Handle) {
  // Check FWI and toggle corresponding LED
  float FWI;

  // Check if FWI data is available
  if (xQueueReceive(FWIQueue, &FWI, portMAX_DELAY) == pdTRUE) {
    if (FWI < 5) { // Green 0-5 normal
      digitalWrite(NORMAL, !digitalRead(NORMAL));
      digitalWrite(MODERATE, LOW);
      digitalWrite(CRITICAL, LOW);
      digitalWrite(DANGEROUS, LOW);
    } else if (FWI < 15) { // Blue 5-15 moderate
      digitalWrite(NORMAL, LOW);
      digitalWrite(MODERATE, !digitalRead(MODERATE));
      digitalWrite(CRITICAL, LOW);
      digitalWrite(DANGEROUS, LOW);
    } else if (FWI < 30) { // Yellow 15-30 critical
      digitalWrite(NORMAL, LOW);
      digitalWrite(MODERATE, LOW);
      digitalWrite(CRITICAL, !digitalRead(CRITICAL));
      digitalWrite(DANGEROUS, LOW);
    } else { // Red 30+ very danderous
      digitalWrite(NORMAL, LOW);
      digitalWrite(MODERATE, LOW);
      digitalWrite(CRITICAL, LOW);
      digitalWrite(DANGEROUS, !digitalRead(DANGEROUS));
    }
  }
}

// Task: Detect motion and send FWI value using ESP-NOW
void Motion_Sensor(void *pvParameter) {
  float FWI;
  while (1) {
    uint8_t message = 0;

    // If motion is detected and FWI data is available
    if (motionDetected) {  // Only send if FWI value available & motion detected
      if (xQueuePeek(FWIQueue, &FWI, 0) == pdTRUE) {
        xQueueReceive(FWIQueue, &FWI, portMAX_DELAY);
        motionDetected = false;

        // Send FWI value as a message via ESP-NOW
        message = FWI;
        esp_now_send(broadcastAddress, &message, sizeof(message));
      }
    }

    // Delay to avoid excessive CPU usage
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void loop() {}
