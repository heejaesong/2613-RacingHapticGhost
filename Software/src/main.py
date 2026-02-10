import vgamepad as vg
import serial
import time

# 1. Initialize the Virtual Xbox 360 Controller
gamepad = vg.VX360Gamepad()

# 2. Setup Serial (Ensure your Arduino is plugged in and Serial Monitor is CLOSED)
# Replace 'COM3' with your actual port
print("Script started")
try:
    ser = serial.Serial('COM4', 115200, timeout=0.01)
    print("Successfully connected to Arduino.")
except Exception as e:
    print(f"Could not connect: {e}")
    exit()

print("Virtual Xbox Controller is live. Press Ctrl+C to exit.")

while True:
    try:
        if ser.in_waiting > 0:
            # Read and parse the data: "Steering,Accel,Button"
            line = ser.readline().decode('ascii').strip()
            data = line.split(',')

            if len(data) == 2:
                # Convert strings to integers
                raw_steer = int(data[0])  # 0 to 1023
                raw_accel = int(data[1])  # 0 to 1023
                # raw_btn   = int(data[2])  # 0 or 1

                # 3. MAPPING LOGIC
                # Steering: Map 0-1023 to -1.0 (Left) to 1.0 (Right)
                # Formula: (value / half_of_max) - 1
                steer_val = (raw_steer / 511.5) - 1.0
                
                # --- ACCELERATION (Forward Only) ---
                # If stick is forward (raw_accel < 512)
                if raw_accel < 512:
                    # Map 512 (center) to 0 (top) -> 0.0 to 1.0
                    accel_val = (512 - raw_accel) / 512.0
                else:
                    # Stick is centered or backward
                    accel_val = 0.0

                # print(f"Steer: {steer_val:.2f}, Accel: {accel_val:.2f}")

                # 4. UPDATE VIRTUAL GAMEPAD
                # Set Left Stick X-axis for steering
                gamepad.left_joystick_float(x_value_float=steer_val, y_value_float=0.0)
                
                # Set Right Trigger for gas
                gamepad.right_trigger_float(value_float=accel_val)

                # Set 'A' Button for the joystick click
                # if raw_btn == 1:
                    # gamepad.press_button(button=vg.XUSB_BUTTON.XUSB_GAMEPAD_A)
                # else:
                    # gamepad.release_button(button=vg.XUSB_BUTTON.XUSB_GAMEPAD_A)

                # Send the update to Windows
                gamepad.update()

    except KeyboardInterrupt:
        print("\nClosing Bridge...")
        break
    except Exception as e:
        # Ignore minor serial glitches
        continue