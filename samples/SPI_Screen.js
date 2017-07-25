// JS for using the SPI screen
//baud rate is set to 20mhz. or 9600?

// tft.initR(INITR_BLACKTAB);
// tft.drawPixel (300,300, ST7735_RED);


/*  writecommand(ST7735_MADCTL);
    writedata(0xC0);
    
*/


// BJONES TODO - Is slave select working properly? Might not be getting data to the device because it isn't selected.  Try a different slave number
// Figure out what zephyr does with the slave number

var timer = false;
var block = false;
var width = 128;
var height = 160;

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
var	ST7735_GREEN =  0x07E0;//[0x07,0xE0];
var ST7735_CYAN  =  [0x07,0xFF];
var ST7735_MAGENTA= [0xF8,0x1F];
var ST7735_YELLOW=  [0xFF,0xE0];
var ST7735_WHITE =  [0xFF,0xFF];

var ystart = xstart = colstart  = rowstart = 0;
var gpio = require('gpio');
var spi = require("spi");
var spiBus = spi.open({speed:20000, bus:1, polarity:0, phase:0, bits:16});
var dcPin = gpio.open({pin: 'IO8', mode: 'in', edge: 'rising'});
var csPin = gpio.open({pin: 'IO10', mode: 'in', edge: 'rising'});
var rstPin = gpio.open({pin: 'IO7', mode: 'in', edge: 'rising'});
console.log("BJONES opening pin rstPin");
//function writecommand(uint8_t c) {
function writecommand(c) {
  /*DC_LOW();
  CS_LOW();
  spiwrite(c);
  CS_HIGH();*/
//console.log("Writecommand " + c);
    dcPin.write(0);
    csPin.write(0);
    var buffer = spiBus.transceive(1, c, "write");
    csPin.write(1);
}

function writedata(c) {
//function writedata(uint8_t c) {
/*
  DC_HIGH();
  CS_LOW();    
  spiwrite(c);
  CS_HIGH();
*/
//console.log("Writedata " + c);
    dcPin.write(1);
    csPin.write(0);
    var buffer = spiBus.transceive(1, c);
    csPin.write(1);  
}
//function setAddrWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
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

function drawPixel(x, y, color) {
//function drawPixel(int16_t x, int16_t y, uint16_t color) {
  console.log("drawPixel x=" + x + " y=" + y + " color=" + color);
  if((x < 0) ||(x >= width) || (y < 0) || (y >= height)) return;

  setAddrWindow(x,y,x+1,y+1);
  console.log("BJONES setAddrWindow done");
  dcPin.write(1);
  csPin.write(0);
  //buffer = spiBus.transceive(1, color);
  buffer = spiBus.transceive(1, [color >> 8]);
  buffer = spiBus.transceive(1, [color]);
  //spiwrite(color >> 8);
  //spiwrite(color);
  csPin.write(1);

}
/*
void Adafruit_ST7735::fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
  uint16_t color) {

  // rudimentary clipping (drawChar w/big text requires this)
  if((x >= _width) || (y >= _height)) return;
  if((x + w - 1) >= _width)  w = _width  - x;
  if((y + h - 1) >= _height) h = _height - y;

  setAddrWindow(x, y, x+w-1, y+h-1);

  uint8_t hi = color >> 8, lo = color;

  dcPin.write(1);
  csPin.write(0);
  for(y=h; y>0; y--) {
    for(x=w; x>0; x--) {
      spiwrite(hi);
      spiwrite(lo);
    }
  }
  csPin.write(1);
}

void Adafruit_ST7735::fillScreen(uint16_t color) {
  fillRect(0, 0,  _width, _height, color);
}
*/
/*
function unblock()
{
    console.log("Unblocked!");
    block = false;
    timer = false;
}

function wait(ms)
{
    block = true;
    setTimeout(function(){console.log("Derp derp!");}, 3000);
    console.log("Blocking for " + ms + " ms");
    while (block){
        
        if (timer === false) {
            timer = true;
            var to = setTimeout(unblock, 100);
        }
        };    // Hang out here until the timeout occurs
    console.log("Leaving wait");
}
*/
function initScreen()
{
    csPin.write(0);
    rstPin.write(1);
      
  //if (_rst != -1) {
    //pinMode(_rst, OUTPUT);
    rstPin.write(1);
    spiBus.ZJSwait(500);
    rstPin.write(0);
    spiBus.ZJSwait(500);
    rstPin.write(1);
    spiBus.ZJSwait(500);
  //}

    console.log("initScreen 1");
    writecommand(ST7735_SWRESET); // software reset
    
    spiBus.ZJSwait(150);

    writecommand(ST7735_SLPOUT);  // out of sleep mode
    spiBus.ZJSwait(500);

    writecommand(ST7735_FRMCTR1);  // frame rate control - normal mode
    writedata([0x01]);  // frame rate = fosc / (1 x 2 + 40) * (LINE + 2C + 2D)
    writedata([0x2C]); 
    writedata([0x2D]); 

    writecommand(ST7735_FRMCTR2);  // frame rate control - idle mode
    writedata([0x01]);  // frame rate = fosc / (1 x 2 + 40) * (LINE + 2C + 2D)
    writedata([0x2C]); 
    writedata([0x2D]); 

    writecommand(ST7735_FRMCTR3);  // frame rate control - partial mode
    writedata([0x01]); // dot inversion mode
    writedata([0x2C]); 
    writedata([0x2D]); 
    writedata([0x01]); // line inversion mode
    writedata([0x2C]); 
    writedata([0x2D]); 
    console.log("initScreen 2");
    writecommand(ST7735_INVCTR);  // display inversion control
    writedata([0x07]);  // no inversion

    writecommand(ST7735_PWCTR1);  // power control
    writedata([0xA2]);      
    writedata([0x02]);      // -4.6V
    writedata([0x84]);      // AUTO mode

    writecommand(ST7735_PWCTR2);  // power control
    writedata([0xC5]);      // VGH25 = 2.4C VGSEL = -10 VGH = 3 * AVDD

    writecommand(ST7735_PWCTR3);  // power control
    writedata([0x0A]);      // Opamp current small 
    writedata([0x00]);      // Boost frequency

    writecommand(ST7735_PWCTR4);  // power control
    writedata([0x8A]);      // BCLK/2, Opamp current small & Medium low
    writedata([0x2A]);     
    console.log("initScreen 3");
    writecommand(ST7735_PWCTR5);  // power control
    writedata([0x8A]);    
    writedata([0xEE]);     

    writecommand(ST7735_VMCTR1);  // power control
    writedata([0x0E]);  

    writecommand(ST7735_INVOFF);    // don't invert display

    writecommand(ST7735_MADCTL);  // memory access control (directions)
    writedata([0xC8]);  // row address/col address, bottom to top refresh
    madctl = [0xC8];

    writecommand(ST7735_COLMOD);  // set color mode
    writedata([0x05]);        // 16-bit color

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

    writecommand(ST7735_GMCTRP1);
    writedata([0x0f]);
    writedata([0x1a]);
    writedata([0x0f]);
    writedata([0x18]);
    writedata([0x2f]);
    writedata([0x28]);
    writedata([0x20]);
    writedata([0x22]);
    writedata([0x1f]);
    writedata([0x1b]);
    writedata([0x23]);
    writedata([0x37]);
    writedata([0x00]);
    writedata([0x07]);
    writedata([0x02]);
    writedata([0x10]);
    writecommand(ST7735_GMCTRN1);
    writedata([0x0f]); 
    writedata([0x1b]); 
    writedata([0x0f]); 
    writedata([0x17]); 
    writedata([0x33]); 
    writedata([0x2c]); 
    writedata([0x29]); 
    writedata([0x2e]); 
    writedata([0x30]); 
    writedata([0x30]); 
    writedata([0x39]); 
    writedata([0x3f]); 
    writedata([0x00]); 
    writedata([0x07]); 
    writedata([0x03]); 
    writedata([0x10]); 
    console.log("initScreen 5");
    writecommand(ST7735_DISPON);
    spiBus.ZJSwait(100);

    writecommand(ST7735_NORON);  // normal display on
    spiBus.ZJSwait(10);
}
console.log("SPI test starting..");
try {

    initScreen();
   // commonInit(Rcmd1);
    //commandList(Rcmd2red);
    //commandList(Rcmd3);
    
    // Do I need direction to be set to write only?
    //writecommand(ST7735_MADCTL);
    //writedata(0xC0);

    //need a wait here?
    setTimeout(function(){
    console.log("BJONES done with init, drawing pixels..");

    drawPixel(64, 80, ST7735_GREEN);
    drawPixel(42, 53, ST7735_GREEN);
    drawPixel(32, 40, ST7735_GREEN);
    drawPixel(25, 32, ST7735_GREEN);
    
    /*drawPixel(width/2, height/2, ST7735_GREEN);
    drawPixel(width/3, height/3, ST7735_GREEN);
    drawPixel(width/4, height/4, ST7735_GREEN);
    drawPixel(width/5, height/5, ST7735_GREEN);*/
    }, 3000);

} catch (err) {
  console.log("SPI error: " + err.message);
}



