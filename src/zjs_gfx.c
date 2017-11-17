// Copyright (c) 2017, Intel Corporation.

// This graphics library was inspired by the Adafruit GFX library
// https://github.com/adafruit/Adafruit-GFX-Library

// ZJS includes
#include "zjs_buffer.h"
#include "zjs_callbacks.h"
#include "zjs_error.h"
#include "zjs_util.h"
#include "zjs_gfx_font.h"

#define COLORBYTES 2    // Number of bytes needed to represent the color

u16_t maxPixels = 800;  // BJONES change this to a capitalized #def

typedef struct gfx_handle {
    u16_t screenW;
    u16_t screenH;
    jerry_value_t pixels;
    zjs_buffer_t *pixelsPtr;
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
bool drawImmediate = true;

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

static void zjs_gfx_reset_touched_pixels(gfx_handle_t *gfxHandle)
{
    gfxHandle->tpX0 = gfxHandle->screenW;
    gfxHandle->tpX1 = 0;
    gfxHandle->tpY0 = gfxHandle->screenH;
    gfxHandle->tpY1 = 0;
}

static void zjs_gfx_touch_pixels(u32_t x, u32_t y, u32_t w, u32_t h, u8_t color[], gfx_handle_t *gfxHandle)
{   // Check that x and y aren't past the screen
    if (x > gfxHandle->screenW || y > gfxHandle->screenH)
        return;

    if (gfxHandle->tpX0 > x ) {
        gfxHandle->tpX0 = x;
    }
    if (gfxHandle->tpX1 < x + w - 1) {
        gfxHandle->tpX1 = x + w -1;
    }
    if (gfxHandle->tpY0 > y) {
        gfxHandle->tpY0 = y;
    }
    if (gfxHandle->tpY1 < y + h - 1) {
        gfxHandle->tpY1 = y + h - 1;
    }

    u16_t pixelsIndex = (x + gfxHandle->screenW * y) * COLORBYTES;

    for (u16_t iY = y; iY < y + h; iY++) {
        // Find the pixel's index in the array
        pixelsIndex = (x + gfxHandle->screenW * iY) * COLORBYTES;
        // Don't write past the memory
        if (pixelsIndex > gfxHandle->pixelsPtr->bufsize)
            return;
        for (u16_t iX = x; iX < x + w; iX++) {
            // Each pixel can be several bytes, fill them in accordingly
            for (u8_t cbyte = 0; cbyte < COLORBYTES; cbyte++) {
                gfxHandle->pixelsPtr->buffer[pixelsIndex + cbyte] = color[cbyte];
            }
            pixelsIndex+=COLORBYTES;
        }
    }
}

// Fills a buffer with the pixels to draw a solid rectangle and calls the provided JS callback to draw it on the screen
static jerry_value_t zjs_gfx_fill_rect_priv (u32_t x, u32_t y, u32_t w, u32_t h, u8_t color[], gfx_handle_t *gfxHandle)
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
    for (int i = 0; i < passes ; i++) {
        jerry_value_t ret = jerry_call_function(gfxHandle->drawDataCB, gfxHandle->jsThis, args, 5);
        if (jerry_value_has_error_flag (ret)) {
            return ret;
        }
    }
    return ZJS_UNDEFINED;
}

static jerry_value_t zjs_gfx_draw_pixels (u32_t x, u32_t y, u32_t w, u32_t h, u8_t color[], gfx_handle_t *gfxHandle)
{
    if (drawImmediate == false) {
        zjs_gfx_touch_pixels(x, y, w, h, color, gfxHandle);
    }
    else {
        zjs_gfx_fill_rect_priv(x, y, w, h, color, gfxHandle);
    }
    return ZJS_UNDEFINED;
}

static jerry_value_t zjs_gfx_call_cb(u32_t x, u32_t y, u32_t w, u32_t h, jerry_value_t data, gfx_handle_t *gfxHandle)
{
    jerry_value_t args[] = {jerry_create_number(x), jerry_create_number(y),
                        jerry_create_number(w),
                        jerry_create_number(h),
                        data};

    jerry_value_t ret = jerry_call_function(gfxHandle->drawDataCB, gfxHandle->jsThis, args, 5);

    if (jerry_value_has_error_flag (ret)) {
        ERR_PRINT("JS callback failed with %i..\n", ret);
        return ret;
    }
    return ZJS_UNDEFINED;
}

static jerry_value_t zjs_gfx_flush(gfx_handle_t *gfxHandle)
{
    u32_t tpW = gfxHandle->tpX1 - gfxHandle->tpX0;
    u32_t tpH = gfxHandle->tpY1 - gfxHandle->tpY0;
    u32_t pixels = tpW * tpH * COLORBYTES;
    zjs_buffer_t *recBuf = NULL;
    u16_t bufferIndex = 0;
    u32_t origIndex = 0;
    u8_t passes = 1;    // Number of times the data buffer needs to be sent
    jerry_value_t ret;

    if (pixels > maxPixels) {
        u32_t pixelsPerRow = tpW * COLORBYTES;
        u32_t rows = maxPixels / pixelsPerRow;
        passes = pixels / (pixelsPerRow * rows);
        // If passes has a remainder, add a pass
        if (pixels % (pixelsPerRow * rows) != 0)
            passes++;

        pixels = pixelsPerRow * rows;
    }

    // If there are a lot of passes, it will be faster to draw the whole buffer
    if (passes > 5) {
        ret = zjs_gfx_call_cb(0, 0, gfxHandle->screenW, gfxHandle->screenH, gfxHandle->pixels, gfxHandle);
    }
    else {
        ZVAL recBufObj =  zjs_buffer_create(pixels, &recBuf);
        u32_t xStart = gfxHandle->tpX0;
        u32_t yStart = gfxHandle->tpY0;
        u32_t currX = xStart;
        u32_t currY = yStart;
        u32_t currW = 0;
        u32_t currH = 0;
        u16_t currPass = 0;

            for (u16_t j = gfxHandle->tpY0; currPass < passes ; j++) {
                currH++;
                xStart = gfxHandle->tpX0;
                for (u16_t i = gfxHandle->tpX0; i < gfxHandle->tpX1; i++) {
                    origIndex = (i + gfxHandle->screenW * j) * COLORBYTES;
                    // Fill the pixel
                    for (u8_t k = 0; k < COLORBYTES; k++) {
                        recBuf->buffer[bufferIndex + k] = gfxHandle->pixelsPtr->buffer[origIndex + k];
                    }
                    bufferIndex+=COLORBYTES;
                    // Width shouldn't be larger than touched pixel width
                    if (currW < tpW) {
                        currW++;
                    }
                    // Send the buffer once its full
                    if (bufferIndex == recBuf->bufsize) {
                        ret = zjs_gfx_call_cb(xStart, yStart, currW, currH , recBufObj, gfxHandle);
                        if (jerry_value_has_error_flag (ret)) {
                            zjs_gfx_reset_touched_pixels(gfxHandle);
                            return ret;
                        }
                        // Reset the buffer to fill from teh beginning
                        bufferIndex = 0;
                        currW = 0;
                        currH = 0;
                        xStart = currX;
                        yStart = currY;
                        currPass++;
                    }
                    currX++;
            }
            currY++;
            currX = gfxHandle->tpX0;
        }
    }
    zjs_gfx_reset_touched_pixels(gfxHandle);
    return ZJS_UNDEFINED;
}

// Draws the rectangles needed to create the given char
static jerry_value_t zjs_gfx_draw_char_priv(u32_t x, u32_t y, char c, u8_t color[], u32_t size, gfx_handle_t *gfxHandle)
{
    u32_t asciiIndex = (u8_t)c - 32;    // To save size our font doesn't include the first 32 chars

    // Check that character is supported
    if (asciiIndex < 0 || asciiIndex > 93) {
        ERR_PRINT("GFX doesn't support '%c'\n", c);
        asciiIndex = 31;    // Set char to ?
    }

    u8_t fontBytes = font_data_descriptors[asciiIndex][0] * 2;  // 2 bytes per pixel
    u16_t index = font_data_descriptors[asciiIndex][1];
    zjs_buffer_t *charBuf = NULL;
    ZVAL charBufObj =  zjs_buffer_create(fontBytes, &charBuf);

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
                zjs_gfx_draw_pixels(recX, recY, size, size, color, gfxHandle);
            }
        }
    }
    return ZJS_UNDEFINED;
}

static ZJS_DECL_FUNC(zjs_gfx_flush_js)
{
    ZJS_GET_HANDLE(this, gfx_handle_t, handle, gfx_type_info);
    return zjs_gfx_flush(handle);
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
    zjs_gfx_draw_pixels(argData.coords[0], argData.coords[1], argData.coords[2], argData.coords[3], argData.color, handle);
    return ZJS_UNDEFINED;
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
    zjs_gfx_draw_pixels(argData.coords[0], argData.coords[1], 1, 1, argData.color, handle);
    return ZJS_UNDEFINED;
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
            zjs_gfx_draw_pixels(x, pos, argData.size, step, argData.color, handle);
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
            zjs_gfx_draw_pixels(pos, y, step, argData.size, argData.color, handle);
            pos = neg == false ? pos + step : pos - step;
        }
    }
    return ZJS_UNDEFINED;
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
    zjs_gfx_draw_pixels(argData.coords[0], argData.coords[1], argData.size, argData.coords[2], argData.color, handle);
    return ZJS_UNDEFINED;
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
    zjs_gfx_draw_pixels(argData.coords[0], argData.coords[1], argData.coords[2], argData.size, argData.color, handle);
    return ZJS_UNDEFINED;
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
    zjs_gfx_draw_pixels(argData.coords[0], argData.coords[1], argData.coords[2], argData.size, argData.color, handle);
    zjs_gfx_draw_pixels(argData.coords[0], argData.coords[1] + argData.coords[3] - argData.size, argData.coords[2], argData.size, argData.color, handle);
    zjs_gfx_draw_pixels(argData.coords[0], argData.coords[1], argData.size, argData.coords[3], argData.color, handle);
    zjs_gfx_draw_pixels(argData.coords[0] + argData.coords[2] - argData.size, argData.coords[1], argData.size, argData.coords[3], argData.color, handle);

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
    u32_t x = argData.coords[0];

    for (u8_t i = 0; i < argData.textSize; i++) {
        u32_t asciiIndex = (u8_t)argData.text[i] - 32;    // To save size our font doesn't include the first 32 chars

        // Check that character is supported
        if (asciiIndex < 0 || asciiIndex > 93) {
            ERR_PRINT("GFX doesn't support '%c'\n", argData.text[i]);
            asciiIndex = 31;    // Set char to ?
        }

        ret = zjs_gfx_draw_char_priv(x, argData.coords[1], argData.text[i], argData.color, argData.size, handle);
        if (jerry_value_has_error_flag (ret)) {
            return ret;
        }

        u8_t charWidth = font_data_descriptors[asciiIndex][0] + 1;  // Add a one for space
        x = x + (charWidth * argData.size);
    }
    return ret;
}

static ZJS_DECL_FUNC(zjs_gfx_set_cb)
{
    // requires: Requires 4 arguments
    //           arg[0] - The width of the screen.
    //           arg[1] - The height of the screen.
    //           arg[2] - The init callback to use.
    //           arg[3] - The drawRect callback to use.
    //           arg[4] - optional JS 'this' object
    //
    //  effects: Initializes the GFX module

    ZJS_VALIDATE_ARGS(Z_NUMBER, Z_NUMBER, Z_FUNCTION, Z_FUNCTION, Z_OPTIONAL Z_OBJECT);
    ZJS_PRINT("BJONS zjs_gfx_set_cb 1\n");
    gfx_handle_t *handle = zjs_malloc(sizeof(gfx_handle_t));

    if (!handle) {
        return ZJS_ERROR("could not allocate handle\n");
    }

    handle->screenW = jerry_get_number_value(argv[0]);
    handle->screenH = jerry_get_number_value(argv[1]);
    handle->screenInitCB = jerry_acquire_value(argv[2]);
    handle->drawDataCB = jerry_acquire_value(argv[3]);
    ZJS_PRINT("BJONS zjs_gfx_set_cb 2\n");
    if (argc > 4)
        handle->jsThis = jerry_acquire_value(argv[4]);
    else
        handle->jsThis = jerry_create_undefined();

    u32_t totalPixels = handle->screenW * handle->screenH * COLORBYTES;
    ZJS_PRINT("BJONS zjs_gfx_set_cb 3\n");
    handle->pixelsPtr = NULL;
    if (!drawImmediate)  //BJONES MAKE THIS NICER
    handle->pixels = zjs_buffer_create(totalPixels, &handle->pixelsPtr);
    zjs_gfx_reset_touched_pixels(handle);
    ZJS_PRINT("BJONS zjs_gfx_set_cb 4\n");
    jerry_value_t gfx_obj = zjs_create_object();
    jerry_set_prototype(gfx_obj, zjs_gfx_prototype);
    ZJS_PRINT("BJONS zjs_gfx_set_cb 5\n");
    jerry_set_object_native_pointer(gfx_obj, handle, &gfx_type_info);
    ZJS_PRINT("BJONS zjs_gfx_set_cb 6\n");
    jerry_call_function(handle->screenInitCB, handle->jsThis, NULL, 0);
    ZJS_PRINT("BJONS zjs_gfx_set_cb 7\n");
    return gfx_obj;
}

static void zjs_gfx_cleanup(void *native)
{
    jerry_release_value(zjs_gfx_prototype);
}

static const jerry_object_native_info_t gfx_module_type_info = {
   .free_cb = zjs_gfx_cleanup
};

static jerry_value_t zjs_gfx_init()
{
    zjs_native_func_t proto[] = {
        { zjs_gfx_fill_rect, "fillRect" },
        { zjs_gfx_draw_pixel, "drawPixel" },
        { zjs_gfx_draw_line, "drawLine" },
        { zjs_gfx_draw_v_line, "drawVLine" },
        { zjs_gfx_draw_h_line, "drawHLine" },
        { zjs_gfx_draw_rect, "drawRect" },
        { zjs_gfx_draw_char, "drawChar" },
        { zjs_gfx_draw_string, "drawString" },
        { zjs_gfx_flush_js, "flush" },
        { NULL, NULL }
    };

    zjs_gfx_prototype = zjs_create_object();
    zjs_obj_add_functions(zjs_gfx_prototype, proto);

    jerry_value_t gfx_obj = zjs_create_object();
    zjs_obj_add_function(gfx_obj, "init", zjs_gfx_set_cb);
    jerry_set_object_native_pointer(gfx_obj, NULL, &gfx_module_type_info);
    return gfx_obj;
}

JERRYX_NATIVE_MODULE(gfx, zjs_gfx_init)
