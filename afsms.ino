#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Detection LED pins
#define TEMPERATURE_LED 7
#define HUMIDITY_LED 6
#define RAIN_LED 5
#define WIND_LED 4

// Degree of seriousness LED pins
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
TaskHandle_t Degree_Handle = NULL;

// Detection tasks prototypes
void Temperature_Detector(void *pvParameter);
void Humidity_Detector(void *pvParameter);
void Rain_Detector(void *pvParameter);
void Wind_Detector(void *pvParameter);

// Blinking LED tasks prototypes
void LED_Blink(void *pvParameter);

// Create Queue Handle
xQueueHandle_t myQueue = NULL;

// Create Semaphore Handle
// TODO:...

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Advanced Fire Sensing and Mitigation System!");

  // Create tasks
  // TODO:...
}

// Create Tasks Functions
// TODO:...

void loop() {}
