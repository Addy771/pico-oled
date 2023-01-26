#include "pico-oled.hpp"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include <stdlib.h>
#include <stdio.h>
#include "pico/float.h"
#include "gfx_font.h"


//#define GFX_DEBUG

#define PRINT_NUM_BUFFER 30     // Length of temporary buffers for printing numbers


pico_oled::pico_oled(OLED_type controller_ic, uint8_t i2c_address, uint8_t screen_width, uint8_t screen_height, uint8_t reset_gpio)
{
    // Init private variables
    i2c_addr = i2c_address;
    oled_width = screen_width;
    oled_height = screen_height;

    // Allocate a buffer for the entire screen + control byte
    screen_buf_length = (oled_height / OLED_PAGE_HEIGHT) * oled_width + 1;
    screen_buffer = (uint8_t*) malloc(screen_buf_length);

    // Control byte, Co = 0, D/C = 1 => the driver expects data to be written to RAM
    screen_buffer[0] = 0x40;  

    // Indicate that font has not been set yet
    font_set = 0;

    // Set cursor default position to something reasonable
    cursor_x = 0;
    cursor_y = 10;

    // Set the draw pixel function to the default
    draw_pixel_fn = &pico_oled::draw_pixel;
    pixel_counter = 0;

    // Store the controller ID
    oled_controller = controller_ic;

    // Store the reset GPIO 
    rst_gpio = reset_gpio;

    // Initialize reset pin and drive it low if a valid gpio pin was given
    if (rst_gpio <= 29)
    {    
        gpio_init(rst_gpio);
        gpio_set_dir(rst_gpio, GPIO_OUT);
        gpio_put(rst_gpio, 0);    // hold display in reset    
    }
}



void pico_oled::oled_send_cmd(uint8_t cmd)
{
    // I2C write process expects a control byte followed by data
    // this "data" can be a command or data to follow up a command

    // Co = 1, D/C = 0 => the driver expects a command
    uint8_t buf[2] = {0x80, cmd};
    i2c_write_blocking(i2c_default, (i2c_addr & OLED_WRITE_MODE), buf, 2, false);   
}


// Write to the display's configuration registers to set it up
void pico_oled::oled_init()
{
    // Only use the reset signaling if a valid GPIO pin is selected
    if (rst_gpio <= 29)
    {
        sleep_ms(100);      // Wait some time in reset to allow display to stabilize
        gpio_put(rst_gpio, 1);    // take display out of reset
        sleep_ms(10);        
    }

    // Run the appropriate init function
    switch (oled_controller)
    {
        case OLED_SSD1309:
            oled_ssd1309_init();
            break;

        case OLED_SSD1306:
        default:
            oled_ssd1306_init();
    }
}

// Set configuration registers for ssd1306 OLED controllers
void pico_oled::oled_ssd1306_init()
{
    oled_send_cmd(OLED_SET_DISP | 0x00); // set display off

    /* memory mapping */
    oled_send_cmd(OLED_SET_MEM_ADDR); // set memory address mode
    oled_send_cmd(0x00); // horizontal addressing mode

    /* resolution and layout */
    oled_send_cmd(OLED_SET_DISP_START_LINE); // set display start line to 0

    oled_send_cmd(OLED_SET_SEG_REMAP | 0x01); // set segment re-map
    // column address 127 is mapped to SEG0

    oled_send_cmd(OLED_SET_MUX_RATIO); // set multiplex ratio
    oled_send_cmd(oled_height - 1); // set OLED vertical resolution

    oled_send_cmd(OLED_SET_COM_OUT_DIR | 0x08); // set COM (common) output scan direction
    // scan from bottom up, COM[N-1] to COM0

    oled_send_cmd(OLED_SET_DISP_OFFSET); // set display offset
    oled_send_cmd(0x00); // no offset

    oled_send_cmd(OLED_SET_COM_PIN_CFG); // set COM (common) pins hardware configuration
    oled_send_cmd(0x12); // 0x12 for alternative COM pin configuration

    /* timing and driving scheme */
    oled_send_cmd(OLED_SET_DISP_CLK_DIV); // set display clock divide ratio 
    oled_send_cmd(0x80); // div ratio of 1, standard freq

    oled_send_cmd(OLED_SET_PRECHARGE); // set pre-charge period
    oled_send_cmd(0xF1); // Vcc internally generated on our board

    oled_send_cmd(OLED_SET_VCOM_DESEL); // set VCOMH deselect level
    oled_send_cmd(0x30); // 0.83xVcc  

    /* display */
    set_brightness(0x0F); // 0 to 255

    oled_send_cmd(OLED_SET_ENTIRE_ON); // set entire display on to follow RAM content

    oled_send_cmd(OLED_SET_NORM_INV); // set normal (not inverted) display

    oled_send_cmd(OLED_SET_CHARGE_PUMP); // set charge pump
    oled_send_cmd(0x14); // Vcc internally generated on our board

    oled_send_cmd(OLED_SET_SCROLL | 0x00); // deactivate horizontal scrolling if set
    // this is necessary as memory writes will corrupt if scrolling was enabled

    // Clear the display
    fill(0);
    render();    

    oled_send_cmd(OLED_SET_DISP | 0x01); // turn display on
}


// Set configuration registers for ssd1309 OLED controllers
void pico_oled::oled_ssd1309_init()
{
    oled_send_cmd(OLED_SET_DISP | 0x00); // set display off

    /* memory mapping */
    oled_send_cmd(OLED_SET_MEM_ADDR); // set memory address mode
    oled_send_cmd(0x00); // horizontal addressing mode 

    /* resolution and layout */
    oled_send_cmd(OLED_SET_DISP_START_LINE); // set display start line to 0

    oled_send_cmd(OLED_SET_SEG_REMAP | 0x01); // set segment re-map ssd1306
    // column address 127 is mapped to SEG0

    oled_send_cmd(OLED_SET_COM_OUT_DIR | 0x08); // set COM (common) output scan direction
    // scan from bottom up, COM[N-1] to COM0

    oled_send_cmd(OLED_SET_MUX_RATIO); // set multiplex ratio
    oled_send_cmd(oled_height - 1); // set OLED vertical resolution

    oled_send_cmd(OLED_SET_DISP_OFFSET); // set display offset
    oled_send_cmd(0x00); // no offset

    oled_send_cmd(OLED_SET_COM_PIN_CFG); // set COM (common) pins hardware configuration
    oled_send_cmd(0x12); // 0x12 for alternative COM pin configuration

    /* timing and driving scheme */
    oled_send_cmd(OLED_SET_DISP_CLK_DIV); // set display clock divide ratio 
    oled_send_cmd(0xa0); // div ratio of 1, osc freq 0xA
    // oled_send_cmd(0x70); // div ratio of 1, default freq

    oled_send_cmd(OLED_SET_PRECHARGE); // set pre-charge period
    //oled_send_cmd(0xF1); // ssd1309
    oled_send_cmd(0xd3); // ssd1309

    oled_send_cmd(OLED_SET_VCOM_DESEL); // set VCOMH deselect level
    oled_send_cmd(0x30);  // 0.83xVcc 

    /* display */
    set_brightness(0x0F); // 0 to 255

    oled_send_cmd(OLED_SET_ENTIRE_ON); // set entire display on to follow RAM content

    oled_send_cmd(OLED_SET_NORM_INV); // set normal (not inverted) display

    oled_send_cmd(OLED_SET_CHARGE_PUMP); // set charge pump
    oled_send_cmd(0x10);  // Disabled, external charge pump for ssd1309

    oled_send_cmd(OLED_SET_SCROLL | 0x00); // deactivate horizontal scrolling if set
    // this is necessary as memory writes will corrupt if scrolling was enabled

    // Clear the display
    fill(0);
    render();    

    oled_send_cmd(OLED_SET_DISP | 0x01); // turn display on    
}


/// @brief Set the contrast (brightness) of the OLED. 
/// @warning high brightness levels can cause burn-in and reduce the lifespan of the display
/// @param brightness 0 to 255, where 0 is the lowest brightness possible.
void pico_oled::set_brightness(uint8_t brightness)
{
    oled_send_cmd(OLED_SET_CONTRAST); 
    oled_send_cmd(brightness);  
}


// Fill entire display with the specified byte
void pico_oled::fill(uint8_t fill)
{
    // Skip first byte since it's a control byte
    for (int i=1; i < screen_buf_length; i++)
    {
        screen_buffer[i] = fill;
    }
}


// Write the entire screen buffer to the OLED 
void pico_oled::render()
{
    // ////// debug stack check
    // uint8_t prev_x = cursor_x;
    // uint8_t prev_y = cursor_y;

    // set_cursor(0, oled_height - font.line_height);
    // print_num("%8X", (uint32_t) &prev_y);

    // set_cursor(prev_x, prev_y);


    // Set the start/end coordinates for drawing the entire screen
    oled_send_cmd(OLED_SET_COL_ADDR);
    oled_send_cmd(0);                       // Start column
    oled_send_cmd(oled_width - 1);    // End column

    oled_send_cmd(OLED_SET_PAGE_ADDR);
    oled_send_cmd(0);                                           // Start page
    oled_send_cmd(oled_height / OLED_PAGE_HEIGHT - 1);    // End page

    i2c_write_blocking(i2c_default, (i2c_addr & OLED_WRITE_MODE), screen_buffer, screen_buf_length, false);
}


// Tell the display whether to turn on all pixels or follow RAM contents
void pico_oled::all_on(uint8_t disp_on)
{
    if (disp_on)
        oled_send_cmd(0xA5);
    else
        oled_send_cmd(0xA4);
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
    if (dest_end_page > (oled_height / OLED_PAGE_HEIGHT))  
    {
        dest_end_page = (oled_height / OLED_PAGE_HEIGHT);
    }

    if (dest_end_col > oled_width - 1)
    {
        dest_end_col = oled_width - 1;
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
        // render();
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
                    screen_buffer[1 + column + screen_page*oled_width] |= source_data >> (OLED_PAGE_HEIGHT - offset_delta);
                }
                else
                {
                    // Mask off top and bottom
                    source_data &= 0xFF << l_mask; 
                    source_data &= 0xFF >> u_mask;

                    // top pixels being shifted down
                    screen_buffer[1 + column + screen_page*oled_width] |= source_data << offset_delta;
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
                    screen_buffer[1 + column + screen_page*oled_width] |= source_data << (OLED_PAGE_HEIGHT + offset_delta); // page height - offset
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
                    screen_buffer[1 + column + screen_page*oled_width] |= source_data >> (-1 * offset_delta);
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
void pico_oled::set_font(gfx_font new_font)
{
    font = new_font;
    font_set = 1;
}


// Draw a single character at the specified position. Font must be previously set.
void pico_oled::draw_char(uint8_t char_c, uint8_t x_pos, uint8_t y_pos)
{
    // Abort if the char is not within the drawable range
    // if (char_c > font.last || char_c < font.first)
    //     return;

#ifdef GFX_DEBUG            
    printf("Character: %c, src_x: %d\n", char_c, font.character[char_c].bitmap_x);
#endif
    
    char_c -= font.first;     // First character is element 0 of table
    
    blit_screen(font.bitmap, font.src_width, font.character[char_c].bitmap_x, 0, font.character[char_c].width, font.character[char_c].height, x_pos + font.character[char_c].x_offset, y_pos + font.character[char_c].y_offset);
}


// Print a basic string. Only basic character drawing is supported, control characters have no effect
void pico_oled::print(const char *print_str)
{
    uint8_t start_x = cursor_x;

    // Abort if no font has been set yet
    if (!font_set)
        return;

    while (*print_str != '\0')
    {
        // Draw character if it's valid
        if (*print_str <= font.last && *print_str >= font.first)
        {
            // If character would be drawn over the edge of the screen
            if (font.character[*print_str - font.first].width + cursor_x >= oled_width)
            {      
                // Reposition cursor at the left edge of the screen, one line down
                cursor_x = 0;
                cursor_y += font.line_height;
            }
            
            draw_char(*print_str, cursor_x, cursor_y);

            cursor_x += font.character[*print_str - font.first].x_advance;
        }
        else if (*print_str == '\n')
        {
            // Reposition cursor one line down, at the x coordinate where printing started
            cursor_x = start_x;
            cursor_y += font.line_height;            
        }
        else if (*print_str == '\r')
        {
            // Reposition cursor back to the left edge of the screen
            cursor_x = 0;
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
    print(output_buf);
}


void pico_oled::print_num(const char *format_str, uint32_t print_data)
{
    // Allocate a buffer big enough to hold the printed data
    char output_buf[PRINT_NUM_BUFFER];
    sprintf(output_buf, format_str, print_data);
    print(output_buf);
}


void pico_oled::print_num(const char *format_str, float print_data)
{
    // Allocate a buffer big enough to hold the printed data
    char output_buf[PRINT_NUM_BUFFER];
    sprintf(output_buf, format_str, print_data);
    print(output_buf);
}

void pico_oled::print_num(const char *format_str, uint8_t print_data)
{
    print_num(format_str, (uint32_t) print_data);
}

void pico_oled::print_num(const char *format_str, int8_t print_data)
{
    print_num(format_str, (int32_t) print_data);
}

void pico_oled::print_num(const char *format_str, uint16_t print_data)
{
    print_num(format_str, (uint32_t) print_data);
}

void pico_oled::print_num(const char *format_str, int16_t print_data)
{
    print_num(format_str, (int32_t) print_data);
}


// Draw a single pixel in the screen buffer
void pico_oled::draw_pixel(uint8_t x, uint8_t y)
{
    // Abort if coordinates are out of bounds
    if (x >= oled_width || y >= oled_height)
        return;

    uint8_t screen_page = y / OLED_PAGE_HEIGHT;

    // Write to the column where the target pixel is
    screen_buffer[1 + x + screen_page*oled_width] |= 1 << (y - screen_page*OLED_PAGE_HEIGHT);
}


// Draw a single pixel every other time this function is called
void pico_oled::draw_pixel_alternating(uint8_t x, uint8_t y)
{
    if (pixel_counter++ > 0)
    {
        pixel_counter = 0;
        draw_pixel(x, y);
    }   
}


// Draw a line with Bresenham's algorithm
void pico_oled::draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
    // Only try to use fast line functions for the default solid line draw_pixel()
    if (draw_pixel_fn == &pico_oled::draw_pixel)
    {
        // Draw straight lines with the fast function instead
        if (y1 == y2)
        {
            draw_fast_hline(x1, x2, y1);
            return;
        }

        if (x1 == x2)
        {
            draw_fast_vline(y1, y2, x1);
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
            (*this.*draw_pixel_fn)(y1, x1);

        else
            (*this.*draw_pixel_fn)(x1, y1);

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
    pixel_counter = 0;    // Reset pixel counter so that calling this function results in the same kind of line draw each time
    draw_pixel_fn = &pico_oled::draw_pixel_alternating;  // Change the draw pixel function so it only draws every other pixel

    draw_line(x1, y1, x2, y2);

    draw_pixel_fn = &pico_oled::draw_pixel;    // Restore the default draw_pixel function
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
    if (x1 >= oled_width  || y >= oled_height)
        return;    

    if (x2 >= oled_width)
        x2 = oled_width - 1;

    uint8_t screen_page = y / OLED_PAGE_HEIGHT;
    uint8_t mask = 1 << (y - screen_page*OLED_PAGE_HEIGHT);

    for (; x1 <= x2; x1++)
    {
        // Write to the column where the target pixel is
        screen_buffer[1 + x1 + screen_page*oled_width] |= mask;    
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
    if (y1 >= oled_height  || x >= oled_width)
        return;    

    if (y2 >= oled_width)
        y2 = oled_width - 1;

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
        screen_buffer[1 + x + page*oled_width] |= mask; 
    }

}


// Draw a vertical progress bar 
// 
void pico_oled::draw_vbar(uint8_t fullness, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    uint8_t height = y1 - y0;
    uint8_t filled_px = (fullness * (height - 2)) / 100;

    // Draw the outline
    draw_box(x0, y0, x1, y1); 

    // Draw lines to fill the internal area
    for (uint8_t y_line = y1 - 1; y_line >= (y1 - 1) - filled_px; y_line--)
        draw_fast_hline(x0 + 1, x1 - 1, y_line);
}


// Draw a horizontal progress bar 
// start_right: Greater than 0 for bar to start on the right instead of the left side
void pico_oled::draw_hbar(uint8_t fullness, uint8_t start_right, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    uint8_t width = x1 - x0;
    uint8_t filled_px = (fullness * (width - 2)) / 100;

    // Draw the outline
    draw_box(x0, y0, x1, y1);

    // Draw lines to fill the internal area
    if (start_right)
    {
        for (uint8_t x_line = x1 - 1; x_line >= (x1 - 1) - filled_px; x_line--)
            draw_fast_vline(y0 + 1, y1 - 1, x_line);
    }
    else
    {
        for (uint8_t x_line = x0 + 1; x_line <= (x0 + 1) + filled_px; x_line++)
            draw_fast_vline(y0 + 1, y1 - 1, x_line);
    }
}


/* Draw a vertical, bitmapped progress bar.
 * empty_bitmap: bitmap of the bar (empty frame) when it is 0% full
 * full_bitmap: bitmap of the bar (with or without frame) when it is 100% full.
*/
void pico_oled::draw_bmp_vbar(uint8_t fullness, const bitmap empty_bitmap, const bitmap full_bitmap, uint8_t x, uint8_t y)
{
    uint8_t filled_px = (fullness * empty_bitmap.height) / 100;   // This assumes the active area is the whole bar bmp, including frame

    // Draw the empty frame 
    blit_screen(empty_bitmap.bitmap, empty_bitmap.width, 0, 0, empty_bitmap.width, empty_bitmap.height, x, y);

    // Draw as much of the full bitmap that would be proportional to fullness (from the bottom)
    blit_screen(full_bitmap.bitmap, full_bitmap.width, 0, full_bitmap.height - filled_px, full_bitmap.width, filled_px, x, y + (full_bitmap.height - filled_px));

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
                screen_buffer[1 + column + page*oled_width] &= ~col_mask;
            else
                screen_buffer[1 + column + page*oled_width] |= col_mask;
        }
    }
}


// Draw a box outline at the given coordinates
void pico_oled::draw_box(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    draw_fast_hline(x0, x1, y0);  // Top 
    draw_fast_hline(x0, x1, y1);  // Bottom
    draw_fast_vline(y0, y1, x0);  // Left
    draw_fast_vline(y0, y1, x1);  // Right       
}


// x = r*cos(th)
// y = r*sin(th)
// th = atan(y/x)
// r = sqrt(x^2 + y^2)


void pico_oled::draw_line_polar(uint8_t origin_x, uint8_t origin_y, uint8_t magnitude, float angle)
{
    float end_x, end_y;

    // Convert degrees to radians
    angle = (angle / 360.0) * 2.0 * M_PI;

    // Convert polar to rect
    sincosf(angle, &end_y, &end_x);
    end_x = end_x * magnitude + origin_x;
    end_y = end_y * magnitude + origin_y;

    draw_line(origin_x, origin_y, end_x, end_y);
}




void pico_oled::get_str_dimensions(const char *input_str, uint8_t *width, uint8_t *height)
{
    uint8_t max_width = 0;
    uint8_t line_width = 0;
    char* current_char = (char *)input_str; 

    *height = font.line_height;

    while (*current_char != '\0')
    {
        if (*current_char != '\n')
        {

            line_width += font.character[*current_char - font.first].x_advance; 
        }
        else
        {
            // Character is newline, height is increased one line's worth
            *height += font.line_height;

            // Save current line's width if it's longest
            if (line_width > max_width)
                max_width = line_width;

            line_width = 0;
        }
        current_char++;
    }

    if (line_width > max_width)
        *width = line_width;
    else
        *width = max_width;

}


/// @brief 
/// @param print_str 
/// @param padding 
/// @param fill_bg 
/// @param x 
/// @param y 
void pico_oled::draw_boxed_text(const char *print_str, uint8_t padding, uint8_t fill_bg, uint8_t x, uint8_t y)
{
    uint8_t text_width, text_height;
    uint8_t tmp_cursor_x = cursor_x;
    uint8_t tmp_cursor_y = cursor_y;
    uint8_t box_x2;
    uint8_t box_y2;
    
    get_str_dimensions(print_str, &text_width, &text_height);

    box_x2 = 2 + x + text_width + 2*padding;
    box_y2 = 2 + y + text_height + 2*padding;

    // Clear the area under the textbox if desired
    if (fill_bg)
        fill_rect(1, x, y, box_x2, box_y2);

    draw_box(x, y, box_x2, box_y2);

    set_cursor(x + padding + 2, y + padding + 1);
    print(print_str);

    // Restore original cursor position
    set_cursor(tmp_cursor_x, tmp_cursor_y);
}



/// @brief An object which handles the configuration and drawing of an analog gauge
/// @param display Address of pico_oled display instance to use for drawing
analog_gauge::analog_gauge(pico_oled *display)
{
    master_display = display;

    // Set variables to reasonable defaults
    set_position(63, 63);
    set_scale(0, 100, 220, 320);
    set_markers(3, 45, 15, 1);

    // _origin_x = 63;
    // _origin_y = 63;
    // _scale_min = 0;
    // _scale_max = 100;
    // _scale_start_deg = 220;
    // _scale_end_deg = 320;
    // _scale_divisions = 3;
    // _needle_len = 45;
    // _marker_len = 15;
    // _half_divisions = 1;

}


void analog_gauge::draw_maj_div_line(float div_angle)
{
    float x1, y1, x2, y2;
    float x_ratio, y_ratio;

    // Convert degrees to radians
    div_angle = (div_angle / 360.0) * 2.0 * M_PI;

    // Determine line start and end points
    sincosf(div_angle, &y_ratio, &x_ratio);
    x1 = (_needle_len - _marker_len) * x_ratio + _origin_x;
    y1 = (_needle_len - _marker_len) * y_ratio + _origin_y;
    x2 = _marker_len * x_ratio + x1;
    y2 = _marker_len * y_ratio + y1;

    master_display->draw_line(x1, y1, x2, y2);

    // master_display->print_num("%.0f, ", x1);
    // master_display->print_num("%.0f,   ", y1);
    // master_display->print_num("%.0f, ", x2);
    // master_display->print_num("%.0f\n", y2);
}


// Draw the gauge using the parameters provided within the class
void analog_gauge::draw()
{
    // //dbg
    // master_display->fill(0);
    // master_display->set_cursor(0,0);
    // master_display->print_num("x=%d, ", _origin_x);
    // master_display->print_num("y=%d, ", _origin_y);
    // master_display->print_num("s_min=%.0f, ", _scale_min);
    // master_display->print_num("s_max=%.0f, ", _scale_max);    
    // master_display->print_num("s_start=%.0f, ", _scale_start_deg);    
    // master_display->print_num("s_end=%.0f, ", _scale_end_deg);    
    // master_display->print_num("s_divs=%d, ", _scale_divisions);    
    // master_display->print_num("needle_l=%d, ", _needle_len);
    // master_display->print_num("marker_l=%d, ", _marker_len);
    // master_display->print_num("half_div=%d ", _half_divisions);    
    // return;


    // Draw scale end lines
    draw_maj_div_line(_scale_start_deg);
    draw_maj_div_line(_scale_end_deg);

    // Draw scale division lines as necessary
    float deg_per_div = (_scale_end_deg - _scale_start_deg) / _scale_divisions;

    for (uint8_t marker_num = 1; marker_num < _scale_divisions; marker_num++)
    {
        draw_maj_div_line(deg_per_div * marker_num + _scale_start_deg);
    }

    // Draw half-division markers if necessary
    if (_half_divisions)
    {
        // Make the marker length shorter for these minor division lines
        uint8_t temp_marker_len = _marker_len;
        _marker_len /= 2;

        for (uint8_t marker_num = 0; marker_num < _scale_divisions; marker_num++)
        {
            draw_maj_div_line(deg_per_div * marker_num + _scale_start_deg + (deg_per_div/2));
        }        

        _marker_len = temp_marker_len;   // Restore original length value

    }

    // Draw needle line
    float needle_angle = (_scale_end_deg - _scale_start_deg) * (_needle_value - _scale_min) / (_scale_max - _scale_min);

    master_display->draw_line_polar(_origin_x, _origin_y, _needle_len, _scale_start_deg + needle_angle);
}


/// @brief Configure range of scale angles and values.
///        Angles are absolute, with 0 degrees pointing to the screen's right (--->)
/// @param scale_min Needle value when the gauge is at the minimum
/// @param scale_max Needle value when the gauge is at the maximum
/// @param scale_start_deg Angle to draw the start of the gauge 
/// @param scale_end_deg  Angle to draw the end of the gauge 
void analog_gauge::set_scale(float scale_min, float scale_max, float scale_start_deg, float scale_end_deg)
{
    // TODO: sanity checks?
    // TODO: determine whether scale goes from left-to-right or vice versa
    _scale_min = scale_min;
    _scale_max = scale_max;
    _scale_start_deg = scale_start_deg;
    _scale_end_deg = scale_end_deg;
}


/// @brief Configure or disable scale division markers
/// @param scale_divisions Number of marked segments of the gauge. Set to 1 or lower to only draw the gauge end markers
/// @param needle_len Length of the gauge needle. Overall size of the gauge is based on this.
/// @param marker_len Length of lines to mark major scale divisions. These are drawn from the outer radius of the gauge and should not be larger than needle_len
/// @param half_divisions Set to nonzero value to enable drawing minor division lines at the middle of each scale division
void analog_gauge::set_markers(uint8_t scale_divisions, uint8_t needle_len, uint8_t marker_len, uint8_t half_divisions)
{
    _scale_divisions = scale_divisions;
    _half_divisions = half_divisions;

    // Make sure marker length is within the valid range
    if (marker_len < needle_len)
        _marker_len = marker_len;
    else
        _marker_len = needle_len;

    _needle_len = needle_len;
}


/// @brief Configure the screen coordinates where the gauge needle starts from
/// @param origin_x 
/// @param origin_y 
void analog_gauge::set_position(int16_t origin_x, int16_t origin_y)
{
    _origin_x = origin_x;
    _origin_y = origin_y;
}


/// @brief Update the value that the gauge will point to
/// @param needle_value 
void analog_gauge::set_value(float needle_value)
{
    _needle_value = needle_value;
}