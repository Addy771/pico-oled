/**
 *  gfxfont.h
 *  Based on Adafruit_GFX's font structure
 */

#ifndef _GFXFONT_H_
    #define _GFXFONT_H_

    // Per character font data
    typedef struct 
    {
        uint16_t bitmap_x;		// x coordinate in font table bitmap
        uint8_t width;
        uint8_t height;
        uint8_t x_advance;		// How much cursor should move after each character
        int8_t x_offset;
        int8_t y_offset;
    } gfx_char;


    typedef struct
    {
        uint8_t *bitmap;       // bitmap table
        gfx_char *character;   // character data array
        uint16_t first;			// First character in font
        uint16_t last; 
        uint8_t line_height;   
        uint16_t src_width;     // Bitmap table image total width
    } gfx_font;
    
#endif