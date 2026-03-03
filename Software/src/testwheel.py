import sdl2
import serial
import time
print("imports ok")

sdl2.SDL_Init(sdl2.SDL_INIT_JOYSTICK)

num_joysticks = sdl2.SDL_NumJoysticks()

print("Joysticks: " ,num_joysticks)

joystick = sdl2.SDL_JoystickOpen(0)

num_axes = sdl2.SDL_JoystickNumAxes(joystick)

while True:
    sdl2.SDL_PumpEvents()

    angles = []

    for axis in range(num_axes):
        axis_angle = sdl2.SDL_JoystickGetAxis(joystick, axis)
        angles.append(axis_angle)
    
    print(angles)

    time.sleep(0.2)
