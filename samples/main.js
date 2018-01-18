// Copyright (c) 2017, Intel Corporation.
// JS for using the SPI LCD screen ST7735 module
// Setup:
// DC - Pin 8 for A101, Pin 9 for all others
// CS - Pin 4
// RST - Pin 7
// SCL - Pin 13
// SDA - Pin 11

var LCD = require("ST7735.js");
var aio = require("aio");
//var fs = require('fs');
// Color definitions
var BLACK =  [0x00, 0x00];
var BLUE  =  [0x00, 0x1F];
var RED   =  [0xF8, 0x00];
var GREEN =  [0x07, 0xE0];
var CYAN  =  [0x07, 0xFF];
var MAGENTA = [0xF8, 0x1F];
var YELLOW =  [0xFF, 0xE0];
var WHITE =  [0xFF, 0xFF];

var lastX = 70;
var lastY = 70;
var lastColor = RED;
// Load the screen, gpio, and GFX modules

var board = require('board');
var drawImmediate = board.name === "arduino_101" ? true : false;
var gpio = require('gpio');
var gfxLib = require("gfx");

console.log("SPI screen test starting..");

try {
    // Initialize the screen
    var GFX = gfxLib.init(LCD.width, LCD.height, LCD.initScreen, LCD.drawCB,
                          drawImmediate, LCD);

    GFX.fillRect(0, 0, LCD.width, LCD.height, BLACK);
    GFX.drawVLine(123, 0, 160, GREEN, 5);
    GFX.drawVLine(118, 0, 160, YELLOW, 3);
    GFX.drawVLine(113, 0, 160, WHITE);
    GFX.drawString(0, 20, "MAIN", RED, 2);
    GFX.drawString(0, 35, "MENU", [0x06, 0x1F], 3);
    GFX.flush();
} catch (err) {
  console.log("Screen error: " + err.message);
}

//*************************************************************************

var pin3 = gpio.open({pin: '3', mode: 'in', edge: 'rising'});
// var pin2 = gpio.open({pin: '12', mode: 'in', edge: 'rising'});
var pin5 = gpio.open({pin: '5', mode: 'in', edge: 'rising'});
var pinX = aio.open({ pin: 'A0' });
var pinY = aio.open({ pin: 'A1' });

pin3.onchange = function(event) {
    console.log("Starting 1.js...");
    runJS("1.js");
};

// pin2.onchange = function(event) {
//     console.log("Starting 2.js...");
//     setBootCfg("2.js");
//     reset();
// };

pin5.onchange = function(event) {
    //stopJS();
    console.log("Starting 3.js...");
    runJS("3.js");
    //setBootCfg("3.js");
    //reset();
};

setInterval(function () {
    // read analog input pin A0
    var valueX = parseInt(pinX.read() * 9 /1024) // + 48;
    var valueY =  parseInt(pinY.read() * 9 / 1024)// + 48;

    if (valueX > 25 +3) {
    //    valueX = (25 - valueX) + lastX;
        valueX = lastX + 3;
    }
    else if (valueX < 24 -3)
        valueX = lastX - 3;// (25 - valueX);
    else {
        valueX = lastX;
    }

    if (valueY > 25+3) {
        valueY = lastY +3; //(25 - valueY) + lastY;
    }
    else if (valueY < 24 -3)
        valueY = lastY - 3// (25 - valueY);
    else {
        valueY = lastY;
    }
//pinX.read();
    //console.log("X val is " + valueX);
    var color = lastColor == RED ? GREEN : RED;
    lastColor = color;

    console.log( valueX + " / " + valueY);

    if (valueX > 3000)
        valueX -= 3000;
    if (valueY > 3000)
        valueY -= 3000;
    if (valueX < LCD.width && valueY < LCD.height) {
    //    GFX.drawPixel(valueX, valueY, GREEN);
        GFX.drawLine(lastX, lastY, valueX, valueY, color, 2);
        lastX = valueX;
        lastY = valueY;
    }
}, 50);
