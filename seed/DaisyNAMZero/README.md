# DaisyNAMZero


## Build Libraries
1. Configure **VS Code** according to [this tutorial](https://www.youtube.com/watch?v=AbvaTdAyJWk). 
2. Set **Git Bash** as the default terminal profile.  
3. Open the command palette (**Ctrl + P**) and run:  **task build_all**
 
## Basic Bootloader Mode
To enter **bootloader mode**:

1. Press and hold **BOOT**.  
2. While holding BOOT, press and hold **RESET**.  
3. Release **RESET** while still holding BOOT.  
4. Finally, release **BOOT**.

## Upload

To build and upload via DFU in bootloader mode, open the command palette (**Ctrl + P**) and run: **task build_and_program_dfu**  

---

## Debug using external device

1) Plug the device to its 9V input and also to its regular usb port.
2) Plug the device to the STLINK-V3MINIE. (Red stripe points away from the USB port)
3) Put the device in debug mode by pressing RESET and then BOOT (it should be done always before debugging).
4) Start debugging from VS Code, setting up some breakpoints.

 
