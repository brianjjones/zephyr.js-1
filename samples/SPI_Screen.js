// JS for using the SPI screen
//baud rate is set to 20mhz. or 9600?

// tft.initR(INITR_BLACKTAB);
// tft.drawPixel (300,300, ST7735_RED);


/*  writecommand(ST7735_MADCTL);
    writedata(0xC0);
    
*/


// BJONES TODO - Make good use of Geoff's newly added buffer fill.  Create a way to draw squares, lines, circles, pixels.
// Look up a way to draw characters.  How to get fonts, how to draw them, etc.
// Go back to fixing bugs after I upload
// Upload SPI_Screen and OLED.js, also SPI fixes.
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
var	ST7735_GREEN =  [0x07,0xE0];
var ST7735_CYAN  =  [0x07,0xFF];
var ST7735_MAGENTA= [0xF8,0x1F];
var ST7735_YELLOW=  [0xFF,0xE0];
var ST7735_WHITE =  [0xFF,0xFF];
//var black_arr = [0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00];
var black_arr = [0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00];
var ystart = xstart = colstart  = rowstart = 0;
var gpio = require('gpio');
var spi = require("spi");
var spiBus = spi.open({speed:5, bus:1, polarity:0, phase:0, bits:8});
var dcPin = gpio.open(8);
var csPin = gpio.open(4);
var rstPin = gpio.open(7);
//var dcPin = gpio.open({pin: 'IO8', mode: 'in'});
//var csPin = gpio.open({pin: 'IO10', mode: 'in'});
//var rstPin = gpio.open({pin: 'IO7', mode: 'in'}); //, edge: 'rising'
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
    var buffer = spiBus.transceive(1, c, "write");
    csPin.write(1);  
}

function writedata2(c) {
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
    var buf = Buffer(1000);
    buf.fill(0x00000000);
    var buffer = spiBus.transceive(1, buf, "write");
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
/*
function fillScreen(color) {
  setAddrWindow(0, 0, width-1, height-1);

  // setup for data
  dcPin.write(1);
  csPin.write(0);
  console.log("BJONES about to fill");
  var arr = [];
  var i = 0;
  //for (var x=0; x < width; x++) {
    //for (var y=0; y < height; y++) {
    for (var x=0; x < 5; x++) {
    for (var y=0; y < 5; y++) {
    //console.log("BJONES filling " + x + " and " + y);
    arr[i] = color;
    arr[i+1] = color;
    i++;
    //buffer = spiBus.transceive(1, color, "write");
   // buffer = spiBus.transceive(1, [color >> 8], "write");
   // buffer = spiBus.transceive(1, [color], "write");   
    }
  }
    console.log("array created");
    buffer = spiBus.transceive(1, color, "write");
    csPin.write(1);
    console.log("BJONES done with fill");
}
*/

function fillScreen(x1,y1,x2,y2,color){
     var i = 0;
     var arr = [];
  /*  for (var x=0; x < x2 - x1; x++) {
        for (var y=0; y < y2 -y1; y++) {        
        arr[i] = 0x00;
        arr[i+1] = 0x00;
        i++;   
        }
    }*/
     // csPin.write(0);
      //spi.write(0x2A,dc);
     // dcPin.write(1);
      writecommand([0x2A]);      
      writedata([0,x1,0,x2]);
     // dcPin.write(0);
      //spi.write(0,x1,0,x2);
     // dcPin.write(1);
      writecommand([0x2B]);
      writedata([0,y1,0,y2]);
     // dcPin.write(0);
      //spi.write(0x2B,dc);
      //spi.write(0,y1,0,y2);
    //  dcPin.write(1);
      writecommand([0x2C]);
      //spi.write(0x2C,dc);
      //spi.write({data:String.fromCharCode(c>>8,c), count:(x2-x1+1)*(y2-y1+1)});
    for ( var j = 0; j < (y2-y1)/2 ; j++)      
        writedata2(black_arr);
    
        //  writedata([0x00,0x00]);
  
  
     // csPin.write(1);
}

function fillScreen2(x1,y1,x2,y2,color){
     var i = 0;
     var arr = [];
  /*  for (var x=0; x < x2 - x1; x++) {
        for (var y=0; y < y2 -y1; y++) {        
        arr[i] = 0x00;
        arr[i+1] = 0x00;
        i++;   
        }
    }*/
     // csPin.write(0);
      //spi.write(0x2A,dc);
     // dcPin.write(1);
      writecommand([0x2A]);      
      writedata([0,x1,0,x2]);
     // dcPin.write(0);
      //spi.write(0,x1,0,x2);
     // dcPin.write(1);
      writecommand([0x2B]);
      writedata([0,y1,0,y2]);
     // dcPin.write(0);
      //spi.write(0x2B,dc);
      //spi.write(0,y1,0,y2);
    //  dcPin.write(1);
      writecommand([0x2C]);
      //spi.write(0x2C,dc);
      //spi.write({data:String.fromCharCode(c>>8,c), count:(x2-x1+1)*(y2-y1+1)});
    for ( var j = 0; j < (x2-x1)*(y2-y1) ; j++)      
        writedata([0x00,0x1F]);
    
        //  writedata([0x00,0x00]);
  
  
     // csPin.write(1);
}

function drawPixel(x, y, color) {
//function drawPixel(int16_t x, int16_t y, uint16_t color) {
  console.log("drawPixel x=" + x + " y=" + y + " color=" + color);
  if((x < 0) ||(x >= width) || (y < 0) || (y >= height)) return;

  setAddrWindow(x,y,x+1,y+1);
  console.log("BJONES setAddrWindow done");
  dcPin.write(1);
  csPin.write(0);
  buffer = spiBus.transceive(1, color, "write");
  /*buffer = spiBus.transceive(1, [color >> 8], "write");
  buffer = spiBus.transceive(1, [color], "write");*/
  //spiwrite(color >> 8);
  //spiwrite(color);
  csPin.write(1);

}

function initScreen()
{
    csPin.write(0);
    rstPin.write(1);
      
  //if (_rst != -1) {
    //pinMode(_rst, OUTPUT);
    /*rstPin.write(1);
    //spiBus.ZJSwait(500);
    spiBus.ZJSwait(500);
    rstPin.write(0);
    spiBus.ZJSwait(500);
    rstPin.write(1);
    spiBus.ZJSwait(500);*/
  //}

    //writecommand(0x01);

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

   
    /*
    writecommand(ST7735_FRMCTR2);  // frame rate control - idle mode
    writedata([0x0e]);  // frame rate = fosc / (1 x 2 + 40) * (LINE + 2C + 2D)
    writedata([0x10]); 
    //writedata([0x2D]); 

    writecommand(ST7735_FRMCTR3);  // frame rate control - partial mode
    writedata([0x01]); // dot inversion mode
    writedata([0x2C]); 
    writedata([0x2D]); 
    writedata([0x01]); // line inversion mode
    writedata([0x2C]); 
    writedata([0x2D]);
    */
    console.log("initScreen 2");
   // writecommand(ST7735_INVCTR);  // display inversion control
    //writedata([0x07]);  // no inversion

    writecommand(ST7735_PWCTR1);  // power control
     writedata([0x08]);
      writedata([0]);      
    //writedata([0xA2]);      
    //writedata([0x02]);      // -4.6V
    //writedata([0x84]);      // AUTO mode

    writecommand(ST7735_PWCTR2);  // power control
    writedata([0x05]);
    //writedata([0xC5]);      // VGH25 = 2.4C VGSEL = -10 VGH = 3 * AVDD
/*
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
*/
    writecommand(ST7735_VMCTR1);  // power control
    writedata([0x38]);
    writedata([0x40]);  
    //writedata([0x0E]);  

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
   /* writedata([0x0f]);
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
    writedata([0x10]); */
    writecommand(ST7735_GMCTRN1);
    writedata([0x00,0x1b,0x1f,0x0f,0x16,0x13,0x31,0x84,0x43,0x06,0x1d,0x21,0x3d,0x3e,0x3f]);
    /*writedata([0x0f]); 
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
    writedata([0x10]); */
    console.log("initScreen 5");
    writecommand(ST7735_DISPON);
    writecommand(ST7735_RAMWR);
    spiBus.ZJSwait(100);

    //writecommand(ST7735_NORON);  // normal display on
   // spiBus.ZJSwait(10);
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

    fillScreen(0, 0, 70, 70, ST7735_BLACK);
    fillScreen2(70, 70, 100, 100, ST7735_RED);
    
    drawPixel(64, 80, ST7735_GREEN);
    drawPixel(42, 53, ST7735_GREEN);
    drawPixel(32, 40, ST7735_GREEN);
    drawPixel(25, 32, ST7735_GREEN);
    
    /*drawPixel(width/2, height/2, ST7735_GREEN);
    drawPixel(width/3, height/3, ST7735_GREEN);
    drawPixel(width/4, height/4, ST7735_GREEN);
    drawPixel(width/5, height/5, ST7735_GREEN);*/
    }, 100);

} catch (err) {
  console.log("SPI error: " + err.message);
}



