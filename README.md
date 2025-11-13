# CANBus-e92_kombi
Short and easy to understand sketch for arduino to make your BMW e9x platform cluster running on the bench

Written to work with [this](https://github.com/coryjfowler/MCP_CAN_lib) library.

Works fine with Arduino MEGA and SeeedStudio CAN-Bus shield 2.0 - **keep in mind that all CAN-Bus shields are not the same. You may need to edit the code to work with your hardware properly (it's usually the CS pin or frequency)**

## Pinout
![pinout](pinout.png)

**Don't forget to connect arduino ground to cluster ground**
