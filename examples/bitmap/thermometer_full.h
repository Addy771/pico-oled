#include "pico/stdlib.h"


const uint8_t thermometer_full_width = 8;
const uint8_t thermometer_full_height = 20;

const uint8_t thermometer_full_bitmap [] = 
{
    // 'thermometer_full', 8x20px
    0x00, 0x24, 0xfe, 0xff, 0xff, 0xfe, 0x24, 0x00, 0xc0, 0xe9, 0xff, 0xff, 0xff, 0xff, 0xe9, 0xc0, 
    0x03, 0x07, 0x0f, 0x0f, 0x0f, 0x0f, 0x07, 0x03
};

const bitmap thermometer_full = {(uint8_t *) thermometer_full_bitmap, thermometer_full_width, thermometer_full_height};