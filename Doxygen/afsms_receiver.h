/**
 * @file afsms_receiver.h
 * @brief Header file for the Fire Warning Display System using ESP-NOW and LCD.
 *
 * This module handles receiving motion/FWI messages via ESP-NOW and displaying them
 * on a 16x2 I2C LCD. It also maintains a software clock with an interrupt timer.
 * 
 * @author Bardia Nasrulai and Ken Do
 * @date 3/20/2025
 */

 #ifndef AFSMS_RECEIVER_H
 #define AFSMS_RECEIVER_H
 
 #include <Arduino.h>
 #include <Wire.h>
 #include <LiquidCrystal_I2C.h>
 #include <esp_now.h>
 #include <WiFi.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 // ============================= Global Variables =============================
 
 /**
  * @brief LCD object for a 16x2 I2C screen (address 0x27).
  */
 extern LiquidCrystal_I2C lcd;
 
 /**
  * @brief Indicates if a message has been received via ESP-NOW.
  */
 extern volatile bool messageReceived;
 
 /**
  * @brief Second counter for timer display.
  */
 extern int second;
 
 /**
  * @brief Minute counter for timer display.
  */
 extern int minute;
 
 /**
  * @brief Hour counter for timer display.
  */
 extern int hour;
 
 /**
  * @brief Number of bytes in the received message.
  */
 extern int messageLength;
 
 /**
  * @brief Buffer to store received ESP-NOW message data (max 16 bytes).
  */
 extern uint8_t messageData[16];
 
 /**
  * @brief Hardware timer used for timekeeping.
  */
 extern hw_timer_t * timer;
 
 // ============================= Function Declarations =============================
 
 /**
  * @brief Callback triggered when data is received via ESP-NOW.
  *
  * Copies up to 16 bytes of the received message into a buffer.
  *
  * @param esp_now_info ESP-NOW metadata (MAC address, etc.).
  * @param incomingData Pointer to received data.
  * @param len Length of received data.
  */
 void IRAM_ATTR dataReceived(const esp_now_recv_info_t * esp_now_info, const uint8_t *incomingData, int len);
 
 /**
  * @brief Timer ISR that increments time counters every second.
  */
 void IRAM_ATTR timerISR();
 
 /**
  * @brief Arduino setup function. Initializes LCD, ESP-NOW, and hardware timer.
  */
 void setup();
 
 /**
  * @brief Arduino main loop function. Displays either message or current time.
  */
 void loop();
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif // AFSMS_RECEIVER_H
 