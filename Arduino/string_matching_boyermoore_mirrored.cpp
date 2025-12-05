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

// Boyer-Moore String Matching Visualization (Mirrored)

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

#define ALPHABET_SIZE 8
int badChar[ALPHABET_SIZE]; // Bad Character Table
int matchedPositions[10];
int matchedCount = 0;
int totalComparisons = 0;

void drawMatchingState(int shift, int compareIdx, bool isMatch,
                       bool showBadChar) {
  clearDisplay();

  // 상단: T 패턴 (행 0)
  for (int i = 0; i < T_len && i < LED_WIDTH; i++) {
    drawItem(i, 0, T[i]);
  }

  // 상단: P 패턴 (행 2)
  for (int i = 0; i < P_len && i < LED_WIDTH; i++) {
    drawItem(i, 2, P[i]);
  }

  // Bad Character Table 표시 (행 3, P 아래)
  if (showBadChar) {
    for (int i = 0; i < P_len && i < LED_WIDTH; i++) {
      int charValue = P[i];
      int shift_amount = badChar[charValue];
      if (shift_amount >= 0) {
        uint8_t brightness = (shift_amount + 1) * 50;
        if (brightness > 255)
          brightness = 255;
        setPixel(i, 3, brightness, brightness / 2, 0);
      }
    }
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

  // 인디케이터 (우->좌 비교 표시)
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

void computeBadCharTable() {
  Serial.println(F("\n=== Computing Bad Character Table ==="));

  // 초기화: 모든 문자는 패턴에 없음 (-1)
  for (int i = 0; i < ALPHABET_SIZE; i++) {
    badChar[i] = -1;
  }

  // 패턴의 각 문자에 대해 마지막 출현 위치 저장
  for (int i = 0; i < P_len; i++) {
    badChar[P[i]] = i;
  }

  Serial.print(F("Bad Character Table: "));
  for (int i = 0; i < ALPHABET_SIZE; i++) {
    Serial.print(badChar[i]);
    Serial.print(" ");
  }
  Serial.println();
  Serial.println(F("=== Bad Character Table Computed ===\n"));
}

int maxVal(int a, int b) { return (a > b) ? a : b; }

void boyerMooreStringMatching() {
  Serial.println(F("\n=== Boyer-Moore String Matching Start (Mirrored) ==="));

  int n = T_len;
  int m = P_len;

  matchedCount = 0;
  totalComparisons = 0;

  drawMatchingState(0, -1, false, true);
  hardwareDelay(1000);

  int shift = 0;

  while (shift <= n - m) {
    Serial.print(F("Trying shift: "));
    Serial.println(shift);

    // 패턴을 오른쪽에서 왼쪽으로 비교
    int j = m - 1;

    // 오른쪽부터 비교하면서 일치하는 동안 계속
    while (j >= 0) {
      totalComparisons++;

      drawMatchingState(shift, j, false, true);
      hardwareDelay(500);

      if (P[j] != T[shift + j]) {
        Serial.print(F("  Mismatch at pattern index "));
        Serial.print(j);
        Serial.print(F(", text char: "));
        Serial.println(T[shift + j]);
        hardwareDelay(500);
        break;
      } else {
        Serial.print(F("  Match at pattern index "));
        Serial.println(j);
      }

      j--;
    }

    // 패턴이 완전히 매칭됨
    if (j < 0) {
      Serial.print(F("Pattern found at shift "));
      Serial.println(shift);

      drawMatchingState(shift, -1, true, true);
      hardwareDelay(1500);

      if (matchedCount < 10) {
        matchedPositions[matchedCount++] = shift;
      }

      // 다음 매칭을 찾기 위해 shift
      // 패턴 끝을 넘어서는 문자가 있으면 그것 기준, 아니면 1칸만 이동
      shift += (shift + m < n) ? m - badChar[T[shift + m]] : 1;
    } else {
      // 불일치 발생 - Bad Character Rule 적용
      int badCharShift = j - badChar[T[shift + j]];
      Serial.print(F("  Bad Character Rule: shift by "));
      Serial.println(maxVal(1, badCharShift));

      shift += maxVal(1, badCharShift);
    }

    hardwareDelay(300);
  }

  Serial.print(F("Total comparisons: "));
  Serial.println(totalComparisons);
  Serial.println(F("=== Boyer-Moore String Matching End ===\n"));
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
  Serial.println(F("  Boyer-Moore String Matching Visualization"));
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

  computeBadCharTable();
}

void loop() {
  while (Serial.available() > 0) {
    Serial.read();
  }

  boyerMooreStringMatching();

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
