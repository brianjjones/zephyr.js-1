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
    struct device *spi_device;
    enum spi_topology topology;
    bool closed;
} spi_handle_t;

static jerry_value_t zjs_spi_prototype;

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

static ZJS_DECL_FUNC(zjs_spi_transceive)
{
    // requires: Writes and reads from the SPI bus, takes one to three arguments
    //           arg[0] - The targeted slave device
    //           arg[1] - Buffer to write to the SPI bus
    //           arg[2] - Direction of the data flow
    //  effects: Writes the buffer to the SPI bus and if one is given,
    //           returns the buffer received on the SPI in reply.  Otherwise
    //           returns NULL.

    ZJS_VALIDATE_ARGS(Z_NUMBER, Z_OPTIONAL Z_ARRAY Z_STRING Z_BUFFER,
                      Z_OPTIONAL Z_STRING);

    //zjs_print_mem_stats();
    ZJS_GET_HANDLE(this, spi_handle_t, handle, spi_type_info);

    //BJONES TEST
    //int arr[100] = { [0 ... 99] = 66 };
    //ZJS_PRINT("BJONES checking array %i\n", arr[40]);


    if (handle->closed == true) {
        ZJS_PRINT("SPI bus is closed\n");
        return jerry_create_null();
    }
    //ZJS_PRINT("BJONES %s : %i\n", __func__, __LINE__);
    size_t len;
    jerry_value_t buffer;
    zjs_buffer_t *tx_buf = NULL;
    zjs_buffer_t *rx_buf = NULL;
    jerry_value_t rx_buf_obj = jerry_create_null();
    jerry_value_t tx_buf_obj;

  //  if (spi_slave_select(handle->spi_device, jerry_get_number_value(argv[0]))) {
  //      return zjs_error("SPI slave select failed\n");
  //  }
    // If only a slave is given, this must be a single read or its invalid
    if (argc == 1) {
        if (handle->topology != ZJS_TOPOLOGY_READ) {
            return ZJS_STD_ERROR(RangeError, "Missing transmit buffer");
        }
    }

    //ZJS_PRINT("BJONES %s : %i\n", __func__, __LINE__);
    jerry_size_t dir_len = 13;
    char dir_str[dir_len];
    // Set the direction default based on the topology.
    enum spi_topology dir_arg = handle->topology;

    // If we have a 'direction' arg, get it and validate
    if (argc >= 3) {
        //ZJS_PRINT("BJONES %s : %i\n", __func__, __LINE__);
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
    //ZJS_PRINT("BJONES %s : %i\n", __func__, __LINE__);
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
            //ZJS_PRINT("BJONES %s : %i\n", __func__, __LINE__);
            if (spi_transceive(handle->spi_device, tx_buf->buffer,
                               tx_buf->bufsize, rx_buf->buffer,
                               rx_buf->bufsize) != 0) {
                //ZJS_PRINT("BJONES %s : %i\n", __func__, __LINE__);
                jerry_release_value(rx_buf_obj);
                jerry_release_value(tx_buf_obj);
                return ZJS_STD_ERROR(SystemError, "SPI transceive failed");
            }
            jerry_release_value(tx_buf_obj);
            //ZJS_PRINT("BJONES %s : %i\n", __func__, __LINE__);
        }
        // This is a write only operation, return a NULL buffer
        else {
          /*  ZJS_PRINT("BJONES %s : %i\n", __func__, __LINE__);
            for (int i = 0; i < tx_buf->bufsize; i++)
                ZJS_PRINT("%i:",tx_buf->buffer[i]);
            
            ZJS_PRINT("\n");*/
            if (spi_write(handle->spi_device, tx_buf->buffer,
                          tx_buf->bufsize) != 0) {
                jerry_release_value(tx_buf_obj);
                return ZJS_STD_ERROR(SystemError, "SPI transceive failed");
            }
            //ZJS_PRINT("BJONES %s : %i\n", __func__, __LINE__);
            jerry_release_value(tx_buf_obj);
            rx_buf_obj = jerry_create_null();
        }
    }  // This is a read only operation
    else {
        //ZJS_PRINT("BJONES %s : %i\n", __func__, __LINE__);
        rx_buf_obj = zjs_buffer_create(MAX_READ_BUFF, &rx_buf);
        // Read the data from the device
        if (spi_read(handle->spi_device,
                     rx_buf->buffer, rx_buf->bufsize) != 0) {
            jerry_release_value(rx_buf_obj);
            return ZJS_STD_ERROR(SystemError, "SPI transceive failed");
        }
        //ZJS_PRINT("BJONES %s : %i\n", __func__, __LINE__);
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


static ZJS_DECL_FUNC(zjs_wait)
{
    u32_t ms = (u32_t)jerry_get_number_value(argv[0]);
    //ZJS_PRINT("Sleeping for %i ms\n", ms);
   // if (ms > 100)
     //   ms -= 100;
    k_sleep(ms);
    //ZJS_PRINT("Done sleeping\n");
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
    double speed = 10;
    bool msbFirst = true;
    u32_t bits = 8;
    u32_t polarity = 0;
    u32_t phase = 0;
    char topology_str[13] = "";
    char bus_str[9];
    enum spi_topology topology = ZJS_TOPOLOGY_FULL_DUPLEX;
    u32_t frame_gap = 0;
    struct spi_config config = { 0 };

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
        if (phase != 0 && phase != 1)
            return ZJS_STD_ERROR(TypeError, "Invalid phase");

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

    config.config |= SPI_WORD(bits);

    if (msbFirst)
        config.config |= SPI_TRANSFER_MSB;
    else
        config.config |= SPI_TRANSFER_LSB;

    if (speed > 0)
        config.max_sys_freq = speed;

    // Note: The mode is determined by adding the polarity and phase bits
    // together, this is why polarity is either 0 or 2
    if (polarity == 2)
        config.config |= SPI_MODE_CPOL;

    if (phase == 1)
        config.config |= SPI_MODE_CPHA;

    struct device *spi_device = device_get_binding(bus_str);

    if (!spi_device) {
        return zjs_error("Could not find SPI driver\n");
    }

    if (spi_configure(spi_device, &config)) {
        return zjs_error("SPI configuration failed\n");
    }

    // Create the SPI object
    jerry_value_t spi_obj = jerry_create_object();
    jerry_set_prototype(spi_obj, zjs_spi_prototype);

    spi_handle_t *handle = zjs_malloc(sizeof(spi_handle_t));
    handle->spi_device = spi_device;
    handle->closed = false;
    handle->topology = topology;
    jerry_set_object_native_pointer(spi_obj, handle, &spi_type_info);
    if (spi_slave_select(handle->spi_device, 1)) {
        return zjs_error("SPI slave select failed\n");
    }
    return spi_obj;
}

jerry_value_t zjs_spi_init()
{
    // Create SPI pin prototype object
    zjs_native_func_t array[] = {
        { zjs_spi_transceive, "transceive" },
        { zjs_spi_close, "close" },
        { zjs_wait, "ZJSwait" },
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
