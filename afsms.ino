#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// I2C communication libraries
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Temperature and humidity sensor library and initialization
#include <Adafruit_AM2320.h>

// Detection LED pins
#define TEMP_LED 7
#define HUMID_LED 6
#define RAIN_LED 5
#define WIND_LED 4

// Degree of seriousness LED pins
#define NORMAL 15
#define SEVERE 16
#define CRITICAL 17
#define EMERGENCY 18

// I2C communcation: Initialize 16x2 LCD (I2C address: 0x27, 16 columns, 2 rows)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Initialize AM2320 sensor
Adafruit_AM2320 am2320 = Adafruit_AM2320();

// Detection handles
TaskHandle_t Temp_Humid_Handle = NULL;
TaskHandle_t Rain_Handle = NULL;
TaskHandle_t Wind_Handle = NULL;

// Degree of seriousness handles
TaskHandle_t Degree_Handle = NULL;

// Detection tasks prototypes
void Temp_Humid_Det(void *pvParameter);
void Wind_Det(void *pvParameter);
void Rain_Det(void *pvParameter);

// Blinking LED tasks prototypes
void LED_Blink(void *pvParameter);

// ============================= Global Variables =============================
volatile int brightness = 0;   // Current LED brightness
volatile int FWI = 0; // Fire Weather Index

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Advanced Fire Sensing and Mitigation System!");

  // Initialize pins
  pinMode(TEMP_LED, OUTPUT);
  pinMode(HUMID_LED, OUTPUT);

  // Initialize I2C communication
  Wire.begin();
    
  // Start the LCD
  lcd.init();
  lcd.backlight(); // Turn on backlight

  // Start the sensor
  am2320.begin();

  // Create tasks
  // TODO:...
  xTaskCreate(Temp_Humid_Det, "Temperture & Humidity", 1024, NULL, 1, &Temp_Humid_Handle);

}

// Create Tasks Functions
// TODO:...
// Temperature and Humidity sensing
void Temp_Humid_Det(void *pvParameter) {
  // Read temperature and humidity from AM2320
  float temp = am2320.readTemperature();
  float humid = am2320.readHumididy();

  int brightnessTemp = map(temp, -40, 80, 0, 255);
  int brightnessHumid = map(temp, 0, 100, 0, 255);


  analogWrite(TEMP_LED, brightnessTemp);
  analogWrite(HUMID_LED, brightnessHumid);
}

void loop() {}
