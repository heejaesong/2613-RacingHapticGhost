import sdl2
import serial
import time
import os
from datetime import datetime
from datetime import date
import csv
print("imports ok")

# Initiate serial connection
try:
    ser = serial.Serial('COM3', 115200, timeout=0.01)
    print("Successfully connected to serial.")
except Exception as e:
    print(f"Could not connect: {e}")
    exit()

# Create daily folder for telemetry values
parent_dir = "C:\\Users\\fizzer\\Documents\\lap_data"
current_date = date.today()
new_path = os.path.join(parent_dir, f"{current_date}")

try:
    os.mkdir(new_path)
    print(f"Created directory: {current_date}!")
except Exception as e:
    print(e)

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

# Create csv file
current_time = datetime.now().strftime("%Hh%Mm%Ss")

with open(os.path.join(new_path, f"{current_time}.csv"), 'a', newline='') as csvfile:
    writer = csv.writer(csvfile)

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
        steering_wheel = angles[0] / 32768

        # Normalize throttle and brake angles
        # Centre of axis found to be ~-216 via testing
        # Create deadzone of -500 to 0
        if(angles[1] < -500) :
            throttle = -(angles[1] + 216.0) / 32552
            brake = 0
        elif (angles[1] > 0) :
            brake = (angles[1] + 216.0) / 32983
            throttle = 0
        else :
            brake = 0
            throttle = 0
        
        # Write to csv
        writer.writerow([f"{steering_wheel:.3f}", f"{throttle:.3f}", f"{throttle:.3f}"])

        # Send message over serial with 3 decimals of precision
        precision = 3
        serial_message = f"{steering_wheel:.{precision}f},{throttle:.{precision}f},{brake:.{precision}f}\n"
        message_binary = serial_message.encode('utf-8')
        ser.write(message_binary)

        # Confirm message that has been sent
        print(serial_message)
        
        # Delay
        time.sleep(delay)
