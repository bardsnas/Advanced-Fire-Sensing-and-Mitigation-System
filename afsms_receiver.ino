#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <esp_now.h>
#include <WiFi.h>


LiquidCrystal_I2C lcd(0x27, 16, 2);
volatile bool messageReceived = false;
int messageLength = 0;
uint8_t messageData[16];

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


// =========> TODO: Write your timer ISR here.


void setup() {
    WiFi.mode(WIFI_STA);
    // Initialize LCD display
    lcd.init();
    lcd.backlight();

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
        lcd.setCursor(7, 1);
        // Prints contents of message on 2nd line of LCD.
        for (int i = 0; i < messageLength; i++) {
            lcd.print(messageData[i], DEC);
        }
    } else {
        lcd.setCursor(0, 0);
        lcd.print("FWI is below");
        lcd.print("dangerous level.");
    }
}
