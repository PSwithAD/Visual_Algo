// 실행 명령어 형식: g++ -std=c++17 -DTARGET_PC "경로/파일명.cpp" -o
// "경로/파일명(.cpp 없음)"

#ifdef TARGET_PC
// ================= PC 빌드용 스텁 =================
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <math.h>
#include <thread>

#define F(x) x
#define HEX 16
#define A2 0

inline void delay(unsigned long ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

struct SerialType {
  void begin(unsigned long) {}

  void print(const char *s) { std::printf("%s", s); }
  void print(char c) { std::printf("%c", c); }
  void print(int v) { std::printf("%d", v); }
  void print(unsigned int v) { std::printf("%u", v); }
  void print(long v) { std::printf("%ld", v); }
  void print(unsigned long v) { std::printf("%lu", v); }
  void print(uint8_t v) { std::printf("%u", v); }

  void print(uint8_t v, int base) {
    if (base == HEX) {
      std::printf("%X", v);
    } else {
      std::printf("%u", v);
    }
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

  int available() { return 0; }
  int read() { return -1; }
} Serial;

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

// Naive String Matching Visualization (Mirrored)

// ============================================================================
// 1. 하드웨어 설정
// ============================================================================

#define LED_PIN A2
#define LED_COUNT 256
#define LED_WIDTH 16
#define LED_HEIGHT 16

// ============================================================================
// 2. 전역 변수
// ============================================================================

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
int currentFrameNumber = 0;
uint8_t lastBrightness = 255;
float animationSpeed = 1.0;

// ============================================================================
// 3. 좌표 변환 함수 (좌우 반전 적용)
// ============================================================================

int xyToIndex(int x, int y) {
  // [수정] X 좌표를 반전시켜 좌우 대칭 문제 해결
  x = LED_WIDTH - 1 - x;

  if (y % 2 == 0) {
    return y * LED_WIDTH + x;
  } else {
    return y * LED_WIDTH + (LED_WIDTH - 1 - x);
  }
}

// ============================================================================
// 4. 유틸리티 함수
// ============================================================================

void printHex(uint8_t value) {
  if (value < 16)
    Serial.print("0");
  Serial.print(value, HEX);
}

void serialPrintFrame() {
  Serial.print(F("FRAME:"));
  Serial.println(currentFrameNumber);

  for (int y = 0; y < LED_HEIGHT; y++) {
    for (int x = 0; x < LED_WIDTH; x++) {
      // PC 시뮬레이션에서는 반전된 좌표를 다시 원래대로 돌려서 보여줄 필요가
      // 있을 수도 있지만, 여기서는 하드웨어 동작 확인이 우선이므로 xyToIndex를
      // 그대로 사용합니다. 따라서 PC 화면상에서는 좌우가 반전되어 보일 수
      // 있습니다.
      int ledIndex = xyToIndex(x, y);
      uint32_t color = strip.getPixelColor(ledIndex);

      uint8_t r = (color >> 16) & 0xFF;
      uint8_t g = (color >> 8) & 0xFF;
      uint8_t b = color & 0xFF;

      printHex(r);
      printHex(g);
      printHex(b);

      if (x < LED_WIDTH - 1)
        Serial.print(" ");
    }
    Serial.println();
  }

  Serial.println(F("---"));
  currentFrameNumber++;
}

// ============================================================================
// 5. 인터페이스 함수
// ============================================================================

void setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
  if (x < 0 || x >= LED_WIDTH || y < 0 || y >= LED_HEIGHT)
    return;
  int index = xyToIndex(x, y);
  strip.setPixelColor(index, strip.Color(r, g, b));
}

void clearDisplay() { strip.clear(); }

void showDisplay() {
  uint8_t currentBrightness = strip.getBrightness();
  if (currentBrightness != lastBrightness) {
    Serial.print(F("BRIGHTNESS:"));
    Serial.println(currentBrightness);
    lastBrightness = currentBrightness;
  }

  strip.show();
  serialPrintFrame();
}

void setBrightness(uint8_t level) { strip.setBrightness(level); }

void hardwareDelay(unsigned long ms) {
  if (animationSpeed <= 0.0)
    animationSpeed = 1.0;
  delay((unsigned long)(ms / animationSpeed));
}

// ============================================================================
// 6. 색상 팔레트 (drawItem 함수 사용)
// ============================================================================

void drawItem(int x, int y, int value) {
  static const uint8_t palette[][3] = {
      {255, 0, 0},    // 0: A - 빨강
      {0, 255, 0},    // 1: B - 초록
      {0, 0, 255},    // 2: C - 파랑
      {255, 255, 0},  // 3: 노랑
      {255, 0, 255},  // 4: 자홍
      {0, 255, 255},  // 5: 청록
      {255, 120, 0},  // 6: 주황
      {200, 200, 200} // 7: 흰색
  };

  if (value >= 0 && value <= 7) {
    setPixel(x, y, palette[value][0], palette[value][1], palette[value][2]);
  }
}

// ============================================================================
// 7. String Matching 시각화
// ============================================================================

// Text: ABCABDABCABCABEF (길이 16)
// Pattern: ABCABE (길이 6)
// 예상 매칭 위치: 6
int T[] = {0, 1, 2, 0, 1, 3, 0, 1, 2, 0, 1, 2, 0, 1, 4, 5};
int P[] = {0, 1, 2, 0, 1, 4};

int T_len = 16;
int P_len = 6;

int matchedPositions[10];
int matchedCount = 0;
int totalComparisons = 0;

void drawMatchingState(int shift, int compareIdx, bool isMatch) {
  clearDisplay();

  // 상단: T 패턴 (행 0)
  for (int i = 0; i < T_len && i < LED_WIDTH; i++) {
    drawItem(i, 0, T[i]);
  }

  // 상단: P 패턴 (행 2)
  for (int i = 0; i < P_len && i < LED_WIDTH; i++) {
    drawItem(i, 2, P[i]);
  }

  // 중간: T 배열 (행 7)
  for (int i = 0; i < T_len && i < LED_WIDTH; i++) {
    drawItem(i, 7, T[i]);
  }

  // 이전 매칭 위치 표시 (행 9)
  for (int m = 0; m < matchedCount; m++) {
    int matchPos = matchedPositions[m];
    for (int i = 0; i < P_len; i++) {
      int xPos = matchPos + i;
      if (xPos >= 0 && xPos < LED_WIDTH) {
        drawItem(xPos, 9, P[i]);
      }
    }
  }

  // 현재 P 배열 (행 11)
  for (int i = 0; i < P_len; i++) {
    int xPos = shift + i;
    if (xPos >= 0 && xPos < LED_WIDTH) {
      drawItem(xPos, 11, P[i]);
    }
  }

  // 인디케이터
  if (compareIdx >= 0 && compareIdx < P_len) {
    int indicatorX = shift + compareIdx;
    if (indicatorX >= 0 && indicatorX < LED_WIDTH) {
      setPixel(indicatorX, 8, 255, 255, 255);
      setPixel(indicatorX, 12, 255, 255, 255);
      setPixel(indicatorX, 13, 255, 255, 255);
    }
  }

  // 매칭 성공 강조
  if (isMatch) {
    for (int i = 0; i < P_len; i++) {
      int xPos = shift + i;
      if (xPos >= 0 && xPos < LED_WIDTH) {
        setPixel(xPos, 10, 255, 255, 255);
      }
    }
  }

  showDisplay();
}

void naiveStringMatching() {
  Serial.println(F("\n=== Naive String Matching Start (Mirrored) ==="));

  int n = T_len;
  int m = P_len;

  matchedCount = 0;
  totalComparisons = 0;

  drawMatchingState(0, -1, false);
  hardwareDelay(1000);

  for (int shift = 0; shift <= n - m; shift++) {
    Serial.print(F("Trying shift: "));
    Serial.println(shift);

    bool matched = true;
    int compareIdx = 0;

    for (int i = 0; i < m; i++) {
      compareIdx = i;
      totalComparisons++;

      drawMatchingState(shift, compareIdx, false);
      hardwareDelay(500);

      if (T[shift + i] != P[i]) {
        matched = false;
        Serial.print(F("  Mismatch at index "));
        Serial.println(i);
        hardwareDelay(500);
        break;
      } else {
        Serial.print(F("  Match at index "));
        Serial.println(i);
      }
    }

    if (matched) {
      Serial.print(F("Pattern found at shift "));
      Serial.println(shift);

      drawMatchingState(shift, -1, true);
      hardwareDelay(1500);

      if (matchedCount < 10) {
        matchedPositions[matchedCount++] = shift;
      }
    }

    hardwareDelay(300);
  }

  Serial.print(F("Total comparisons: "));
  Serial.println(totalComparisons);
  Serial.println(F("=== Naive String Matching End ===\n"));
  hardwareDelay(2000);
}

// ============================================================================
// 8. Setup & Loop
// ============================================================================

void setup() {
  Serial.begin(115200);
  delay(2000);

#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif

  strip.begin();
  strip.show();
  strip.setBrightness(15);

  Serial.println(F("=== LED Display Simulator (Mirrored) ==="));
  Serial.print(F("SIZE:"));
  Serial.print(LED_WIDTH);
  Serial.print(F("x"));
  Serial.println(LED_HEIGHT);
  Serial.print(F("BRIGHTNESS:"));
  Serial.println(strip.getBrightness());
  Serial.println(F("========================================"));

  lastBrightness = strip.getBrightness();
  currentFrameNumber = 0;

  Serial.println(F(""));
  Serial.println(F("========================================"));
  Serial.println(F("   Naive String Matching Visualization"));
  Serial.println(F("========================================"));
  Serial.println(F(""));

  Serial.print(F("Text T (ABCABDABCABCABEF): "));
  for (int i = 0; i < T_len; i++) {
    Serial.print(T[i]);
    Serial.print(" ");
  }
  Serial.println();

  Serial.print(F("Pattern P (ABCABE): "));
  for (int i = 0; i < P_len; i++) {
    Serial.print(P[i]);
    Serial.print(" ");
  }
  Serial.println();
}

void loop() {
  while (Serial.available() > 0) {
    Serial.read();
  }

  naiveStringMatching();

  hardwareDelay(3000);
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
