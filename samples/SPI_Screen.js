// JS for using the SPI screen

// BJONES TODO - Make good use of Geoff's newly added buffer fill.  Create a way to draw squares, lines, circles, pixels.
// Look up a way to draw characters.  How to get fonts, how to draw them, etc.
// Go back to fixing bugs after I upload
// Upload SPI_Screen and OLED.js, also SPI fixes.
var timer = false;
var block = false;
var width = 128;
var height = 160;
var maxPixels = width * height;

var ST7735_NOP = [0x0];
var ST7735_SWRESET = [0x01];
var ST7735_RDDID = [0x04];
var ST7735_RDDST = [0x09];

var ST7735_SLPIN  = [0x10];
var ST7735_SLPOUT  = [0x11];
var ST7735_PTLON  = [0x12];
var ST7735_NORON  = [0x13];

var ST7735_INVOFF = [0x20];
var ST7735_INVON = [0x21];
var ST7735_DISPOFF = [0x28];
var ST7735_DISPON = [0x29];
var ST7735_CASET = [0x2A];
var ST7735_RASET = [0x2B];
var ST7735_RAMWR = [0x2C];
var ST7735_RAMRD = [0x2E];

var ST7735_COLMOD = [0x3A];
var ST7735_MADCTL = [0x36];

var ST7735_FRMCTR1 = [0xB1];
var ST7735_FRMCTR2 = [0xB2];
var ST7735_FRMCTR3 = [0xB3];
var ST7735_INVCTR = [0xB4];
var ST7735_DISSET5 = [0xB6];

var ST7735_PWCTR1 = [0xC0];
var ST7735_PWCTR2 = [0xC1];
var ST7735_PWCTR3 = [0xC2];
var ST7735_PWCTR4 = [0xC3];
var ST7735_PWCTR5 = [0xC4];
var ST7735_VMCTR1 = [0xC5];

var ST7735_RDID1 = [0xDA];
var ST7735_RDID2 = [0xDB];
var ST7735_RDID3 = [0xDC];
var ST7735_RDID4 = [0xDD];

var ST7735_PWCTR6 = [0xFC];

var ST7735_GMCTRP1 = [0xE0];
var ST7735_GMCTRN1 = [0xE1];

var ST7735_CASET =  [0x2A];
var ST7735_RASET =  [0x2B];
var ST7735_RAMWR =  [0x2C];
var ST7735_MADCTL = [0x36];
// Color definitions
var	ST7735_BLACK =  [0x00,0x00];
var	ST7735_BLUE  =  [0x00,0x1F];
var	ST7735_RED   =  [0xF8,0x00];
var	ST7735_GREEN =  [0x07,0xE0];
var ST7735_CYAN  =  [0x07,0xFF];
var ST7735_MAGENTA= [0xF8,0x1F];
var ST7735_YELLOW=  [0xFF,0xE0];
var ST7735_WHITE =  [0xFF,0xFF];
var ystart = xstart = colstart  = rowstart = 0;
var gpio = require('gpio');
var spi = require("spi");
var spiBus = spi.open({bus:1, polarity:0, phase:0, bits:8});
var dcPin = gpio.open(8);
var csPin = gpio.open(4);
var rstPin = gpio.open(7);

function writecommand(c) {
    dcPin.write(0);
    csPin.write(0);
    var buffer = spiBus.transceive(1, c, "write");
    csPin.write(1);
}

function writedata(c) {
    dcPin.write(1);
    csPin.write(0);    
    spiBus.transceive(1, c, "write");
    csPin.write(1);  
}

function writedata2(c) {
    dcPin.write(1);
    csPin.write(0);
    //var buf = Buffer(width * 2);
    var buf = Buffer(c);
    buf.fill(0x00000000);
    spiBus.transceive(1, buf, "write");
    spiBus.transceive(1, buf, "write");
    csPin.write(1);  
}

function setAddrWindow(x0, y0, x1, y1) {

  console.log("setAddrWindow ");
  writecommand(ST7735_CASET); // Column addr set
  writedata([0x00]);
  writedata([x0+xstart]);     // XSTART 
  writedata([0x00]);
  writedata([x1+xstart]);     // XEND

  writecommand(ST7735_RASET); // Row addr set
  writedata([0x00]);
  writedata([y0+ystart]);     // YSTART
  writedata([0x00]);
  writedata([y1+ystart]);     // YEND

  writecommand(ST7735_RAMWR); // write to RAM  
}

function fillScreen(x1,y1,x2,y2,color){
     var i = 0;
     var arr = [];
      writecommand([0x2A]);      
      writedata([0,x1,0,x2]);
      writecommand([0x2B]);
      writedata([0,y1,0,y2]);
      writecommand([0x2C]);
      for ( var j = 0; j < (y2-y1) ; j++)      
        writedata2(1);
}

function fillScreen2(x1,y1,x2,y2,color){
     var i = 0;
     var arr = [];
      writecommand([0x2A]);      
      writedata([0,x1,0,x2]);
      writecommand([0x2B]);
      writedata([0,y1,0,y2]);
      writecommand([0x2C]);

    for ( var j = 0; j < (x2-x1)*(y2-y1) ; j++)      
        writedata([0x00,0x1F]);
}
// 17 34 4386
function fillRect(x1,y1,x2,y2,color){
    var i = 0;
    var arr = [];
    writecommand([0x2A]);      
    writedata([0,x1,0,x2]);
    writecommand([0x2B]);
    writedata([0,y1,0,y2]);
    writecommand([0x2C]);
    var w = x2 - x1;
    var h = y2 - y1;
    var pixels = w * h * 2; // Each pixel has two bytes of data
    var passes = 1;
    if (pixels > maxPixels) {
        passes = pixels / maxPixels;
        var whole = parseInt(passes);
        var dec = passes - whole;
        if (dec > 0.0) {
            console.log("BJONES whole = " + whole + " dec = " + dec);
            passes += 1;
        }
        pixels = maxPixels; // Set pixels to the maximum size
    }
    console.log("*** passes = " + passes + " pixels = " + pixels);
    var buf = Buffer(pixels);
    //BJONES need to double this, need a second set.
    buf.fill(color[0] << 8 + color[1]);
    for ( var j = 0; j < passes ; j++)      
        writedata(buf);
}

function drawPixel(x, y, color) {
  console.log("drawPixel x=" + x + " y=" + y + " color=" + color);
  if((x < 0) ||(x >= width) || (y < 0) || (y >= height)) return;

  setAddrWindow(x,y,x+1,y+1);
  console.log("BJONES setAddrWindow done");
  dcPin.write(1);
  csPin.write(0);
  buffer = spiBus.transceive(1, color, "write");
  csPin.write(1);

}

function initScreen()
{
    csPin.write(0);
    rstPin.write(1);

    console.log("initScreen 1");
    writecommand(ST7735_SWRESET); // software reset
    
    spiBus.ZJSwait(100);

    writecommand(ST7735_SLPOUT);  // out of sleep mode
    spiBus.ZJSwait(50);

    writecommand([0x26]);
    writedata([0x04]);

    writecommand(ST7735_FRMCTR1);  // frame rate control - normal mode
    writedata([0x0e]);  // frame rate = fosc / (1 x 2 + 40) * (LINE + 2C + 2D)
    writedata([0x10]); 

    console.log("initScreen 2");

    writecommand(ST7735_PWCTR1);  // power control
     writedata([0x08]);
      writedata([0]);      

    writecommand(ST7735_PWCTR2);  // power control
    writedata([0x05]);

    writecommand(ST7735_VMCTR1);  // power control
    writedata([0x38]);
    writedata([0x40]);  

    writecommand(ST7735_INVOFF);    // don't invert display

    writecommand(ST7735_COLMOD);  // set color mode
    writedata([0x05]);        // 16-bit color
    
    writecommand(ST7735_MADCTL);  // memory access control (directions)
    writedata([0xC8]);  // row address/col address, bottom to top refresh
    madctl = [0xC8];

    writecommand(ST7735_CASET);  // column addr set
    writedata([0x00]);
    writedata([0x00]);   // XSTART = 0
    writedata([0x00]);
    writedata([0x7F]);   // XEND = 127
    console.log("initScreen 4");
    writecommand(ST7735_RASET);  // row addr set
    writedata([0x00]);
    writedata([0x00]);    // XSTART = 0
    writedata([0x00]);
    writedata([0x9F]);    // XEND = 159

    writecommand(ST7735_INVCTR);  // display inversion control
    writedata([0x00]);  // no inversion

    writecommand([0xF2]);
    writedata([1]);
    
    writecommand(ST7735_GMCTRP1);
    writedata([0x3f,0x22,0x20,0x30,0x29,0x0c,0x4e,0xb7,0x3c,0x19,0x22,0x1e,0x02,0x01,0x00]);

    writecommand(ST7735_GMCTRN1);
    writedata([0x00,0x1b,0x1f,0x0f,0x16,0x13,0x31,0x84,0x43,0x06,0x1d,0x21,0x3d,0x3e,0x3f]);

    console.log("initScreen 5");
    writecommand(ST7735_DISPON);
    writecommand(ST7735_RAMWR);
    spiBus.ZJSwait(100);
}
console.log("SPI test starting..");
try {

    initScreen();

    fillRect(0, 0, width, height, ST7735_BLACK);
    fillRect(70, 70, 100, 100, ST7735_RED);
    
    drawPixel(64, 80, ST7735_GREEN);
    drawPixel(42, 53, ST7735_GREEN);
    drawPixel(32, 40, ST7735_GREEN);
    drawPixel(25, 32, ST7735_GREEN);

} catch (err) {
  console.log("SPI error: " + err.message);
}



