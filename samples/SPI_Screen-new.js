// JS for using the SPI screen

var timer = false;
var block = false;
var width = 128;
var height = 160;
var maxPixels = width * height;


// Color definitions
var	BLACK =  [0x00,0x00];
var	BLUE  =  [0x00,0x1F];
var	RED   =  [0xF8,0x00];
var	GREEN =  [0x07,0xE0];
var CYAN  =  [0x07,0xFF];
var MAGENTA = [0xF8,0x1F];
var YELLOW =  [0xFF,0xE0];
var WHITE =  [0xFF,0xFF];

var LCD = require("ST7735.js");
/*var gpio = require('gpio');
var spi = require("spi");
var spiBus = spi.open({bus:1, polarity:0, phase:0, bits:8});
var dcPin = gpio.open(8);   // Command / Data select pin
var csPin = gpio.open(4);   // SPI slave pin
var rstPin = gpio.open(7);  // Reset pin
*/
console.log("SPI test starting..");
try {

    LCD.initScreen();

    LCD.fillRect(0, 0, width - 1, height - 1, BLACK);
    LCD.fillRect(70, 70, 100, 100, RED);
    
    LCD.drawPixel(64, 80, RED);
    LCD.drawPixel(42, 53, CYAN);
    LCD.drawPixel(32, 40, MAGENTA);
    LCD.drawPixel(25, 32, YELLOW);

} catch (err) {
  console.log("SPI error: " + err.message);
}



