# pico-oled
A library for driving SSD1306 displays on Pi Pico (RP2040) and similar platforms.

pico-oled uses a screen buffer in memory to allow for fast drawing routines. For a typical SSD1306 128x64 OLED screen, the buffer consumes 1K of RAM. pico-oled is written for use with the Raspberry Pi Pico SDK, though it should be simple to port to other platforms.


# Adding pico-oled to your project
Copy the pico-oled folder into your project folder either manually or as a git submodule. 

Edit your CMakeLists.txt file and modify the add_executable statement to include __pico-oled/pico-oled.cpp__


# License
This project is licensed under the [CC BY-NC 4.0 license](https://creativecommons.org/licenses/by-nc/4.0/).

