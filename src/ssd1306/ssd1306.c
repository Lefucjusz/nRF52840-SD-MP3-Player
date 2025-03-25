#include "ssd1306.h"
#include <stdlib.h>
#include <string.h>
#include <zephyr/drivers/display.h>

#define SSD1306_PIXELS_PER_BYTE 8

typedef struct
{
    const struct device *display;
    struct display_buffer_descriptor image_buffer_desc;
    uint8_t *image_buffer;
    uint16_t current_x;
    uint16_t current_y;
} ssd1306_ctx_t;

static ssd1306_ctx_t ctx;

int ssd1306_init(void)
{
    ctx.display = DEVICE_DT_GET(DT_ALIAS(oled));
    if (!device_is_ready(ctx.display)) {
        return -ENODEV;
    }

    display_blanking_off(ctx.display);

    /* Get display resolution */
    struct display_capabilities capabilities;
    display_get_capabilities(ctx.display, &capabilities);

    /* Configure image buffer */
    ctx.image_buffer_desc.width = capabilities.x_resolution;
    ctx.image_buffer_desc.height = capabilities.y_resolution;
    ctx.image_buffer_desc.pitch = capabilities.x_resolution;
    ctx.image_buffer_desc.buf_size = ctx.image_buffer_desc.width * ctx.image_buffer_desc.height / SSD1306_PIXELS_PER_BYTE;
    ctx.image_buffer = malloc(ctx.image_buffer_desc.buf_size);
    if (ctx.image_buffer == NULL) {
        return -ENOMEM;
    }

    /* Configure display */
    ssd1306_set_contrast(0xFF); // Maximum contrast
    ssd1306_fill(SSD1306_BLACK); // Clear display
    ssd1306_update_screen();
    ssd1306_set_cursor(0, 0);

    return 0;
}

void ssd1306_deinit(void)
{
    free(ctx.image_buffer);
}

void ssd1306_set_cursor(uint16_t x, uint16_t y)
{
    ctx.current_x = x;
    ctx.current_y = y;
}

void ssd1306_set_contrast(uint8_t value)
{
    display_set_contrast(ctx.display, value);
}

void ssd1306_update_screen(void)
{
    display_write(ctx.display, 0, 0, &ctx.image_buffer_desc, ctx.image_buffer);
}

void ssd1306_fill(ssd1306_color_t color)
{
    memset(ctx.image_buffer, (color == SSD1306_BLACK) ? 0x00 : 0xFF, ctx.image_buffer_desc.buf_size);
}

void ssd1306_draw_pixel(uint16_t x, uint16_t y, ssd1306_color_t color)
{
    /* Don't write outside the buffer */
    if ((x >= ctx.image_buffer_desc.width) || (y >= ctx.image_buffer_desc.height)) {
        return;
    }
   
    /* Draw pixel */
    if (color == SSD1306_WHITE) {
        ctx.image_buffer[x + (y / 8) * ctx.image_buffer_desc.width] |= 1 << (y % 8);
    } 
    else { 
        ctx.image_buffer[x + (y / 8) * ctx.image_buffer_desc.width] &= ~(1 << (y % 8));
    }
}

char ssd1306_write_char(char ch, ssd1306_font_t font, ssd1306_color_t color)
{
    char ch_to_show = ch;
    
    /* Check if character is printable */
    if ((ch < 32) || (ch > 128)) {
    	ch_to_show = '?'; // Display unknown chars as '?'
    }
    
    /* Check remaining space in current line */
    if (((ctx.current_x + font.width) > ctx.image_buffer_desc.width) || ((ctx.current_y + font.height) > ctx.image_buffer_desc.height)) {
        return 0;
    }
    
    /* Write char to buffer */
    for (size_t i = 0; i < font.height; ++i) {
        const uint16_t row_data = font.data[(ch_to_show - 32) * font.height + i];
        for (size_t j = 0; j < font.width; ++j) {
            if ((row_data << j) & 0x8000)  {
                ssd1306_draw_pixel(ctx.current_x + j, (ctx.current_y + i), color);
            } 
            else {
                ssd1306_draw_pixel(ctx.current_x + j, (ctx.current_y + i), !color);
            }
        }
    }
    
    /* Update current x position */
    ctx.current_x += font.char_width ? font.char_width[ch_to_show - 32] : font.width;
    
    /* Return written char for validation */
    return ch;
}

char ssd1306_write_string(const char* str, ssd1306_font_t font, ssd1306_color_t color)
{
    while (*str != '\0') {
        if (ssd1306_write_char(*str, font, color) != *str) {
            return *str;
        }
        ++str;
    }

    return *str;
}
