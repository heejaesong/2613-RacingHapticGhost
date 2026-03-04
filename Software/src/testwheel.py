import sdl2
import serial
import time
print("imports ok")

try:
    ser = serial.Serial('COM3', 115200, timeout=0.01)
    print("Successfully connected to ESP-32.")
except Exception as e:
    print(f"Could not connect: {e}")
    exit()

sdl2.SDL_Init(sdl2.SDL_INIT_JOYSTICK)

num_joysticks = sdl2.SDL_NumJoysticks()

print("Joysticks: " ,num_joysticks)

joystick = sdl2.SDL_JoystickOpen(0)

num_axes = sdl2.SDL_JoystickNumAxes(joystick)

steering_wheel = 0
throttle = 0
brake = 0


while True:
    sdl2.SDL_PumpEvents()

    angles = []

    for axis in range(num_axes):
        axis_angle = sdl2.SDL_JoystickGetAxis(joystick, axis)
        angles.append(axis_angle)
    
    steering_wheel = angles[0] / 32768
    if(angles[1] < -500) :
        throttle = -(angles[1] + 216.0) / 32552
        brake = 0
    elif (angles[1] > 0) :
        brake = (angles[1] + 216.0) / 32983
        throttle = 0
    else :
        brake = 0
        throttle = 0

    precision = 3
    serial_message = f"{steering_wheel:.{precision}f},{throttle:.{precision}f},{brake:.{precision}f}\n"
    message_binary = serial_message.encode('utf-8')

    print(serial_message)
    ser.write(message_binary)

    
    print(f"Normalized values: Steering: {steering_wheel}, Throttle: {throttle}, Brake: {brake}")


    time.sleep(0.2)
