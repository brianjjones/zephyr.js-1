// Copyright (c) 2017, Intel Corporation.

// C includes
#include <stdlib.h>
#include <string.h>

#ifndef ZJS_LINUX_BUILD
// Zephyr includes
#include <misc/util.h>
#include <spi.h>
#include <zephyr.h>
#endif

// ZJS includes
#include "zjs_buffer.h"
#include "zjs_callbacks.h"
#include "zjs_error.h"
#include "zjs_spi.h"
#include "zjs_util.h"

#define SPI_BUS "SPI_"
#define MAX_SPI_BUS 127
#define MAX_READ_BUFF 1000
#define MAX_DIR_LEN 13

enum spi_topology {
    ZJS_TOPOLOGY_FULL_DUPLEX,
    ZJS_TOPOLOGY_READ,
    ZJS_TOPOLOGY_WRITE,
    ZJS_TOPOLOGY_MULTIPLEXED,
    ZJS_TOPOLOGY_DAISY_CHAIN
};

typedef struct spi_handle {
    struct spi_config spi_config;
    struct spi_cs_control spi_cs;
    enum spi_topology topology;
    bool closed;
} spi_handle_t;

static jerry_value_t zjs_spi_prototype;
/*
struct spi_config spi_config;// = {
    //.frequency = 128000,
   // .operation = SPI_OP_MODE_MASTER | SPI_MODE_CPOL |
   // SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_LINES_SINGLE,
   // .slave = 1
    //.cs = SPI_CS,
};

struct spi_cs_control spi_cs = {
    .gpio_pin = 0,
    .delay = 0
};
*/
static void zjs_spi_callback_free(void *native)
{
    // requires: handle is the native pointer we registered with
    //             jerry_set_object_native_handle
    //  effects: frees the spi item
    spi_handle_t *handle = (spi_handle_t *)native;
    handle->closed = true;
    zjs_free(handle);
}

static const jerry_object_native_info_t spi_type_info = {
   .free_cb = zjs_spi_callback_free
};

static void get_gpio_dev(u32_t pin_val) {    
    char devname[20];
    int pin;
    int rval = zjs_board_find_pin(pin_val, devname, &pin);

    if (rval == FIND_PIN_INVALID) {
        return TYPE_ERROR("bad pin argument");
    }
    spi_cs.gpio_dev = device_get_binding(devname);
    if (!spi_cs.pio_dev || rval == FIND_PIN_FAILURE) {
        return zjs_error("SPI unable to find CS pin");
    }        
}

static ZJS_DECL_FUNC(zjs_spi_transceive)
{
    // requires: Writes and reads from the SPI bus, takes one to three arguments
    //           arg[0] - The targeted slave device
    //           arg[1] - Buffer to write to the SPI bus
    //           arg[2] - Direction of the data flow
    //  effects: Writes the buffer to the SPI bus and if one is given,
    //           returns the buffer received on the SPI in reply.  Otherwise
    //           returns NULL.

    ZJS_VALIDATE_ARGS(Z_OPTIONAL Z_ARRAY Z_STRING Z_BUFFER,
                      Z_OPTIONAL Z_STRING);

    ZJS_GET_HANDLE(this, spi_handle_t, handle, spi_type_info);

    if (handle->closed == true) {
        ZJS_PRINT("SPI bus is closed\n");
        return jerry_create_null();
    }

    size_t len;
    jerry_value_t buffer;
    zjs_buffer_t *tx_buf = NULL;
    zjs_buffer_t *rx_buf = NULL;
    jerry_value_t rx_buf_obj = jerry_create_null();
    jerry_value_t tx_buf_obj;
    // BJONES u32_t slave_num = (u32_t)jerry_get_number_value(argv[0]);
    // Valid numbers are 0 thru 127
    //if (slave_num > 127) {
      //  return zjs_error("SPI slave invalid\n");
   // }

    // If only a slave is given, this must be a single read or its invalid
    if (argc == 0) {
        if (handle->topology != ZJS_TOPOLOGY_READ) {
            return ZJS_STD_ERROR(RangeError, "Missing transmit buffer");
        }
    }

    jerry_size_t dir_len = 13;
    char dir_str[dir_len];
    // Set the direction default based on the topology.
    enum spi_topology dir_arg = handle->topology;

    // If we have a 'direction' arg, get it and validate
    if (argc >= 3) {
        zjs_copy_jstring(argv[2], dir_str, &dir_len);

        if (strncmp(dir_str, "read-write", 11) == 0)
            dir_arg = ZJS_TOPOLOGY_FULL_DUPLEX;
        else if (strncmp(dir_str, "read", 5) == 0)
            dir_arg = ZJS_TOPOLOGY_READ;
        else if (strncmp(dir_str, "write", 6) == 0)
            dir_arg = ZJS_TOPOLOGY_WRITE;
        else
            return ZJS_STD_ERROR(Error, "Invalid direction");

        // If topology conflicts with direction given
        if ((handle->topology == ZJS_TOPOLOGY_WRITE &&
             dir_arg != ZJS_TOPOLOGY_WRITE) ||
            (handle->topology == ZJS_TOPOLOGY_READ &&
             dir_arg != ZJS_TOPOLOGY_READ)) {
            return ZJS_STD_ERROR(NotSupportedError,
                                 "Direction conflicts with topology");
        }
        // If reading only, the 2nd arg should be NULL
        if (dir_arg == ZJS_TOPOLOGY_READ && !jerry_value_is_null(argv[1])) {
            return ZJS_STD_ERROR(
                NotSupportedError,
                "Buffer should be NULL when direction is read");
        }

        if (dir_arg != ZJS_TOPOLOGY_READ && jerry_value_is_null(argv[1])) {
            return ZJS_STD_ERROR(NotSupportedError, "Write buffer is NULL");
        }
    }

    // If we need to write a buffer
    if (dir_arg != ZJS_TOPOLOGY_READ) {
        buffer = argv[1];
        // Figure out if the buffer is an array or string, handle accordingly
        if (jerry_value_is_array(argv[1])) {
            len = jerry_get_array_length(buffer);
            tx_buf_obj = zjs_buffer_create(len, &tx_buf);
            if (tx_buf) {
                for (int i = 0; i < len; i++) {
                    ZVAL item = jerry_get_property_by_index(buffer, i);
                    if (jerry_value_is_number(item)) {
                        tx_buf->buffer[i] = (u8_t)jerry_get_number_value(item);
                    } else {
                        ERR_PRINT(
                            "non-numeric value in array, treating as 0\n");
                        tx_buf->buffer[i] = 0;
                    }
                }
            }
        } else if (jerry_value_is_string(argv[1])) {
            tx_buf_obj = zjs_buffer_create(jerry_get_string_size(buffer),
                                           &tx_buf);
            // zjs_copy_jstring adds a null terminator, which we don't want
            // so make a new string instead and remove it.
            char *tmpBuf = zjs_alloc_from_jstring(argv[1], NULL);
            strncpy(tx_buf->buffer, tmpBuf, tx_buf->bufsize);
            zjs_free(tmpBuf);
        } else {
            // If we were passed a buffer just use it as is
            tx_buf = zjs_buffer_find(argv[1]);
            tx_buf_obj = buffer;
        }
        // If this is a read / write
        if (dir_arg == ZJS_TOPOLOGY_FULL_DUPLEX) {
            rx_buf_obj = zjs_buffer_create(tx_buf->bufsize, &rx_buf);
            // Send the data and read from the device
            if (spi_transceive(handle->spi_config, tx_buf->buffer,
                               tx_buf->bufsize, rx_buf->buffer,
                               rx_buf->bufsize) != 0) {
                jerry_release_value(rx_buf_obj);
                return ZJS_STD_ERROR(SystemError, "SPI transceive failed");
            }
        }
        // This is a write only operation, return a NULL buffer
        else {
            if (spi_write(handle->spi_config, tx_buf->buffer,
                          tx_buf->bufsize) != 0) {
                return ZJS_STD_ERROR(SystemError, "SPI transceive failed");
            }
            rx_buf_obj = jerry_create_null();
        }
    }  // This is a read only operation
    else {
        rx_buf_obj = zjs_buffer_create(MAX_READ_BUFF, &rx_buf);
        // Read the data from the device
        if (spi_read(handle->spi_config,
                     rx_buf->buffer, rx_buf->bufsize) != 0) {
            jerry_release_value(rx_buf_obj);
            return ZJS_STD_ERROR(SystemError, "SPI transceive failed");
        }
    }
    return rx_buf_obj;
}

static ZJS_DECL_FUNC(zjs_spi_close)
{
    // requires: Closes SPI
    //  effects: Bus is closed

    ZJS_GET_HANDLE(this, spi_handle_t, handle, spi_type_info);
    handle->closed = true;
    return jerry_create_boolean(true);
}

static ZJS_DECL_FUNC(zjs_spi_open)
{
    // requires: This is a SPI object from zjs_spi_open, it has one optional
    //             args
    //           arg[0] - config object
    //  effects: Creates the SPI object

    ZJS_VALIDATE_ARGS(Z_OPTIONAL Z_OBJECT);

    // Default values


    u32_t bus = 0;
    u32_t slave = 1;
    u32_t cs = 0;
    double speed = 128000;
    bool msbFirst = true;
    u32_t bits = 8;
    u32_t polarity = 0;
    u32_t phase = 0;
    char topology_str[13] = "";
    char bus_str[9];
    enum spi_topology topology = ZJS_TOPOLOGY_FULL_DUPLEX;
    u32_t frame_gap = 0;
    //struct spi_config config = { 0 };

    // Get any provided optional args
    if (argc >= 1) {
        zjs_obj_get_uint32(argv[0], "bus", &bus);
        if (bus < MAX_SPI_BUS) {
            snprintf(bus_str, 9, "%s%u", SPI_BUS, bus);
        } else
            return ZJS_STD_ERROR(RangeError, "Invalid bus");

        // Bus speed in MHz
        zjs_obj_get_double(argv[0], "speed", &speed);

        // Most significant bit sent first
        zjs_obj_get_boolean(argv[0], "msbFirst", &msbFirst);

        // Number of data bits, valid options are 1, 2, 4, 8, or 16
        zjs_obj_get_uint32(argv[0], "bits", &bits);
        if (bits > 16 || (bits != 1 && bits % 2 != 0))
            return ZJS_STD_ERROR(RangeError, "Invalid bits");

        // Polarity value, valid options are 0 or 2
        zjs_obj_get_uint32(argv[0], "polarity", &polarity);
        if (polarity != 0 && polarity != 2)
            return ZJS_STD_ERROR(TypeError, "Invalid polarity");

        // Clock phase value, valid options are 0 or 1
        zjs_obj_get_uint32(argv[0], "phase", &phase);
        if (phase != 0 && phase != 2)
            return ZJS_STD_ERROR(TypeError, "Invalid phase");

        zjs_obj_get_uint32(argv[0], "slave", &slave);
        zjs_obj_get_uint32(argv[0], "cs", &cs);
        
        // Connection type
        zjs_obj_get_string(argv[0], "topology", topology_str, 13);
        if (strncmp(topology_str, "full-duplex", 12) == 0)
            topology = ZJS_TOPOLOGY_FULL_DUPLEX;
        else if (strncmp(topology_str, "read", 12) == 0)
            topology = ZJS_TOPOLOGY_READ;
        else if (strncmp(topology_str, "write", 12) == 0)
            topology = ZJS_TOPOLOGY_WRITE;
        else if (strncmp(topology_str, "multiplexed", 11) == 0)
            topology = ZJS_TOPOLOGY_MULTIPLEXED;
        else if (strncmp(topology_str, "daisy-chain", 11) == 0)
            topology = ZJS_TOPOLOGY_DAISY_CHAIN;
        else if (strncmp(topology_str, "", 2) != 0)
            return ZJS_STD_ERROR(RangeError, "Invalid topology");

        zjs_obj_get_uint32(argv[0], "frame_gap", &frame_gap);
    }

    handle->spi_config->operation |= SPI_WORD_SET(bits);

    if (msbFirst)
        handle->spi_config->operation |= SPI_TRANSFER_MSB;
    else
        handle->spi_config->operation |= SPI_TRANSFER_LSB;

    if (speed > 0)
        handle->spi_config->frequency = speed;

    // Note: The mode is determined by adding the polarity and phase bits
    // together, this is why polarity is either 0 or 2
    if (polarity == 2)
        handle->spi_config->operation |= SPI_MODE_CPOL;

    if (phase == 1)
        handle->spi_config->operation |= SPI_MODE_CPHA;

    handle->spi_config->slave = slave;

    handle->spi_config->dev = device_get_binding(bus_str);    
    
    if (!handle->spi_config.dev) {
        return zjs_error("Could not find SPI driver\n");
    }

    // Get the GPIO CS pin if provided
    get_gpio_dev(cs);
    
   /* if (spi_configure(spi_device, &spi_config)) {
        return zjs_error("SPI configuration failed\n");
    }*/

    // Create the SPI object
    jerry_value_t spi_obj = jerry_create_object();
    jerry_set_prototype(spi_obj, zjs_spi_prototype);

    spi_handle_t *handle = zjs_malloc(sizeof(spi_handle_t));    
    handle->closed = false;
    handle->topology = topology;
    jerry_set_object_native_pointer(spi_obj, handle, &spi_type_info);
    return spi_obj;
}

jerry_value_t zjs_spi_init()
{
    // Create SPI pin prototype object
    zjs_native_func_t array[] = {
        { zjs_spi_transceive, "transceive" },
        { zjs_spi_close, "close" },
        { NULL, NULL }
    };
    zjs_spi_prototype = jerry_create_object();
    zjs_obj_add_functions(zjs_spi_prototype, array);

    // Create SPI object
    jerry_value_t spi_obj = jerry_create_object();
    zjs_obj_add_function(spi_obj, zjs_spi_open, "open");

    return spi_obj;
}

void zjs_spi_cleanup()
{
    jerry_release_value(zjs_spi_prototype);
}
