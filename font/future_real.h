/**
 * future_real.h
 * manually created
 */
#include "../gfx_font.h"

 const uint8_t future_real_bitmaps[] = 
 {
// 'Future_real', 90x21px
0x00, 0x00, 0x00, 0xf0, 0xf0, 0xf0, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 
0xff, 0xff, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0x00, 0x00, 0x00, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0x00, 0x00, 0x00, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 
0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0xf0, 0xf0, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0xf0, 0xf0, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 
0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x01, 0x01, 0x01, 0xf1, 
0xf1, 0xf1, 0xf0, 0xf0, 0xf0, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0x00, 0x00, 0x00, 0x00, 0xf0, 0xf0, 
0xf0, 0xf1, 0xf1, 0xf1, 0x00, 0x00, 0x00, 0xf0, 0xf0, 0xf0, 0xf1, 0xf1, 0xf1, 0x01, 0x01, 0x01, 
0x00, 0x00, 0x00, 0xf1, 0xf1, 0xf1, 0x01, 0x01, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0xf1, 0xf1, 0xf1, 0x00, 0x00, 0x00, 0xf0, 0xf0, 0xf0, 0xf1, 0xf1, 
0xf1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x1f, 0x1f, 0x1f, 
0x00, 0x00, 0x00, 0x1f, 0x1f, 0x1f, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x00, 0x00, 
0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 
0x00, 0x00, 0x00, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 
0x01, 0x01, 0x00, 0x00, 0x00, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x00, 0x00, 0x00

 };


 const gfx_char future_real_chars[] = 
 {
	{ 0  ,  9, 20, 10, 0, 1},		//  48: '0'
	{ 9  ,  9, 20, 10, 0, 1},		//  49: '1'
	{ 18 ,  9, 20, 10, 0, 1},		//  50: '2'
	{ 27 ,  9, 20, 10, 0, 1},		//  51: '3'
	{ 36 ,  9, 20, 10, 0, 1},		//  52: '4'
	{ 45 ,  9, 20, 10, 0, 1},		//  53: '5'
	{ 54 ,  9, 20, 10, 0, 1},		//  54: '6'
	{ 63 ,  9, 20, 10, 0, 1},		//  55: '7'
	{ 72 ,  9, 20, 10, 0, 1},		//  56: '8'
	{ 81 ,  9, 20, 10, 0, 1}		//  57: '9'

 };


 const gfx_font future_real = {(uint8_t *)future_real_bitmaps, (gfx_char *)future_real_chars, 48, 57, 22, 90};
