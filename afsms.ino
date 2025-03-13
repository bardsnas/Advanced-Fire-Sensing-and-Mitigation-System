#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Detection GPIO pins
#define TEMPERATURE 7
#define HUMIDITY 6
#define RAIN 5
#define WIND 4

// Degree of seriousness GPIO pins
#define NORMAL 15
#define SEVERE 16
#define CRITICAL 17
#define EMERGENCY 18

// Detection handles
TaskHandle_t Temperature_Handle = NULL;
TaskHandle_t Humidity_Handle = NULL;
TaskHandle_t Rain_Handle = NULL;
TaskHandle_t Wind_Handle = NULL;

// Degree of seriousness handles
// TODO:

// Detection tasks prototypes
void Temperature_Detector(void *pvParameter);
void Humidity_Detector(void *pvParameter);
void Rain_Detector(void *pvParameter);
void Wind_Detector(void *pvParameter);

// Blinking LED tasks prototypes
// TODO:

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Advanced Fire Sensing and Mitigation System!");

  // Create tasks
}

void loop() {}
