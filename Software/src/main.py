 # run "pip install pysdl2 pysdl2-dll" in terminal to download
import sdl2
import serial
import time
import os

print("imports ok")

# Initiate serial connection
try:
    # Change COM port by trial and error
    ser = serial.Serial('COM3', 115200, timeout=0.01)
    print("Successfully connected to serial.")
except Exception as e:
    print(f"Could not connect: {e}")
    exit()

# Initiate SDL2 API to interface with HID devices
sdl2.SDL_Init(sdl2.SDL_INIT_JOYSTICK)

# Get number of joysticks visible to computer
# Only one joystick (-x: left, +x: right, -y: throttle, +y: brake)
num_joysticks = sdl2.SDL_NumJoysticks()

print("Joysticks: " ,num_joysticks)

# Open joystick for use
joystick = sdl2.SDL_JoystickOpen(0)

# Number of axes on joystick
num_axes = sdl2.SDL_JoystickNumAxes(joystick)

# Initialize data values
steering_wheel = 0
throttle = 0
brake = 0

# Initialize loop delay time
freq = 50 #Hz
delay = 1/freq

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

while True:
    # Update position data
    sdl2.SDL_PumpEvents()

    # Initialize position array
    angles = []

    # Iterate through both axes and add to array
    for axis in range(num_axes):
        axis_angle = sdl2.SDL_JoystickGetAxis(joystick, axis)
        angles.append(axis_angle)
    
    # Normalize steering angle
    # -1.0 -> approximately 110 degrees counterclockwise
    # 1.0 -> approximately 110 degrees clockwise
    steering_wheel = angles[0] / 32768

    # Normalize throttle and brake angles
    # Centre of axis found to be ~-216 via testing
    # Create deadzone of -500 to 0
    # 1.0 -> full throttle and brake
    if(angles[1] < -500) :
        throttle = -(angles[1] + 216.0) / 32552
        brake = 0
    elif (angles[1] > 0) :
        brake = (angles[1] + 216.0) / 32983
        throttle = 0
    else :
        brake = 0
        throttle = 0

    # Call state function
    is_live, ac_steering, ac_throttle, ac_brake = ac_state()


    if is_live:
        steering_out = steering_wheel
        throttle_out = throttle
        brake_out = brake
        source = "WHEEL"
    else:
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
