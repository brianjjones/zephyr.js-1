function QUAD1() {

    var quad1API = {};
    var aio = require("aio");
    console.log("QUAD1 1 loaded");
    quad1API.init  = function (screen, x, y, w, h) {
        this.screen = screen;
        this.x = x;
        this.y = y;
        this.width = w;
        this.height = h;
        this.screen.fillRect(this.x, this.y, this.width, this.height, GREEN);
        this.pin = aio.open({ pin: 'A2' });
        this.intervalID = setInterval(this.checkTemp.bind(this), 1000);
        console.log("QUAD1 3 + " + this.pin.read());
    }

    // pins

    //
    quad1API.checkTemp = function() {
        var rawValue = this.pin.read();
        if (rawValue == 0) {
            console.log("Invalid temperature value");
        //    return 0;
        } else {
            var voltage = (rawValue * 3.3) / 1024;
            var celsius = (voltage - 0.5) * 10;
            console.log(celsius + " : " + rawValue + " : ");
            celsius = celsius | 0;
            console.log(celsius + "C");
            this.screen.fillRect(this.x, this.height / 3, this.width, this.height / 3, GREEN);
            this.screen.drawString(this.width / 5, this.height / 5,  celsius + "C" , BLUE, 2);
        }
    }

    return quad1API;
}
module.exports.QUAD1 = new QUAD1();

var BLACK =  [0x00, 0x00];
var BLUE  =  [0x00, 0x1F];
var RED   =  [0xF8, 0x00];
var GREEN =  [0x07, 0xE0];
var CYAN  =  [0x07, 0xFF];
var MAGENTA = [0xF8, 0x1F];
var YELLOW =  [0xFF, 0xE0];
var WHITE =  [0xFF, 0xFF];
var gfxLib = require("gfx");
var lcd = require("ST7735.js");
var gfx = gfxLib.init(lcd.width, lcd.height, lcd.initScreen, lcd.drawCB,
                      true, lcd);
console.log("Starting main screen.. 2");
var quadW = (lcd.width / 2);
var quadH = (lcd.height / 2);
var quad1 = require("QUAD1.js");
quad1.init(gfx, 0, 0, quadW, quadH);
//quad1.checkTemp();
//gfx.drawString(0, 20, "Hello", RED, 2);
 // setInterval(quad1.checkTemp.bind(this), 1000);
