// Copyright (c) 2016, Intel Corporation.

#ifndef __zjs_neopixel_h__
#define __zjs_neopixel_h__

#include "jerry-api.h"

/**
 * Initialize the neopixel module, or reinitialize after cleanup
 *
 * @return I2C API object
 */
jerry_value_t zjs_neopixel_init();

/** Release resources held by the neopixel module */
void zjs_neopixel_cleanup();

#endif  // __zjs_neopixel_h__
