#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// I2C communication libraries
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Temperature and humidity sensor library and initialization
#include <Adafruit_AM2320.h>

// Sensor input pins
#define POT_DMC 14
#define POT_DC 17
#define POT_WIND 6
#define SDA 8
#define SCL 9

// Degree of seriousness LED pins
#define NORMAL 1
#define MODERATE 2
#define CRITICAL 42
#define DANGEROUS 41

// I2C communcation: Initialize 16x2 LCD (I2C address: 0x27, 16 columns, 2 rows)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Initialize AM2320 sensor
Adafruit_AM2320 am2320 = Adafruit_AM2320();

// Task handles
TaskHandle_t Data_Read_Handle = NULL;
TaskHandle_t FWI_Calc_Handle = NULL;
TimerHandle_t LED_Timer_Handle;

// Create queue handles
QueueHandle_t dataQueue;

// Task prototypes
void Data_Read(void *pvParameter);
void FWI_Calc(void *pvParameter);
void Blink_LED(TimerHandle_t);

// ============================= Global Variables =============================
volatile int FWI = 0; // Fire Weather Index

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Advanced Fire Sensing and Mitigation System!");

  // Initialize pins
  pinMode(POT_DMC, INPUT);
  pinMode(POT_DC, INPUT);
  pinMode(POT_WIND, INPUT);

  pinMode(NORMAL, OUTPUT);
  pinMode(MODERATE, OUTPUT);
  pinMode(CRITICAL, OUTPUT);
  pinMode(DANGEROUS, OUTPUT);

  // Initialize I2C communication
  Wire.begin();
  
  // Start the LCD
  lcd.init();
  lcd.backlight(); // Turn on backlight

  // Start the sensor
  am2320.begin();

  // data[0] = temp data[1] = humid   data[2] = wind  data[3] = dmc   data[4] = dc
  dataQueue = xQueueCreate(5, sizeof(float) * 5);

  // Create tasks
  // TODO:...
  xTaskCreate(Data_Read, "Data Read", 4096, NULL, 1, &Data_Read_Handle);
  xTaskCreate(FWI_Calc, "FWI", 4096, NULL, 1, &FWI_Calc_Handle);

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

// Create Tasks Functions
// TODO:...
// Temperature and Humidity sensing
void Data_Read(void *pvParameter) {
  int temp;
  int humid;
  int dmc;
  int dc;
  int potWind;
  int wind;
  float data[5];
  while (1) {  // Infinite loop to keep the task running
    data[0] = am2320.readTemperature();
    data[1] = am2320.readHumidity();
    potWind = analogRead(POT_WIND);
    data[2] = ((float)potWind / 4095.0) * 50.0;   // Scale value between 0-50 km/h
    dmc = analogRead(POT_DMC);
    data[3] = ((float)dmc / 4095.0) * 1000.0;
    dc = analogRead(POT_DC);
    data[4] = ((float)dc / 4095.0) * 800.0;
    
    xQueueSend(dataQueue, data, portMAX_DELAY);
    
    vTaskDelay(pdMS_TO_TICKS(500));  // Delay to prevent CPU overload
  }
}

void FWI_Calc(void *pvParameter) {
  float receivedData[5];
  
  while (1) {
    if (xQueueReceive(dataQueue, receivedData, portMAX_DELAY) == pdTRUE) {
      Serial.print("Wind: ");
      Serial.println(receivedData[2], 2);

      Serial.print("Temp: ");
      Serial.println(receivedData[0], 2);

      Serial.print("Humidity: ");
      Serial.println(receivedData[1], 2);

      Serial.print("Duff Moisture Code: ");
      Serial.println(receivedData[3], 2);

      Serial.print("Drought Code: ");
      Serial.println(receivedData[4], 2);
      
      FWI += 1;
      Serial.println(FWI);

    }
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void Blink_LED(TimerHandle_t LED_Timer_Handle) {
  // Green 0-5 low
  // Blue 5-15 moderate
  // Yellow 15-30 high
  // Red 30+ very high
  // Check FWI and toggle corresponding LED
  if (FWI < 6) {
    digitalWrite(NORMAL, !digitalRead(NORMAL));
    digitalWrite(MODERATE, LOW);
    digitalWrite(CRITICAL, LOW);
    digitalWrite(DANGEROUS, LOW);
  } else if (FWI < 16) {
    digitalWrite(NORMAL, LOW);
    digitalWrite(MODERATE, !digitalRead(MODERATE));
    digitalWrite(CRITICAL, LOW);
    digitalWrite(DANGEROUS, LOW);
  } else if (FWI < 31) {
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

// ============================= ESP-NOW Wireless Communication =============================
// TODO:

void loop() {}