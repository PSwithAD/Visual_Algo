// 통합 하드웨어 인터페이스 V2 - Arduino 단일 파일 템플릿
// territory_game_flip_v.cpp의 검증된 로직(상하 반전, 핀 매핑) 적용
// .ino 파일로 복사해서 바로 사용 가능

#ifdef TARGET_PC
// ================= PC 빌드용 스텁 =================
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

#define F(x) x
#define HEX 16
#define A2 0
#define INPUT_PULLUP 0
#define OUTPUT 1
#define HIGH 1
#define LOW 0

// 시간 함수 모의
inline void delay(unsigned long ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

inline void delayMicroseconds(int us) {
  std::this_thread::sleep_for(std::chrono::microseconds(us));
}

unsigned long millis() {
  using namespace std::chrono;
  static auto start = steady_clock::now();
  auto now = steady_clock::now();
  return duration_cast<milliseconds>(now - start).count();
}

// GPIO 함수 모의
void pinMode(int pin, int mode) {}
void digitalWrite(int pin, int value) {}
int digitalRead(int pin) { return 1; } // 기본적으로 눌리지 않음(HIGH)

// Serial 모의 클래스 (통합)
struct SerialMock {
  void begin(unsigned long) {}

  void print(const char *s) { std::printf("%s", s); }
  void print(char c) { std::printf("%c", c); }
  void print(int v) { std::printf("%d", v); }
  void print(unsigned int v) { std::printf("%u", v); }
  void print(long v) { std::printf("%ld", v); }
  void print(unsigned long v) { std::printf("%lu", v); }
  void print(uint8_t v) { std::printf("%u", v); }

  void print(uint8_t v, int base) {
    if (base == HEX)
      std::printf("%X", v);
    else
      std::printf("%u", v);
  }

  void println() {
    std::printf("\n");
    std::fflush(stdout);
  }
  void println(const char *s) {
    std::printf("%s\n", s);
    std::fflush(stdout);
  }
  void println(char c) {
    std::printf("%c\n", c);
    std::fflush(stdout);
  }
  void println(int v) {
    std::printf("%d\n", v);
    std::fflush(stdout);
  }
  void println(unsigned int v) {
    std::printf("%u\n", v);
    std::fflush(stdout);
  }
  void println(long v) {
    std::printf("%ld\n", v);
    std::fflush(stdout);
  }
  void println(unsigned long v) {
    std::printf("%lu\n", v);
    std::fflush(stdout);
  }
  void println(uint8_t v) {
    std::printf("%u\n", v);
    std::fflush(stdout);
  }

  int available() { return 0; } // PC에서는 직접 입력 받음
  int read() { return -1; }

  // PC 입력 처리
  int parseInt() {
    char buffer[10];
    if (fgets(buffer, sizeof(buffer), stdin)) {
      return atoi(buffer);
    }
    return -1;
  }
} Serial;

// NeoPixel 모의 클래스
#define NEO_GRB 0
#define NEO_KHZ800 0

class Adafruit_NeoPixel {
public:
  Adafruit_NeoPixel(int n, int /*pin*/, int /*flags*/)
      : _n(n), _brightness(20) {
    _pixels = new uint32_t[_n];
    clear();
  }
  ~Adafruit_NeoPixel() { delete[] _pixels; }

  void begin() {}
  void show() {}
  void setBrightness(uint8_t b) { _brightness = b; }
  uint8_t getBrightness() const { return _brightness; }
  void clear() {
    for (int i = 0; i < _n; ++i)
      _pixels[i] = 0;
  }

  uint32_t Color(uint8_t r, uint8_t g, uint8_t b) {
    return (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(b);
  }

  void setPixelColor(int i, uint32_t c) {
    if (i >= 0 && i < _n)
      _pixels[i] = c;
  }

  uint32_t getPixelColor(int i) const {
    if (i >= 0 && i < _n)
      return _pixels[i];
    return 0;
  }

private:
  int _n;
  uint8_t _brightness;
  uint32_t *_pixels;
};

#else
// ================= 실제 Arduino 빌드용 =================
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif
#endif

// ============================================================================
// 1. 하드웨어 설정 (Configuration)
// ============================================================================

// 자석 모듈 핀 설정 (6x6 그리드)
// Row output pins (6개): 13, 12, 11, 10, 9, 8
#define ROW_PIN_START 13
#define ROW_PIN_END 8
#define ROW_COUNT 6

// Column input pins (6개): 7, 6, 5, 4, 3, 2
#define COL_PIN_START 7
#define COL_PIN_END 2
#define COL_COUNT 6

// LED 설정
#define LED_PIN A2
#define LED_COUNT 256
#define W 16
#define H 16

// 전역 객체
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// ============================================================================
// 2. 자료형 및 전역 변수
// ============================================================================

enum InputMode {
  INPUT_MAGNETIC, // 자석 모듈 입력
  INPUT_SERIAL    // 시리얼 입력
};

// 내부 상태
static unsigned long lastInputTime = 0;
static const unsigned long DEBOUNCE_DELAY = 200;
static int lastNodeInput = -1;
InputMode currentInputMode = INPUT_SERIAL;

// 디스플레이 상태
int currentFrameNumber = 0;
uint8_t lastBrightness = 255;
float animationSpeed = 1.0;

// ============================================================================
// 3. 좌표 변환 함수 (핵심 로직)
// ============================================================================

// 자석 모듈 (행, 열) -> 노드 번호 (0~35)
int coordToNode(int row, int col) {
  if (row < 0 || row >= ROW_COUNT || col < 0 || col >= COL_COUNT) {
    return -1;
  }
  return row * COL_COUNT + col;
}

// 노드 번호 -> 자석 모듈 (행, 열)
void nodeToCoord(int node, int *row, int *col) {
  if (node < 0 || node >= ROW_COUNT * COL_COUNT) {
    *row = -1;
    *col = -1;
    return;
  }
  *row = node / COL_COUNT;
  *col = node % COL_COUNT;
}

// LED 좌표 (x, y) -> LED 인덱스 (0~255)
// [상하 반전 적용됨: territory_game_flip_v.cpp 로직]
int xyToIndex(int x, int y) {
  // y 좌표를 반전 (0 -> 15, 15 -> 0)
  int mirroredY = (H - 1) - y;

  if (mirroredY % 2 == 0)
    return mirroredY * W + x;
  else
    return mirroredY * W + (W - 1 - x);
}

// 노드 번호 -> LED 좌표 (x, y)
// 6x6 그리드가 16x16 LED 매트릭스에 꽉 차게 배치되도록 매핑 (3칸 간격)
void nodeToLED(int node, int *ledX, int *ledY) {
  int row, col;
  nodeToCoord(node, &row, &col);

  if (row >= 0 && col >= 0) {
    *ledX = col * 3;
    *ledY = row * 3;
  } else {
    *ledX = -1;
    *ledY = -1;
  }
}

// ============================================================================
// 4. 입력 함수
// ============================================================================

void inputInit() {
  // Row 핀 (13~8): OUTPUT으로 설정
  for (int pin = ROW_PIN_END; pin <= ROW_PIN_START; pin++) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH); // 기본값 HIGH
  }

  // Column 핀 (7~2): INPUT_PULLUP으로 설정
  for (int pin = COL_PIN_END; pin <= COL_PIN_START; pin++) {
    pinMode(pin, INPUT_PULLUP);
  }

  lastInputTime = 0;
  lastNodeInput = -1;
}

int readMagneticInput() {
#ifdef TARGET_PC
  return -1;
#else
  unsigned long currentTime = millis();

  if (currentTime - lastInputTime < DEBOUNCE_DELAY) {
    return -1;
  }

  for (int row = 0; row < ROW_COUNT; row++) {
    int rowPin = ROW_PIN_START - row;
    digitalWrite(rowPin, LOW);
    delayMicroseconds(10);

    for (int col = 0; col < COL_COUNT; col++) {
      int colPin = COL_PIN_START - col;
      if (digitalRead(colPin) == LOW) {
        int nodeNum = coordToNode(row, col);
        digitalWrite(rowPin, HIGH);
        for (int r = 0; r < ROW_COUNT; r++)
          digitalWrite(ROW_PIN_START - r, HIGH);

        if (nodeNum == lastNodeInput)
          return -1;

        lastInputTime = currentTime;
        lastNodeInput = nodeNum;
        return nodeNum;
      }
    }
    digitalWrite(rowPin, HIGH);
  }
  return -1;
#endif
}

int readSerialInput() {
#ifdef TARGET_PC
  char buffer[10];
  if (fgets(buffer, sizeof(buffer), stdin)) {
    int node = atoi(buffer);
    if (node >= 0 && node < ROW_COUNT * COL_COUNT) {
      if (node == lastNodeInput)
        return -1;
      lastNodeInput = node;
      return node;
    }
  }
  return -1;
#else
  if (Serial.available() > 0) {
    int node = Serial.parseInt();
    if (node >= 0 && node < ROW_COUNT * COL_COUNT) {
      while (Serial.available() > 0)
        Serial.read();
      if (node == lastNodeInput)
        return -1;
      lastNodeInput = node;
      return node;
    }
  }
  return -1;
#endif
}

int readInput(InputMode mode) {
  if (mode == INPUT_MAGNETIC)
    return readMagneticInput();
  else
    return readSerialInput();
}

void setInputMode(InputMode mode) {
  currentInputMode = mode;
  if (mode == INPUT_MAGNETIC)
    Serial.println(F("Input mode: Magnetic sensor"));
  else
    Serial.println(F("Input mode: Serial"));
}

// ============================================================================
// 5. 출력 함수 (LED & Serial)
// ============================================================================

void printHex(uint8_t value) {
  if (value < 16)
    Serial.print("0");
  Serial.print(value, HEX);
}

void serialPrintHeader() {
#if USE_SERIAL
  Serial.println(F("=== Unified Hardware Template V2 ==="));
  Serial.print(F("SIZE:"));
  Serial.print(W);
  Serial.print(F("x"));
  Serial.println(H);
  Serial.print(F("BRIGHTNESS:"));
  Serial.println(strip.getBrightness());
  Serial.println(F("============================"));
#endif
}

void serialPrintBrightnessChange() {
#if USE_SERIAL
  uint8_t currentBrightness = strip.getBrightness();
  if (currentBrightness != lastBrightness) {
    Serial.print(F("BRIGHTNESS:"));
    Serial.println(currentBrightness);
    lastBrightness = currentBrightness;
  }
#endif
}

void serialPrintFrame() {
#if USE_SERIAL
  Serial.print(F("FRAME:"));
  Serial.println(currentFrameNumber);
  for (int y = 0; y < H; y++) {
    for (int x = 0; x < W; x++) {
      int ledIndex = xyToIndex(x, y);
      uint32_t color = strip.getPixelColor(ledIndex);
      uint8_t r = (color >> 16) & 0xFF;
      uint8_t g = (color >> 8) & 0xFF;
      uint8_t b = color & 0xFF;
      printHex(r);
      printHex(g);
      printHex(b);
      if (x < W - 1)
        Serial.print(" ");
    }
    Serial.println();
  }
  Serial.println(F("---"));
  currentFrameNumber++;
#endif
}

void displayInit() {
#if USE_SERIAL
  Serial.begin(115200);
  delay(2000);
#endif

#ifndef TARGET_PC
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  strip.begin();
  strip.show();
  strip.setBrightness(20);
#else
  strip.begin();
  strip.setBrightness(20);
#endif

  serialPrintHeader();
  lastBrightness = strip.getBrightness();
}

void setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
  if (x < 0 || x >= W || y < 0 || y >= H)
    return;
  int index = xyToIndex(x, y);
  strip.setPixelColor(index, strip.Color(r, g, b));
}

void displayShow() {
  serialPrintBrightnessChange();
#ifndef TARGET_PC
  strip.show();
#endif
  serialPrintFrame();
}

void displayClear() { strip.clear(); }

void setBrightness(uint8_t level) { strip.setBrightness(level); }

void displayDelay(unsigned long ms) {
  if (animationSpeed <= 0.0)
    animationSpeed = 1.0;
  delay((unsigned long)(ms / animationSpeed));
}

// ============================================================================
// 6. 고수준 헬퍼 함수
// ============================================================================

// 노드 강조 표시
void highlightNode(int node, uint8_t r, uint8_t g, uint8_t b) {
  int ledX, ledY;
  nodeToLED(node, &ledX, &ledY);
  if (ledX >= 0 && ledY >= 0) {
    setPixel(ledX, ledY, r, g, b);
  }
}

// ============================================================================
// 7. 메인 Setup & Loop (예제)
// ============================================================================

void setup() {
  displayInit();
  inputInit();

  Serial.println(F("Setup Complete."));

#ifdef TARGET_PC
  setInputMode(INPUT_SERIAL);
#else
  setInputMode(INPUT_MAGNETIC);
#endif

  // 초기화 테스트: 모든 노드 위치에 회색 점 표시
  displayClear();
  for (int i = 0; i < 36; i++) {
    highlightNode(i, 20, 20, 20);
  }
  displayShow();
}

void loop() {
  // 예제: 입력받은 노드를 초록색으로 표시하고 깜빡임

  // 입력 대기 중 깜빡임 처리 (Non-blocking)
  static unsigned long lastBlinkTime = 0;
  static bool blinkState = false;
  static int selectedNode = -1;

  unsigned long currentMillis = millis();

  // 입력 읽기
  int inputNode = readInput(currentInputMode);
  if (inputNode >= 0 && inputNode < ROW_COUNT * COL_COUNT) {
    selectedNode = inputNode;
    Serial.print(F("Selected Node: "));
    Serial.println(selectedNode);
  }

  // 200ms마다 깜빡임
  if (currentMillis - lastBlinkTime >= 200) {
    lastBlinkTime = currentMillis;
    blinkState = !blinkState;

    displayClear();

    // 배경: 모든 노드 약하게 표시
    for (int i = 0; i < 36; i++) {
      highlightNode(i, 10, 10, 10);
    }

    // 선택된 노드 깜빡임
    if (selectedNode != -1) {
      if (blinkState) {
        highlightNode(selectedNode, 0, 255, 0); // 켜짐 (초록)
      } else {
        highlightNode(selectedNode, 0, 0, 0); // 꺼짐
      }
    }

    displayShow();
  }

  delay(10);
}

#ifdef TARGET_PC
int main() {
  setup();
  while (true) {
    loop();
  }
  return 0;
}
#endif
