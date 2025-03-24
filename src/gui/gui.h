/*
 * gui.h
 *
 *  Created on: Sep 28, 2023
 *      Author: lefucjusz
 */

#pragma once

#include <stdbool.h>

#define GUI_SCROLL_DELAY_MS 350

#define GUI_VOLUME_MIN 0
#define GUI_VOLUME_MAX 100 // %
#define GUI_VOLUME_STEP 2
#define GUI_VOLUME_DEFAULT 50

int gui_init(void);
void gui_deinit(void);

void gui_task(void);
