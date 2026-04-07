import serial
import time
import os
import serial

# Initiate serial connection
try:
    # Change COM port by trial and error
    ser = serial.Serial('COM10', 115200, timeout=0.01)
    print("Successfully connected to serial.")
except Exception as e:
    print(f"Could not connect: {e}")
    exit()

# Path to the file written by the AC app
AC_STATE_FILE = r"C:/Program Files (x86)/Steam/steamapps/common/assettocorsa/apps/python/RacingHapticGhost/ac_state.txt"

# Get live status and telemetry values from state file
def ac_state():
    try:
        if not os.path.exists(AC_STATE_FILE):
            return 1, 0.0, 0.0, 0.0

        f = open(AC_STATE_FILE, "r")
        line = f.readline().strip()
        f.close()

        parts = line.split(",")
        if len(parts) != 4:
            return 1, 0.0, 0.0, 0.0

        is_live = int(parts[0])
        steering = float(parts[1])
        throttle = float(parts[2])
        brake = float(parts[3])

        return is_live, steering, throttle, brake

    except:
        return 1, 0.0, 0.0, 0.0
    
# Initialize loop delay time
freq = 5 #Hz
delay = 1/freq

while True:
    is_live, ac_steering, ac_throttle, ac_brake = ac_state()

    steering_out = ac_steering
    throttle_out = ac_throttle
    brake_out = ac_brake
    source = "REPLAY"

    # Send message over serial with 3 decimals of precision
    precision = 3
    serial_message = f"{steering_out:.{precision}f},{throttle_out:.{precision}f},{brake_out:.{precision}f}\n"
    message_binary = serial_message.encode('utf-8')
    ser.write(message_binary)

    # Confirm message that has been sent
    print(serial_message + ", Source: " + source)
    
    # Delay
    time.sleep(delay)
