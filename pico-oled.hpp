#include "pico/stdlib.h"
#include "gfx_font.h"


// SSD1306 commands
#define OLED_SET_CONTRAST _u(0x81)
#define OLED_SET_ENTIRE_ON _u(0xA4)
#define OLED_SET_NORM_INV _u(0xA6)
#define OLED_SET_DISP _u(0xAE)
#define OLED_SET_MEM_ADDR _u(0x20)
#define OLED_SET_COL_ADDR _u(0x21)
#define OLED_SET_PAGE_ADDR _u(0x22)
#define OLED_SET_DISP_START_LINE _u(0x40)
#define OLED_SET_SEG_REMAP _u(0xA0)
#define OLED_SET_MUX_RATIO _u(0xA8)         
#define OLED_SET_COM_OUT_DIR _u(0xC0)
#define OLED_SET_DISP_OFFSET _u(0xD3)
#define OLED_SET_COM_PIN_CFG _u(0xDA)
#define OLED_SET_DISP_CLK_DIV _u(0xD5)
#define OLED_SET_PRECHARGE _u(0xD9)
#define OLED_SET_VCOM_DESEL _u(0xDB)
#define OLED_SET_CHARGE_PUMP _u(0x8D)
#define OLED_SET_HORIZ_SCROLL _u(0x26)
#define OLED_SET_SCROLL _u(0x2E)

#define OLED_WRITE_MODE _u(0xFE)
#define OLED_READ_MODE _u(0xFF)
#define OLED_PAGE_HEIGHT _u(8)

// Display parameters
//#define OLED_NUM_PAGES OLED_HEIGHT / OLED_PAGE_HEIGHT
//#define OLED_BUF_LEN (OLED_NUM_PAGES * OLED_WIDTH)


class pico_oled
{
    private:
        uint8_t i2c_addr;
        uint8_t oled_height, oled_width;
        uint8_t *screen_buffer;
        int screen_buf_length;
        gfx_font font;
        uint8_t font_set;
        uint8_t cursor_x;
        uint8_t cursor_y;
        void *draw_pixel_fn(uint8_t, uint8_t);

    public:
        pico_oled(uint8_t i2c_address, uint8_t screen_width, uint8_t screen_height);    
        void oled_init();
        void oled_send_cmd(uint8_t cmd);
        void fill(uint8_t fill);
        void all_on(uint8_t disp_on);   
        void render();
        void blit_screen(const uint8_t *src_bitmap, uint16_t src_width, uint16_t src_x, uint8_t src_y, uint8_t blit_width, uint8_t blit_height, uint8_t screen_x, uint8_t screen_y);

        void draw_bmp(const uint8_t *src_bitmap, uint8_t src_width, uint8_t src_height, uint8_t screen_x, uint8_t screen_y)
        {
            blit_screen(src_bitmap, src_width, 0, 0, src_width, src_height, screen_x, screen_y);
        }

        void set_cursor(uint8_t cursor_x, uint8_t cursor_y)
        {
            this->cursor_x = cursor_x;
            this->cursor_y = cursor_y;
        }
   
        void set_font(gfx_font font);
        void draw_char(uint8_t char_c, uint8_t x_pos, uint8_t y_pos);
        void print(const char *print_str);
        void print_num(const char *format_str, int32_t print_data);
        void print_num(const char *format_str, uint32_t print_data);
        void print_num(const char *format_str, float print_data);
        void print_num(const char *format_str, uint16_t print_data);
        void print_num(const char *format_str, int16_t print_data);
        void print_num(const char *format_str, uint8_t print_data);
        void print_num(const char *format_str, int8_t print_data);        
        void draw_pixel(uint8_t x, uint8_t y);
        void draw_pixel_alternating(uint8_t x, uint8_t y);
        void draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
        void draw_line_dotted(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
        void draw_fast_hline(uint8_t x1, uint8_t x2, uint8_t y);
        void draw_fast_vline(uint8_t y1, uint8_t y2, uint8_t x);  
        void draw_vbar(uint8_t fullness, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1); 
        void draw_hbar(uint8_t fullness, uint8_t start_right, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);   
        void draw_bmp_vbar(uint8_t fullness, const uint8_t *empty_bitmap, const uint8_t *full_bitmap, uint8_t x, uint8_t y);
        void fill_rect(uint8_t blank, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);

        uint8_t pixel_counter;
};

typedef struct
{
    const uint8_t *bitmap;
    uint16_t width;
    uint16_t height;
} bitmap;
