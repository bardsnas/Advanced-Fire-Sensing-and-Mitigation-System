/**
 * @file afsms_receiver.c
 * @brief Implementation of the Fire Warning Display System using ESP-NOW and LCD.
 *
 * Receives motion alerts via ESP-NOW and displays them on an LCD.
 * Also shows a real-time timer using a hardware interrupt.
 * 
 * @author Bardia Nasrulai and Ken Do
 * @date 3/20/2025
 */

 #include "receiver_display.h"

 LiquidCrystal_I2C lcd(0x27, 16, 2);               ///< I2C LCD display
 volatile bool messageReceived = false;            ///< Flag for ESP-NOW message reception
 int second = 0;                                   ///< Seconds counter
 int minute = 0;                                   ///< Minutes counter
 int hour = 0;                                     ///< Hours counter
 int messageLength = 0;                            ///< Length of received message
 uint8_t messageData[16];                          ///< Buffer for received message
 hw_timer_t * timer = NULL;                        ///< Hardware timer pointer
 
 /**
  * @brief ESP-NOW callback for receiving data.
  * Copies up to 16 bytes into messageData buffer and sets messageReceived flag.
  */
 void IRAM_ATTR dataReceived(const esp_now_recv_info_t * esp_now_info, const uint8_t *incomingData, int len)
 {
     messageReceived = true;
 
     messageLength = (len > 16) ? 16 : len;
     memcpy(messageData, incomingData, messageLength);
 }
 
 /**
  * @brief ISR triggered every second by the hardware timer.
  * Updates software-based hours, minutes, and seconds.
  */
 void IRAM_ATTR timerISR() {
     second++;
     if (second == 60) {
         minute++;
         second = 0;
     }
     if (minute == 60) {
         hour++;
         minute = 0;
     }
 }
 
 /**
  * @brief Initializes LCD, WiFi, ESP-NOW, hardware timer, and interrupt handlers.
  */
 void setup() {
     WiFi.mode(WIFI_STA);
     lcd.init();
     lcd.backlight();
 
     timer = timerBegin(8000);                  // Prescaler: 8000
     timerAttachInterrupt(timer, &timerISR);
     timerAlarm(timer, 10000, true, 0);         // Interrupt every 1s
 
     if (esp_now_init() != ESP_OK) return;
     esp_now_register_recv_cb(dataReceived);
 }
 
 /**
  * @brief Displays either a received ESP-NOW message or the current time on the LCD.
  */
 void loop() {
     if (messageReceived) {
         lcd.clear();
         lcd.setCursor(0, 0);
         lcd.print("Motion detected:");
         lcd.setCursor(0, 1);
         lcd.print("FWI: ");
         lcd.setCursor(5, 1);
         for (int i = 0; i < messageLength; i++) {
             lcd.print(messageData[i], DEC);
         }
 
         second = minute = hour = 0;
         vTaskDelay(2000);
         lcd.clear();
         messageReceived = false;
     } else {
         // Time formatting and display
         lcd.setCursor(8, 1);
         if (hour < 10) lcd.print("0");
         lcd.print(hour);
         lcd.print(":");
 
         if (minute < 10) lcd.print("0");
         lcd.print(minute);
         lcd.print(":");
 
         if (second < 10) lcd.print("0");
         lcd.print(second);
     }
 }
 