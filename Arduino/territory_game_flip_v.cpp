// 실행 명령어 형식: g++ -std=c++17 -DTARGET_PC territory_game_flip_v.cpp -o
// territory_game_flip_v 아두이노 IDE 업로드 시: 이 파일 전체를 복사해서 .ino
// 파일에 붙여넣으세요. [상하 반전 버전]

#ifdef TARGET_PC
// ============================================================================
// [PC] 빌드 환경 설정 (스텁 및 모의 객체)
// ============================================================================
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
// ============================================================================
// [Arduino] 실제 하드웨어 라이브러리
// ============================================================================
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif
#endif

// ============================================================================
// 1. 상수 및 설정 (Configuration)
// ============================================================================

// 하드웨어 핀 설정
#define LED_PIN A2
#define LED_COUNT 256
#define W 16
#define H 16

// 자석 모듈 핀 설정
// Row output pins (6개): 13, 12, 11, 10, 9, 8
#define ROW_PIN_START 13
#define ROW_PIN_END 8
#define ROW_COUNT 6

// Column input pins (6개): 7, 6, 5, 4, 3, 2
#define COL_PIN_START 7
#define COL_PIN_END 2
#define COL_COUNT 6

// 게임 설정
#define GRID_SIZE 6
#define TOTAL_NODES 36
#define MAX_TURNS 100
#define WIN_NODES 19

#define USE_LED 1
#define USE_SERIAL 1

// ============================================================================
// 2. 자료형 정의 (Enums & Structs)
// ============================================================================

enum InputMode {
  INPUT_MAGNETIC, // 자석 모듈 입력
  INPUT_SERIAL    // 시리얼 입력
};

enum NodeState {
  NEUTRAL = 0,      // 중립
  P1_TEMP = 1,      // P1 임시
  P1_CONFIRMED = 2, // P1 확정
  P2_TEMP = 3,      // P2 임시
  P2_CONFIRMED = 4  // P2 확정
};

struct Node {
  int x, y; // LED 좌표
};

// ============================================================================
// 3. 전역 변수 (Global Variables)
// ============================================================================

// LED 객체
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// 디스플레이 상태
int currentFrameNumber = 0;
uint8_t lastBrightness = 255;
float animationSpeed = 1.0;

// 입력 인터페이스 상태
static unsigned long lastInputTime = 0;
static const unsigned long DEBOUNCE_DELAY = 200;
static int lastNodeInput = -1;
InputMode currentInputMode = INPUT_SERIAL;

// 게임 상태
Node gridNodes[TOTAL_NODES];
NodeState nodeStates[TOTAL_NODES];
int p1Position = 0;
int p2Position = 35;
int p1LastMove = -1;
int p2LastMove = -1;
int currentPlayer = 1;
int currentTurn = 0;
int winner = 0;

// ============================================================================
// 4. 함수 원형 선언 (Forward Declarations)
// ============================================================================

// 입력 관련
void inputInit();
int readMagneticInput();
int readSerialInput();
int readInput(InputMode mode);
int coordToNode(int row, int col);
void nodeToCoord(int node, int *row, int *col);
void setInputMode(InputMode mode);

// 디스플레이 관련
int xyToIndex(int x, int y);
void printHex(uint8_t value);
void serialPrintHeader();
void serialPrintBrightnessChange();
void serialPrintFrame();
void displayInit();
void setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);
void displayShow();
void displayClear();
void setBrightness(uint8_t level);
void displayDelay(unsigned long ms);

// 게임 로직 관련
void initGrid();
uint32_t getNodeColor(int nodeIdx, bool highlight);
void drawNodeBorder(int nodeIdx, uint8_t r, uint8_t g, uint8_t b);
void drawCurrentPlayerBox(int nodeIdx);
void drawBoard(bool highlightCurrent);
void getGridCoord(int nodeIdx, int &row, int &col);
int getNodeIndex(int row, int col);
bool isAdjacent(int from, int to);
bool isValidMove(int fromNode, int toNode, int player);
void claimNode(int nodeIdx, int player);
bool checkIsolation(int player);
int checkWinCondition();
void showVictoryAnimation(int winnerPlayer);
void showError(int nodeIdx);
void processTurn(int selectedNode);

// ============================================================================
// 5. 입력 인터페이스 구현 (Input Implementation)
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

int coordToNode(int row, int col) {
  if (row < 0 || row >= ROW_COUNT || col < 0 || col >= COL_COUNT) {
    return -1;
  }
  return row * COL_COUNT + col;
}

void nodeToCoord(int node, int *row, int *col) {
  if (node < 0 || node >= ROW_COUNT * COL_COUNT) {
    *row = -1;
    *col = -1;
    return;
  }
  *row = node / COL_COUNT;
  *col = node % COL_COUNT;
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
    if (node >= 0 && node < TOTAL_NODES) {
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
    if (node >= 0 && node < TOTAL_NODES) {
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
  else if (mode == INPUT_SERIAL)
    return readSerialInput();
  return -1;
}

void setInputMode(InputMode mode) {
  currentInputMode = mode;
  if (mode == INPUT_MAGNETIC)
    Serial.println(F("Input mode: Magnetic sensor"));
  else
    Serial.println(F("Input mode: Serial"));
}

// ============================================================================
// 6. 디스플레이 구현 (Display Implementation)
// ============================================================================

// [상하 반전]
int xyToIndex(int x, int y) {
  // y 좌표를 반전 (0 -> 15, 15 -> 0)
  int mirroredY = (H - 1) - y;

  if (mirroredY % 2 == 0)
    return mirroredY * W + x;
  else
    return mirroredY * W + (W - 1 - x);
}

void printHex(uint8_t value) {
  if (value < 16)
    Serial.print("0");
  Serial.print(value, HEX);
}

void serialPrintHeader() {
#if USE_SERIAL
  Serial.println(F("=== Territory Conquest Game ==="));
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
// 7. 게임 로직 구현 (Game Logic Implementation)
// ============================================================================

void initGrid() {
  int idx = 0;
  for (int row = 0; row < GRID_SIZE; row++) {
    for (int col = 0; col < GRID_SIZE; col++) {
      gridNodes[idx].x = col * 3;
      gridNodes[idx].y = row * 3;
      nodeStates[idx] = NEUTRAL;
      idx++;
    }
  }
  nodeStates[0] = P1_TEMP;
  nodeStates[35] = P2_TEMP;
  p1Position = 0;
  p2Position = 35;
  p1LastMove = -1;
  p2LastMove = -1;
  currentPlayer = 1;
  currentTurn = 0;
  winner = 0;
}

uint32_t getNodeColor(int nodeIdx, bool highlight) {
  NodeState state = nodeStates[nodeIdx];
  if (highlight)
    return strip.Color(255, 255, 255);

  switch (state) {
  case NEUTRAL:
    return strip.Color(20, 20, 20);
  case P1_TEMP:
    return strip.Color(50, 150, 50);
  case P1_CONFIRMED:
    return strip.Color(0, 255, 0);
  case P2_TEMP:
    return strip.Color(0, 100, 255);
  case P2_CONFIRMED:
    return strip.Color(0, 0, 255);
  default:
    return strip.Color(0, 0, 0);
  }
}

void drawNodeBorder(int nodeIdx, uint8_t r, uint8_t g, uint8_t b) {
  int cx = gridNodes[nodeIdx].x;
  int cy = gridNodes[nodeIdx].y;
  int directions[8][2] = {{-1, -1}, {0, -1}, {1, -1}, {-1, 0},
                          {1, 0},   {-1, 1}, {0, 1},  {1, 1}};
  for (int i = 0; i < 8; i++) {
    int nx = cx + directions[i][0];
    int ny = cy + directions[i][1];
    if (nx >= 0 && nx < W && ny >= 0 && ny < H) {
      setPixel(nx, ny, r, g, b);
    }
  }
}

void drawCurrentPlayerBox(int nodeIdx) {
  int cx = gridNodes[nodeIdx].x;
  int cy = gridNodes[nodeIdx].y;

  // 중심점
  if (cx >= 0 && cx < W && cy >= 0 && cy < H) {
    setPixel(cx, cy, 255, 255, 255);
  }

  // 상하좌우 1칸 (플러스 모양)
  int directions[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
  for (int i = 0; i < 4; i++) {
    int nx = cx + directions[i][0];
    int ny = cy + directions[i][1];

    if (nx >= 0 && nx < W && ny >= 0 && ny < H) {
      setPixel(nx, ny, 255, 255, 255);
    }
  }
}

void drawBoard(bool highlightCurrent) {
  displayClear();
  for (int i = 0; i < TOTAL_NODES; i++) {
    NodeState state = nodeStates[i];
    uint32_t color = getNodeColor(i, false);
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    setPixel(gridNodes[i].x, gridNodes[i].y, r, g, b);
    if (state == P1_CONFIRMED || state == P2_CONFIRMED) {
      drawNodeBorder(i, r, g, b);
    }
  }
  if (highlightCurrent) {
    int currentPos = (currentPlayer == 1) ? p1Position : p2Position;
    drawCurrentPlayerBox(currentPos);
  }
  displayShow();
}

void getGridCoord(int nodeIdx, int &row, int &col) {
  row = nodeIdx / GRID_SIZE;
  col = nodeIdx % GRID_SIZE;
}

int getNodeIndex(int row, int col) {
  if (row < 0 || row >= GRID_SIZE || col < 0 || col >= GRID_SIZE)
    return -1;
  return row * GRID_SIZE + col;
}

bool isAdjacent(int from, int to) {
  int fromRow, fromCol, toRow, toCol;
  getGridCoord(from, fromRow, fromCol);
  getGridCoord(to, toRow, toCol);
  int diffRow = abs(toRow - fromRow);
  int diffCol = abs(toCol - fromCol);
  return (diffRow == 1 && diffCol == 0) || (diffRow == 0 && diffCol == 1);
}

bool isValidMove(int fromNode, int toNode, int player) {
  if (toNode < 0 || toNode >= TOTAL_NODES) {
    Serial.println(F("Error: Node out of range"));
    return false;
  }
  if (!isAdjacent(fromNode, toNode)) {
    Serial.println(F("Error: Not adjacent"));
    return false;
  }
  int lastMove = (player == 1) ? p1LastMove : p2LastMove;
  if (toNode == fromNode && lastMove == fromNode) {
    Serial.println(F("Error: Cannot stay in same node consecutively"));
    return false;
  }
  NodeState targetState = nodeStates[toNode];
  if (player == 1 && targetState == P2_CONFIRMED) {
    Serial.println(F("Error: Cannot move to opponent's confirmed node"));
    return false;
  }
  if (player == 2 && targetState == P1_CONFIRMED) {
    Serial.println(F("Error: Cannot move to opponent's confirmed node"));
    return false;
  }
  int confirmedCount = 0;
  for (int i = 0; i < TOTAL_NODES; i++) {
    if ((player == 1 && nodeStates[i] == P1_CONFIRMED) ||
        (player == 2 && nodeStates[i] == P2_CONFIRMED)) {
      confirmedCount++;
    }
  }
  if (confirmedCount == 0 && toNode == fromNode) {
    Serial.println(F("Error: Must move to new node when no confirmed nodes"));
    return false;
  }
  return true;
}

void claimNode(int nodeIdx, int player) {
  NodeState currentState = nodeStates[nodeIdx];
  if (player == 1) {
    if (currentState == NEUTRAL) {
      nodeStates[nodeIdx] = P1_TEMP;
      Serial.println(F("P1: Claimed neutral node (temp)"));
    } else if (currentState == P1_TEMP) {
      nodeStates[nodeIdx] = P1_CONFIRMED;
      Serial.println(F("P1: Confirmed own node"));
    } else if (currentState == P2_TEMP) {
      nodeStates[nodeIdx] = P1_CONFIRMED;
      Serial.println(F("P1: Stole opponent's temp node!"));
    }
  } else {
    if (currentState == NEUTRAL) {
      nodeStates[nodeIdx] = P2_TEMP;
      Serial.println(F("P2: Claimed neutral node (temp)"));
    } else if (currentState == P2_TEMP) {
      nodeStates[nodeIdx] = P2_CONFIRMED;
      Serial.println(F("P2: Confirmed own node"));
    } else if (currentState == P1_TEMP) {
      nodeStates[nodeIdx] = P2_CONFIRMED;
      Serial.println(F("P2: Stole opponent's temp node!"));
    }
  }
}

bool checkIsolation(int player) {
  int opponentPos = (player == 1) ? p2Position : p1Position;
  int opponentPlayer = (player == 1) ? 2 : 1;
  int opRow, opCol;
  getGridCoord(opponentPos, opRow, opCol);
  int directions[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
  for (int i = 0; i < 4; i++) {
    int newRow = opRow + directions[i][0];
    int newCol = opCol + directions[i][1];
    int newNode = getNodeIndex(newRow, newCol);
    if (newNode == -1)
      continue;
    NodeState state = nodeStates[newNode];
    if (state == NEUTRAL)
      return false;
    if (opponentPlayer == 1 && state == P2_TEMP)
      return false;
    if (opponentPlayer == 2 && state == P1_TEMP)
      return false;
  }
  return true;
}

int checkWinCondition() {
  if (checkIsolation(currentPlayer)) {
    Serial.print(F("Player "));
    Serial.print(currentPlayer);
    Serial.println(F(" wins by isolation!"));
    return currentPlayer;
  }
  int p1Total = 0, p2Total = 0;
  int p1Confirmed = 0, p2Confirmed = 0;
  for (int i = 0; i < TOTAL_NODES; i++) {
    if (nodeStates[i] == P1_TEMP || nodeStates[i] == P1_CONFIRMED) {
      p1Total++;
      if (nodeStates[i] == P1_CONFIRMED)
        p1Confirmed++;
    }
    if (nodeStates[i] == P2_TEMP || nodeStates[i] == P2_CONFIRMED) {
      p2Total++;
      if (nodeStates[i] == P2_CONFIRMED)
        p2Confirmed++;
    }
  }
  if (p1Total >= WIN_NODES) {
    Serial.println(F("Player 1 wins by majority!"));
    return 1;
  }
  if (p2Total >= WIN_NODES) {
    Serial.println(F("Player 2 wins by majority!"));
    return 2;
  }
  if (currentTurn >= MAX_TURNS) {
    Serial.println(F("Max turns reached!"));
    if (p1Total > p2Total)
      return 1;
    else if (p2Total > p1Total)
      return 2;
    else {
      if (p1Confirmed > p2Confirmed)
        return 1;
      else if (p2Confirmed > p1Confirmed)
        return 2;
      else
        return 3;
    }
  }
  return 0;
}

void showVictoryAnimation(int winnerPlayer) {
  displayClear();
  for (int i = 0; i < TOTAL_NODES; i++) {
    NodeState state = nodeStates[i];
    uint32_t color = getNodeColor(i, false);
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    setPixel(gridNodes[i].x, gridNodes[i].y, r, g, b);
    bool isWinnerNode = false;
    if (winnerPlayer == 1 && (state == P1_TEMP || state == P1_CONFIRMED))
      isWinnerNode = true;
    if (winnerPlayer == 2 && (state == P2_TEMP || state == P2_CONFIRMED))
      isWinnerNode = true;
    if (winnerPlayer == 3)
      isWinnerNode = true;
    if (isWinnerNode)
      drawNodeBorder(i, 255, 255, 0);
  }
  displayShow();
  displayDelay(2000);
  displayClear();
  uint8_t finalR, finalG, finalB;
  if (winnerPlayer == 1) {
    finalR = 0;
    finalG = 255;
    finalB = 0;
  } else if (winnerPlayer == 2) {
    finalR = 0;
    finalG = 0;
    finalB = 255;
  } else {
    finalR = 255;
    finalG = 255;
    finalB = 255;
  }
  for (int i = 0; i < TOTAL_NODES; i++) {
    setPixel(gridNodes[i].x, gridNodes[i].y, finalR, finalG, finalB);
  }
  displayShow();
  displayDelay(2000);
}

void showError(int nodeIdx) {
  setPixel(gridNodes[nodeIdx].x, gridNodes[nodeIdx].y, 255, 0, 0);
  displayShow();
  displayDelay(500);
  drawBoard(false);
}

void processTurn(int selectedNode) {
  int currentPos = (currentPlayer == 1) ? p1Position : p2Position;
  Serial.print(F("Player "));
  Serial.print(currentPlayer);
  Serial.print(F(" selected node "));
  Serial.println(selectedNode);

  if (!isValidMove(currentPos, selectedNode, currentPlayer)) {
    showError(selectedNode);
    return;
  }

  if (currentPlayer == 1) {
    p1LastMove = p1Position;
    p1Position = selectedNode;
  } else {
    p2LastMove = p2Position;
    p2Position = selectedNode;
  }

  claimNode(selectedNode, currentPlayer);
  drawBoard(false);
  displayDelay(300);

  winner = checkWinCondition();
  if (winner != 0) {
    showVictoryAnimation(winner);
    return;
  }

  currentPlayer = (currentPlayer == 1) ? 2 : 1;
  currentTurn++;
  Serial.print(F("Turn "));
  Serial.print(currentTurn);
  Serial.print(F(" - Player "));
  Serial.print(currentPlayer);
  Serial.println(F("'s turn"));

  for (int i = 0; i < 2; i++) {
    drawBoard(true);
    displayDelay(200);
    drawBoard(false);
    displayDelay(200);
  }
}

// ============================================================================
// 8. 메인 Setup & Loop
// ============================================================================

void setup() {
  displayInit();
  setBrightness(15);
  inputInit();

  Serial.println(F(""));
  Serial.println(F("========================================"));
  Serial.println(F("   Territory Conquest Game"));
  Serial.println(F("========================================"));
  Serial.println(F(""));

  initGrid();
  drawBoard(false);

  Serial.println(F("Game started!"));
  Serial.println(F("Enter node number (0-35)"));
  Serial.println(F("P1 (Green) starts at top-left (0)"));
  Serial.println(F("P2 (Blue) starts at bottom-right (35)"));
  Serial.println(F(""));

#ifdef TARGET_PC
  setInputMode(INPUT_SERIAL);
#else
  setInputMode(INPUT_MAGNETIC);
#endif

  for (int i = 0; i < 2; i++) {
    drawBoard(true);
    displayDelay(200);
    drawBoard(false);
    displayDelay(200);
  }
}

void loop() {
  if (winner != 0) {
    Serial.println(F(""));
    Serial.println(F("========================================"));
    if (winner == 1)
      Serial.println(F("   Player 1 (Green) WINS!"));
    else if (winner == 2)
      Serial.println(F("   Player 2 (Blue) WINS!"));
    else
      Serial.println(F("   DRAW!"));
    Serial.println(F("========================================"));
    Serial.println(F(""));
    displayDelay(5000);
    Serial.println(F("Restarting game..."));
    initGrid();
    drawBoard(false);
    for (int i = 0; i < 2; i++) {
      drawBoard(true);
      displayDelay(200);
      drawBoard(false);
      displayDelay(200);
    }
    return;
  }

  // 입력 대기 중 깜빡임 처리 (Non-blocking)
  static unsigned long lastBlinkTime = 0;
  static bool blinkState = false;
  unsigned long currentMillis = millis();

  if (currentMillis - lastBlinkTime >= 200) {
    lastBlinkTime = currentMillis;
    blinkState = !blinkState;
    drawBoard(blinkState);
  }

  int selectedNode = readInput(currentInputMode);
  if (selectedNode >= 0 && selectedNode < TOTAL_NODES) {
    processTurn(selectedNode);
    // 턴 처리 후 즉시 화면 갱신 및 깜빡임 상태 초기화
    blinkState = true;
    drawBoard(blinkState);
    lastBlinkTime = millis();
  }

  // displayDelay(50) 제거 (Non-blocking 루프를 위해)
  // 대신 짧은 지연만 줌
  delay(10);
}

#ifdef TARGET_PC
int main() {
  setup();
  Serial.println("Enter node numbers (0-35) to play:");
  Serial.println("Example moves:");
  Serial.println("  P1 (Green): 0 -> 1 -> 2 -> 7 -> 6 -> ...");
  Serial.println("  P2 (Blue):  35 -> 34 -> 33 -> 28 -> 29 -> ...");
  Serial.println("");
  while (true) {
    loop();
  }
  return 0;
}
#endif
