#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <gui.h>
#include <dir.h>

#define SD_MOUNT_POINT "/SD:"

LOG_MODULE_REGISTER(main);

void up(void)
{
	
}

void down(void)
{
	
}

void left(void)
{

}

void right(void)
{

}

void enter(void)
{
	
}

int main(void)
{
	dir_init(SD_MOUNT_POINT);
	gui_init();

	while (1) {
		gui_task();
		k_msleep(20);
	}

	return 0;
}
