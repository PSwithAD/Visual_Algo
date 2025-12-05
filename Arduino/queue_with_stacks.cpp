// 실행 명령어 형식: g++ -std=c++17 -DTARGET_PC "경로/파일명.cpp" -o "
// 경로/파일명(.cpp 없음)"

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

// Queue Implementation Using Two Stacks - LED Visualization

// ============================================================================
// 1. 하드웨어 설정
// ============================================================================

#define LED_PIN A2
#define LED_COUNT 256
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
float animationSpeed = 1.0;

// ============================================================================
// 4. 큐 구현 (Two Stacks) - 전역 변수 직후로 이동
// ============================================================================

#define MAX_STACK_SIZE 12

class TwoStackQueue {
private:
  int stack1[MAX_STACK_SIZE]; // enqueue용 스택
  int top1;

  int stack2[MAX_STACK_SIZE]; // dequeue용 스택
  int top2;

public:
  TwoStackQueue() : top1(-1), top2(-1) {}

  bool isEmpty() { return (top1 == -1 && top2 == -1); }

  bool isFull() { return (top1 == MAX_STACK_SIZE - 1); }

  void enqueue(int value) {
    if (isFull())
      return;
    stack1[++top1] = value;
  }

  int dequeue() {
    if (isEmpty())
      return -1;

    // stack2가 비어있으면 stack1의 모든 원소를 stack2로 옮김
    if (top2 == -1) {
      while (top1 != -1) {
        stack2[++top2] = stack1[top1--];
      }
    }

    // stack2에서 pop
    return stack2[top2--];
  }

  int getStack1Size() { return top1 + 1; }
  int getStack2Size() { return top2 + 1; }
  int getStack1(int idx) {
    return (idx >= 0 && idx <= top1) ? stack1[idx] : -1;
  }
  int getStack2(int idx) {
    return (idx >= 0 && idx <= top2) ? stack2[idx] : -1;
  }

  // 시각화를 위한 헬퍼 함수들
  int popStack1() {
    if (top1 == -1)
      return -1;
    return stack1[top1--];
  }

  void pushStack2(int value) {
    if (top2 < MAX_STACK_SIZE - 1) {
      stack2[++top2] = value;
    }
  }

  int popStack2() {
    if (top2 == -1)
      return -1;
    return stack2[top2--];
  }
};

// ============================================================================
// 5. 좌표 변환 함수
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
  if (value < 16)
    Serial.print("0");
  Serial.print(value, HEX);
}

// ============================================================================
// 6. 시리얼 출력 함수
// ============================================================================

void serialPrintHeader() {
#if USE_SERIAL
  Serial.println(F("=== Queue with Two Stacks ==="));
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

      if (x < W - 1)
        Serial.print(" ");
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
  if (x < 0 || x >= W || y < 0 || y >= H)
    return;
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

void displayClear() { strip.clear(); }

void setBrightness(uint8_t level) { strip.setBrightness(level); }

void displayDelay(unsigned long ms) {
  if (animationSpeed <= 0.0)
    animationSpeed = 1.0;
  delay((unsigned long)(ms / animationSpeed));
}

// ============================================================================
// 8. 큐 구현 (Two Stacks) - 위로 이동됨
// ============================================================================

// ============================================================================
// 9. 시각화 함수들
// ============================================================================

// 스택 물통 테두리 그리기 (흰색)
// 스택 물통 테두리 그리기 (색상 지정 가능)
void drawStackContainer(int startX, int startY, int width, int height,
                        uint8_t r, uint8_t g, uint8_t b) {
  // 좌우 테두리
  for (int y = startY; y < startY + height; y++) {
    setPixel(startX, y, r, g, b);
    setPixel(startX + width - 1, y, r, g, b);
  }
  // 바닥 테두리
  for (int x = startX; x < startX + width; x++) {
    setPixel(x, startY + height - 1, r, g, b);
  }
}

// 물건 블록 그리기 (값에 따라 다른 색상)
void drawItem(int x, int y, int value) {
  // 값에 따라 색상을 다르게 표현 (Palette)
  uint8_t r, g, b;
  switch (value % 7) {
  case 1:
    r = 255;
    g = 0;
    b = 0;
    break; // 빨강
  case 2:
    r = 0;
    g = 255;
    b = 0;
    break; // 초록
  case 3:
    r = 0;
    g = 0;
    b = 255;
    break; // 파랑
  case 4:
    r = 255;
    g = 255;
    b = 0;
    break; // 노랑
  case 5:
    r = 255;
    g = 0;
    b = 255;
    break; // 자홍
  case 6:
    r = 0;
    g = 255;
    b = 255;
    break; // 청록
  default:
    r = 255;
    g = 128;
    b = 0;
    break; // 주황 (0 or 7 etc)
  }

  // 아이템을 2x2 블록으로 그리기
  setPixel(x, y, r, g, b);
  setPixel(x + 1, y, r, g, b);
  setPixel(x, y + 1, r, g, b);
  setPixel(x + 1, y + 1, r, g, b);
}

// 전체 스택 상태 그리기
void drawQueueState(TwoStackQueue &queue) {
  displayClear();

  // Stack1 (왼쪽 물통) - 2열~5열 (4칸 너비)
  int stack1X = 2;
  int stack1Y = 2;
  int stackWidth = 4;
  int stackHeight = 13;

  // Stack1 (왼쪽): 흰색 테두리
  drawStackContainer(stack1X, stack1Y, stackWidth, stackHeight, 255, 255, 255);

  // Stack1의 아이템 그리기 (아래에서 위로)
  int stack1Size = queue.getStack1Size();
  for (int i = 0; i < stack1Size; i++) {
    int value = queue.getStack1(i);
    int itemY = stack1Y + stackHeight - 3 - (i * 2);
    int itemX = stack1X + 1;
    if (itemY >= stack1Y) {
      drawItem(itemX, itemY, value);
    }
  }

  // Stack2 (오른쪽 물통) - 10열~13열 (4칸 너비)
  int stack2X = 10;
  int stack2Y = 2;

  // Stack2 (오른쪽): 흰색 테두리
  drawStackContainer(stack2X, stack2Y, stackWidth, stackHeight, 255, 255, 255);

  // Stack2의 아이템 그리기 (아래에서 위로)
  int stack2Size = queue.getStack2Size();
  for (int i = 0; i < stack2Size; i++) {
    int value = queue.getStack2(i);
    int itemY = stack2Y + stackHeight - 3 - (i * 2);
    int itemX = stack2X + 1;
    if (itemY >= stack2Y) {
      drawItem(itemX, itemY, value);
    }
  }

  displayShow();
}

// 아이템이 이동하는 애니메이션
void animateEnqueue(TwoStackQueue &queue, int value) {
  int stack1X = 2;
  int stack1Y = 2;
  int stackHeight = 13;
  int itemX = 3;

  // 현재 스택1의 크기에 따라 목표 위치 계산
  int stack1Size = queue.getStack1Size();
  int targetY = stack1Y + stackHeight - 3 - (stack1Size * 2);

  // 위쪽에서 목표 위치까지 떨어지는 애니메이션
  for (int y = 0; y <= targetY; y++) {
    drawQueueState(queue);

    // 떨어지는 아이템 그리기
    drawItem(itemX, y, value);

    displayShow();
    displayDelay(80);
  }

  // 실제 enqueue 수행
  queue.enqueue(value);
  drawQueueState(queue);
  displayDelay(300);
}

// Stack1에서 Stack2로 아이템 하나를 이동하는 애니메이션
void animateTransfer(TwoStackQueue &queue) {
  if (queue.getStack1Size() == 0)
    return;

  int stack1X = 2;
  int stack1Y = 2;
  int stack2X = 10;
  int stack2Y = 2;
  int stackHeight = 13;

  // Stack1의 top 원소 값 가져오기
  int value = queue.getStack1(queue.getStack1Size() - 1);

  // Stack1에서의 현재 위치
  int stack1Size = queue.getStack1Size();
  int startY = stack1Y + stackHeight - 3 - ((stack1Size - 1) * 2);

  // Stack2에서의 목표 위치
  int stack2Size = queue.getStack2Size();
  int targetY = stack2Y + stackHeight - 3 - (stack2Size * 2);

  // 1단계: Stack1에서 위로 올라가기
  for (int y = startY; y >= 0; y--) {
    drawQueueState(queue);
    drawItem(3, y, value);
    displayShow();
    displayDelay(50);
  }

  // 2단계: 왼쪽에서 오른쪽으로 이동
  for (int x = 3; x <= 11; x++) {
    drawQueueState(queue);
    drawItem(x, 0, value);
    displayShow();
    displayDelay(50);
  }

  // 3단계: Stack2로 내려가기
  for (int y = 0; y <= targetY; y++) {
    drawQueueState(queue);
    drawItem(11, y, value);
    displayShow();
    displayDelay(50);
  }

  // 실제로 스택 간 이동 (큐의 내부 구조 직접 조작)
  // Stack1에서 pop
  int transferValue = queue.getStack1(queue.getStack1Size() - 1);
  // 임시로 내부 변수 조작 (정상적인 방법은 아니지만 시각화를 위해)
  // 대신 새로운 헬퍼 함수를 추가해야 함
}

// Dequeue 애니메이션 (stack1 -> stack2 이동 포함)
void animateDequeue(TwoStackQueue &queue) {
  int stack1X = 2;
  int stack1Y = 2;
  int stack2X = 10;
  int stack2Y = 2;
  int stackHeight = 13;

  // Stack2가 비어있고 Stack1에 원소가 있으면 모든 원소를 옮겨야 함
  if (queue.getStack2Size() == 0 && queue.getStack1Size() > 0) {
    Serial.println(F("Transferring from Stack1 to Stack2..."));

    // Stack1의 모든 원소를 하나씩 옮기기
    while (queue.getStack1Size() > 0) {
      // Stack1에서의 현재 위치 (top)
      int currentStack1Size = queue.getStack1Size();
      int startY = stack1Y + stackHeight - 3 - ((currentStack1Size - 1) * 2);

      // Stack2에서의 목표 위치
      int currentStack2Size = queue.getStack2Size();
      int targetY = stack2Y + stackHeight - 3 - (currentStack2Size * 2);

      // Stack1에서 pop
      int value = queue.popStack1();

      // 1단계: Stack1에서 위로 올라가기
      for (int y = startY; y >= 0; y--) {
        drawQueueState(queue);
        drawItem(3, y, value);
        displayShow();
        displayDelay(40);
      }

      // 2단계: 왼쪽에서 오른쪽으로 이동
      for (int x = 3; x <= 11; x++) {
        drawQueueState(queue);
        drawItem(x, 0, value);
        displayShow();
        displayDelay(40);
      }

      // 3단계: Stack2로 내려가기
      for (int y = 0; y <= targetY; y++) {
        drawQueueState(queue);
        drawItem(11, y, value);
        displayShow();
        displayDelay(40);
      }

      // Stack2에 push
      queue.pushStack2(value);
      drawQueueState(queue);
      displayDelay(200);
    }
  }

  // Dequeue 수행 (Stack2에서 pop)
  if (queue.getStack2Size() == 0) {
    Serial.println(F("Queue is empty!"));
    return;
  }

  // Stack2에서의 시작 위치 (top)
  int stack2Size = queue.getStack2Size();
  int startY = stack2Y + stackHeight - 3 - ((stack2Size - 1) * 2);

  // Stack2에서 pop
  int value = queue.popStack2();

  if (value != -1) {
    // 위로 사라지는 애니메이션
    for (int y = startY; y >= 0; y--) {
      drawQueueState(queue);
      drawItem(11, y, value);
      displayShow();
      displayDelay(60);
    }
  }

  drawQueueState(queue);
  displayDelay(300);
}

// ============================================================================
// 10. 데모 시나리오
// ============================================================================

void queueDemo() {
  TwoStackQueue queue;

  Serial.println(F("\n=== Queue Demo Start ==="));

  // 초기 상태
  drawQueueState(queue);
  displayDelay(1000);

  // Enqueue 연속 5개
  Serial.println(F("Enqueue: 1, 2, 3, 4, 5"));
  animateEnqueue(queue, 1);
  animateEnqueue(queue, 2);
  animateEnqueue(queue, 3);
  animateEnqueue(queue, 4);
  animateEnqueue(queue, 5);

  displayDelay(1000);

  // Dequeue 2개
  Serial.println(F("Dequeue 2 times"));
  animateDequeue(queue);
  animateDequeue(queue);

  displayDelay(1000);

  // Enqueue 2개 더
  Serial.println(F("Enqueue: 6, 7"));
  animateEnqueue(queue, 6);
  animateEnqueue(queue, 7);

  displayDelay(1000);

  // 나머지 모두 Dequeue
  Serial.println(F("Dequeue all remaining"));
  animateDequeue(queue);
  animateDequeue(queue);
  animateDequeue(queue);
  animateDequeue(queue);
  animateDequeue(queue);

  displayDelay(2000);

  Serial.println(F("=== Queue Demo End ===\n"));
}

// ============================================================================
// 11. Setup & Loop
// ============================================================================

void setup() {
  displayInit();
  setBrightness(15);

  Serial.println(F(""));
  Serial.println(F("========================================"));
  Serial.println(F("   Queue with Two Stacks"));
  Serial.println(F("========================================"));
  Serial.println(F(""));
}

void loop() {
  while (Serial.available() > 0) {
    Serial.read();
  }

  queueDemo();

  displayDelay(3000);
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
