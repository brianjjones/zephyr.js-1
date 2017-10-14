// Copyright (c) 2017, Intel Corporation.
// JS for using the SPI LCD screen ST7735 module
// Setup:
// DC - Pin 8
// CS - Pin 4
// RST - Pin 7
// SCL - Pin 13
// SDA - Pin 11

// Color definitions
var BLACK =  [0x00, 0x00];
var BLUE  =  [0x00, 0x1F];
var RED   =  [0xF8, 0x00];
var GREEN =  [0x07, 0xE0];
var CYAN  =  [0x07, 0xFF];
var MAGENTA = [0xF8, 0x1F];
var YELLOW =  [0xFF, 0xE0];
var WHITE =  [0xFF, 0xFF];

var LCD = require("ST7735.js");
var gpio = require('gpio');
var gfxLib = require("gfx");
console.log("SPI screen test starting..");

try {
    console.log("w / h = " + LCD.width + ' / ' + LCD.height);
    var GFX = gfxLib.init(LCD.width, LCD.height, LCD.initScreen, LCD.drawCB, LCD);
    GFX.fillRect(0, 0, LCD.width, LCD.height, BLACK);
    GFX.drawVLine(123, 0, 160, RED, 5);
    GFX.drawVLine(118, 0, 160, YELLOW, 3);
    GFX.drawVLine(113, 0, 160, WHITE);
    GFX.drawLine(0, 20, 100, 160, WHITE, 15);
    GFX.drawLine(0, 10, 115, 160, BLUE, 10);
    GFX.drawLine(0, 0, 128, 160, RED);
    GFX.drawString(0, 20, "Hello", RED, 1);
    GFX.drawString(0, 35, "WORLD", [0x06, 0x1F], 1);
    GFX.drawChar(20, 60,'Z', YELLOW, 2);
    GFX.drawChar(40, 70,'J', YELLOW, 2);
    GFX.drawChar(60, 80,'S', YELLOW, 2);
} catch (err) {
  console.log("SPI error: " + err.message);
}
