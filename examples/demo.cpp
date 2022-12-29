#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include "../pico-oled.hpp"
#include "../gfx_font.h"
#include "../font/press_start_2p.h"
#include "../font/too_simple.h"

#include "bitmap/raspberry.h"

#define DISPLAY_I2C_ADDR _u(0x3C) //_u(0x3C)
#define DISPLAY_WIDTH _u(128)
#define DISPLAY_HEIGHT _u(64)


int main()
{
    // Init i2c and configure it's GPIO pins
    // i2c_init(i2c_default, 400 * 1000);   // Standard i2c clock (400kHz)
    i2c_init(i2c_default, 1000 * 1000);     // Fast clock (1000 kHz), some SSD1306 devices may work up to 1100-1200 kHz
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

    // Init display with SSD1309 driver IC and a reset pin
    pico_oled display(OLED_SSD1309, DISPLAY_I2C_ADDR, DISPLAY_WIDTH, DISPLAY_HEIGHT, /*reset_gpio=*/ 15);   

    // Init display with SSD1306 driver IC
    // pico_oled display(OLED_SSD1306, DISPLAY_I2C_ADDR, DISPLAY_WIDTH, DISPLAY_HEIGHT);       

    display.oled_init();
    display.set_font(press_start_2p);

    uint8_t x, y, x_dir, y_dir;
    uint8_t min_y = 9;
    uint16_t i;

    display.fill(0);    // Clear display    
    display.set_cursor(0,0);             
    display.print("Pico-OLED\nLibrary Demo");
    display.render();
    sleep_ms(2000);    

    while (1)
    {
        // Text demo
        display.fill(0);    // Clear display     
        display.set_cursor(0,0);           
        display.print("Text demo\n\n");
        display.print("Font:\n2P Press Start");
        display.render();
        sleep_ms(2000);       


        display.fill(0);    // Clear display     
        display.set_cursor(0,0);           
        display.print("Text demo\n\n");
        display.set_font(too_simple);
        display.print("Font:\nToo Simple");
        display.render();
        sleep_ms(2000);  


        display.fill(0);    // Clear display     
        display.set_cursor(0,0);           
        display.set_font(press_start_2p);
        display.print("Text demo\n\n");
        display.print_num("Float print:\n%.2f\n\n", 1.2345f);
        display.print_num("Int print:\n%d", (uint8_t) 123);   // When printing variables, the cast is not needed

        display.render();
        sleep_ms(2000);


        // Bitmap demo
        x = 0;
        y = min_y;
        x_dir = 1;
        y_dir = 1;

        for(i = 0; i < 500; i++)
        {
            display.fill(0);    // Clear display    
            display.set_cursor(0,0);    
            display.print("Bitmaps");     

            display.draw_bmp(raspberry.bitmap, raspberry.width, raspberry.height, x, y);
            display.render();

            // Move x,y point
            if (x_dir)
            {
                if (++x >= DISPLAY_WIDTH - raspberry.width - 1)
                    x_dir = 0;
            }
            else
            {
                if (--x == 0)
                    x_dir = 1;
            }

            if (y_dir)
            {
                if (++y >= DISPLAY_HEIGHT - raspberry.height - 1)
                    y_dir = 0;
            }
            else
            {
                if (--y == min_y)
                    y_dir = 1;
            }            

        }    


        // Line demo
        x = DISPLAY_WIDTH / 2;
        y = DISPLAY_HEIGHT / 2;
        x_dir = 1;
        y_dir = 0;

        // Draw solid lines
        for(i = 0; i < 400; i++)
        {
            display.fill(0);    // Clear display    
            display.set_cursor(0,0);    
            display.print("Solid lines");

            // Draw lines from corners of active area to x,y coordinate
            display.draw_line(0, min_y, x, y);
            display.draw_line(0, DISPLAY_HEIGHT - 1, x, y);
            display.draw_line(DISPLAY_WIDTH - 1, min_y, x, y);
            display.draw_line(DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1, x, y);

            display.render();

            // Move x,y point
            if (x_dir)
            {
                if (++x >= DISPLAY_WIDTH - 2)
                    x_dir = 0;
            }
            else
            {
                if (--x == 0)
                    x_dir = 1;
            }

            if (y_dir)
            {
                if (++y >= DISPLAY_HEIGHT - 2)
                    y_dir = 0;
            }
            else
            {
                if (--y == min_y)
                    y_dir = 1;
            }     

            sleep_ms(10);
        }


        // Draw dotted lines
        for(i = 0; i < 400; i++)
        {
            display.fill(0);    // Clear display        
            display.set_cursor(0,0);        
            display.print("Dotted lines");

            // Draw lines from corners of active area to x,y coordinate
            display.draw_line_dotted(0, min_y, x, y);
            display.draw_line_dotted(0, DISPLAY_HEIGHT - 1, x, y);
            display.draw_line_dotted(DISPLAY_WIDTH - 1, min_y, x, y);
            display.draw_line_dotted(DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1, x, y);

            display.render();

            // Move x,y point
            if (x_dir)
            {
                if (++x >= DISPLAY_WIDTH - 2)
                    x_dir = 0;
            }
            else
            {
                if (--x == 0)
                    x_dir = 1;
            }

            if (y_dir)
            {
                if (++y >= DISPLAY_HEIGHT - 2)
                    y_dir = 0;
            }
            else
            {
                if (--y == min_y)
                    y_dir = 1;
            }     

            sleep_ms(10);
        }    



        // Rectangle demo
        uint8_t box_w, box_h;
        uint8_t blank = 0;

        display.fill(0);    // Clear display     
        display.set_cursor(0,0);   
        display.print("Filled Rects");   


        for(i = 0; i < 100; i++)
        {
            box_w = rand() % (DISPLAY_WIDTH / 2);
            box_h = rand() % (DISPLAY_HEIGHT / 2);
            x = rand() % (DISPLAY_WIDTH - box_w);
            y = min_y + rand() % (DISPLAY_HEIGHT - min_y - box_h);        

            display.fill_rect(blank, x, y, x + box_w, y + box_h);

            blank = !blank;
            display.render();

            sleep_ms(100);
        }


        // Box demo
        display.fill(0);    // Clear display     
        display.set_cursor(0,0);   
        display.print("Boxes");   


        for(i = 0; i < 50; i++)
        {
            box_w = rand() % (DISPLAY_WIDTH / 2);
            box_h = rand() % (DISPLAY_HEIGHT / 2);
            x = rand() % (DISPLAY_WIDTH - box_w);
            y = min_y + rand() % (DISPLAY_HEIGHT - min_y - box_h);        

            display.draw_box(x, y, x + box_w, y + box_h);

            display.render();

            sleep_ms(100);
        }

        
        // Bar graph demo
        uint8_t count_down, fullness;

        for (i = 0; i < 5; i++)     
        {
            count_down = 0;
            for (fullness = 0; !count_down && fullness != 0; count_down ? fullness-- : fullness++)
            {
                display.fill(0);    // Clear display     
                display.set_cursor(0,0);   
                display.print("Bar Graphs");                

                display.draw_vbar(fullness, 0, min_y, 9, DISPLAY_HEIGHT - 1);
                display.draw_hbar(fullness, false, 20, min_y, DISPLAY_WIDTH - 20, min_y + 10);
                display.draw_hbar(fullness, true, 20, min_y + 12, DISPLAY_WIDTH - 20, min_y + 22);
                //display.draw_bmp_vbar(fullness,);

                display.render();

                // Flip direction when the top is reached
                if (fullness == 100)
                    count_down = 1;
            }
        }
        sleep_ms(2000);


    }

}