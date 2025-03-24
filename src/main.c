#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <ssd1306.h>
#include <ssd1306_fonts.h>

LOG_MODULE_REGISTER(main);

int main(void)
{
	ssd1306_Init();

	ssd1306_WriteString("Siema", Font_6x8, White);
	ssd1306_UpdateScreen();

	return 0;
}
