#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <esp_now.h>
#include <WiFi.h>


LiquidCrystal_I2C lcd(0x27, 16, 2);
volatile bool messageReceived = false;
int second = 0;
int minute = 0;
int hour = 0;
int messageLength = 0;
uint8_t messageData[16];

hw_timer_t * timer = NULL; // Declare a timer variable and initialize to null

void IRAM_ATTR dataReceived(const esp_now_recv_info_t * esp_now_info, const uint8_t *incomingData, int len)
{
    // This callback function will be invoked when signal is received
    // over ESP-NOW and trigger the message to the LCD.
    messageReceived = true;
    
    // Limit message to 16 characters or less to compress to LCD dimensions.
    if (len > 16) {
        messageLength = 16;
    } else {
        messageLength = len;
    }

    // Copies messageLength bytes from incomingData to messageData.
    memcpy(messageData, incomingData, messageLength);
}

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

void setup() {
    WiFi.mode(WIFI_STA);
    // Initialize LCD display
    lcd.init();
    lcd.backlight();

    // update the counter every second.
    timer = timerBegin(8000); // prescaler 8000
    // Attach dataReceived function to the timer
    timerAttachInterrupt(timer, &timerISR);
    // Set alarm to trigger interrupt every second, repeating (true),
    // number of autoreloads (0=unlimited)
    timerAlarm(timer, 10000, true, 0);


    // Initializes ESP-NOW and check if it was successful; if not, exit the setup function
    if (esp_now_init() != ESP_OK) return;
    // Registers the callback function 'dataReceived' to be called when data is received   
    // via ESP-NOW
    esp_now_register_recv_cb(dataReceived);
}


void loop() {
    // If a signal has been received over ESP-NOW, print out an alert message on the LCD
    if (messageReceived) {
        lcd.clear(); // Remove screen contents.
        lcd.setCursor(0, 0);
        lcd.print("Motion detected:");
        lcd.setCursor(0, 1);
        lcd.print("FWI: ");
        lcd.setCursor(5, 1);
        // Prints contents of message on 2nd line of LCD.
        for (int i = 0; i < messageLength; i++) {
            lcd.print(messageData[i], DEC);
        }
        second = 0;
        minute = 0;
        hour = 0;
        vTaskDelay(2000);
        lcd.clear(); // Remove screen contents.
        messageReceived = false;

    } else {
        if (hour < 10) {
          lcd.setCursor(8, 1);
          lcd.print("0");
          lcd.setCursor(9, 1);
        } else {
          lcd.setCursor(8, 1);
        }
        lcd.print(hour);
        lcd.setCursor(10, 1);
        lcd.print(":");
        if (minute < 10) {
          lcd.setCursor(11, 1);
          lcd.print("0");
          lcd.setCursor(12, 1);
        } else {
          lcd.setCursor(11, 1);
        }
        lcd.print(minute);
        lcd.setCursor(13, 1);
        lcd.print(":");
        if (second < 10) {
          lcd.setCursor(14, 1);
          lcd.print("0");
          lcd.setCursor(15, 1);
        } else {
          lcd.setCursor(14, 1);
        }
        lcd.print(second);
    }
}