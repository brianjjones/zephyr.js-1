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
#define MAXPIXELS  1000 * COLORBYTES

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
    bool touched;
    u8_t fillStyle[COLORBYTES];
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

// Whether we are in draw immediate mode.  This mode takes far less RAM, but
// is much slower and shows the lines being drawn in real time vs all at once.
bool drawImmediate = true;
bool big = false;   //BJONES
// Draw function depends on whether we are in immediate draw mode or not.
// It gets set by init
void (*zjs_gfx_draw_pixels)(u32_t, u32_t, u32_t, u32_t, u8_t[], gfx_handle_t*);

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
    gfxHandle->touched = false;
}

static void zjs_gfx_touch_pixels(u32_t x, u32_t y, u32_t w, u32_t h, u8_t color[], gfx_handle_t *gfxHandle)
{   // Check that x and y aren't past the screen

    if (x > 127)
        ZJS_PRINT("Touch - X > 127 == %i, w == %i, curr X0, X1 = %i : %i\n", x, w, gfxHandle->tpX0, gfxHandle->tpX1);


    // if (x >= gfxHandle->screenW || x < 0 || y >= gfxHandle->screenH || y < 0){
    //     ZJS_PRINT("SKIPPING!\n");
    //     return;
    // }
    if (x > gfxHandle->screenW || y > gfxHandle->screenH)
        return;

    if (x >= gfxHandle->screenW) {
        ZJS_PRINT("X IS EQUAL\n");
        x = gfxHandle->screenW - 1;
    }
    if (y >= gfxHandle->screenH) {
        ZJS_PRINT("Y IS EQUAL\n");
        y = gfxHandle->screenH - 1;
    }

    if (gfxHandle->tpX1 < x + w - 1) {
        if (x + w -1 < gfxHandle->screenW) {
            gfxHandle->tpX1 = x + w -1;
            //ZJS_PRINT("BJONES smaller than w %i < %i\n", gfxHandle->tpX1, gfxHandle->screenW);
        }
        else{
            ZJS_PRINT("BEFORE tpX1 = %i, x = %i, y =%i, w = %i, h = %i\n", gfxHandle->tpX1, x, y, w, h);
            gfxHandle->tpX1 = gfxHandle->screenW - 1;   //BJONES do I need the -1?
           ZJS_PRINT("BJONES BIGGER than w %i > %i\n", gfxHandle->tpX1, gfxHandle->screenW);
           big = true;
           if (big)
               ZJS_PRINT("BJONE BIG SET\n");
        }

    }


    if (gfxHandle->tpY0 > y) {
        gfxHandle->tpY0 = y;
    }
    if (gfxHandle->tpY1 < y + h - 1) {
        if (y + h - 1 < gfxHandle->screenH) {
            gfxHandle->tpY1 = y + h - 1;
        //    ZJS_PRINT("BJONES smaller than h %i < %i\n", gfxHandle->tpY1, gfxHandle->screenH);
        }
        else {
            gfxHandle->tpY1 = gfxHandle->screenH - 1;
                        ZJS_PRINT("BJONES BIGGER than h %i < %i\n", gfxHandle->tpY1, gfxHandle->screenH);
        }
    }

    if (gfxHandle->tpX0 > x ) {
        if (x == 127)
            ZJS_PRINT("BJONES!! X0 = %i,  X1 = %i, Y0 = %i, Y1=%i\n", x, gfxHandle->tpX1, gfxHandle->tpY0, gfxHandle->tpY1);
            gfxHandle->tpX0 = x;
    }

    if (big)
        ZJS_PRINT("BJONES before !drawImmediate\n");

    if (gfxHandle->tpX0 > gfxHandle->tpX1)
        ZJS_PRINT("X IS BIGGER THAN X1 %i > %i\n", gfxHandle->tpX0, gfxHandle->tpX1);
    if (!drawImmediate) {
        // Update the pixel map
        if (big)
            ZJS_PRINT("BJONES in !drawImmediate\n");
        u16_t pixelsIndex = (x + gfxHandle->screenW * y) * COLORBYTES;

        for (u16_t iY = y; iY < y + h; iY++) {
            // Find the pixel's index in the array
            pixelsIndex = (x + gfxHandle->screenW * iY) * COLORBYTES;
            // Don't write past the memory
            if (pixelsIndex > gfxHandle->pixelsPtr->bufsize) {
                ZJS_PRINT("Pixel index > bufsize!\n");
                return;
            }
            for (u16_t iX = x; iX < x + w; iX++) {
                // Each pixel can be several bytes, fill them in accordingly
                for (u8_t cbyte = 0; cbyte < COLORBYTES; cbyte++) {
                    if(big)
                        ZJS_PRINT("ABOUT TO WRITE TO %i, %i / %i\n", pixelsIndex + cbyte, pixelsIndex, cbyte);
                    gfxHandle->pixelsPtr->buffer[pixelsIndex + cbyte] = color[cbyte];
                }
                pixelsIndex+=COLORBYTES;
            }
        }
    }
    gfxHandle->touched = true;
}

static jerry_value_t zjs_gfx_call_cb(u32_t x, u32_t y, u32_t w, u32_t h, jerry_value_t data, gfx_handle_t *gfxHandle)
{
    jerry_value_t args[] = {jerry_create_number(x), jerry_create_number(y),
                        jerry_create_number(w),
                        jerry_create_number(h),
                        data};

    jerry_value_t ret = jerry_call_function(gfxHandle->drawDataCB, gfxHandle->jsThis, args, 5);

    if (jerry_value_has_error_flag (ret)) {
        ERR_PRINT("JS callback failed with %u..\n", ret);
        return ret;
    }
    return ZJS_UNDEFINED;
}

static jerry_value_t zjs_gfx_flush(gfx_handle_t *gfxHandle)
{
    if (!gfxHandle->touched)
        return ZJS_UNDEFINED;
    u32_t tpW = gfxHandle->tpX1 - gfxHandle->tpX0 + 1 ;
    u32_t tpH = gfxHandle->tpY1 - gfxHandle->tpY0 + 1;
    u32_t pixels = tpW * tpH * COLORBYTES;
    zjs_buffer_t *recBuf = NULL;
    u16_t bufferIndex = 0;
    u32_t origIndex = 0;
    u8_t passes = 1;    // Number of times the data buffer needs to be sent
    jerry_value_t ret;
    //ZJS_PRINT("bjones tpW %i : tpH %i - tpx1 = %i, tpy1 = %i\n", tpW, tpH, gfxHandle->tpX1, gfxHandle->tpY1);

    if (pixels > MAXPIXELS) {
        u32_t  bytesPerRow = tpW * COLORBYTES;
        u32_t rowsPerBuf = MAXPIXELS /  bytesPerRow;
        passes = pixels / ( bytesPerRow * rowsPerBuf);
        // If passes has a remainder, add a pass
        if (pixels % ( bytesPerRow * rowsPerBuf) != 0)
            passes++;

        pixels =  bytesPerRow * rowsPerBuf;
        //ZJS_PRINT("BJONES MORE THAN MAX pixels = %i, passes = %i, rowsPerBuf = %i, bytesPerR = %i\n", pixels, passes, rowsPerBuf, bytesPerRow);
    }
    // If there are a lot of passes, it will be faster to draw the whole buffer
    // Not valid for draw immidiate mode

    if (passes > 10 && !drawImmediate) {    //BJONES TODO if total pixels > 50% of the screen, draw the whole thing
        ret = zjs_gfx_call_cb(0, 0, gfxHandle->screenW, gfxHandle->screenH, gfxHandle->pixels, gfxHandle);
    }
    else {
    //    ZJS_PRINT("BJONES pixels = %i, passes = %i,\n", pixels, passes);


        ZVAL recBufObj =  zjs_buffer_create(pixels, &recBuf);
        u32_t xStart = gfxHandle->tpX0;
        u32_t yStart = gfxHandle->tpY0;
        u32_t currX = xStart;
        u32_t currY = yStart;
        u32_t currW = 0;
        u32_t currH = 0;
        u16_t currPass = 0;

    //BJONES    for (u16_t j = gfxHandle->tpY0; j <= gfxHandle->tpY1 && currPass < passes ; j++) {
            for (u16_t j = gfxHandle->tpY0; currPass < passes ; j++) {
                if (j > gfxHandle->tpY1)
                    ZJS_PRINT("BIGGER Y j = %i, y1 = %i\n", j, gfxHandle->tpY1);
            if(big)
                ZJS_PRINT("zjs_gfx_flush line %d, j = %i, currPass = %i, \n", __LINE__, j, currPass);
            currH++;
            currY++;
            xStart = gfxHandle->tpX0;
            //ZJS_PRINT("BJONES origIndex last run was %i\n", origIndex);
            if (big)
                ZJS_PRINT("X0 = %i, X1 = %i\n", gfxHandle->tpX0, gfxHandle->tpX1);
            for (u16_t i = gfxHandle->tpX0; i <= gfxHandle->tpX1 && currPass < passes; i++) {
                if(big)
                    ZJS_PRINT("zjs_gfx_flush line %d, j = %i, curPass = %i, passes = %i, x = %i, xEnd = %i \n", __LINE__, j, currPass, passes, i, gfxHandle->tpX1);
                origIndex = (i + gfxHandle->screenW * j) * COLORBYTES;

                // Fill the pixel
                for (u8_t k = 0; k < COLORBYTES; k++) {
                    if (!drawImmediate) {
                        // Use cached pixel map
                        recBuf->buffer[bufferIndex + k] = gfxHandle->pixelsPtr->buffer[origIndex + k];
                    }
                    else {
                        // We are just drawing a solid rectangle, use fillStyle
                        recBuf->buffer[bufferIndex + k] = gfxHandle->fillStyle[k];
                    }
                }
                bufferIndex+=COLORBYTES;
                // Width shouldn't be larger than touched pixel width
                // This keeps track of widest point of the current buffer
                if (currW < tpW) {
                    currW++;
                }
                // Send the buffer once its full or we are at the end
                if (bufferIndex == recBuf->bufsize || currY > gfxHandle->tpY1 + 1) {
                    if(big)
                        ZJS_PRINT("zjs_gfx_flush line %d\n", __LINE__);
                    // If we didn't fill the last buffer completely, reset the size
                    if (currY > gfxHandle->tpY1 + 1)
                        ZJS_PRINT("BJONES currY %i> than tpY1 %i\n", currY, gfxHandle->tpY1);
                    if (bufferIndex < recBuf->bufsize) {
                        recBuf->bufsize = bufferIndex;
                    }
                    //ZJS_PRINT("BJONES x = %i, y = %i, w = %i, h = %i, bufsize = %i, X/Y = %i / %i\n", xStart, yStart, currW, currH, recBuf->bufsize, currX, currY );
                    ret = zjs_gfx_call_cb(xStart, yStart, currW, currH, recBufObj, gfxHandle);
                    if(big)
                        ZJS_PRINT("zjs_gfx_flush line %d\n", __LINE__);
                    if (jerry_value_has_error_flag (ret)) {
                        zjs_gfx_reset_touched_pixels(gfxHandle);
                        return ret;
                    }
                    // Reset the buffer to fill from the beginning
                    bufferIndex = 0;
                    currW = 0;
                    currH = 0;
                    xStart = currX;
                    yStart = currY;
                    currPass++;
                }
                currX++;
            }
            currX = gfxHandle->tpX0;
        }
    }
    zjs_gfx_reset_touched_pixels(gfxHandle);
    return ZJS_UNDEFINED;
}

static void zjs_gfx_fill_rect_priv (u32_t x, u32_t y, u32_t w, u32_t h, u8_t color[], gfx_handle_t *gfxHandle)
{
    zjs_gfx_touch_pixels(x, y, w, h, color, gfxHandle);
    for (u8_t cbyte = 0; cbyte < COLORBYTES; cbyte++) {

        gfxHandle->fillStyle[cbyte] = color[cbyte];
    }
    if (big)
        ZJS_PRINT("Rect Priv about to call flush\n");
    zjs_gfx_flush(gfxHandle);
//    ZJS_PRINT("RECT PRIV DONE WITH FLUSH\n");
    //u32_t pixels = w * h * COLORBYTES; // Each pixel has several bytes of data
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
    // This function only does something when not in draw immediate mode
    if (!drawImmediate) {
        ZJS_GET_HANDLE(this, gfx_handle_t, handle, gfx_type_info);
        return zjs_gfx_flush(handle);
    }
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

    ZJS_PRINT("IN DRAW LINE\n");
    ZJS_VALIDATE_ARGS(Z_NUMBER, Z_NUMBER, Z_NUMBER, Z_NUMBER, Z_ARRAY, Z_OPTIONAL Z_NUMBER);
    ZJS_GET_HANDLE(this, gfx_handle_t, handle, gfx_type_info);
    gfx_data_t argData;
    args_to_data(&argData, argc, argv);
    int xLen = argData.coords[2] > argData.coords[0] ? argData.coords[2] - argData.coords[0] : argData.coords[0] - argData.coords[2];
    int yLen = argData.coords[3] > argData.coords[1] ? argData.coords[3] - argData.coords[1] : argData.coords[1] - argData.coords[3];
    xLen = xLen == 0 ? 1 : xLen; // Line width has to be at least a pixel
    yLen = yLen == 0 ? 1 : yLen;
    bool neg = false;

    ZJS_PRINT("BJONES xlen = %i, ylen = %i, args = %i, %i, %i, %i, %i\n", xLen, yLen, argData.coords[0], argData.coords[1], argData.coords[2], argData.coords[3], argData.size);
    if (xLen <= yLen) {
        ZJS_PRINT("In xLen < yLen\n");
        // We always draw left to right, swap if argData.coords[0] is larger
        if (argData.coords[0] > argData.coords[2]) {
            ZJS_PRINT("BJONES swapping X data\n");
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
        u32_t step = yLen / xLen;
        u32_t trueStep = (yLen * 100) / xLen;
        // BJONES TODO instead do xLen / yLen to get the number with decimal moved right two places.  Now chop offf the last two didgits,
        // Thats your remainder.
        u32_t stepRemain = trueStep - (step * 100);
        u32_t leftoverStep = 0;
        u16_t currStep = step;
        //int step = yLen / xLen;
        ZJS_PRINT("STEP = %i\n", step);

        for (u32_t x = argData.coords[0]; x <= argData.coords[2]; x++) {
            if (leftoverStep > 100) {
                currStep = step + 1;
                leftoverStep -= 100;
            }
            else {
                currStep = step;
            }
            zjs_gfx_draw_pixels(x, pos, argData.size, currStep, argData.color, handle);
            pos = neg == false ? pos + currStep : pos - currStep;
            leftoverStep += stepRemain;
            if (x + 1 > argData.coords[2])
                ZJS_PRINT("FINAL LINE = %i, %i, %i, %i\n", pos, x, currStep, leftoverStep);
        }
    }
    else {
        ZJS_PRINT("In yLen < xLen\n");
        // We always draw left to right, swap if argData.coords[1] is larger
        if (argData.coords[1] > argData.coords[3]) {
            ZJS_PRINT("BJONES swapping Y data\n");
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
        u32_t step = xLen / yLen;
        u32_t trueStep = (xLen * 100) / yLen;
        // BJONES TODO instead do xLen / yLen to get the number with decimal moved right two places.  Now chop offf the last two didgits,
        // Thats your remainder.
        u32_t stepRemain = trueStep - (step * 100);
        u32_t leftoverStep = 0;
        u16_t currStep = step;
        ZJS_PRINT("STEP = %i, stepRemain = %u, true = %u\n", step, stepRemain, trueStep);
        ZJS_PRINT("MOD = %i\n", xLen % yLen);
        for (u32_t y = argData.coords[1]; y <= argData.coords[3]; y++) {
            if (leftoverStep > 100) {
                currStep = step + 1;
                leftoverStep -= 100;
            }
            else {
                currStep = step;
            }
            zjs_gfx_draw_pixels(pos, y, currStep, argData.size, argData.color, handle);
            pos = neg == false ? pos + currStep : pos - currStep;
            leftoverStep += stepRemain;
            if (y + 1 > argData.coords[3])
                ZJS_PRINT("FINAL LINE = %i, %i, %i, %i\n", pos, y, currStep, leftoverStep);
        }
    }
    ZJS_PRINT("Line draw is done\n");
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

    ZJS_VALIDATE_ARGS(Z_NUMBER, Z_NUMBER, Z_FUNCTION, Z_FUNCTION, Z_BOOL, Z_OPTIONAL Z_OBJECT);
    gfx_handle_t *handle = zjs_malloc(sizeof(gfx_handle_t));

    if (!handle) {
        return ZJS_ERROR("could not allocate handle\n");
    }

    handle->screenW = jerry_get_number_value(argv[0]);
    handle->screenH = jerry_get_number_value(argv[1]);
    handle->screenInitCB = jerry_acquire_value(argv[2]);
    handle->drawDataCB = jerry_acquire_value(argv[3]);
    drawImmediate = jerry_get_boolean_value(argv[4]);
    if (argc > 5)
        handle->jsThis = jerry_acquire_value(argv[5]);
    else
        handle->jsThis = jerry_create_undefined();

    handle->pixelsPtr = NULL;
    zjs_gfx_reset_touched_pixels(handle);

    if (!drawImmediate) {
        ZJS_PRINT("BJONES in NOT immediate mode\n");
        u32_t totalPixels = handle->screenW * handle->screenH * COLORBYTES;
        handle->pixels = zjs_buffer_create(totalPixels, &handle->pixelsPtr);
        zjs_gfx_draw_pixels = zjs_gfx_touch_pixels;
    }
    else {
        ZJS_PRINT("BJONES in immediate mode\n");
        handle->pixels = 0;
        handle->pixelsPtr = NULL;
        zjs_gfx_draw_pixels = zjs_gfx_fill_rect_priv;
    }

    jerry_value_t gfx_obj = zjs_create_object();
    jerry_set_prototype(gfx_obj, zjs_gfx_prototype);
    jerry_set_object_native_pointer(gfx_obj, handle, &gfx_type_info);
    jerry_call_function(handle->screenInitCB, handle->jsThis, NULL, 0);
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
