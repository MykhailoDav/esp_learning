#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN 48
#define NUM_PIXELS 1

Adafruit_NeoPixel strip(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup()
{
  Serial.begin(115200);

  delay(500);
  Serial.println("\n\n=== Starting WS2812 RGB LED test ===");
  Serial.println("Board: 4D Systems ESP32-S3");
  Serial.println("LED Pin: GPIO 48");

  strip.begin();
  strip.setBrightness(100); // Set brightness (0-255)
  strip.show();

  Serial.println("Setup complete!\n");
}

void loop()
{
  // Red
  strip.setPixelColor(0, strip.Color(255, 0, 0));
  strip.show();
  Serial.println("RED");
  delay(1000);

  // Green
  strip.setPixelColor(0, strip.Color(0, 255, 0));
  strip.show();
  Serial.println("GREEN");
  delay(1000);

  // Blue
  strip.setPixelColor(0, strip.Color(0, 0, 255));
  strip.show();
  Serial.println("BLUE");
  delay(1000);

  // Yellow
  strip.setPixelColor(0, strip.Color(255, 255, 0));
  strip.show();
  Serial.println("YELLOW");
  delay(1000);

  // Cyan
  strip.setPixelColor(0, strip.Color(0, 255, 255));
  strip.show();
  Serial.println("CYAN");
  delay(1000);

  // Magenta
  strip.setPixelColor(0, strip.Color(255, 0, 255));
  strip.show();
  Serial.println("MAGENTA");
  delay(1000);

  // White
  strip.setPixelColor(0, strip.Color(255, 255, 255));
  strip.show();
  Serial.println("WHITE");
  delay(1000);

  // Off
  strip.setPixelColor(0, strip.Color(0, 0, 0));
  strip.show();
  Serial.println("OFF");
  delay(1000);
}