/*
Student MCU firmware.
*/

#include <Arduino.h>
#include <SPI.h>

#include <ACAN2517FD.h>
#include <Moteus.h>

#define USE_POSITION_COMMANDS 1 // 1 = position commands (Moteus PID control), 0 = torque commands ('manual' ESP P control)

#define STEERING_WHEEL_ID 1
#define ACCEL_PEDAL_ID    4
#define BRAKE_PEDAL_ID    6

#define STEER_CENTRE      -0.3f
#define STEER_VAL_TO_REVS 110.0f / 360.0f
#define STEER_MAX_POS_VAL 0.2f
#define STEER_MIN_POS_VAL -0.8f

#define PEDALS_POS_RANGE 0.29f

// Increasing the limits further might genuinely endanger the student - especially if they are holding the wheel tightly
#define VELOCITY_LIM_STEER     50.0f
#define ACCELERATION_LIM_STEER 25.0f

#define VELOCITY_LIM_PEDALS     50.0f
#define ACCELERATION_LIM_PEDALS 30.0f

#define KP 1

#define CONTROL_LOOP_MS 10

#define RX_TIMEOUT_MS 200

// Oscillator frequency of the CAN Breakout Board is 40MHz
static constexpr ACAN2517FDSettings::Oscillator kCanOsc =
  ACAN2517FDSettings::OSC_40MHz;

// SPI bus pins
static constexpr int PIN_SPI_SCK  = 12;   // Shared clock
static constexpr int PIN_SPI_MOSI = 11;  // Shared TX/SDI
static constexpr int PIN_SPI_MISO = 13;  // Shared RX/SDO

// MCP2517FD pins
static constexpr int PIN_CAN1_CS  = 10;  // Chip select for adapter 1
static constexpr int PIN_CAN1_INT = 9;  // Interrupt from adapter 1

static constexpr int PIN_CAN2_CS  = 5; // Chip select for adapter 2
static constexpr int PIN_CAN2_INT = 6; // Interrupt from adapter 2

static constexpr int PIN_CAN3_CS  = 7; // Chip select for adapter 3
static constexpr int PIN_CAN3_INT = 8; // Interrupt from adapter 3

// Serial comms with laptop
static char lineBuf[96];
static size_t lineLen = 0;

// Positions of pedals (for reset if belt slipping occurs)
float ACCEL_MAX_POS_VAL; // 0.23f
float ACCEL_MIN_POS_VAL; // -0.06f
float ACCEL_RESTING_POS_VAL; // -0.06f

float BRAKE_MAX_POS_VAL; // 0.30f
float BRAKE_MIN_POS_VAL; // 0.01f
float BRAKE_RESTING_POS_VAL; // 0.30f

// Holds the most recently received teacher inputs
static float latest_steer = 0.0f;
static float latest_accel = 0.0f;
static float latest_brake = 0.0f;
static bool have_input = false;

// CAN and Moteus objects
ACAN2517FD can1(PIN_CAN1_CS, SPI, PIN_CAN1_INT);
ACAN2517FD can2(PIN_CAN2_CS, SPI, PIN_CAN2_INT);
ACAN2517FD can3(PIN_CAN3_CS, SPI, PIN_CAN3_INT);

// IRQ flags: set in interrupt, consumed in loop
volatile bool g_can1_irq = false;
volatile bool g_can2_irq = false;
volatile bool g_can3_irq = false;

// ISR stubs
void IRAM_ATTR onCan1Int() { g_can1_irq = true; }
void IRAM_ATTR onCan2Int() { g_can2_irq = true; }
void IRAM_ATTR onCan3Int() { g_can3_irq = true; }

Moteus steering_wheel(can1, []() {
  Moteus::Options o;
  o.id = STEERING_WHEEL_ID;
  return o;
}());
Moteus accel_pedal(can2, []() {
  Moteus::Options o;
  o.id = ACCEL_PEDAL_ID;
  return o;
}());
Moteus brake_pedal(can3, []() {
  Moteus::Options o;
  o.id = BRAKE_PEDAL_ID;
  return o;
}());

Moteus::PositionMode::Command sw_cmd;
Moteus::PositionMode::Format sw_fmt;

Moteus::PositionMode::Command acl_cmd;
Moteus::PositionMode::Format acl_fmt;

Moteus::PositionMode::Command brk_cmd;
Moteus::PositionMode::Format brk_fmt;

static bool parseTeacherInputs(const char* s, float &steer, float &accel, float &brake)
{
  char* end = nullptr;

  steer = strtof(s, &end);
  if (!end || *end != ',')
    return false;

  accel = strtof(end + 1, &end);
  if (!end || *end != ',')
    return false;

  brake = strtof(end + 1, &end);

  return true;
}

static void setPedalPositions()
{
  const auto &acl_vals = accel_pedal.last_result().values;
  const auto &brk_vals = brake_pedal.last_result().values;

  ACCEL_MAX_POS_VAL = acl_vals.position;
  ACCEL_MIN_POS_VAL = ACCEL_MAX_POS_VAL - PEDALS_POS_RANGE;
  ACCEL_RESTING_POS_VAL = ACCEL_MIN_POS_VAL;

  BRAKE_MIN_POS_VAL = brk_vals.position;
  BRAKE_MAX_POS_VAL = BRAKE_MIN_POS_VAL + PEDALS_POS_RANGE;
  BRAKE_RESTING_POS_VAL = BRAKE_MAX_POS_VAL;
}

static void setupCan()
{
  pinMode(PIN_CAN1_CS, OUTPUT); digitalWrite(PIN_CAN1_CS, HIGH);
  pinMode(PIN_CAN2_CS, OUTPUT); digitalWrite(PIN_CAN2_CS, HIGH);
  pinMode(PIN_CAN3_CS, OUTPUT); digitalWrite(PIN_CAN3_CS, HIGH);

  pinMode(PIN_CAN1_INT, INPUT_PULLUP);
  pinMode(PIN_CAN2_INT, INPUT_PULLUP);
  pinMode(PIN_CAN3_INT, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(PIN_CAN1_INT), onCan1Int, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_CAN2_INT), onCan2Int, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_CAN3_INT), onCan3Int, FALLING);

  SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI);
  delay(10);

  ACAN2517FDSettings settings(
    kCanOsc,
    1000ll * 1000ll,       // 1 Mbit arbitration
    DataBitRateFactor::x1  // BRS disabled
  );

  settings.mArbitrationSJW = 2;
  settings.mDriverTransmitFIFOSize = 8;
  settings.mDriverReceiveFIFOSize = 32;

  const uint32_t err1 = can1.begin(settings, [] {});
  if (err1 != 0) {
    Serial.print("CAN1 begin failed, err=0x"); Serial.println(err1, HEX);
    while (1) delay(1000);
  }

  const uint32_t err2 = can2.begin(settings, [] {});
  if (err2 != 0) {
    Serial.print("CAN2 begin failed, err=0x"); Serial.println(err2, HEX);
    while (1) delay(1000);
  }

  const uint32_t err3 = can3.begin(settings, [] {});
  if (err3 != 0) {
    Serial.print("CAN3 begin failed, err=0x"); Serial.println(err3, HEX);
    while (1) delay(1000);
  }

  g_can1_irq = g_can2_irq = g_can3_irq = false;
}

static inline void serviceCan()
{
  bool f1, f2, f3;
  noInterrupts();
  f1 = g_can1_irq; g_can1_irq = false;
  f2 = g_can2_irq; g_can2_irq = false;
  f3 = g_can3_irq; g_can3_irq = false;
  interrupts();

  if (f1) can1.isr();
  if (f2) can2.isr();
  if (f3) can3.isr();

  if (digitalRead(PIN_CAN1_INT) == LOW) can1.isr();
  if (digitalRead(PIN_CAN2_INT) == LOW) can2.isr();
  if (digitalRead(PIN_CAN3_INT) == LOW) can3.isr();
}

static void setupMoteusFormat()
{
  sw_fmt.velocity_limit = Moteus::kFloat;
  sw_fmt.accel_limit = Moteus::kFloat;

  sw_cmd.velocity_limit = VELOCITY_LIM_STEER;
  sw_cmd.accel_limit = ACCELERATION_LIM_STEER;

  acl_fmt.velocity_limit = Moteus::kFloat;
  acl_fmt.accel_limit = Moteus::kFloat;

  acl_cmd.velocity_limit = VELOCITY_LIM_PEDALS;
  acl_cmd.accel_limit = ACCELERATION_LIM_PEDALS;

  brk_fmt.velocity_limit = Moteus::kFloat;
  brk_fmt.accel_limit = Moteus::kFloat;

  brk_cmd.velocity_limit = VELOCITY_LIM_PEDALS;
  brk_cmd.accel_limit = ACCELERATION_LIM_PEDALS;
}

static void commandPositions(float sw_pos, float acl_pos, float brk_pos)
{
  sw_cmd.position = STEER_CENTRE + sw_pos;
  steering_wheel.SetPosition(sw_cmd, &sw_fmt);
  acl_cmd.position = acl_pos;
  accel_pedal.SetPosition(acl_cmd, &acl_fmt);
  brk_cmd.position = brk_pos;
  brake_pedal.SetPosition(brk_cmd, &brk_fmt);
}

static void commandTorques(float sw_tq, float acl_tq, float brk_tq)
{
  // sw_cmd.
}

static void printMotorSatuses()
{
  const auto &a = steering_wheel.last_result().values;
  const auto &b = accel_pedal.last_result().values;
  const auto &c = brake_pedal.last_result().values;

  Serial.print("STEERING WHEEL: mode="); Serial.print((int)a.mode);
  Serial.print(" pos="); Serial.print(a.position);
  Serial.print(" fault="); Serial.println((int)a.fault);

  Serial.print("ACCEL PEDAL: mode="); Serial.print((int)b.mode);
  Serial.print(" pos="); Serial.print(b.position);
  Serial.print(" fault="); Serial.println((int)b.fault);

  Serial.print("BRAKE PEDAL: mode="); Serial.print((int)c.mode);
  Serial.print(" pos="); Serial.print(c.position);
  Serial.print(" fault="); Serial.println((int)c.fault);

  Serial.println();
}

static void getStudentPositions(float &sw_pos, float &acl_pos, float &brk_pos) {
  const auto &sw_vals = steering_wheel.last_result().values;
  const auto &acl_vals = accel_pedal.last_result().values;
  const auto &brk_vals = brake_pedal.last_result().values;

  sw_pos = sw_vals.position;
  acl_pos = acl_vals.position;
  brk_pos = brk_vals.position;
}

void setup()
{
  Serial.begin(115200);
  delay(200);

  setupCan();
  setupMoteusFormat();

  steering_wheel.SetStop();
  delay(20);
  accel_pedal.SetStop();
  delay(20);
  brake_pedal.SetStop();
  delay(20);

  const uint32_t t0 = millis();
  while (millis() - t0 < 200) {
    serviceCan();
    delay(1);
  }

  setPedalPositions();

  Serial.println("Setup complete");
}

void loop()
{
  serviceCan();

  // Read serial data from laptop as quick as possible
  static uint32_t last_rx_ms = 0;
  while (Serial.available())
  {
    char c = (char)Serial.read();

    if (c == '\n') {
      lineBuf[lineLen] = '\0';
      lineLen = 0;

      float steer, accel, brake;
      if (parseTeacherInputs(lineBuf, steer, accel, brake))
      {
        latest_steer = steer;
        latest_accel = accel;
        latest_brake = brake;
        have_input = true;
        last_rx_ms = millis();
      }
    }
    else if (c != '\r')
    {
      if (lineLen < sizeof(lineBuf) - 1)
        lineBuf[lineLen++] = c;
      else
        lineLen = 0;  // overflow reset
    }
  }

  // 10ms control loop using most recent teacher inputs
  static uint32_t last_cmd_ms = 0;
  const uint32_t now = millis();
  if (now - last_cmd_ms >= CONTROL_LOOP_MS) {
    last_cmd_ms = now;

    // If nothing has been received yet, do nothing
    if (!have_input)
      return;

    // If no new data has been received, stop the motors
    if (now - last_rx_ms > RX_TIMEOUT_MS)
    {
      steering_wheel.SetStop();
      accel_pedal.SetStop();
      brake_pedal.SetStop();
      return;
    }

    float des_sw_pos = latest_steer * STEER_VAL_TO_REVS;
    if (des_sw_pos > STEER_MAX_POS_VAL)
      des_sw_pos = STEER_MAX_POS_VAL;
    else if (des_sw_pos < STEER_MIN_POS_VAL)
      des_sw_pos = STEER_MIN_POS_VAL;

    float des_acl_pos = ACCEL_RESTING_POS_VAL + latest_accel * (ACCEL_MAX_POS_VAL - ACCEL_MIN_POS_VAL);
    if (des_acl_pos > ACCEL_MAX_POS_VAL)
      des_acl_pos = ACCEL_MAX_POS_VAL;
    else if (des_acl_pos < ACCEL_MIN_POS_VAL)
      des_acl_pos = ACCEL_MIN_POS_VAL;

    float des_brk_pos = BRAKE_RESTING_POS_VAL - latest_brake * (BRAKE_MAX_POS_VAL - BRAKE_MIN_POS_VAL);
    if (des_brk_pos > BRAKE_MAX_POS_VAL)
      des_brk_pos = BRAKE_MAX_POS_VAL;
    else if (des_brk_pos < BRAKE_MIN_POS_VAL)
      des_brk_pos = BRAKE_MIN_POS_VAL;

    if (USE_POSITION_COMMANDS)
    {
      commandPositions(des_sw_pos, des_acl_pos, des_brk_pos);
    }
    // else
    // {
    //   float sw_pos, acl_pos, brk_pos;
    //   getStudentPositions(sw_pos, acl_pos, brk_pos);

    //   sw_tq = KP * (des_sw_pos - sw_pos);
    //   acl_tq = KP * (des_acl_pos - acl_pos);
    //   brk_tq = KP * (des_brk_pos - brk _pos);
    //   commandTorques(sw_tq, acl_tq, brk_tq);
    // }
    
    serviceCan();
  }
}
