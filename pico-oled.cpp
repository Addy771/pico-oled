#include "pico-oled.hpp"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include <stdlib.h>
#include <stdio.h>
#include "gfx_font.h"

//#define GFX_DEBUG

#define PRINT_NUM_BUFFER 30     // Length of temporary buffers for printing numbers

// pico-oled constructor
pico_oled::pico_oled(uint8_t i2c_address, uint8_t screen_width, uint8_t screen_height)
{
    // Init private variables
    this->i2c_addr = i2c_address;
    this->oled_width = screen_width;
    this->oled_height = screen_height;

    // Allocate a buffer for the entire screen + control byte
    this->screen_buf_length = (oled_height / OLED_PAGE_HEIGHT) * oled_width + 1;
    this->screen_buffer = (uint8_t*) malloc(this->screen_buf_length);

    // Control byte, Co = 0, D/C = 1 => the driver expects data to be written to RAM
    this->screen_buffer[0] = 0x40;  

    // Indicate that font has not been set yet
    this->font_set = 0;

    // Set cursor default position to something reasonable
    this->cursor_x = 0;
    this->cursor_y = 10;

    // Set the draw pixel function to the default
    this->draw_pixel_fn = &this->draw_pixel;
    this->pixel_counter = 0;
}



void pico_oled::oled_send_cmd(uint8_t cmd)
{
    // I2C write process expects a control byte followed by data
    // this "data" can be a command or data to follow up a command

    // Co = 1, D/C = 0 => the driver expects a command
    uint8_t buf[2] = {0x80, cmd};
    i2c_write_blocking(i2c_default, (this->i2c_addr & OLED_WRITE_MODE), buf, 2, false);   
}


// Write to the display's configuration registers to set it up
void pico_oled::oled_init()
{
    // some of these commands are not strictly necessary as the reset
    // process defaults to some of these but they are shown here
    // to demonstrate what the initialization sequence looks like

    // some configuration values are recommended by the board manufacturer

    this->oled_send_cmd(OLED_SET_DISP | 0x00); // set display off

    /* memory mapping */
    this->oled_send_cmd(OLED_SET_MEM_ADDR); // set memory address mode
    this->oled_send_cmd(0x00); // horizontal addressing mode

    /* resolution and layout */
    this->oled_send_cmd(OLED_SET_DISP_START_LINE); // set display start line to 0

    this->oled_send_cmd(OLED_SET_SEG_REMAP | 0x01); // set segment re-map
    // column address 127 is mapped to SEG0

    this->oled_send_cmd(OLED_SET_MUX_RATIO); // set multiplex ratio
    this->oled_send_cmd(this->oled_height - 1); // set OLED vertical resolution

    this->oled_send_cmd(OLED_SET_COM_OUT_DIR | 0x08); // set COM (common) output scan direction
    // scan from bottom up, COM[N-1] to COM0

    this->oled_send_cmd(OLED_SET_DISP_OFFSET); // set display offset
    this->oled_send_cmd(0x00); // no offset

    this->oled_send_cmd(OLED_SET_COM_PIN_CFG); // set COM (common) pins hardware configuration
    this->oled_send_cmd(0x12); // 0x12 for alternative COM pin configuration

    /* timing and driving scheme */
    this->oled_send_cmd(OLED_SET_DISP_CLK_DIV); // set display clock divide ratio 
    this->oled_send_cmd(0x80); // div ratio of 1, standard freq

    this->oled_send_cmd(OLED_SET_PRECHARGE); // set pre-charge period
    this->oled_send_cmd(0xF1); // Vcc internally generated on our board

    this->oled_send_cmd(OLED_SET_VCOM_DESEL); // set VCOMH deselect level
    this->oled_send_cmd(0x30); // 0.83xVcc  

    /* display */
    this->oled_send_cmd(OLED_SET_CONTRAST); // set contrast control 
    this->oled_send_cmd(0x0F); // 0 to 255

    this->oled_send_cmd(OLED_SET_ENTIRE_ON); // set entire display on to follow RAM content

    this->oled_send_cmd(OLED_SET_NORM_INV); // set normal (not inverted) display

    this->oled_send_cmd(OLED_SET_CHARGE_PUMP); // set charge pump
    this->oled_send_cmd(0x14); // Vcc internally generated on our board

    this->oled_send_cmd(OLED_SET_SCROLL | 0x00); // deactivate horizontal scrolling if set
    // this is necessary as memory writes will corrupt if scrolling was enabled

    // Clear the display
    this->fill(0);
    this->render();    

    this->oled_send_cmd(OLED_SET_DISP | 0x01); // turn display on
}


// Fill entire display with the specified byte
void pico_oled::fill(uint8_t fill)
{
    // Skip first byte since it's a control byte
    for (int i=1; i < this->screen_buf_length; i++)
    {
        this->screen_buffer[i] = fill;
    }
}


// Write the entire screen buffer to the OLED 
void pico_oled::render()
{
    // Set the start/end coordinates for drawing the entire screen
    oled_send_cmd(OLED_SET_COL_ADDR);
    oled_send_cmd(0);                       // Start column
    oled_send_cmd(this->oled_width - 1);    // End column

    oled_send_cmd(OLED_SET_PAGE_ADDR);
    oled_send_cmd(0);                                           // Start page
    oled_send_cmd(this->oled_height / OLED_PAGE_HEIGHT - 1);    // End page

    i2c_write_blocking(i2c_default, (this->i2c_addr & OLED_WRITE_MODE), this->screen_buffer, this->screen_buf_length, false);
}


// Tell the display whether to turn on all pixels or follow RAM contents
void pico_oled::all_on(uint8_t disp_on)
{
    if (disp_on)
        this->oled_send_cmd(0xA5);
    else
        this->oled_send_cmd(0xA4);
}


// Copy a block from the source src_bitmap to the screen buffer
void pico_oled::blit_screen(const uint8_t *src_bitmap, uint16_t src_width, uint16_t src_x, uint8_t src_y, uint8_t blit_width, uint8_t blit_height, uint8_t screen_x, uint8_t screen_y)
{
    uint8_t dest_start_page = screen_y / OLED_PAGE_HEIGHT;
    uint8_t dest_page_offset = screen_y - dest_start_page * OLED_PAGE_HEIGHT;
    uint8_t dest_end_page = (screen_y + blit_height - 1) / OLED_PAGE_HEIGHT;
    uint8_t dest_end_col = screen_x + blit_width - 1;
    uint8_t src_start_page = src_y / OLED_PAGE_HEIGHT;
    uint8_t src_page_offset = src_y - src_start_page * OLED_PAGE_HEIGHT;
    //uint8_t src_end_page = (src_y + blit_height - 1) / OLED_PAGE_HEIGHT; 
    int8_t offset_delta;

#ifdef GFX_DEBUG
    printf("\n\nBeginning blit of bitmap of %dx%d, from source pos %d,%d to screen position %d,%d\n", blit_width, blit_height, src_x, src_y, screen_x, screen_y);
#endif
    // Make sure drawing happens within screen bounds
    if (dest_end_page > (this->oled_height / OLED_PAGE_HEIGHT))  
    {
        dest_end_page = (this->oled_height / OLED_PAGE_HEIGHT);
    }

    if (dest_end_col > this->oled_width - 1)
    {
        dest_end_col = this->oled_width - 1;
    }

    // Make sure source data is within source bitmap bounds


    offset_delta = dest_page_offset - src_page_offset;

    uint8_t screen_line = screen_y;
    uint8_t src_line = src_y;
    uint8_t end_line = src_y + blit_height - 1;
    uint8_t src_page, screen_page, lines_available, lines_drawable, u_mask, l_mask;
    uint8_t exit_flag = 0;
    uint8_t last_src_page = 255;    // Init to invalid number so it's different to src_page by default
    uint8_t last_screen_page = 255;
    int8_t mask_amt;

#ifdef GFX_DEBUG
    printf("end_line: %d, offset: %d\n", end_line, offset_delta);
#endif    

    while (!exit_flag)
    {
        // Calc. how much data can be sourced from bmp page
        src_page = src_line / OLED_PAGE_HEIGHT;
        lines_available =  OLED_PAGE_HEIGHT - (src_line - src_page * OLED_PAGE_HEIGHT);

        // Calc. how much of that data can be drawn starting at the screen line
        screen_page = screen_line / OLED_PAGE_HEIGHT;
        lines_drawable = OLED_PAGE_HEIGHT - (screen_line - screen_page * OLED_PAGE_HEIGHT);  

        // No masking by default
        u_mask = 0;
        l_mask = 0;

        // Limit available lines at the end of the image
        if (src_line + lines_available > end_line)
        {
            lines_available = end_line - src_line + 1;
            u_mask = OLED_PAGE_HEIGHT - lines_available; // Mask off unwanted upper bits
            
        }

        // Limit lines at the start of the image
        if (src_line == src_y)
        {
            l_mask = src_page_offset; // Mask off unwanted lower bits
        }
        
        // Use available lines or drawable lines, whichever is smaller
        if (lines_drawable > lines_available)
        {
            lines_drawable = lines_available;
        }

        // If there's no offset, draw a whole page  //// is this necessary?
        if  (lines_drawable == 0)
            lines_drawable = OLED_PAGE_HEIGHT;

#ifdef GFX_DEBUG
        // // DEBUG stuff
        // this->render();
        // sleep_ms(100);
        printf("screen pg:%d line:%d, src pg:%d line:%d, drawable:%d, available:%d, u_mask:%d, l_mask:%d\n", screen_page, screen_line, src_page, src_line, lines_drawable, lines_available, u_mask, l_mask);
        // //printf("last_src_page: %d, src_page %d\n", last_src_page, src_page);
        // sleep_ms(1000);
        // u_mask = 0;
        // l_mask = 0;
#endif        
        
        for (uint8_t column = screen_x; column <= dest_end_col; column++)
        {
            uint8_t source_data = src_bitmap[(column + src_x - screen_x) + (src_page)*src_width];   

            //source_data &= 0xFF >> u_mask;   // Mask off upper bits
            //source_data &= 0xFF << l_mask;   // Mask off lower bits            

            // >> = shift to higher screen position
            // << = shift to lower screen position

            // Draw to a screen page
            if (offset_delta >= 0)
            {
                // Shift source data by offset_delta so it ends up in the right position
                // Shift is different when drawing from the same page a second time
                if (last_src_page == src_page)   
                {
                    // Mask off bottom - offset when necessary
                    mask_amt = u_mask - (OLED_PAGE_HEIGHT - offset_delta);
                    if (mask_amt > 0)
                        source_data &= 0xFF >> mask_amt;  

                    // bottom pixels being shifted up
                    this->screen_buffer[1 + column + screen_page*this->oled_width] |= source_data >> (OLED_PAGE_HEIGHT - offset_delta);
                }
                else
                {
                    // Mask off top and bottom
                    source_data &= 0xFF << l_mask; 
                    source_data &= 0xFF >> u_mask;

                    // top pixels being shifted down
                    this->screen_buffer[1 + column + screen_page*this->oled_width] |= source_data << offset_delta;
                }
            }
            else
            {
                // Shift source data by offset_delta so it ends up in the right position
                // Shift is different when drawing to the same page a second time
                if (last_screen_page == screen_page)
                {
                    // Mask off bottom - offset when necessary
                    mask_amt = u_mask - (OLED_PAGE_HEIGHT + offset_delta);
                    //if (u_mask > (-1*offset_delta))
                    if (mask_amt > 0 && lines_drawable > u_mask)
                    {
                        source_data &= 0xFF >> mask_amt;  
                    }
                    
                    // top pixels being shifted down
                    this->screen_buffer[1 + column + screen_page*this->oled_width] |= source_data << (OLED_PAGE_HEIGHT + offset_delta); // page height - offset
                }
                else
                {
                    // Mask off top and bottom
                    source_data &= 0xFF << l_mask; 

                    if (u_mask > (-1*offset_delta))
                    {
                        //printf("u_mask: %d, offset: %d, sum: %d\n",u_mask, offset_delta, u_mask + offset_delta);
                        source_data &= 0xFF >> (u_mask + offset_delta);
                    }

                    // bottom pixels being shifted up
                    this->screen_buffer[1 + column + screen_page*this->oled_width] |= source_data >> (-1 * offset_delta);
                }
            }
            
        }

        // Increment screen and source line counts by how much data was drawn
        screen_line += lines_drawable;
        src_line += lines_drawable;

        // Update last pages
        last_src_page = src_page;
        last_screen_page = screen_page;

        // End if all necessary lines were drawn
        //if (screen_line >= screen_y + blit_height - 1)
        if (src_line > end_line)
        {
            exit_flag = 1;
#ifdef GFX_DEBUG            
            printf("END! screen_line: %d, src_line: %d\n", screen_line, src_line);
#endif
        }

    }
}


// Set the font for future text drawing
void pico_oled::set_font(gfx_font font)
{
    this->font = font;
    this->font_set = 1;
}


// Draw a single character at the specified position. Font must be previously set.
void pico_oled::draw_char(uint8_t char_c, uint8_t x_pos, uint8_t y_pos)
{
    // Abort if the char is not within the drawable range
    // if (char_c > this->font.last || char_c < this->font.first)
    //     return;

#ifdef GFX_DEBUG            
    printf("Character: %c, src_x: %d\n", char_c, this->font.character[char_c].bitmap_x);
#endif
    
    char_c -= this->font.first;     // First character is element 0 of table
    
    this->blit_screen(this->font.bitmap, this->font.src_width, this->font.character[char_c].bitmap_x, 0, this->font.character[char_c].width, this->font.character[char_c].height, x_pos + this->font.character[char_c].x_offset, y_pos + this->font.character[char_c].y_offset);
}


// Print a basic string. Only basic character drawing is supported, control characters have no effect
void pico_oled::print(const char *print_str)
{
    // Abort if no font has been set yet
    if (!this->font_set)
        return;

    while (*print_str != '\0')
    {
        // Draw character if it's valid
        if (*print_str <= this->font.last && *print_str >= this->font.first)
        {
            // If character would be drawn over the edge of the screen
            if (this->font.character[*print_str - this->font.first].width + this->cursor_x >= this->oled_width)
            {      
                // Reposition cursor at the left edge of the screen, one line down
                this->cursor_x = 0;
                this->cursor_y += this->font.line_height;
            }
            
            this->draw_char(*print_str, this->cursor_x, this->cursor_y);

            this->cursor_x += this->font.character[*print_str - this->font.first].x_advance;
        }
        else if (*print_str == '\n')
        {
            // Reposition cursor at the left edge of the screen, one line down
            this->cursor_x = 0;
            this->cursor_y += this->font.line_height;            
        }

        print_str++;
    }
}


// Print numbers by outsourcing formatting to sprintf()

void pico_oled::print_num(const char *format_str, int32_t print_data)
{
    // Allocate a buffer big enough to hold the printed data
    char output_buf[PRINT_NUM_BUFFER];
    sprintf(output_buf, format_str, print_data);
    this->print(output_buf);
}


void pico_oled::print_num(const char *format_str, uint32_t print_data)
{
    // Allocate a buffer big enough to hold the printed data
    char output_buf[PRINT_NUM_BUFFER];
    sprintf(output_buf, format_str, print_data);
    this->print(output_buf);
}


void pico_oled::print_num(const char *format_str, float print_data)
{
    // Allocate a buffer big enough to hold the printed data
    char output_buf[PRINT_NUM_BUFFER];
    sprintf(output_buf, format_str, print_data);
    this->print(output_buf);
}

void pico_oled::print_num(const char *format_str, uint8_t print_data)
{
    this->print_num(format_str, (uint32_t) print_data);
}

void pico_oled::print_num(const char *format_str, int8_t print_data)
{
    this->print_num(format_str, (int32_t) print_data);
}

void pico_oled::print_num(const char *format_str, uint16_t print_data)
{
    this->print_num(format_str, (uint32_t) print_data);
}

void pico_oled::print_num(const char *format_str, int16_t print_data)
{
    this->print_num(format_str, (int32_t) print_data);
}


// Draw a single pixel in the screen buffer
void pico_oled::draw_pixel(uint8_t x, uint8_t y)
{
    // Abort if coordinates are out of bounds
    if (x >= this->oled_width || y >= this->oled_height)
        return;

    uint8_t screen_page = y / OLED_PAGE_HEIGHT;

    // Write to the column where the target pixel is
    this->screen_buffer[1 + x + screen_page*this->oled_width] |= 1 << (y - screen_page*OLED_PAGE_HEIGHT);
}


// Draw a single pixel every other time this function is called
void pico_oled::draw_pixel_alternating(uint8_t x, uint8_t y)
{
    if (this->pixel_counter++ > 0)
    {
        this->pixel_counter = 0;
        this->draw_pixel(x, y);
    }   
}


// Draw a line with Bresenham's algorithm
void pico_oled::draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
    // Only try to use fast line functions for the default solid line draw_pixel()
    if (this->draw_pixel_fn == &this->draw_pixel)
    {
        // Draw straight lines with the fast function instead
        if (y1 == y2)
        {
            this->draw_fast_hline(x1, x2, y1);
            return;
        }

        if (x1 == x2)
        {
            this->draw_fast_vline(y1, y2, x1);
            return;
        }
    }

    int16_t dx, dy, p;
    int16_t tmp;
    int16_t steep = abs(y2 - y1) > abs(x2 - x1);

    if (steep)
    {
        // Swap x/y roles
        tmp = x1;   // Save x1
        x1 = y1;    // y1 -> x1
        y1 = tmp;   // x1 -> y1

        tmp = x2;   // Save x2
        x2 = y2;    // y2 -> x2
        y2 = tmp;   // x2 -> y2

    }

    // Switch coordinates around so that x1 < x2
    if (x1 > x2)
    {
        tmp = x1;   // Save x1
        x1 = x2;    // x2 -> x1
        x2 = tmp;   // x1 -> x2

        tmp = y1;   // Save y1
        y1 = y2;    // y2 -> y1
        y2 = tmp;   // y1 -> y2
    }


    dx = x2 - x1;
    dy = abs(y2 - y1);

    int16_t y_step;

    if (y1 < y2)
        y_step = 1;
    else
        y_step = -1;

    //p = 2*dy - dx;
    p = dx / 2;

    for (; x1 <= x2; x1++)
    {
        // If slope is less than one, coordinates are swapped
        if (steep)
            this->draw_pixel_fn(y1, x1);
        else
            this->draw_pixel_fn(x1, y1);

        p -= dy;

        if (p < 0)
        {
            y1 += y_step;
            p += dx;
        }

    }
}


// Draw a dotted line by switching out the draw_pixel function
void pico_oled::draw_line_dotted(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
    this->pixel_counter = 0;    // Reset pixel counter so that calling this function results in the same kind of line draw each time
    this->draw_pixel_fn = &this->draw_pixel_alternating;  // Change the draw pixel function so it only draws every other pixel
    
    this->draw_line(x1, y1, x2, y2);

    this->draw_pixel_fn = &this->draw_pixel;    // Restore the default draw_pixel function
}


// Simplified line drawing for horizontal lines
void pico_oled::draw_fast_hline(uint8_t x1, uint8_t x2, uint8_t y)
{

    // Flip x coordinates to keep x1 < x2
    if (x1 > x2)
    {
        uint8_t tmp = x1;
        x1 = x2;
        x2 = tmp;
    }

    // fix coordinates or abort if coordinates are out of bounds
    if (x1 >= this->oled_width - 1  || y >= this->oled_height - 1)
        return;    

    if (x2 >= this->oled_width)
        x2 = this->oled_width - 1;

    uint8_t screen_page = y / OLED_PAGE_HEIGHT;
    uint8_t mask = 1 << (y - screen_page*OLED_PAGE_HEIGHT);

    for (; x1 <= x2; x1++)
    {
        // Write to the column where the target pixel is
        this->screen_buffer[1 + x1 + screen_page*this->oled_width] |= mask;    
    }
}


// Simplified line drawing for vertical lines
void pico_oled::draw_fast_vline(uint8_t y1, uint8_t y2, uint8_t x)
{
    // Flip y coordinates to keep y1 < y2
    if (y1 > y2)
    {
        uint8_t tmp = y1;
        y1 = y2;
        y2 = tmp;
    }

    // fix coordinates or abort if coordinates are out of bounds
    if (y1 >= this->oled_height - 1  || x >= this->oled_width - 1)
        return;    

    if (y2 >= this->oled_width)
        y2 = this->oled_width - 1;

    uint8_t first_page = y1 / OLED_PAGE_HEIGHT;    
    uint8_t last_page = y2 / OLED_PAGE_HEIGHT;
    uint8_t top_offset = y1 - first_page*OLED_PAGE_HEIGHT;
    uint8_t bottom_offset = y2 - last_page*OLED_PAGE_HEIGHT + 1;

    for (uint8_t page = first_page; page <= last_page; page++)
    {
        uint8_t mask = 0xFF;

        if (page == first_page)
        {
            mask &= 0xFF << top_offset; // Pull zeros into LSBs
        }

        if (page == last_page)
        {
            mask &= 0xFF >> (OLED_PAGE_HEIGHT - bottom_offset);
        }

        // Draw current page
        this->screen_buffer[1 + x + page*this->oled_width] |= mask; 
    }

}


// Draw a vertical progress bar 
// 
void pico_oled::draw_vbar(uint8_t fullness, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    uint8_t height = y1 - y0;
    uint8_t filled_px = (fullness * (height - 2)) / 100;

    // Draw the outline
    this->draw_fast_hline(x0, x1, y0);  // Top 
    this->draw_fast_hline(x0, x1, y1);  // Bottom
    this->draw_fast_vline(y0, y1, x0);  // Left
    this->draw_fast_vline(y0, y1, x1);  // Right    

    // Draw lines to fill the internal area
    for (uint8_t y_line = y1 - 1; y_line >= (y1 - 1) - filled_px; y_line--)
        this->draw_fast_hline(x0 + 1, x1 - 1, y_line);
}


// Draw a horizontal progress bar 
// start_right: Greater than 0 for bar to start on the right instead of the left side
void pico_oled::draw_hbar(uint8_t fullness, uint8_t start_right, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    uint8_t width = x1 - x0;
    uint8_t filled_px = (fullness * (width - 2)) / 100;

    // Draw the outline
    this->draw_fast_hline(x0, x1, y0);  // Top 
    this->draw_fast_hline(x0, x1, y1);  // Bottom
    this->draw_fast_vline(y0, y1, x0);  // Left
    this->draw_fast_vline(y0, y1, x1);  // Right    

    // Draw lines to fill the internal area
    if (start_right)
    {
        for (uint8_t x_line = x1 - 1; x_line >= (x1 - 1) - filled_px; x_line--)
            this->draw_fast_vline(y0 + 1, y1 - 1, x_line);
    }
    else
    {
        for (uint8_t x_line = x0 + 1; x_line <= (x0 + 1) + filled_px; x_line++)
            this->draw_fast_vline(y0 + 1, y1 - 1, x_line);
    }
}


/* Draw a vertical, bitmapped progress bar.
 * empty_bitmap: bitmap of the bar (empty frame) when it is 0% full
 * full_bitmap: bitmap of the bar (with or without frame) when it is 100% full.
*/
void pico_oled::draw_bmp_vbar(uint8_t fullness, const uint8_t *empty_bitmap, const uint8_t *full_bitmap, uint8_t x, uint8_t y)
{
    uint8_t filled_px = (fullness * empty_bitmap.height) / 100;   // This assumes the active area is the whole bar bmp, including frame

    // Draw the empty frame 
    this->blit_screen(empty_bitmap, empty_bitmap.width, 0, 0, empty_bitmap.width, empty_bitmap.height, x, y);

    // Draw as much of the full bitmap that would be proportional to fullness (from the bottom)
    this->blit_screen(full_bitmap, full_bitmap.width, 0, full_bitmap.height - filled_px, full_bitmap.width, filled_px, x, y + (full_bitmap.height - filled_px));

}


/* Solid fill a rectangular region. 
 * blank: if true, rectangle will be cleared (off) instead of filled
*/
void pico_oled::fill_rect(uint8_t blank, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
    // Switch coordinates around so that y1 < y2
    if (y1 > y2)
    {
        uint8_t tmp;
        tmp = x1;   // Save x1
        x1 = x2;    // x2 -> x1
        x2 = tmp;   // x1 -> x2

        tmp = y1;   // Save y1
        y1 = y2;    // y2 -> y1
        y2 = tmp;   // y1 -> y2
    }    

    uint8_t start_page = y1 / OLED_PAGE_HEIGHT;
    uint8_t end_page = y2 / OLED_PAGE_HEIGHT;    
    uint8_t col_mask;

    for (uint8_t page = start_page; page <= end_page; page++)
    {
        for(uint8_t column = x1; column != x2; (x1 <= x2) ? ++column : --column)
        {
            col_mask = 0xFF;

            // For the first page, we may not need to fill every pixel
            if (page == start_page)
                col_mask &= 0xFF << (y1 - page*OLED_PAGE_HEIGHT);   // Shift zeros into LSBs

            // For the last page, we may not need to fill every pixel
            if (page == end_page)
                col_mask &= 0xFF >> (y2 - page*OLED_PAGE_HEIGHT);   // Shift zeros into MSBs
            
            // Modify the screen contents according to the specified mode
            if (blank)
                this->screen_buffer[1 + column + page*this->oled_width] &= ~col_mask;
            else
                this->screen_buffer[1 + column + page*this->oled_width] |= col_mask;
        }
    }
}