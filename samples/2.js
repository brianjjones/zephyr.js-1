// Copyright (c) 2017, Intel Corporation.
// JS for using the SPI LCD screen ST7735 module
// Setup:
// DC - Pin 8 for A101, Pin 9 for all others
// CS - Pin 4
// RST - Pin 7
// SCL - Pin 13
// SDA - Pin 11
var fs = require('fs');
// Color definitions
var BLACK =  [0x00, 0x00];
var BLUE  =  [0x00, 0x1F];
var RED   =  [0xF8, 0x00];
var GREEN =  [0x07, 0xE0];
var CYAN  =  [0x07, 0xFF];
var MAGENTA = [0xF8, 0x1F];
var YELLOW =  [0xFF, 0xE0];
var WHITE =  [0xFF, 0xFF];

// Load the screen, gpio, and GFX modules
var LCD = require("ST7735.js");
var board = require('board');
var drawImmediate = board.name === "arduino_101" ? true : false;
var gpio = require('gpio');
var gfxLib = require("gfx");

console.log("Program 2 starting..");

try {
    // Initialize the screen
    var GFX = gfxLib.init(LCD.width, LCD.height, LCD.initScreen, LCD.drawCB,
                          drawImmediate, LCD);
    GFX.fillRect(0, 0, LCD.width, LCD.height, [0X03, 0X03]);
    GFX.drawString(0, 20, "Program 2", BLUE, 2);
    GFX.flush();
} catch (err) {
  console.log("Screen error: " + err.message);
}
//*************************************************************************

var pin3 = gpio.open({pin: '3', mode: 'in', edge: 'rising'});
var pin4 = gpio.open({pin: '3', mode: 'in', edge: 'rising'});
var pin6 = gpio.open({pin: '3', mode: 'in', edge: 'rising'});

pin3.onchange = function(event) {
    console.log("Starting 1.js...");
    setBootCfg("1.js");
    reset();
};

pin4.onchange = function(event) {
    console.log("Starting 2.js...");
    setBootCfg("2.js");
    reset();
};

pin6.onchange = function(event) {
    console.log("Starting 3.js...");
    setBootCfg("3.js");
    reset();
};
