// Prim MST 알고리즘 시각화
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
#define INF 9999

struct NodePos {
  int x, y;
};

struct Edge {
  int u, v;
  int weight;
};

// 10개 노드를 비대칭적으로 배치 (크루스칼과 동일)
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

// 인접 행렬로 그래프 표현
int adjMatrix[MAX_NODES][MAX_NODES];

int nodeCount = 10;

void initializeGraph() {
  // 인접 행렬 초기화 (INF로)
  for (int i = 0; i < nodeCount; i++) {
    for (int j = 0; j < nodeCount; j++) {
      adjMatrix[i][j] = (i == j) ? 0 : INF;
    }
  }

  // 간선 추가 (크루스칼과 동일한 20개 간선)
  // 가중치 = sqrt((x2-x1)^2 + (y2-y1)^2) 반올림
  int edges[][3] = {
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
      {8, 3, 2}, // 중복 (이미 3-8로 표현)
      {9, 1, 4}  // 중복 (이미 1-9로 표현)
  };

  for (int i = 0; i < 20; i++) {
    int u = edges[i][0];
    int v = edges[i][1];
    int w = edges[i][2];
    adjMatrix[u][v] = w;
    adjMatrix[v][u] = w; // 무방향 그래프
  }
}

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
// color: 0=회색(기본), 1=노란색(고려중), 2=초록(MST 선택)
void drawEdge(int u, int v, int color) {
  if (u < 0 || u >= nodeCount || v < 0 || v >= nodeCount)
    return;
  if (adjMatrix[u][v] == INF)
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
  default:
    r = 40;
    g = 40;
    b = 40;
    break;
  }

  drawLine(nodes[u].x, nodes[u].y, nodes[v].x, nodes[v].y, r, g, b);
}

// 전체 그래프 그리기
void drawGraph() {
  // 모든 간선 그리기
  for (int i = 0; i < nodeCount; i++) {
    for (int j = i + 1; j < nodeCount; j++) {
      if (adjMatrix[i][j] != INF) {
        drawEdge(i, j, 0);
      }
    }
  }

  // 모든 노드 그리기
  for (int i = 0; i < nodeCount; i++) {
    drawNode(i, 0);
  }
}

void clearAndDrawGraph() {
  clearDisplay();
  drawGraph();
  showDisplay();
}

// ============================================================================
// 6. Prim MST 알고리즘
// ============================================================================

bool inMST[MAX_NODES];
int parent[MAX_NODES];
int key[MAX_NODES];

struct MSTEdge {
  int u, v, weight;
};

MSTEdge mstEdges[MAX_NODES];
int mstCount = 0;

void primMST() {
  Serial.println(F("\n=== Prim MST Algorithm ==="));

  // 초기화
  for (int i = 0; i < nodeCount; i++) {
    inMST[i] = false;
    key[i] = INF;
    parent[i] = -1;
  }

  // 시작 노드 (0번)
  key[0] = 0;
  int totalWeight = 0;

  Serial.println(F("Starting from node 0"));

  // 초기 그래프 표시
  clearAndDrawGraph();
  drawNode(0, 2); // 시작 노드 빨강
  showDisplay();
  hardwareDelay(1500);

  // n-1개의 간선을 선택
  for (int count = 0; count < nodeCount; count++) {
    // 1. MST에 포함되지 않은 노드 중 key 값이 최소인 노드 찾기
    int minKey = INF;
    int u = -1;

    for (int v = 0; v < nodeCount; v++) {
      if (!inMST[v] && key[v] < minKey) {
        minKey = key[v];
        u = v;
      }
    }

    if (u == -1)
      break;

    // 2. 선택된 노드를 MST에 추가
    inMST[u] = true;

    Serial.print(F("\nAdding node "));
    Serial.print(u);
    Serial.print(F(" to MST"));

    if (parent[u] != -1) {
      Serial.print(F(" (via edge "));
      Serial.print(parent[u]);
      Serial.print(F("-"));
      Serial.print(u);
      Serial.print(F(" weight: "));
      Serial.print(adjMatrix[parent[u]][u]);
      Serial.println(F(")"));

      mstEdges[mstCount].u = parent[u];
      mstEdges[mstCount].v = u;
      mstEdges[mstCount].weight = adjMatrix[parent[u]][u];
      mstCount++;
      totalWeight += adjMatrix[parent[u]][u];
    } else {
      Serial.println(F(" (starting node)"));
    }

    // 시각화: MST에 추가된 모습
    clearDisplay();
    drawGraph();

    // MST 간선 표시 (초록색)
    for (int i = 0; i < mstCount; i++) {
      drawEdge(mstEdges[i].u, mstEdges[i].v, 2);
    }

    // MST 노드 표시 (초록색)
    for (int i = 0; i < nodeCount; i++) {
      if (inMST[i]) {
        drawNode(i, 0);
      }
    }

    // 현재 추가된 노드 (빨강)
    drawNode(u, 2);

    showDisplay();
    hardwareDelay(1000);

    // 3. 인접 노드의 key 값 업데이트
    for (int v = 0; v < nodeCount; v++) {
      if (adjMatrix[u][v] != INF && !inMST[v] && adjMatrix[u][v] < key[v]) {
        parent[v] = u;
        key[v] = adjMatrix[u][v];

        Serial.print(F("  Update key["));
        Serial.print(v);
        Serial.print(F("] = "));
        Serial.println(key[v]);
      }
    }

    // 업데이트된 경계 노드 표시
    clearDisplay();
    drawGraph();

    // MST 간선
    for (int i = 0; i < mstCount; i++) {
      drawEdge(mstEdges[i].u, mstEdges[i].v, 2);
    }

    // MST 노드
    for (int i = 0; i < nodeCount; i++) {
      if (inMST[i]) {
        drawNode(i, 0);
      }
    }

    // 경계 노드 (key < INF인 미방문 노드) - 노란색
    for (int v = 0; v < nodeCount; v++) {
      if (!inMST[v] && key[v] < INF) {
        drawNode(v, 3);
        // 경계 간선도 노란색으로
        if (parent[v] != -1) {
          drawEdge(parent[v], v, 1);
        }
      }
    }

    showDisplay();
    hardwareDelay(800);
  }

  // 최종 MST 표시
  Serial.println(F("\n=== MST Complete ==="));
  Serial.print(F("Total MST weight: "));
  Serial.println(totalWeight);
  Serial.print(F("Edges in MST: "));
  Serial.println(mstCount);

  clearDisplay();
  drawGraph();
  for (int i = 0; i < mstCount; i++) {
    drawEdge(mstEdges[i].u, mstEdges[i].v, 2);
    drawNode(mstEdges[i].u, 0);
    drawNode(mstEdges[i].v, 0);
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
  Serial.println(F("   Prim MST Visualization"));
  Serial.println(F("========================================"));

  initializeGraph();
  hardwareDelay(1000);
  primMST();
}

void loop() {
  // 알고리즘 완료 후 대기
  hardwareDelay(1000);
}

#ifdef TARGET_PC
int main() {
  setup();
  for (int i = 0; i < 3; i++) {
    loop();
  }
  return 0;
}
#endif
