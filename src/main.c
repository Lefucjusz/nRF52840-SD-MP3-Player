#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/fs/fs.h>
#include <ff.h>
#include <gui.h>
#include <dir.h>
#include <player.h>

#define SD_MOUNT_POINT "/SD:"

#define GUI_THREAD_STACK_SIZE (1024 * 2)
#define GUI_THREAD_PRIORITY 10
#define GUI_THREAD_START_DELAY_MS 500

#define PLAYER_THREAD_STACK_SIZE (1024 * 6)
#define PLAYER_THREAD_PRIORITY 9

static FATFS fs;
static struct fs_mount_t mp = {
	.fs_data = &fs,
	.type = FS_FATFS,
	.mnt_point = SD_MOUNT_POINT
};

LOG_MODULE_REGISTER(main);

K_THREAD_DEFINE(player_thread, PLAYER_THREAD_STACK_SIZE, player_task, NULL, NULL, NULL, PLAYER_THREAD_PRIORITY, 0, 0);
K_THREAD_DEFINE(gui_thread, GUI_THREAD_STACK_SIZE, gui_task, NULL, NULL, NULL, GUI_THREAD_PRIORITY, 0, GUI_THREAD_START_DELAY_MS);

int main(void)
{
	const int err = fs_mount(&mp);
	if (err) {
		LOG_ERR("Failed to mount FS, error %d", err);
		return err;
	}
	
	dir_init(SD_MOUNT_POINT);

	LOG_INF("Disk mounted!");

	return 0;
}
