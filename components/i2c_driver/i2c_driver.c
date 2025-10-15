#include "i2c_driver.h"
#include "driver/i2c.h"
#define I2C_PORT I2C_NUM_0
#define SDA_PIN 8
#define SCL_PIN 9
#define MCP3221_ADDR 0x4D

void i2cScan() {
  Serial.println("Scanning...");
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    uint8_t err = Wire.endTransmission();
    if (err == 0) {
      Serial.print("Found 0x"); Serial.println(addr, HEX);
    }
  }
}

bool mcp3221Read(uint16_t &code) {
  Wire.requestFrom(MCP3221_ADDR, 2);
  if (Wire.available() < 2) return false;
  uint8_t msb = Wire.read();
  uint8_t lsb = Wire.read();
  uint16_t raw = ((uint16_t)msb << 8) | lsb;
  raw >>= 4;                 // 12-bit right-aligned
  code = raw & 0x0FFF;
  return true;
}

void setup() {
  Serial.begin(115200);
  // For ESP32, you can pass custom pins:
  Wire.begin(SDA_PIN, SCL_PIN, 400000); // 400 kHz
  delay(100);
  i2cScan();
}

void loop() {
  uint16_t code;
  if (mcp3221Read(code)) {
    float volts = (code / 4095.0f) * 3.3f;
    Serial.print("MCP3221: code="); Serial.print(code);
    Serial.print("  V="); Serial.println(volts, 3);
  } else {
    Serial.println("MCP3221 read failed");
  }
  delay(200);
}