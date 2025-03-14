#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// I2C communication libraries
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Temperature and humidity sensor library and initialization
#include <Adafruit_AM2320.h>

// Detection LED pins
#define TEMP_LED 36
#define HUMID_LED 38
#define RAIN_LED 5
#define WIND_LED 4

// Degree of seriousness LED pins
#define NORMAL 15
#define SEVERE 16
#define CRITICAL 17
#define EMERGENCY 18

#define POT_PIN 6

// I2C communcation: Initialize 16x2 LCD (I2C address: 0x27, 16 columns, 2 rows)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Initialize AM2320 sensor
Adafruit_AM2320 am2320 = Adafruit_AM2320();

// Detection handles
TaskHandle_t Temp_Humid_Handle = NULL;
TaskHandle_t Rain_Handle = NULL;
TaskHandle_t Wind_Handle = NULL;

// FWI Calculation handle
TaskHandle_t FWI_Calc_Handle = NULL;

// Detection tasks prototypes
void Temp_Humid_Det(void *pvParameter);
void Wind_Det(void *pvParameter);
void Rain_Det(void *pvParameter);

// FWI Calculation task prototype
void FWI_Calc(void *pvParameter);

// ============================= Global Variables =============================
volatile int wind = 0;
volatile float temp = 0;
volatile float humid = 0;
volatile int FWI = 0; // Fire Weather Index

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Advanced Fire Sensing and Mitigation System!");

  // Initialize pins
  pinMode(TEMP_LED, OUTPUT);
  pinMode(HUMID_LED, OUTPUT);
  pinMode(POT_PIN, INPUT);

  // Initialize I2C communication
  Wire.begin();
    
  // Start the LCD
  lcd.init();
  lcd.backlight(); // Turn on backlight

  // Start the sensor
  am2320.begin();

  // Create tasks
  // TODO:...
  xTaskCreate(Temp_Humid_Det, "Temperture & Humidity", 4096, NULL, 1, &Temp_Humid_Handle);
  xTaskCreate(Wind_Det, "Wind", 4096, NULL, 1, &Wind_Handle);
  xTaskCreate(FWI_Calc, "FWI", 4096, NULL, 1, &FWI_Calc_Handle);
}

// Create Tasks Functions
// TODO:...
// Temperature and Humidity sensing
void Temp_Humid_Det(void *pvParameter) {
  while (1) {  // Infinite loop to keep the task running
    temp = am2320.readTemperature();
    humid = am2320.readHumidity();

    vTaskDelay(pdMS_TO_TICKS(500));  // Delay to prevent CPU overload
  }
}

void Wind_Det(void *pvParameter) {
  while (1) {
    int potValue = analogRead(POT_PIN);
    wind = (potValue / 4095.0) * 50.0;   // Scale value between 0-50 km/h
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void FWI_Calc(void *pvParameter) {
  while (1) {
    Serial.print("Wind: ");
    Serial.println(wind);

    Serial.print("Temp: ");
    Serial.println(temp);

    Serial.print("Humidity: ");
    Serial.println(humid);
    
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}


void loop() {}