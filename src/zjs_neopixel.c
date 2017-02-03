// Copyright (c) 2016, Intel Corporation.
#ifndef QEMU_BUILD

// Zephyr includes
#include <string.h>

// ZJS includes
#include "zjs_neopixel.h"
#include "zjs_util.h"
#include "zjs_buffer.h"

static jerry_value_t zjs_neopixel_prototype;

static jerry_value_t zjs_neopixel_open(const jerry_value_t function_obj,
                                  const jerry_value_t this,
                                  const jerry_value_t argv[],
                                  const jerry_length_t argc)
{
    // requires: Requires two arguments
    //           arg[0] - I2C bus number you want to open a connection to.
    //           arg[1] - Bus speed in kbps.
    //  effects: Creates a I2C object connected to the bus number specified.

    if (argc < 1 || !jerry_value_is_object(argv[0])) {
        return zjs_error("zjs_neopixel_open: invalid argument");
    }

    jerry_value_t data = argv[0];
    uint32_t bus;
    uint32_t speed;

    if (!zjs_obj_get_uint32(data, "bus", &bus)) {
        return zjs_error("zjs_neopixel_open: missing required field (bus)");
    }

    if (!zjs_obj_get_uint32(data, "speed", &speed)) {
        return zjs_error("zjs_neopixel_open: missing required field (speed)");
    }

    if (bus < MAX_I2C_BUS) {
        char neopixel_bus[6];
        snprintf(neopixel_bus, 6, "I2C_%i", (uint8_t)bus);
        neopixel_device[bus] = device_get_binding(neopixel_bus);

        if (!neopixel_device[bus]) {
            return zjs_error("I2C bus not found");
        }

        /* TODO remove these hard coded numbers
         * once the config API is made */
        union dev_config cfg;
        cfg.raw = 0;
        cfg.bits.use_10_bit_addr = 0;
        cfg.bits.speed = I2C_SPEED_STANDARD;
        cfg.bits.is_master_device = 1;

        int reply = neopixel_configure(neopixel_device[bus], cfg.raw);

        if (reply < 0) {
            ERR_PRINT("I2C bus %s configure failed with error: %i\n", neopixel_bus,
                      reply);
        }

    } else {
        ERR_PRINT("I2C bus %i is not a valid I2C bus\n", (uint8_t)bus);
        return zjs_error("I2C bus not found");
    }

    // create the I2C object
    jerry_value_t neopixel_obj = jerry_create_object();
    jerry_set_prototype(neopixel_obj, zjs_neopixel_prototype);

    zjs_obj_add_readonly_number(neopixel_obj, bus, "bus");
    zjs_obj_add_readonly_number(neopixel_obj, speed, "speed");

    return neopixel_obj;
}

jerry_value_t zjs_neopixel_init()
{
    k_sem_init(&neopixel_sem, 0, 1);

    zjs_native_func_t array[] = {
        { zjs_neopixel_read, "read" },
        { zjs_neopixel_burst_read, "burstRead" },
        { zjs_neopixel_write, "write" },
        { zjs_neopixel_abort, "abort" },
        { zjs_neopixel_close, "close" },
        { NULL, NULL }
    };
    zjs_neopixel_prototype = jerry_create_object();
    zjs_obj_add_functions(zjs_neopixel_prototype, array);

    // create global I2C object
    jerry_value_t neopixel_obj = jerry_create_object();
    zjs_obj_add_function(neopixel_obj, zjs_neopixel_open, "open");
    return neopixel_obj;
}

void zjs_neopixel_cleanup()
{
    jerry_release_value(zjs_neopixel_prototype);
}

#endif // QEMU_BUILD
