#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1  // Reset pin (not needed)

#define SDA_PIN 8  // Correct I2C SDA pin for ESP32-S3
#define SCL_PIN 7  // Correct I2C SCL pin for ESP32-S3

uint8_t SCREEN_ADDRESS = 0x3C;  // Default I2C address for SSD1306

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void scanI2CDevices() {
    Serial.println("Scanning I2C devices...");
    for (byte address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) {
            Serial.print("Found I2C device at 0x");
            Serial.println(address, HEX);
            SCREEN_ADDRESS = address;  // Use detected address
        }
    }
}

void setup() {
    Serial.begin(115200);
    Wire.begin(SDA_PIN, SCL_PIN);  // Initialize I2C with correct pins

    scanI2CDevices();  // Scan for I2C devices before initializing display

    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        while (1);  // Halt execution if display fails
    }

    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 20);
    display.println("Hello");
    display.display();
}

void loop() {
    // Nothing needed here
}
