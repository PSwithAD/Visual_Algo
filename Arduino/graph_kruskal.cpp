// Kruskal MST 알고리즘 시각화
// 통합 하드웨어 템플릿 기반

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
#define INPUT_PULLUP 0
#define OUTPUT 1
#define HIGH 1
#define LOW 0

inline void delay(unsigned long ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void delayMicroseconds(int us) {
  std::this_thread::sleep_for(std::chrono::microseconds(us));
}

void pinMode(int pin, int mode) {}
void digitalWrite(int pin, int value) {}
int digitalRead(int pin) { return HIGH; }

unsigned long millis() {
  static auto start = std::chrono::steady_clock::now();
  auto now = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::milliseconds>(now - start)
      .count();
}

struct SerialType {
  void begin(unsigned long) {}
  void print(const char *s) { std::printf("%s", s); }
  void print(int v) { std::printf("%d", v); }
  void print(uint8_t v, int base) {
    if (base == HEX)
      std::printf("%x", v); // Arduino prints without leading zeros
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
  void println(int v) {
    std::printf("%d\n", v);
    std::fflush(stdout);
  }
  int available() { return 0; }
  int read() { return -1; }
  int parseInt() { return -1; }
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
#include <Arduino.h>
#ifdef __AVR__
#include <avr/power.h>
#endif
#endif

// ============================================================================
// 1. 하드웨어 설정
// ============================================================================

#define LED_PIN A2
#define LED_COUNT 256
#define LED_WIDTH 16
#define LED_HEIGHT 16

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// ============================================================================
// 2. 좌표 변환 함수
// ============================================================================

int xyToIndex(int x, int y) {
  if (x < 0 || x >= LED_WIDTH || y < 0 || y >= LED_HEIGHT)
    return -1;
  if (y % 2 == 0) {
    return y * LED_WIDTH + x;
  } else {
    return y * LED_WIDTH + (LED_WIDTH - 1 - x);
  }
}

// ============================================================================
// 3. 출력 함수 (LED)
// ============================================================================

static int currentFrameNumber = 0;
static uint8_t lastBrightness = 255;
static float animationSpeed = 1.0;

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

void setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
  int index = xyToIndex(x, y);
  if (index >= 0) {
    strip.setPixelColor(index, strip.Color(r, g, b));
  }
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

void setAnimationSpeed(float speed) { animationSpeed = speed; }

// ============================================================================
// 4. 그래프 알고리즘 - 데이터 구조
// ============================================================================

#define MAX_NODES 10
#define MAX_EDGES 25

struct NodePos {
  int x, y;
};

struct Edge {
  int u, v;
  int weight;
};

// 10개 노드를 비대칭적으로 배치 (무작위 느낌)
NodePos nodes[MAX_NODES] = {
    {3, 2},   // 0
    {11, 3},  // 1
    {14, 7},  // 2
    {13, 13}, // 3
    {7, 14},  // 4
    {2, 11},  // 5
    {5, 7},   // 6
    {9, 9},   // 7
    {12, 11}, // 8
    {7, 4}    // 9
};

// 20개 간선 (실제 유클리드 거리 기반 가중치)
// 가중치 = sqrt((x2-x1)^2 + (y2-y1)^2) 반올림
Edge edges[MAX_EDGES] = {
    {0, 1, 8}, // (3,2)-(11,3): sqrt(64+1)=8.06
    {0, 5, 9}, // (3,2)-(2,11): sqrt(1+81)=9.06
    {0, 9, 5}, // (3,2)-(7,4): sqrt(16+4)=4.47
    {1, 2, 5}, // (11,3)-(14,7): sqrt(9+16)=5
    {1, 9, 4}, // (11,3)-(7,4): sqrt(16+1)=4.12
    {1, 7, 6}, // (11,3)-(9,9): sqrt(4+36)=6.32
    {2, 3, 6}, // (14,7)-(13,13): sqrt(1+36)=6.08
    {2, 8, 5}, // (14,7)-(12,11): sqrt(4+16)=4.47
    {3, 4, 6}, // (13,13)-(7,14): sqrt(36+1)=6.08
    {3, 8, 2}, // (13,13)-(12,11): sqrt(1+4)=2.24
    {4, 5, 6}, // (7,14)-(2,11): sqrt(25+9)=5.83
    {4, 6, 7}, // (7,14)-(5,7): sqrt(4+49)=7.28
    {4, 7, 6}, // (7,14)-(9,9): sqrt(4+25)=5.39
    {5, 6, 5}, // (2,11)-(5,7): sqrt(9+16)=5
    {6, 7, 5}, // (5,7)-(9,9): sqrt(16+4)=4.47
    {6, 9, 4}, // (5,7)-(7,4): sqrt(4+9)=3.61
    {7, 8, 4}, // (9,9)-(12,11): sqrt(9+4)=3.61
    {7, 9, 6}, // (9,9)-(7,4): sqrt(4+25)=5.39
    {8, 3, 2}, // 중복 제거 (이미 3-8로 표현)
    {9, 1, 4}  // 중복 제거 (이미 1-9로 표현)
};

int nodeCount = 10;
int edgeCount = 20;

// ============================================================================
// 5. 그래프 시각화 함수
// ============================================================================

// Bresenham 선 그리기
void drawLine(int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b) {
  int dx = abs(x1 - x0);
  int dy = abs(y1 - y0);
  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;
  int err = dx - dy;

  while (true) {
    setPixel(x0, y0, r, g, b);

    if (x0 == x1 && y0 == y1)
      break;

    int e2 = 2 * err;
    if (e2 > -dy) {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dx) {
      err += dx;
      y0 += sy;
    }
  }
}

// 노드 그리기 (1x1 픽셀)
// color: 0=회색(기본), 1=흰색(밝게), 2=빨강(현재 선택)
void drawNode(int nodeId, int color) {
  if (nodeId < 0 || nodeId >= nodeCount)
    return;

  int x = nodes[nodeId].x;
  int y = nodes[nodeId].y;

  uint8_t r, g, b;
  switch (color) {
  case 0:
    r = 0;
    g = 0;
    b = 255;
    break; // 파랑 (기본)
  case 1:
    r = 255;
    g = 255;
    b = 255;
    break; // 흰색 (밝게)
  case 2:
    r = 255;
    g = 0;
    b = 0;
    break; // 빨강 (현재 선택)
  default:
    r = 100;
    g = 100;
    b = 100;
    break;
  }

  setPixel(x, y, r, g, b);
}

// 간선 그리기
// color: 0=회색(기본), 1=노란색(고려중), 2=초록(MST 선택), 3=빨강(거부-사이클)
void drawEdge(int u, int v, int color) {
  if (u < 0 || u >= nodeCount || v < 0 || v >= nodeCount)
    return;

  uint8_t r, g, b;
  switch (color) {
  case 0:
    r = 200;
    g = 200;
    b = 200;
    break; // 흰색 (기본)
  case 1:
    r = 255;
    g = 255;
    b = 0;
    break; // 노란색 (고려중)
  case 2:
    r = 0;
    g = 255;
    b = 0;
    break; // 초록 (MST 선택)
  case 3:
    r = 255;
    g = 0;
    b = 0;
    break; // 빨강 (거부-사이클)
  default:
    r = 40;
    g = 40;
    b = 40;
    break;
  }

  int x0 = nodes[u].x;
  int y0 = nodes[u].y;
  int x1 = nodes[v].x;
  int y1 = nodes[v].y;

  drawLine(x0, y0, x1, y1, r, g, b);
}

// 그래프 전체 그리기 (모든 간선 + 모든 노드)
void drawGraph() {
  // 간선 먼저
  for (int i = 0; i < edgeCount; i++) {
    drawEdge(edges[i].u, edges[i].v, 0);
  }
  // 노드 나중에 (위에 표시)
  for (int i = 0; i < nodeCount; i++) {
    drawNode(i, 0);
  }
}

void clearAndDrawGraph() {
  clearDisplay();
  drawGraph();
}

// ============================================================================
// 6. Kruskal MST 알고리즘 (Union-Find)
// ============================================================================

int parent[MAX_NODES];
int rank_[MAX_NODES];

void makeSet(int v) {
  parent[v] = v;
  rank_[v] = 0;
}

int findSet(int v) {
  if (v == parent[v])
    return v;
  return parent[v] = findSet(parent[v]); // path compression
}

bool unionSets(int u, int v) {
  int rootU = findSet(u);
  int rootV = findSet(v);

  if (rootU == rootV)
    return false; // 사이클 발생

  // union by rank
  if (rank_[rootU] < rank_[rootV]) {
    parent[rootU] = rootV;
  } else if (rank_[rootU] > rank_[rootV]) {
    parent[rootV] = rootU;
  } else {
    parent[rootV] = rootU;
    rank_[rootU]++;
  }
  return true;
}

// 간선 정렬 (버블 소트)
void sortEdges() {
  for (int i = 0; i < edgeCount - 1; i++) {
    for (int j = 0; j < edgeCount - i - 1; j++) {
      if (edges[j].weight > edges[j + 1].weight) {
        Edge temp = edges[j];
        edges[j] = edges[j + 1];
        edges[j + 1] = temp;
      }
    }
  }
}

// MST에 포함된 간선 추적
bool inMST[MAX_EDGES];

void kruskalMST() {
  Serial.println(F("\n=== Kruskal MST Algorithm ==="));

  // 1. 초기화
  for (int i = 0; i < nodeCount; i++) {
    makeSet(i);
  }
  for (int i = 0; i < MAX_EDGES; i++) {
    inMST[i] = false;
  }

  // 2. 간선 정렬
  sortEdges();

  Serial.println(F("Edges sorted by weight:"));
  for (int i = 0; i < edgeCount; i++) {
    Serial.print(F("  Edge "));
    Serial.print(edges[i].u);
    Serial.print(F("-"));
    Serial.print(edges[i].v);
    Serial.print(F(" weight: "));
    Serial.println(edges[i].weight);
  }

  // 초기 그래프 표시
  clearAndDrawGraph();
  showDisplay();
  hardwareDelay(2000);

  // 3. MST 구성
  int mstEdgeCount = 0;
  int mstWeight = 0;

  for (int i = 0; i < edgeCount && mstEdgeCount < nodeCount - 1; i++) {
    int u = edges[i].u;
    int v = edges[i].v;
    int w = edges[i].weight;

    Serial.print(F("\nConsidering edge "));
    Serial.print(u);
    Serial.print(F("-"));
    Serial.print(v);
    Serial.print(F(" (weight "));
    Serial.print(w);
    Serial.println(F(")"));

    // 기존 MST를 유지하면서 현재 간선만 노란색으로 강조
    clearDisplay();

    // 1. 모든 간선을 어두운 회색으로
    for (int j = 0; j < edgeCount; j++) {
      drawEdge(edges[j].u, edges[j].v, 0);
    }

    // 2. MST 간선들을 초록색으로 (누적)
    for (int j = 0; j < i; j++) {
      if (inMST[j]) {
        drawEdge(edges[j].u, edges[j].v, 2);
      }
    }

    // 3. 현재 고려 중인 간선을 노란색으로
    drawEdge(u, v, 1);

    // 4. 모든 노드를 회색으로, 현재 선택된 두 노드만 빨간색으로
    for (int k = 0; k < nodeCount; k++) {
      drawNode(k, 0);
    }
    drawNode(u, 2);
    drawNode(v, 2);

    showDisplay();
    hardwareDelay(800);

    // Union-Find로 사이클 검사
    if (unionSets(u, v)) {
      Serial.println(F("  -> Added to MST"));
      inMST[i] = true;
      mstEdgeCount++;
      mstWeight += w;

      // MST 간선으로 확정 - 초록색으로 변경
      clearDisplay();

      // 모든 간선을 회색으로
      for (int j = 0; j < edgeCount; j++) {
        drawEdge(edges[j].u, edges[j].v, 0);
      }

      // MST 간선들을 초록색으로
      for (int j = 0; j <= i; j++) {
        if (inMST[j]) {
          drawEdge(edges[j].u, edges[j].v, 2);
        }
      }

      // 모든 노드를 회색으로
      for (int k = 0; k < nodeCount; k++) {
        drawNode(k, 0);
      }

      showDisplay();
      hardwareDelay(500);

    } else {
      Serial.println(F("  -> Rejected (forms cycle)"));

      // 거부된 간선 잠깐 빨간색으로 표시
      clearDisplay();

      // 모든 간선을 회색으로
      for (int j = 0; j < edgeCount; j++) {
        drawEdge(edges[j].u, edges[j].v, 0);
      }

      // MST 간선들을 초록색으로
      for (int j = 0; j < i; j++) {
        if (inMST[j]) {
          drawEdge(edges[j].u, edges[j].v, 2);
        }
      }

      // 거부된 간선을 빨간색으로
      drawEdge(u, v, 3);

      // 모든 노드를 회색으로
      for (int k = 0; k < nodeCount; k++) {
        drawNode(k, 0);
      }

      showDisplay();
      hardwareDelay(500);
    }
  }

  // 최종 결과
  Serial.println(F("\n=== MST Complete ==="));
  Serial.print(F("Total edges in MST: "));
  Serial.println(mstEdgeCount);
  Serial.print(F("Total weight: "));
  Serial.println(mstWeight);

  // 최종 MST 표시 (MST 간선만 밝게)
  clearDisplay();

  // MST 간선들만 밝은 초록색으로
  for (int i = 0; i < edgeCount; i++) {
    if (inMST[i]) {
      drawEdge(edges[i].u, edges[i].v, 2);
    }
  }

  // 노드들을 밝은 흰색으로 (간선과 구분)
  for (int i = 0; i < nodeCount; i++) {
    drawNode(i, 0);
  }

  showDisplay();
  hardwareDelay(3000);
}

// ============================================================================
// 7. Setup & Loop
// ============================================================================

void setup() {
  Serial.begin(115200);

#ifndef TARGET_PC
#ifdef __AVR__
  if (F_CPU == 16000000)
    clock_prescale_set(clock_div_1);
#endif
#endif

  strip.begin();
  strip.setBrightness(20);
  strip.show();

  Serial.println(F("========================================"));
  Serial.println(F("   Kruskal MST Visualization"));
  Serial.println(F("========================================"));

  hardwareDelay(1000);
  kruskalMST();
}

void loop() {
  // 알고리즘 완료 후 대기
  hardwareDelay(1000);
}

#ifdef TARGET_PC
int main() {
  setup();
  return 0;
}
#endif
