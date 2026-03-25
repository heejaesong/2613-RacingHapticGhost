/*
Student MCU firmware.
*/

#include <Arduino.h>
#include <SPI.h>

#include <ACAN2517FD.h>
#include <Moteus.h>

#define STEERING_WHEEL_ID 1
#define ACCEL_PEDAL_ID    6
#define BRAKE_PEDAL_ID    99 // TODO: Set to ID of the other Moteus controller

#define STEER_CENTRE      -0.3f
#define TEACH_VAL_TO_REVS 110.0f / 360.0f

// Increasing limits further leads to possibility of overshoot
#define VELOCITY_LIM     3.0f
#define ACCELERATION_LIM 6.0f

#define CONTROL_LOOP_MS 10

#define RX_TIMEOUT_MS 200

// Will not work if not set to 40MHz (characteristic of the SPI to CAN-FD adapter)
static constexpr ACAN2517FDSettings::Oscillator kCanOsc =
  ACAN2517FDSettings::OSC_40MHz;

// SPI pins:
static constexpr int PIN_SPI_SCK = 12;   // Clock
static constexpr int PIN_SPI_MOSI = 11;  // TX/SDI
static constexpr int PIN_SPI_MISO = 13;  // RX/SDO

// MCP2517FD pins:
static constexpr int PIN_CAN_CS = 10;   // Chip select
static constexpr int PIN_CAN_INT = 9;   // Interrupt from MCP2517FD

// Serial comms with laptop
static char lineBuf[96];
static size_t lineLen = 0;

// Holds the most recently received teacher inputs
static float latest_steer = 0.0f;
static float latest_accel   = 0.0f;
static float latest_brake   = 0.0f;
static bool  have_input   = false;

// TODO: Remove these testing variables once ready
static bool forward = true;
static uint32_t lastToggle = 0;

// CAN and Moteus objects
ACAN2517FD can(PIN_CAN_CS, SPI, PIN_CAN_INT);

Moteus steering_wheel(can, []() {
  Moteus::Options o;
  o.id = STEERING_WHEEL_ID;
  return o;
}());
Moteus accel_pedal(can, []() {
  Moteus::Options o;
  o.id = ACCEL_PEDAL_ID;
  return o;
}());
Moteus brake_pedal(can, []() {
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

static void setupCan()
{
  SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, PIN_CAN_CS);

  ACAN2517FDSettings settings(
    kCanOsc,
    1000ll * 1000ll,       // 1 Mbit arbitration
    DataBitRateFactor::x1  // BRS disabled
  );

  settings.mArbitrationSJW = 2;
  settings.mDriverTransmitFIFOSize = 8;
  settings.mDriverReceiveFIFOSize = 32;

  const uint32_t err = can.begin(settings, [] {
    can.isr();
  });
  if (err != 0)
  {
    Serial.print("CAN begin failed, err=0x");
    Serial.println(err, HEX);
    while (1)
      delay(1000);
  }
}

static void setupMoteusFormat()
{
  sw_fmt.velocity_limit = Moteus::kFloat;
  sw_fmt.accel_limit = Moteus::kFloat;

  sw_cmd.velocity_limit = VELOCITY_LIM;
  sw_cmd.accel_limit = ACCELERATION_LIM;

  acl_fmt.velocity_limit = Moteus::kFloat;
  acl_fmt.accel_limit = Moteus::kFloat;

  acl_cmd.velocity_limit = VELOCITY_LIM;
  acl_cmd.accel_limit = ACCELERATION_LIM;

  brk_fmt.velocity_limit = Moteus::kFloat;
  brk_fmt.accel_limit = Moteus::kFloat;

  brk_cmd.velocity_limit = VELOCITY_LIM;
  brk_cmd.accel_limit = ACCELERATION_LIM;
}

static void commandPositions(float sw_pos)
{
  sw_cmd.position = STEER_CENTRE + sw_pos;
  steering_wheel.SetPosition(sw_cmd, &sw_fmt);
  // cmd.position = p2;
  // accel_pedal.SetPosition(acl_cmd, &acl_fmt);
  // cmd.position = p3;
  // brake_pedal.SetPosition(brk_cmd, &brk_fmt);
}

static void printLocalEncoders()
{
  const auto &a = steering_wheel.last_result().values;
  const auto &b = accel_pedal.last_result().values;
  const auto &c = brake_pedal.last_result().values;

  Serial.print("STEERING WHEEL: mode="); Serial.print((int)a.mode);
  Serial.print(" pos="); Serial.print(a.position);
  Serial.print(" fault="); Serial.println((int)a.fault);

  // TODO: Uncomment based on which motors are being controlled
  // Serial.print("ACCEL PEDAL: mode="); Serial.print((int)b.mode);
  // Serial.print(" pos="); Serial.print(b.position);
  // Serial.print(" fault="); Serial.println((int)b.fault);

  // Serial.print("BRAKE PEDAL: mode="); Serial.print((int)c.mode);
  // Serial.print(" pos="); Serial.print(c.position);
  // Serial.print(" fault="); Serial.println((int)c.fault);

  Serial.println();
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

  Serial.println("Setup complete");
}

void loop()
{
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

    // If no new data has been received, stop the motor
    if (now - last_rx_ms > RX_TIMEOUT_MS)
    {
      steering_wheel.SetStop();
      return;
    }

    const float sw_pos = latest_steer * TEACH_VAL_TO_REVS;
    commandPositions(sw_pos);
  }
}


/*
// TEST CODE - commands motor to go forward and back every 2 seconds
    if (millis() - lastToggle > 2000)
    {
      forward = !forward;
      lastToggle = millis();
      printLocalEncoders();
    }

    float sw_pos = forward ? (STEER_CENTRE + 0.5f) : (STEER_CENTRE - 0.5f);
    commandPositions(sw_pos);
*/
