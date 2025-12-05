// 실행 명령어 형식: g++ -std=c++17 -DTARGET_PC "경로/파일명.cpp" -o "경로/파일명(.cpp 없음)"

#ifdef TARGET_PC
// ================= PC 빌드용 스텁 =================
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <thread>
#include <chrono>
#include <math.h>

#define F(x) x
#define HEX 16
#define A2 0

inline void delay(unsigned long ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

struct SerialType {
  void begin(unsigned long) {}

  void print(const char* s)         { std::printf("%s", s); }
  void print(char c)                { std::printf("%c", c); }
  void print(int v)                 { std::printf("%d", v); }
  void print(unsigned int v)        { std::printf("%u", v); }
  void print(long v)                { std::printf("%ld", v); }
  void print(unsigned long v)       { std::printf("%lu", v); }
  void print(uint8_t v)             { std::printf("%u", v); }

  void print(uint8_t v, int base) {
    if (base == HEX) {
      std::printf("%X", v);
    } else {
      std::printf("%u", v);
    }
  }

  void println()                    { std::printf("\n"); std::fflush(stdout); }
  void println(const char* s)       { std::printf("%s\n", s); std::fflush(stdout); }
  void println(char c)              { std::printf("%c\n", c); std::fflush(stdout); }
  void println(int v)               { std::printf("%d\n", v); std::fflush(stdout); }
  void println(unsigned int v)      { std::printf("%u\n", v); std::fflush(stdout); }
  void println(long v)              { std::printf("%ld\n", v); std::fflush(stdout); }
  void println(unsigned long v)     { std::printf("%lu\n", v); std::fflush(stdout); }
  void println(uint8_t v)           { std::printf("%u\n", v); std::fflush(stdout); }

  int available()                   { return 0; }
  int read()                        { return -1; }
} Serial;

#define NEO_GRB   0
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
    for (int i = 0; i < _n; ++i) _pixels[i] = 0;
  }

  uint32_t Color(uint8_t r, uint8_t g, uint8_t b) {
    return (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(b);
  }

  void setPixelColor(int i, uint32_t c) {
    if (i >= 0 && i < _n) _pixels[i] = c;
  }
  uint32_t getPixelColor(int i) const {
    if (i >= 0 && i < _n) return _pixels[i];
    return 0;
  }

private:
  int _n;
  uint8_t _brightness;
  uint32_t* _pixels;
};

#else
// ================= 실제 Arduino 빌드용 =================
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif
#endif

// 8-Queens Backtracking Visualization

// ============================================================================
// 1. 하드웨어 설정
// ============================================================================

#define LED_PIN    A2
#define LED_COUNT  256
#define W 16
#define H 16

// ============================================================================
// 2. 출력 모드 설정
// ============================================================================

#define USE_LED 1
#define USE_SERIAL 1

// ============================================================================
// 3. 전역 변수
// ============================================================================

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
int currentFrameNumber = 0;
uint8_t lastBrightness = 255;

float animationSpeed = 5.0;

// ============================================================================
// 4. 좌표 변환 함수
// ============================================================================

int xyToIndex(int x, int y) {
  if (y % 2 == 0) {
    return y * W + x;
  } else {
    return y * W + (W - 1 - x);
  }
}

// ============================================================================
// 5. 유틸리티 함수
// ============================================================================

void printHex(uint8_t value) {
  if (value < 16) Serial.print("0");
  Serial.print(value, HEX);
}

// ============================================================================
// 6. 시리얼 출력 함수
// ============================================================================

void serialPrintHeader() {
  #if USE_SERIAL
    Serial.println(F("=== 8-Queens Backtracking ===" ));
    Serial.print(F("SIZE:"));
    Serial.print(W);
    Serial.print(F("x"));
    Serial.println(H);
    Serial.print(F("BRIGHTNESS:"));
    Serial.println(strip.getBrightness());
    Serial.println(F("============================="));
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

        if (x < W - 1) Serial.print(" ");
      }
      Serial.println();
    }

    Serial.println(F("---"));
    currentFrameNumber++;
  #endif
}

// ============================================================================
// 7. 공개 인터페이스
// ============================================================================

void displayInit() {
  #if USE_SERIAL
    Serial.begin(115200);
    delay(2000);
  #endif

  #if USE_LED
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
  if (x < 0 || x >= W || y < 0 || y >= H) return;
  int index = xyToIndex(x, y);
  strip.setPixelColor(index, strip.Color(r, g, b));
}

void displayShow() {
  serialPrintBrightnessChange();

  #if USE_LED
    strip.show();
  #endif
    serialPrintFrame();
}

void displayClear() {
  strip.clear();
}

void setBrightness(uint8_t level) {
  strip.setBrightness(level);
}

void displayDelay(unsigned long ms) {
  if (animationSpeed <= 0.0) animationSpeed = 1.0;
  delay((unsigned long)(ms / animationSpeed));
}

// ============================================================================
// 8. 8-Queens 알고리즘
// ============================================================================

#define N 8  // 8x8 체스판

// 퀸의 위치를 저장 (각 행에 하나씩)
int queens[N];  // queens[row] = col

// 통계
int solutionCount = 0;
int backtrackCount = 0;

// ============================================================================
// 9. 체스판 그리기 함수
// ============================================================================

// 2x2 픽셀 블록으로 체스판 칸 그리기
void drawSquare(int col, int row, uint8_t r, uint8_t g, uint8_t b) {
  int baseX = col * 2;
  int baseY = row * 2;

  for (int dy = 0; dy < 2; dy++) {
    for (int dx = 0; dx < 2; dx++) {
      setPixel(baseX + dx, baseY + dy, r, g, b);
    }
  }
}

// 퀸이 공격할 수 있는지 확인
bool isUnderAttack(int testRow, int testCol) {
  for (int r = 0; r < testRow; r++) {
    int c = queens[r];

    // 같은 열
    if (c == testCol) return true;

    // 대각선
    if (abs(r - testRow) == abs(c - testCol)) return true;
  }
  return false;
}

// 전체 체스판 그리기
void drawBoard(int currentRow, int tryCol, bool showAttack) {
  displayClear();

  for (int row = 0; row < N; row++) {
    for (int col = 0; col < N; col++) {
      uint8_t r = 0, g = 0, b = 0;

      // 체스판 패턴 배경색
      bool isWhiteSquare = (row + col) % 2 == 0;

      // 배치된 퀸인가?
      bool hasQueen = false;
      if (row < currentRow && queens[row] == col) {
        hasQueen = true;
      }

      if (hasQueen) {
        // 배치된 퀸 - 파란색
        r = 0; g = 0; b = 255;
      }
      else if (row == currentRow && col == tryCol) {
        // 현재 시도 중인 위치
        if (showAttack && isUnderAttack(currentRow, tryCol)) {
          // 공격당하는 위치 - 빨간색
          r = 255; g = 0; b = 0;
        } else {
          // 시도 중 - 노란색
          r = 255; g = 255; b = 0;
        }
      }
      else if (row == currentRow && showAttack) {
        // 현재 행에서 공격 범위 표시
        if (isUnderAttack(currentRow, col)) {
          r = 100; g = 0; b = 0;  // 어두운 빨강
        } else {
          r = 0; g = 80; b = 0;   // 어두운 초록 (안전)
        }
      }
      else {
        // 빈 칸 - 체스판 패턴
        if (isWhiteSquare) {
          r = 80; g = 80; b = 80;   // 밝은 회색 (흰 칸)
        } else {
          r = 20; g = 20; b = 20;   // 어두운 회색 (검은 칸)
        }
      }

      drawSquare(col, row, r, g, b);
    }
  }

  displayShow();
}

// 해를 찾았을 때 강조 표시
void showSolution() {
  displayClear();

  for (int row = 0; row < N; row++) {
    for (int col = 0; col < N; col++) {
      uint8_t r, g, b;

      // 체스판 패턴 배경색
      bool isWhiteSquare = (row + col) % 2 == 0;

      if (queens[row] == col) {
        // 퀸 - 초록색
        r = 0; g = 255; b = 0;
      } else {
        // 빈 칸 - 체스판 패턴
        if (isWhiteSquare) {
          r = 80; g = 80; b = 80;   // 밝은 회색 (흰 칸)
        } else {
          r = 20; g = 20; b = 20;   // 어두운 회색 (검은 칸)
        }
      }

      drawSquare(col, row, r, g, b);
    }
  }

  displayShow();
}

// 백트래킹 표시
void showBacktrack(int row) {
  Serial.print(F("Backtracking from row "));
  Serial.println(row);
  backtrackCount++;

  // 주황색으로 깜빡임
  for (int i = 0; i < 2; i++) {
    drawSquare(queens[row], row, 255, 120, 0);
    displayShow();
    displayDelay(200);

    drawSquare(queens[row], row, 0, 0, 0);
    displayShow();
    displayDelay(200);
  }
}

// ============================================================================
// 10. 백트래킹 알고리즘
// ============================================================================

bool solveNQueens(int row) {
  // 모든 퀸을 배치했으면 성공
  if (row == N) {
    solutionCount++;
    Serial.print(F("Solution #"));
    Serial.print(solutionCount);
    Serial.print(F(" found! (Backtracks: "));
    Serial.print(backtrackCount);
    Serial.println(F(")"));

    showSolution();
    displayDelay(3000);

    // 첫 번째 해만 찾고 종료
    return true;
  }

  // 현재 행의 각 열을 시도
  for (int col = 0; col < N; col++) {
    Serial.print(F("Trying row "));
    Serial.print(row);
    Serial.print(F(", col "));
    Serial.println(col);

    // 시도 중인 위치 표시
    drawBoard(row, col, false);
    displayDelay(300);

    // 공격 범위 확인 표시
    drawBoard(row, col, true);
    displayDelay(500);

    if (!isUnderAttack(row, col)) {
      // 안전한 위치 - 퀸 배치
      queens[row] = col;

      Serial.print(F("Placing queen at row "));
      Serial.print(row);
      Serial.print(F(", col "));
      Serial.println(col);

      drawBoard(row + 1, -1, false);
      displayDelay(800);

      // 다음 행으로 재귀
      if (solveNQueens(row + 1)) {
        return true;
      }

      // 백트래킹
      showBacktrack(row);
    } else {
      Serial.println(F("Position under attack, trying next..."));
      displayDelay(300);
    }
  }

  // 이 행에서 해를 못 찾음
  return false;
}

// ============================================================================
// 11. Setup & Loop
// ============================================================================

void setup() {
  displayInit();
  setBrightness(15);

  Serial.println(F(""));
  Serial.println(F("========================================"));
  Serial.println(F("   8-Queens Backtracking Visualization"));
  Serial.println(F("========================================"));
  Serial.println(F(""));

  // 초기화
  for (int i = 0; i < N; i++) {
    queens[i] = -1;
  }
  solutionCount = 0;
  backtrackCount = 0;
}

void loop() {
  while (Serial.available() > 0) {
    Serial.read();
  }

  Serial.println(F("Starting 8-Queens solver..."));

  // 빈 보드 표시
  drawBoard(0, -1, false);
  displayDelay(1500);

  // 알고리즘 실행
  if (!solveNQueens(0)) {
    Serial.println(F("No solution found!"));
  }

  Serial.print(F("Total solutions: "));
  Serial.println(solutionCount);
  Serial.print(F("Total backtracks: "));
  Serial.println(backtrackCount);

  displayDelay(5000);

  // 리셋
  for (int i = 0; i < N; i++) {
    queens[i] = -1;
  }
  solutionCount = 0;
  backtrackCount = 0;
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
