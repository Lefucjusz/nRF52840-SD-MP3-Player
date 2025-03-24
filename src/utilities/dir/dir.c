/*
 * dir.c
 *
 *  Created on: Sep 28, 2023
 *      Author: lefucjusz
 */

#include "dir.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/syslimits.h>
#include <zephyr/fs/fs.h>
#include <strnatcmp.h>

typedef struct
{
	char path[PATH_MAX];
	size_t depth;
} dir_ctx_t;

static dir_ctx_t ctx;

static int path_append(const char *name)
{
	const size_t cur_path_len = strlen(ctx.path);
	const size_t space_left = sizeof(ctx.path) - cur_path_len;
	if (strlen(name) >= space_left) {
		return -ENAMETOOLONG;
	}

	snprintf(ctx.path + cur_path_len, space_left, "/%s", name);

	return 0;
}

static int path_remove(void)
{
	if (ctx.depth == 0) {
		return -ENOENT;
	}

	char *last_slash = strrchr(ctx.path, '/');
	*last_slash = '\0';

	return 0;
}

static bool compare_ascending(const void *val1, const void *val2)
{
	const struct fs_dirent *fno1 = val1;
	const struct fs_dirent *fno2 = val2;

	return (strnatcmp(fno1->name, fno2->name) > 0);
}

void dir_init(const char *root_path)
{
	strncpy(ctx.path, root_path, sizeof(ctx.path));
	ctx.depth = 0;
}

int dir_enter(const char *name)
{
	const int ret = path_append(name);
	if (ret) {
		return ret;
	}
	--ctx.depth;

	return 0;
}

int dir_return(void)
{
	const int ret = path_remove();
	if (ret) {
		return ret;
	}
	++ctx.depth;

	return 0;
}

const char *dir_get_fs_path(void)
{
	return ctx.path;
}

dir_list_t *dir_list(void)
{
	int err;
	struct fs_dir_t dirp;
	struct fs_dirent entry;

	fs_dir_t_init(&dirp);
	err = fs_opendir(&dirp, ctx.path);
	if (err) {
		return NULL;
	}

	dir_list_t *list = list_create();

	while (1) {
		err = fs_readdir(&dirp, &entry);
		if ((err != 0) || (entry.name[0] == '\0')) {
			break;
		}

		list_add(list, &entry, sizeof(entry), LIST_PREPEND);
	}

	/* Sort alphabetically ascending */
	list_sort(list, compare_ascending);

	fs_closedir(&dirp);

	return list;
}

dir_entry_t *dir_get_prev(dir_list_t *list, dir_entry_t *current)
{
	if ((list == NULL) || (current == NULL)) {
		return NULL;
	}

	dir_entry_t *entry = list->head;
	do {
		if (entry->data == current->data) {
			break;
		}
		entry = entry->next;

	} while (entry != NULL);

	if ((entry == NULL) || (entry->prev == NULL)) {
		return list->tail;
	}
	return entry->prev;
}

dir_entry_t *dir_get_next(dir_list_t *list, dir_entry_t *current)
{
	if ((list == NULL) || (current == NULL)) {
		return NULL;
	}

	dir_entry_t *entry = list->head;
	do {
		if (entry->data == current->data) {
			break;
		}
		entry = entry->next;

	} while (entry != NULL);

	if ((entry == NULL) || (entry->next == NULL)) {
		return list->head;
	}
	return entry->next;
}

void dir_list_free(dir_list_t *list)
{
	list_destroy(list);
}
