// Copyright (c) 2017, Intel Corporation.

// This graphics library was inspired by the Adafruit GFX library
// https://github.com/adafruit/Adafruit-GFX-Library

// ZJS includes
#include "zjs_buffer.h"
#include "zjs_callbacks.h"
#include "zjs_error.h"
#include "zjs_gfx.h"
#include "zjs_util.h"
#include "zjs_gfx_font.h"

#define COLORBYTES 2

u16_t maxPixels = 128 * 160 * 3;

typedef struct gfx_handle {
    u16_t screenW;
    u16_t screenH;
    u8_t **pixels;
    // Touched pixels values will hold a rectangle containing the buffer to draw
    u16_t tpX0;     // Touched pixels x start
    u16_t tpX1;     // Touched pixels x end
    u16_t tpY0;     // Touched pixels y start
    u16_t tpY1;     // Touched pixels y end
    jerry_value_t screenInitCB;
    jerry_value_t drawDataCB;
    jerry_value_t jsThis;
} gfx_handle_t;

typedef struct gfx_data {
    u32_t coords[4];
    u8_t color[COLORBYTES];
    char *text;
    jerry_size_t textSize;
    u32_t size;
} gfx_data_t;

static jerry_value_t zjs_gfx_prototype;

static void zjs_gfx_callback_free(void *native)
{
    // requires: handle is the native pointer we registered with
    //             jerry_set_object_native_handle
    //  effects: frees the gfx module

    gfx_handle_t *handle = (gfx_handle_t *)native;
    zjs_free(handle);
}

static const jerry_object_native_info_t gfx_type_info = {
   .free_cb = zjs_gfx_callback_free
};

// Extracts the args from jerry values and puts them into the data struct
static void args_to_data(gfx_data_t *data, u32_t argc, const jerry_value_t argv[])
{
    // Init the struct
    for (u8_t i = 0; i < 4; i++)
        data->coords[i] = 0;

    data->size = 1;
    data->textSize = 0;

    if (argc < 7) {
        for (u8_t i = 0; i < argc; i++) {
            if (jerry_value_is_number(argv[i])) {
                if (i < 4)
                    data->coords[i] = (u32_t)jerry_get_number_value(argv[i]);
                else
                    data->size = (u32_t)jerry_get_number_value(argv[i]);
            }
            else if (jerry_value_is_string(argv[i])) {
                data->text = zjs_alloc_from_jstring(argv[i], &data->textSize);
                if (!data->text) {
                    ERR_PRINT ("GFX failed to copy text");
                }
            }
            else if (jerry_value_is_array(argv[i])) {
                for (u8_t j = 0; j < COLORBYTES; j++) {
                   data->color[j] = (u8_t)jerry_get_number_value(jerry_get_property_by_index(argv[i], j));
               }
            }
        }
    }
}
/*
// Fills a buffer with the pixels to draw a solid rectangle and calls the provided JS callback to draw it on the screen
static jerry_value_t zjs_gfx_fill_buffer (u32_t x, u32_t y, u32_t w, u32_t h, u8_t color[], gfx_handle_t *gfxHandle)
{
    u32_t pixels = w * h * COLORBYTES; // Each pixel has several bytes of data
    u8_t passes = 1;    // Number of times the data buffer needs to be sent

    if (pixels > maxPixels) {
        pixels = maxPixels;
        passes = (pixels + (maxPixels -1)) / maxPixels;
    }

    zjs_buffer_t *recBuf = NULL;
    ZVAL recBufObj =  zjs_buffer_create(pixels, &recBuf);

    for (int i = 0; i < pixels; i++) {
        if (i % 2)
            recBuf->buffer[i] = color[1];
        else
            recBuf->buffer[i] = color[0];
    }

    jerry_value_t args[] = {jerry_create_number(x), jerry_create_number(y),
                            jerry_create_number(w), jerry_create_number(h), recBufObj};

    // Send the buffer as many times as needed to fill the rectangle
    for ( int i = 0; i < passes ; i++) {
        jerry_value_t ret = jerry_call_function(gfxHandle->drawDataCB, gfxHandle->jsThis, args, 5);
        if (jerry_value_has_error_flag (ret)) {
            return ret;
        }
    }
    return ZJS_UNDEFINED;
}
*/
/*

*/

static void zjs_gfx_mark_pixels(u32_t x, u32_t y, u32_t w, u32_t h, u8_t color[], gfx_handle_t *gfxHandle)
{   x = x > 0 ? x - 1 : 0;
    y = y > 0 ? y - 1 : 0;
    bool print = false;
    if (color[0] == 0x00 && color [1] == 0x00)
        print = true;
    if (print)
        ZJS_PRINT("VALUES for mark pixel = %u, %u, %u, %u\n", x,y,w,h);

    if (gfxHandle->tpX0 > x ) {
        gfxHandle->tpX0 = x;
        if (print)
            ZJS_PRINT("bjones set x0 %u\n", x );
    }
    if (gfxHandle->tpX1 < x + w) {
        gfxHandle->tpX1 = x + w;
        if (print)
        ZJS_PRINT("bjones set x1 %u\n", x + w);
    }
    if (gfxHandle->tpY0 > y) {
        gfxHandle->tpY0 = y;
        if (print)
        ZJS_PRINT("bjones set y0 %u\n", y);
    }
    if (gfxHandle->tpY1 < y + h) {
        gfxHandle->tpY1 = y + h;
        if (print)
        ZJS_PRINT("bjones set y1 %u\n", y + h );
    }
        if (print)
            ZJS_PRINT("bjones marked values = %u, %u, %u, %u\n", gfxHandle->tpX0, gfxHandle->tpX1, gfxHandle->tpY0,gfxHandle->tpY1);
    //int totalPixels = w * h;
    u16_t pixelsIndex = x + gfxHandle->screenW * y;

    for (u16_t iY = y; iY < y + h; iY++) {
        for (u16_t iX = 0; iX < x + w; iX++) {
            for (u8_t cbyte = 0; cbyte < COLORBYTES; cbyte++) {
                gfxHandle->pixels[pixelsIndex][cbyte] = color[cbyte];   //BJONES need to do this for COLORBYTES
            }
        }
    }
}

static void zjs_gfx_flush(gfx_handle_t *gfxHandle)
{

    if (gfxHandle->tpX1 - gfxHandle->tpX0 > 100)
        ZJS_PRINT("bjones in flush\n");

    u32_t tpW = gfxHandle->tpX1 - gfxHandle->tpX0;
    u32_t tpH = gfxHandle->tpY1 - gfxHandle->tpY0;
    u32_t pixels = tpW * tpH * COLORBYTES; // Each pixel has several bytes of data
    zjs_buffer_t *recBuf = NULL;
    ZVAL flushBufObj =  zjs_buffer_create(pixels, &recBuf);
    u16_t bufferIndex = 0;

    for (u16_t i = gfxHandle->tpX0; i < gfxHandle->tpX1; i++) {
        for (u16_t j = gfxHandle->tpY0; j < gfxHandle->tpY1; j++) {
            recBuf->buffer[bufferIndex] = gfxHandle->pixels[i + gfxHandle->screenW * j][0];      //BJONES NEED TO DO FOR COLORBYTES
            recBuf->buffer[bufferIndex + 1] =  gfxHandle->pixels[i + gfxHandle->screenW * j][1];     //BJONES could I do buffIndex++ each time?
            bufferIndex+=2;     //BJONES note I need to make sure I don't go too far and write outside memory
        }
    }
    jerry_value_t args[] = {jerry_create_number(gfxHandle->tpX0), jerry_create_number(gfxHandle->tpY0),
                        jerry_create_number(tpW),
                        jerry_create_number(tpH),
                        flushBufObj};

    jerry_call_function(gfxHandle->drawDataCB, gfxHandle->jsThis, args, 5); //BJONES  gfxHandle->pixels[i + gfxHandle->screenW * j][0];

    // Reset touched pixels
    gfxHandle->tpX0 = gfxHandle->screenW;
    gfxHandle->tpX1 = 0;
    gfxHandle->tpY0 = gfxHandle->screenH;
    gfxHandle->tpY1 = 0;
    //if (jerry_value_has_error_flag (ret)) {
    //    return ret;
    //}
}


// Fills a buffer with the pixels to draw a solid rectangle and calls the provided JS callback to draw it on the screen
static jerry_value_t zjs_gfx_fill_rect_priv (u32_t x, u32_t y, u32_t w, u32_t h, u8_t color[], gfx_handle_t *gfxHandle)
{
    /*
    u32_t pixels = w * h * COLORBYTES; // Each pixel has several bytes of data
    u8_t passes = 1;    // Number of times the data buffer needs to be sent

    if (pixels > maxPixels) {
        pixels = maxPixels;
        passes = (pixels + (maxPixels -1)) / maxPixels;
    }

    zjs_buffer_t *recBuf = NULL;
    ZVAL recBufObj =  zjs_buffer_create(pixels, &recBuf);

    for (int i = 0; i < pixels; i++) {
        if (i % 2)
            recBuf->buffer[i] = color[1];
        else
            recBuf->buffer[i] = color[0];
    }

    jerry_value_t args[] = {jerry_create_number(x), jerry_create_number(y),
                            jerry_create_number(w), jerry_create_number(h), recBufObj};

    // Send the buffer as many times as needed to fill the rectangle
    for ( int i = 0; i < passes ; i++) {
        jerry_value_t ret = jerry_call_function(gfxHandle->drawDataCB, gfxHandle->jsThis, args, 5);
        if (jerry_value_has_error_flag (ret)) {
            return ret;
        }
    }*/
    if (w > 100)
        ZJS_PRINT("bjones in rect_priv\n");
    zjs_gfx_mark_pixels(x, y, w, h, color, gfxHandle);
    zjs_gfx_flush(gfxHandle);
    return ZJS_UNDEFINED;
}

// Draws the rectangles needed to create the given char
static jerry_value_t zjs_gfx_draw_char_priv(u32_t x, u32_t y, char c, u8_t color[], u32_t size, gfx_handle_t *gfxHandle)
{
    u32_t asciiIndex = (u8_t)c - 33;    // To save size our font doesn't include the first 33 chars
    u8_t fontBytes = font_data_descriptors[asciiIndex][0] * 2;  // 2 bytes per pixel
    //u16_t bufferBytes = fontBytes * size * 2;
    //u8_t w = font_data_descriptors[asciiIndex][0] * size;
    //u8_t h = 16 * size;
    u16_t index = font_data_descriptors[asciiIndex][1];
    //jerry_value_t ret = ZJS_UNDEFINED;
    zjs_buffer_t *charBuf = NULL;
    //zjs_buffer_t *pixBuf = NULL;
    ZVAL charBufObj =  zjs_buffer_create(fontBytes, &charBuf);
    // pixelBufObj = zjs_buffer_create(bufferBytes, &pixBuf);
    if (charBuf) {
        for (int i = 0; i < fontBytes; i++) {
            charBuf->buffer[i] = font_data[index + i];
        }
    }

    for(int i = 0; i < fontBytes; i+=2) {
        u16_t line = (((u16_t)charBuf->buffer[i]) << 8) | charBuf->buffer[i+1];
        for(int j = 0; j < 16; j++) {
            if((line >> j)  & 1) {
                int recX = x + (i/2) * size;
                int recY = y + j * size;
                // Draw each bit
                //ret = zjs_gfx_fill_rect_priv(recX, recY, size, size, color, gfxHandle);
                zjs_gfx_mark_pixels(recX, recY, size, size, color, gfxHandle);
                // if (jerry_value_has_error_flag (ret)) {
                //     return ret;
                // }
            }
        }
    }

    zjs_gfx_flush(gfxHandle);

/* BJONES
    2D / 1D - mapping is pretty simple. Given x and y, and 2D array sizes width and height, you can calculate the according index i in 1D space (zero-based) by

    i = x + width*y;
    and the reverse operation is

    x = i % width;    // % is the "modulo operator", the remainder of i / width;
    y = i / width;    // where "/" is an integer division

    so set xStart xEnd
    if ((i%width >= xStart && i%width < xEnd) && (i/width > yStart && i/width < yEnd)) {
        buffer[curr] = pixels[i][0];
        buffer[curr+1] = pixels[i][1];
    }
}
*/
/*
    for (int i = 0; i < fontBytes; i++) {
        u16_t line = (((u16_t)charBuf->buffer[i]) << 8) | charBuf->buffer[i+1];
        for (int j = 0; j < bufferBytes; j+=(size * 2)) {
            if((line >> j)  & 1) {
                for (int k = 0; k < (size * 2); k+=2) {
                    pixBuf->buffer[j + k] = color[0];
                    pixBuf->buffer[j + k + 1] = color[1];
                }
            }
        }
    }

    jerry_value_t args[] = {jerry_create_number(x), jerry_create_number(y),
                            jerry_create_number(w), jerry_create_number(h), pixelBufObj};

    ret = jerry_call_function(gfxHandle->drawDataCB, gfxHandle->jsThis, args, 5);
    if (jerry_value_has_error_flag (ret)) {
        return ret;
    }
    */



    return ZJS_UNDEFINED;
}

static ZJS_DECL_FUNC(zjs_gfx_fill_rect)
{
    // requires: Requires 5 arguments
    //           arg[0] - x coord of the top left.
    //           arg[1] - y coord of the top left.
    //           arg[2] - width.
    //           arg[3] - height.
    //           arg[4] - color array.
    //
    //  effects: Draws a filled rectangle on the screen

    ZJS_VALIDATE_ARGS(Z_NUMBER, Z_NUMBER, Z_NUMBER, Z_NUMBER, Z_ARRAY);
    ZJS_GET_HANDLE(this, gfx_handle_t, handle, gfx_type_info);
    gfx_data_t argData;
    args_to_data(&argData, argc, argv);
    return zjs_gfx_fill_rect_priv(argData.coords[0], argData.coords[1], argData.coords[2], argData.coords[3], argData.color, handle);
}

static ZJS_DECL_FUNC(zjs_gfx_draw_pixel)
{
    // requires: Requires 3 arguments
    //           arg[0] - x coord of the top left.
    //           arg[1] - y coord of the top left.
    //           arg[2] - color array.
    //
    //  effects: Draws a pixel on the screen

    ZJS_VALIDATE_ARGS(Z_NUMBER, Z_NUMBER, Z_ARRAY);
    ZJS_GET_HANDLE(this, gfx_handle_t, handle, gfx_type_info);
    gfx_data_t argData;
    args_to_data(&argData, argc, argv);
    return zjs_gfx_fill_rect_priv(argData.coords[0], argData.coords[1], 1, 1, argData.color, handle);
}

static ZJS_DECL_FUNC(zjs_gfx_draw_line)
{
    // requires: Requires 5 arguments
    //           arg[0] - x coord of the start.
    //           arg[1] - y coord of the start.
    //           arg[2] - x coord of the end.
    //           arg[3] - y coord of the end.
    //           arg[4] - color array.
    //           arg[5] - optional size of line.
    //
    //  effects: Draws a line on the screen

    ZJS_VALIDATE_ARGS(Z_NUMBER, Z_NUMBER, Z_NUMBER, Z_NUMBER, Z_ARRAY, Z_OPTIONAL Z_NUMBER);
    ZJS_GET_HANDLE(this, gfx_handle_t, handle, gfx_type_info);
    gfx_data_t argData;
    args_to_data(&argData, argc, argv);
    int xLen = argData.coords[2] > argData.coords[0] ? argData.coords[2] - argData.coords[0] : argData.coords[0] - argData.coords[2];
    int yLen = argData.coords[3] > argData.coords[1] ? argData.coords[3] - argData.coords[1] : argData.coords[1] - argData.coords[3];
    xLen = xLen == 0 ? 1 : xLen; // Line width has to be at least a pixel
    yLen = yLen == 0 ? 1 : yLen;
    bool neg = false;
    jerry_value_t ret = ZJS_UNDEFINED;

    if (xLen <= yLen) {
        // We always draw left to right, swap if argData.coords[0] is larger
        if (argData.coords[0] > argData.coords[2]) {
            u32_t tmp = argData.coords[0];
            argData.coords[0] = argData.coords[2];
            argData.coords[2] = tmp;
            tmp = argData.coords[1];
            argData.coords[1] = argData.coords[3];
            argData.coords[3] = tmp;
        }
        // Line is going up
        if (argData.coords[3] < argData.coords[1])
                neg = true;

        u32_t pos = argData.coords[1];
        int step = yLen / xLen;

        for (u32_t x = argData.coords[0]; x <= argData.coords[2]; x++) {
            ret = zjs_gfx_fill_rect_priv(x, pos, argData.size, step, argData.color, handle);
            if (jerry_value_has_error_flag (ret)){
                    return ret;
                }
            pos = neg == false ? pos + step : pos - step;
        }
    }
    else {
        // We always draw left to right, swap if argData.coords[1] is larger
        if (argData.coords[1] > argData.coords[3]) {
            u32_t tmp = argData.coords[0];
            argData.coords[0] = argData.coords[2];
            argData.coords[2] = tmp;
            tmp = argData.coords[1];
            argData.coords[1] = argData.coords[3];
            argData.coords[3] = tmp;
        }
        // Line is going up
        if (argData.coords[2] < argData.coords[0])
            neg = true;

        u32_t pos = argData.coords[0];
        int step = xLen / yLen;

        for (u32_t y = argData.coords[1]; y <= argData.coords[3]; y++) {
            ret = zjs_gfx_fill_rect_priv(pos, y, step, argData.size, argData.color, handle);
            if (jerry_value_has_error_flag (ret)) {
                    return ret;
                }
            pos = neg == false ? pos + step : pos - step;
        }
    }
    return ret;
}

static ZJS_DECL_FUNC(zjs_gfx_draw_v_line)
{
    // requires: Requires 4 arguments
    //           arg[0] - x coord of the start.
    //           arg[1] - y coord of the start.
    //           arg[2] - height of line.
    //           arg[3] - color array.
    //           arg[4] - optional size of line.
    //
    //  effects: Draws a vertical line on the screen.

    ZJS_VALIDATE_ARGS(Z_NUMBER, Z_NUMBER, Z_NUMBER, Z_ARRAY, Z_OPTIONAL Z_NUMBER);
    ZJS_GET_HANDLE(this, gfx_handle_t, handle, gfx_type_info);
    gfx_data_t argData;
    args_to_data(&argData, argc, argv);
    return zjs_gfx_fill_rect_priv(argData.coords[0], argData.coords[1], argData.size, argData.coords[2], argData.color, handle);
}

static ZJS_DECL_FUNC(zjs_gfx_draw_h_line)
{
    // requires: Requires 4 arguments
    //           arg[0] - x coord of the start.
    //           arg[1] - y coord of the start.
    //           arg[2] - width of line.
    //           arg[3] - color array.
    //           arg[4] - optional size of line.
    //
    //  effects: Draws a horizontal line on the screen

    ZJS_VALIDATE_ARGS(Z_NUMBER, Z_NUMBER, Z_NUMBER, Z_ARRAY, Z_OPTIONAL Z_NUMBER);
    ZJS_GET_HANDLE(this, gfx_handle_t, handle, gfx_type_info);
    gfx_data_t argData;
    args_to_data(&argData, argc, argv);
    return zjs_gfx_fill_rect_priv(argData.coords[0], argData.coords[1], argData.coords[2], argData.size, argData.color, handle);
}

static ZJS_DECL_FUNC(zjs_gfx_draw_rect)
{
    // requires: Requires 5 arguments
    //           arg[0] - x coord of the top left.
    //           arg[1] - y coord of the top left.
    //           arg[2] - width.
    //           arg[3] - height.
    //           arg[4] - color array.
    //           arg[5] - optional size of line.
    //
    //  effects: Draws a rectangle on the screen

    ZJS_VALIDATE_ARGS(Z_NUMBER, Z_NUMBER, Z_NUMBER, Z_NUMBER, Z_ARRAY, Z_OPTIONAL Z_NUMBER);
    ZJS_GET_HANDLE(this, gfx_handle_t, handle, gfx_type_info);
    gfx_data_t argData;
    args_to_data(&argData, argc, argv);
    zjs_gfx_fill_rect_priv(argData.coords[0], argData.coords[1], argData.coords[2], argData.size, argData.color, handle);
    zjs_gfx_fill_rect_priv(argData.coords[0], argData.coords[1] + argData.coords[3] - argData.size, argData.coords[2], argData.size, argData.color, handle);
    zjs_gfx_fill_rect_priv(argData.coords[0], argData.coords[1], argData.size, argData.coords[3], argData.color, handle);
    zjs_gfx_fill_rect_priv(argData.coords[0] + argData.coords[2] - argData.size, argData.coords[1], argData.size, argData.coords[3], argData.color, handle);

    return ZJS_UNDEFINED;
}

static ZJS_DECL_FUNC(zjs_gfx_draw_char)
{
    // requires: Requires 4 arguments
    //           arg[0] - x coord of the top left.
    //           arg[1] - y coord of the top left.
    //           arg[2] - character.
    //           arg[3] - color array.
    //           arg[4] - optional size of character.
    //
    //  effects: Draws a character on the screen

    ZJS_VALIDATE_ARGS(Z_NUMBER, Z_NUMBER, Z_STRING, Z_ARRAY, Z_OPTIONAL Z_NUMBER);
    ZJS_GET_HANDLE(this, gfx_handle_t, handle, gfx_type_info);
    gfx_data_t argData;
    args_to_data(&argData, argc, argv);
    return zjs_gfx_draw_char_priv(argData.coords[0], argData.coords[1], argData.text[0], argData.color, argData.size, handle);
}

static ZJS_DECL_FUNC(zjs_gfx_draw_string)
{
    // requires: Requires 4 arguments
    //           arg[0] - x coord of the top left.
    //           arg[1] - y coord of the top left.
    //           arg[2] - text.
    //           arg[3] - color array.
    //           arg[4] - optional size of character.
    //
    //  effects: Draws a character on the screen

    ZJS_VALIDATE_ARGS(Z_NUMBER, Z_NUMBER, Z_STRING, Z_ARRAY, Z_OPTIONAL Z_NUMBER);
    ZJS_GET_HANDLE(this, gfx_handle_t, handle, gfx_type_info);
    gfx_data_t argData;
    args_to_data(&argData, argc, argv);
    jerry_value_t ret = ZJS_UNDEFINED;

    for (u8_t i = 0; i < argData.textSize; i++) {
        ret = zjs_gfx_draw_char_priv(argData.coords[0] + (i * 7 * argData.size), argData.coords[1], argData.text[i], argData.color, argData.size, handle);
        if (jerry_value_has_error_flag (ret)) {
            return ret;
        }
    }
    return ret;
}

static ZJS_DECL_FUNC(zjs_gfx_set_cb)
{
    // requires: Requires 4 arguments
    //           arg[0] - The width of the screen.
    //           arg[1] - The hight of the screen.
    //           arg[2] - The init callback to use.
    //           arg[3] - The drawRect callback to use.
    //           arg[4] - optional JS 'this' object
    //
    //  effects: Initializes the GFX module

    ZJS_VALIDATE_ARGS(Z_NUMBER, Z_NUMBER, Z_FUNCTION, Z_FUNCTION, Z_OPTIONAL);

    gfx_handle_t *handle = zjs_malloc(sizeof(gfx_handle_t));

    if (!handle) {
        return ZJS_ERROR("could not allocate handle\n");
    }

    handle->screenW = jerry_get_number_value(argv[0]);
    handle->screenH = jerry_get_number_value(argv[1]);
    handle->screenInitCB = jerry_acquire_value(argv[2]);
    handle->drawDataCB = jerry_acquire_value(argv[3]);
    handle->jsThis = jerry_acquire_value(argv[4]);
    u32_t totalPixels = handle->screenW * handle->screenH;
    ZJS_PRINT("arg vals = %i, %i\n", argv[0], argv[1]);
    ZJS_PRINT("malloc 1 %i = %i * %i\n", totalPixels, handle->screenW, handle->screenH);
    handle->pixels = malloc(sizeof *handle->pixels * (totalPixels));

    // Reset touched pixels
    handle->tpX0 = handle->screenW;
    handle->tpX1 = 0;
    handle->tpY0 = handle->screenH;
    handle->tpY1 = 0;

    // Init the screen pixel array.
    if (handle->pixels)
    {
        ZJS_PRINT("malloc 2\n");
      for (u32_t i = 0; i < totalPixels; i++)
      {
        handle->pixels[i] = malloc(sizeof *handle->pixels[i] * COLORBYTES);
      }
      ZJS_PRINT("malloc 3\n");
    }

    jerry_value_t gfx_obj = zjs_create_object();
    jerry_set_prototype(gfx_obj, zjs_gfx_prototype);
    jerry_set_object_native_pointer(gfx_obj, handle, &gfx_type_info);
    jerry_call_function(handle->screenInitCB, handle->jsThis, NULL, 0);


    return gfx_obj;
}

jerry_value_t zjs_gfx_init()
{
    ZJS_PRINT("BJONES IN GFX INIT \n");
    zjs_native_func_t proto[] = {
        { zjs_gfx_fill_rect, "fillRect" },
        { zjs_gfx_draw_pixel, "drawPixel" },
        { zjs_gfx_draw_line, "drawLine" },
        { zjs_gfx_draw_v_line, "drawVLine" },
        { zjs_gfx_draw_h_line, "drawHLine" },
        { zjs_gfx_draw_rect, "drawRect" },
        { zjs_gfx_draw_char, "drawChar" },
        { zjs_gfx_draw_string, "drawString" },
        { NULL, NULL }
    };

    zjs_gfx_prototype = zjs_create_object();
    zjs_obj_add_functions(zjs_gfx_prototype, proto);

    jerry_value_t gfx_obj = zjs_create_object();
    zjs_obj_add_function(gfx_obj, "init", zjs_gfx_set_cb);
    return gfx_obj;
}

void zjs_gfx_cleanup() {
    jerry_release_value(zjs_gfx_prototype);
}
