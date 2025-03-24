#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/fs/fs.h>
#include <ff.h>
#include <decoder_task.h>
#include <utils.h>

static FATFS fs;
static struct fs_mount_t mp = {
	.fs_data = &fs,
	.type = FS_FATFS
};

static const char *const mount_point = "/SD:";

LOG_MODULE_REGISTER(main);

K_THREAD_DEFINE(audio_thread, 1024 * 6, decoder_task, NULL, NULL, NULL, 9, 0, 0);

static void for_each_file(const char *dir_path, const char *extension, void (*callback)(const char *path))
{
	int err;
	struct fs_dir_t dirp;
	struct fs_dirent entry;

	char path_buf[256];

	fs_dir_t_init(&dirp);

	err = fs_opendir(&dirp, dir_path);
	if (err) {
		LOG_ERR("Failed to open directory!");
		return;
	}

	while (1) {
		err = fs_readdir(&dirp, &entry);

		/* End of directories */
		if (err || entry.name[0] == '\0') {
			break;
		}

		/* Skip directories */
		if (entry.type == FS_DIR_ENTRY_DIR) {
			continue;
		}

		if (!is_extension(entry.name, extension)) {
			continue;
		}

		if (callback != NULL) {
			snprintf(path_buf, sizeof(path_buf), "%s/%s", dir_path, entry.name);
			callback(path_buf);
		}
	}

	fs_closedir(&dirp);
}

static void play_callback(const char *path)
{
	LOG_INF("File: %s", path);
	
	struct k_msgq *q = decoder_get_queue();

	decoder_req_t req;
	strlcpy(req.path, path, sizeof(req.path));
	req.state = PLAY;

	k_msgq_put(q, &req, K_FOREVER);

	k_msleep(1000);

	while (decoder_get_state() == PLAY) {
		k_msleep(500);
	}
}

int main(void)
{
	int err;

	mp.mnt_point = mount_point;

	err = fs_mount(&mp);
	if (err) {
		LOG_ERR("Failed to mount FS, error %d", err);
		return 0;
	}

	LOG_INF("Disk mounted!");

	for_each_file("/SD:/test", ".mp3", play_callback);

	fs_unmount(&mp);

	return 0;
}
