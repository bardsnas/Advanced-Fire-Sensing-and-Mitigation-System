// Include necessary libraries
#include <Wire.h>
#include <Adafruit_AM2320.h>
#include <LiquidCrystal_I2C.h>

// Initialize AM2320 sensor
Adafruit_AM2320 am2320 = Adafruit_AM2320();

// Initialize 16x2 LCD (I2C address: 0x27, 16 columns, 2 rows)
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
    // Initialize I2C communication
    Wire.begin();
    
    // Start the LCD
    lcd.init();
    lcd.backlight(); // Turn on backlight

    // Start the sensor
    if (!am2320.begin()) {
        lcd.setCursor(0, 0);
        lcd.print("Sensor Error!");
        while (1); // Stop execution if sensor fails
    }
    
    lcd.setCursor(0, 0);
    lcd.print("AM2320 Ready");
    delay(2000);
    lcd.clear();
}