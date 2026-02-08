#include <Wire.h>
#include <U8g2lib.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// SH1106 128x64, hardware I2C, no reset pin
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

const int MPU_ADDR_RIGHT = 0x68;
const int MPU_ADDR_LEFT = 0x69;

int16_t axRight, ayRight, azRight;
int16_t axLeft, ayLeft, azLeft;

const int emgPinRight = PA0;  // Right EMG sensor pin
const int emgPinLeft = PA1;   // Left EMG sensor pin

void setup() {
  Wire.begin();
  Serial.begin(115200);

  u8g2.begin();

  // Wake up MPU6050 Right
  Wire.beginTransmission(MPU_ADDR_RIGHT);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  // Wake up MPU6050 Left
  Wire.beginTransmission(MPU_ADDR_LEFT);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  Serial.println("System ready");
  delay(1000);
}

void loop() {
  // Read EMG sensors
  int emgValueRight = analogRead(emgPinRight);
  int emgValueLeft = analogRead(emgPinLeft);

  // Read MPU6050 data
  readMPU6050(MPU_ADDR_RIGHT, &axRight, &ayRight, &azRight);
  readMPU6050(MPU_ADDR_LEFT, &axLeft, &ayLeft, &azLeft);

  // Determine individual gestures
  char gestureRight = ' ';
  if (axRight > 10000) gestureRight = 'B';  // Back
  else if (axRight < -10000) gestureRight = 'A';  // Front
  else if (ayRight > 10000) gestureRight = 'C';  // Right
  else if (ayRight < -10000) gestureRight = 'D';  // Left

  char gestureLeft = ' ';
  if (axLeft > 10000) gestureLeft = 'F';  // Back
  else if (axLeft < -10000) gestureLeft = 'E';  // Front
  else if (ayLeft > 10000) gestureLeft = 'G';  // Right
  else if (ayLeft < -10000) gestureLeft = 'H';  // Left

  // EMG thresholds
  int emgThresholdRight = 100;
  int emgThresholdLeft = 600;

  // Determine combined gesture
  char combinedGesture = ' ';

  if (emgValueRight > emgThresholdRight && emgValueLeft > emgThresholdLeft) {
    // Both same directions
    if (axRight < -10000 && axLeft < -10000) combinedGesture = 'I';  // Both Front

    else if (axRight > 10000 && axLeft > 10000) combinedGesture = 'J';  // Both Back
    else if (ayRight > 10000 && ayLeft > 10000) combinedGesture = 'K';  // Both Right
    else if (ayRight < -10000 && ayLeft < -10000) combinedGesture = 'M';  // Both Left

    // Opposite & new patterns
    else if (axLeft < -10000 && axRight > 10000) combinedGesture = 'N';  // Left Front, Right Back
    else if (axLeft > 10000 && axRight < -10000) combinedGesture = 'O';  // Left Back, Right Front
    else if (ayLeft < -10000 && ayRight > 10000) combinedGesture = 'P';  // Left Left, Right Right
    else if (ayLeft > 10000 && ayRight < -10000) combinedGesture = 'Q';  // Left Right, Right Left

    // Cross combinations
    else if (axLeft < -10000 && ayRight < -10000) combinedGesture = 'R';  // Left Front, Right Left
    else if (axLeft < -10000 && ayRight > 10000) combinedGesture = 'S';  // Left Front, Right Right
    else if (axLeft > 10000 && ayRight < -10000) combinedGesture = 'T';  // Left Back, Right Left
    else if (axLeft > 10000 && ayRight > 10000) combinedGesture = 'U';  // Left Back, Right Right
    else if (ayLeft < -10000 && axRight < -10000) combinedGesture = 'V';  // Left Left, Right Front
    else if (ayLeft > 10000 && axRight < -10000) combinedGesture = 'W';  // Left Right, Right Front
    else if (ayLeft < -10000 && axRight > 10000) combinedGesture = 'X';  // Left Left, Right Back
    else if (ayLeft > 10000 && axRight > 10000) combinedGesture = 'Y';  // Left Right, Right Back
  }

  // Clear display
  u8g2.clearBuffer();
  delay(100);  // Small delay to avoid flicker

  // Top-left corner: left EMG value
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.setCursor(0, 8);  // Top-left
  u8g2.print("L:");
  u8g2.print(emgValueLeft);

  // Top-right corner: right EMG value
  String emgRightStr = "R:" + String(emgValueRight);
  uint16_t wRightStr = u8g2.getStrWidth(emgRightStr.c_str());
  u8g2.setCursor(SCREEN_WIDTH - wRightStr, 8);  // Top-right
  u8g2.print(emgRightStr);

  // Determine which gesture letter to show in center
  char letterToDisplay = ' ';
  if (combinedGesture != ' ') {
    letterToDisplay = combinedGesture;
    Serial.print("Combined Gesture: ");
    Serial.println(letterToDisplay);
  } else if (emgValueRight > emgThresholdRight && gestureRight != ' ') {
    letterToDisplay = gestureRight;
    Serial.print("Right Gesture: ");
    Serial.println(letterToDisplay);
  } else if (emgValueLeft > emgThresholdLeft && gestureLeft != ' ') {
    letterToDisplay = gestureLeft;
    Serial.print("Left Gesture: ");
    Serial.println(letterToDisplay);
  }

  // Centered gesture letter
  if (letterToDisplay != ' ') {
    u8g2.setFont(u8g2_font_fub35_tr);
    char buf[2] = {letterToDisplay, '\0'};
    uint16_t w = u8g2.getStrWidth(buf);
    uint16_t h = u8g2.getMaxCharHeight();
    int16_t x = (SCREEN_WIDTH - w) / 2;
    int16_t y = (SCREEN_HEIGHT + h) / 2;
    u8g2.drawStr(x, y, buf);
  }

  u8g2.sendBuffer();
  delay(100);
}

void readMPU6050(int addr, int16_t* ax, int16_t* ay, int16_t* az) {
  Wire.beginTransmission(addr);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(addr, 6, true);

  *ax = Wire.read() << 8 | Wire.read();
  *ay = Wire.read() << 8 | Wire.read();
  *az = Wire.read() << 8 | Wire.read();
}
