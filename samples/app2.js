var aio = require("aio");
var gpio = require("gpio");
var spi = require("spi");
var board = require('board');
var gfxLib = require("gfx");
var lcd = require("ST7735.js");
var pin = aio.open({ pin: 'A3' });
var gpioPin = gpio.open({pin: '2', mode: 'in', edge: 'any'});

if (!lcd)
    console.log("lcd failed to load")

var BLACK =  [0x00, 0x00];
var BLUE  =  [0x00, 0x1F];
var RED   =  [0xF8, 0x00];
var GREEN =  [0x07, 0xE0];
var CYAN  =  [0x07, 0xFF];
var MAGENTA = [0xF8, 0x1F];
var YELLOW =  [0xFF, 0xE0];
var WHITE =  [0xFF, 0xFF];

console.log("Starting app 2");

// Initialize the screen
var gfx = gfxLib.init(lcd.width, lcd.height, lcd.initScreen, lcd.drawCB,
                      true, lcd);

function checkAngle()
{
    var rawValue = pin.read();
    rawValue *= 10;
    rawValue += ' ';
    console.log(rawValue);
    gfx.fillRect(0, lcd.height / 2, lcd.width,
                         lcd.height / 4, YELLOW);
    gfx.drawString(lcd.width / 5, lcd.height / 5,
                           rawValue+'', BLACK, 2);
}


gfx.fillRect(0, 0, lcd.width, lcd.height, YELLOW);

gpioPin.onchange = function(event) {
    runJS("app1.js");
}
checkAngle();
//setInterval(checkAngle, 2000);
