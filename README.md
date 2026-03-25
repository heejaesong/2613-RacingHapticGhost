# 2613 - Racing Haptic Ghost
Jordan Werstiuk, Heejae Song, Matteo Cappellano

ENPH459, Jan-Apr 2026

## Operation Instructions
These are the steps required to get one of the motors moving and being controlled by the ESP, as of 25/3 with the breadboard testing setup.

1. Use the LiPo voltage tester to ensure none of the 6 cells are below 3.5V. The leftmost pin of the tester connects into the black wire side of the LiPo connector. It will beep loudly when plugged in.
2. With the LiPo still unplugged from the motor, plug the connector coming out of the grey block on the SPI to CAN-FD adapter into the CAN connector on the Moteus controller. It is the white connector located on the same side as the only yellow connector on the board.
3. Plug the ESP into the USB port on the laptop. It will automatically start running whatever code was last flashed. Note that the ESP has two USB-C ports, and the one on the right side (when looking at the MCU with the text right side up) must be used.
4. If you want to flash new code, open ArduinoIDE, and either open the file you want, or copy and paste code from GitHub into the editor. You must select the right board and port: ESP32 Dev Module and COM_ (the IDE might automatically detect it, you can also check in Device Manager). Click the Upload button (icon is arrow pointing to the right) in the top left corner of the IDE.
5. Once the code is running (either the code you just flashed or the code that was already flashed), if the Serial Monitor doesn't automatically open, click on tools at the top, and select Serial Monitor.
6. Connect the LiPo battery to the Moteus controller. If the motor doesn't immediately start moving, click the 'EN / RST' button on the ESP. If it still doesn't start moving, unplug the ESP from the laptop, and plug it back in.
7. You will see the mode, position, and fault values being printed in the Serial Monitor. To interpret a fault value, ask an LLM (I don't know yet where to find the list of fault value meanings). 
8. To make the motor stop moving, unplug the battery. This can take a moment as it is difficult to unplug, so to make it stop sooner, unplug the ESP from the laptop, and then unplug the battery (do not leave the battery plugged in).
