#pragma once

#include "decoder_interface.h"

const struct decoder_interface_t *decoder_get_interface(const char *filename);
