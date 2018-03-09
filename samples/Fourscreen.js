// TODO do I need to include all the requires needed here? Probably should
var gfxLib = require("gfx");
var lcd = require("ST7735.js");
var quad1 = require("QUAD1.js");
var quad2 = require("QUAD2.js");
var quad3 = require("QUAD3.js");
var quad4 = require("QUAD4.js");

var BLACK =  [0x00, 0x00];
var BLUE  =  [0x00, 0x1F];
var RED   =  [0xF8, 0x00];
var GREEN =  [0x07, 0xE0];
var CYAN  =  [0x07, 0xFF];
var MAGENTA = [0xF8, 0x1F];
var YELLOW =  [0xFF, 0xE0];
var WHITE =  [0xFF, 0xFF];

console.log("Starting main screen..");

// Initialize the screen
var gfx = gfxLib.init(lcd.width, lcd.height, lcd.initScreen, lcd.drawCB,
                      true, lcd);
console.log("Starting main screen.. 2");
var quadW = (lcd.width / 2);
var quadH = (lcd.height / 2);
console.log("Starting main screen.. 3");
// Pass the screens their allotted screen space
quad1.init(gfx, 0, 0, quadW, quadH);
console.log("Starting main screen.. 4");
quad2.init(gfx, quadW, 0, quadW, quadH);
quad3.init(gfx, 0, quadH, quadW, quadH);
console.log("Starting main screen.. 5");
quad4.init(gfx, quadW, quadH, quadW, quadH);
console.log("Starting main screen.. 6");
