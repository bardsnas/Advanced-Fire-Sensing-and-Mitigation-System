#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <esp_now.h>
#include <WiFi.h>


// Initialize the LCD display at I2C address 0x27 with 16 columns and 2 rows
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Flag to indicate if a message has been received
volatile bool messageReceived = false;

// Time tracking variables
int second = 0;
int minute = 0;
int hour = 0;

// Message buffer and length
int messageLength = 0;
uint8_t messageData[16]; // Buffer to hold received message (max 16 characters)

// Timer handle
hw_timer_t * timer = NULL; // Declare a timer variable and initialize to null

// ESP-NOW callback function to handle incoming data
// This callback function will be invoked when signal is received
// over ESP-NOW and trigger the message to the LCD.
void IRAM_ATTR dataReceived(const esp_now_recv_info_t * esp_now_info, const uint8_t *incomingData, int len)
{
    // Set flag to indicate message reception
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

// Timer Interrupt Service Routine (ISR) to update the time counter
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
    // Set WiFi mode to Station (STA) for ESP-NOW communication
    WiFi.mode(WIFI_STA);

    // Initialize LCD display
    lcd.init();
    lcd.backlight();

    // Initialize timer to update the counter every second
    timer = timerBegin(8000); // Set prescaler to 8000
    timerAttachInterrupt(timer, &timerISR); // Attach ISR function to timer
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

        // Display message data on the second line
        lcd.setCursor(0, 1);
        lcd.print("FWI: ");
        lcd.setCursor(5, 1);

        // Prints contents of message on 2nd line of LCD.
        for (int i = 0; i < messageLength; i++) {
            lcd.print(messageData[i], DEC);
        }

        // Reset time counters when a message is received
        second = 0;
        minute = 0;
        hour = 0;

        vTaskDelay(2000); // Delay for 2 seconds
        lcd.clear(); // Remove screen contents.
        messageReceived = false; // Reset message flag

    } else {
        // If no message is received, display elapsed time
        // Display formatted time in HH:MM:SS format on LCD
        lcd.setCursor(8, 1);
        if (hour < 10) lcd.print("0");
        lcd.print(hour);
        lcd.print(":");

        lcd.setCursor(11, 1);
        if (minute < 10) lcd.print("0");
        lcd.print(minute);
        lcd.print(":");

        lcd.setCursor(14, 1);
        if (second < 10) lcd.print("0");
        lcd.print(second);
    }
}