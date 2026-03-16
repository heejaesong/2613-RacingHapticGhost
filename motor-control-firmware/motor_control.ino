/*
Student MCU Arduino code.
*/

#include <Arduino.h>
#include <SPI.h>

#include <ACAN2517FD.h>
#include <Moteus.h>

// If true: use incoming encoder positions as position commands
// If false: stop motors (but still read encoders)
static constexpr bool kEnableFollow = true;

// Note that the CAN adapter oscillator must match the MCP2517FD board or whatever is selected (currently set to 20MHz placeholder)
static constexpr ACAN2517FDSettings::Oscillator kCanOsc =
  ACAN2517FDSettings::OSC_40MHz;

// SPI pins:
static constexpr int PIN_SPI_SCK = 12;
static constexpr int PIN_SPI_MOSI = 11;
static constexpr int PIN_SPI_MISO = 13;

// MCP2517FD pins:
static constexpr int PIN_CAN_CS = 10;  // Chip select
static constexpr int PIN_CAN_INT = 9;  // Interrupt from MCP2517FD

// UART link FROM the first ESP32-S3
static constexpr int PIN_UART_RX = 17;  // Student RX <- Teacher TX
static constexpr int PIN_UART_TX = 18;  // Student TX -> Teacher RX (not needed)
static constexpr uint32_t UART_BAUD = 921600;

static bool forward = true;
static uint32_t lastToggle = 0;

// CAN and Moteus objects
ACAN2517FD can(PIN_CAN_CS, SPI, PIN_CAN_INT);

Moteus steering_wheel(can, []() {
  Moteus::Options o;
  o.id = 6;
  return o;
}());
Moteus accel_pedal(can, []() {
  Moteus::Options o;
  o.id = 5;
  return o;
}());
Moteus brake_pedal(can, []() {
  Moteus::Options o;
  o.id = 4;
  return o;
}());

Moteus::PositionMode::Command cmd;
Moteus::PositionMode::Format fmt;

HardwareSerial UartLink(1);

// UART line parser
static char lineBuf[256];
static size_t lineLen = 0;


// TODO: Modify this to work with Heejae's sending format
static bool parseIncoming(const char *s, float &p1, float &p2, float &p3) {
  int field = 0;
  const char *start = s;

  float vals[4] = { 0 };  // t, p1, p2, p3

  while (*s) {
    if (*s == ',' || *s == '\n' || *s == '\r') {
      if (field < 4) {
        char tmp[32];
        size_t n = (size_t)(s - start);
        if (n >= sizeof(tmp)) n = sizeof(tmp) - 1;
        memcpy(tmp, start, n);
        tmp[n] = '\0';
        vals[field] = (float)atof(tmp);
      }
      field++;
      if (*s == ',') start = s + 1;
    }
    s++;
  }

  // Parse the final token if needed.
  if (field < 4 && *start) {
    char tmp[32];
    size_t n = strlen(start);
    if (n >= sizeof(tmp)) n = sizeof(tmp) - 1;
    memcpy(tmp, start, n);
    tmp[n] = '\0';
    vals[field] = (float)atof(tmp);
    field++;
  }

  if (field < 4) return false;

  p1 = vals[1];
  p2 = vals[2];
  p3 = vals[3];
  return true;
}

static void SetupCan() {
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
  if (err != 0) {
    Serial.print("CAN begin failed, err=0x");
    Serial.println(err, HEX);
    while (1) delay(1000);
  }
}

static void setupMoteusFormat() {
  fmt.velocity_limit = Moteus::kFloat;
  fmt.accel_limit = Moteus::kFloat;

  cmd.velocity_limit = 5.0f;
  cmd.accel_limit = 20.0f;
}

static void commandPosition(float p1) {
  if (!kEnableFollow) {
    steering_wheel.SetStop();
    // accel_pedal.SetStop();
    // brake_pedal.SetStop();
    return;
  }

  cmd.position = p1;
  steering_wheel.SetPosition(cmd, &fmt);
  // cmd.position = p2;
  // accel_pedal.SetPosition(cmd, &fmt);
  // cmd.position = p3;
  // brake_pedal.SetPosition(cmd, &fmt);
}

static void printLocalEncoders() {
  const auto &a = steering_wheel.last_result().values;
  const auto &b = accel_pedal.last_result().values;
  const auto &c = brake_pedal.last_result().values;

  // Serial.print("local_pos: ");
  // Serial.print(a.position, 6);
  // Serial.print(", ");
  // Serial.print(b.position, 6);
  // Serial.print(", ");
  // Serial.print(c.position, 6);
  // Serial.println();

  Serial.print("m4 mode="); Serial.print((int)a.mode);
  Serial.print(" pos="); Serial.println(a.position);

  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(200);

  // UART from leader ESP32
  // UartLink.begin(UART_BAUD, SERIAL_8N1, PIN_UART_RX, PIN_UART_TX);

  SetupCan();
  setupMoteusFormat();

  steering_wheel.SetStop();
  delay(20);
  accel_pedal.SetStop();
  delay(20);
  brake_pedal.SetStop();
  delay(20);

  Serial.println("Follower started: UART in -> CAN-FD position commands.");
}

void loop() {
  // Read UART bytes and build lines
  // while (UartLink.available()) {
  //   char c = (char)UartLink.read();

  //   if (c == '\n') {
  //     lineBuf[lineLen] = '\0';

  //     float p1 = 0, p2 = 0, p3 = 0;
  //     if (parseIncoming(lineBuf, p1, p2, p3)) {
  //       commandPosition(p1, p2, p3);
  //     }

  //     lineLen = 0;  // reset for next line
  //   } else {
  //     if (lineLen < sizeof(lineBuf) - 1) {
  //       lineBuf[lineLen++] = c;
  //     } else {
  //       // overflow; reset
  //       lineLen = 0;
  //     }
  //   }
  // }


  // Testing
  // commandPosition(1);
  // delay(400);
  // printLocalEncoders();

  // commandPosition(0);
  // delay(400);
  // printLocalEncoders();


  // More testing
  if (millis() - lastToggle > 750) {
    forward = !forward;
    lastToggle = millis();
    printLocalEncoders();
  }

  cmd.position = forward ? 1.0f : 0.0f;
  steering_wheel.SetPosition(cmd, &fmt);  // keep sending
  delay(5);  // 200 Hz
}
