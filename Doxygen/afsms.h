/**
 * @file afsms.h
 * @brief Header file for the Fire Sensing and Mitigation System using ESP-NOW and FreeRTOS.
 *
 * This module contains declarations and macros for handling sensor data,
 * Fire Weather Index (FWI) calculations, motion detection, and LED indicators.
 *
 * @author Bardia Nasrulai and Ken Do
 * @date 3/20/2025
 */

#ifndef AFSMS_H
#define AFSMS_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/queue.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Adafruit_AM2320.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================= Input Pins =============================

/** @brief Pin for Duff Moisture Code potentiometer */
#define POT_DMC 14

/** @brief Pin for Drought Code potentiometer */
#define POT_DC 17

/** @brief Pin for wind speed potentiometer */
#define POT_WIND 6

/** @brief I2C SDA pin */
#define SDA 8

/** @brief I2C SCL pin */
#define SCL 9

/** @brief PIR sensor input pin */
#define PIR_PIN 7

// ============================= Output Pins (LEDs) =============================

/** @brief LED pin for normal fire risk */
#define NORMAL 1

/** @brief LED pin for moderate fire risk */
#define MODERATE 2

/** @brief LED pin for critical fire risk */
#define CRITICAL 42

/** @brief LED pin for dangerous fire risk */
#define DANGEROUS 41

// ============================= External Devices =============================

/**
 * @brief 16x2 LCD screen object (I2C address: 0x27)
 */
extern LiquidCrystal_I2C lcd;

/**
 * @brief AM2320 temperature and humidity sensor object
 */
extern Adafruit_AM2320 am2320;

// ============================= Task Handles =============================

/** @brief Handle for the sensor data reading task */
extern TaskHandle_t Data_Read_Handle;

/** @brief Handle for the FWI calculation task */
extern TaskHandle_t FWI_Calc_Handle;

/** @brief Handle for the motion sensor task */
extern TaskHandle_t Motion_Sensor_Handle;

/** @brief Handle for the LED blinking timer */
extern TimerHandle_t LED_Timer_Handle;

/** @brief Queue for passing sensor data */
extern QueueHandle_t dataQueue;

/** @brief Queue for passing FWI values */
extern QueueHandle_t FWIQueue;

// ============================= Global Variables =============================

/** @brief Indicates motion was detected by the PIR sensor */
extern volatile bool motionDetected;

/** @brief MAC address of the peer ESP-NOW receiver */
extern uint8_t broadcastAddress[];

// ============================= Function Declarations =============================

/**
 * @brief Task that reads temperature, humidity, and analog inputs from sensors.
 * @param pvParameter FreeRTOS task parameter (unused).
 */
void Data_Read(void *pvParameter);

/**
 * @brief Task that computes the Fire Weather Index from sensor inputs.
 * @param pvParameter FreeRTOS task parameter (unused).
 */
void FWI_Calc(void *pvParameter);

/**
 * @brief Task that handles motion detection and sends data using ESP-NOW.
 * @param pvParameter FreeRTOS task parameter (unused).
 */
void Motion_Sensor(void *pvParameter);

/**
 * @brief Callback function to handle LED status based on FWI.
 * @param LED_Timer_Handle FreeRTOS software timer handle.
 */
void Blink_LED(TimerHandle_t LED_Timer_Handle);

/**
 * @brief Interrupt service routine triggered by the PIR sensor.
 */
void IRAM_ATTR timerInterrupt();

/**
 * @brief ESP-NOW send callback function.
 * @param mac_addr MAC address of the peer.
 * @param status Status of the send operation.
 */
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);

#ifdef __cplusplus
}
#endif

#endif // AFSMS_H