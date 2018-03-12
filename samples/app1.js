var aio = require("aio");
var gpio = require("gpio");
var spi = require("spi");
var board = require('board');
var gfxLib = require("gfx");
var lcd = require("ST7735.js");
var pin = aio.open({ pin: 'A2' });
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

console.log("Starting app 1");
// Initialize the screen
var gfx = gfxLib.init(lcd.width, lcd.height, lcd.initScreen, lcd.drawCB,
                      true, lcd);

function checkTemp()
{
    var rawValue = pin.read();
    if (rawValue == 0) {
        console.log("Invalid temperature value");
    }
    else {
        var voltage = (rawValue * 3.3) / 1024;
        var celsius = (voltage - 0.5) * 10;
        celsius = celsius | 0;
        console.log(celsius + "C");
        gfx.fillRect(0, lcd.height / 3, lcd.width,
                             lcd.height / 3, GREEN);
        gfx.drawString(lcd.width / 5, lcd.height / 5,  celsius +
                               "C" , BLUE, 2);
    }
}



gfx.fillRect(0, 0, lcd.width, lcd.height, GREEN);
checkTemp();
gpioPin.onchange = function(event) {
    console.log("clicked");
    runJS("app2.js");
}

// setInterval(checkTemp, 2000);
