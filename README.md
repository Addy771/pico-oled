# pico-oled
A library for driving SSD1306 displays on Pi Pico (RP2040) and similar platforms.

pico-oled uses a screen buffer in memory to allow for fast drawing routines. For a typical SSD1306 128x64 OLED screen, the buffer consumes 1K of RAM. pico-oled is written for use with the Raspberry Pi Pico SDK, though it should be simple to port to other platforms.
