#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/fs/fs.h>
#include <ff.h>
#include <gui.h>
#include <dir.h>
#include <player.h>
#include <ssd1306.h>
#include <ssd1306_fonts.h>

#define SD_MOUNT_POINT "/SD:"

/* TODO 
 * - check WAV and FLAC
 */

static FATFS fs;
static struct fs_mount_t mp = {
	.fs_data = &fs,
	.type = FS_FATFS,
	.mnt_point = SD_MOUNT_POINT
};

LOG_MODULE_REGISTER(main);

int main(void)
{
	/* Initialize SSD1306 display */
	ssd1306_init();

	/* Mount FS */
	const int err = fs_mount(&mp);
	if (err) {
		ssd1306_write_string("SD card mount failed!", Font_6x8, SSD1306_WHITE);
		ssd1306_update_screen();
		return err;
	}
	
	/* Initialize dir */
	dir_init(SD_MOUNT_POINT);

	/* Start player thread */
	player_init();

	/* Start GUI thread */
	gui_init();

	return 0;
}
