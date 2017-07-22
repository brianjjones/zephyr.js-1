// JS for using the SPI screen
//baud rate is set to 20mhz. or 9600?

// tft.initR(INITR_BLACKTAB);
// tft.drawPixel (300,300, ST7735_RED);


/*  writecommand(ST7735_MADCTL);
    writedata(0xC0);
    
*/

var width = 128;
var height = 160;
var ST7735_CASET =  0x2A;
var ST7735_RASET =  0x2B;
var ST7735_RAMWR =  0x2C;
var ST7735_MADCTL = 0x36;
// Color definitions
var	ST7735_BLACK =  0x0000;
var	ST7735_BLUE  =  0x001F;
var	ST7735_RED  =  0xF800;
var	ST7735_GREEN =  0x07E0;
var ST7735_CYAN  =  0x07FF;
var ST7735_MAGENTA= 0xF81F;
var ST7735_YELLOW=  0xFFE0;
var ST7735_WHITE =  0xFFFF;

var ystart = xstart = colstart  = rowstart = 0;
var gpio = require('gpio');
var spi = require("spi");
var spiBus = spi.open({speed:20000, bus:1, polarity:0, phase:0, bits:16});
var dcPin = gpio.open({pin: 'IO8', mode: 'in', edge: 'rising'});
var csPin = gpio.open({pin: 'IO10', mode: 'in', edge: 'rising'});

//function writecommand(uint8_t c) {
function writecommand(c) {
  /*DC_LOW();
  CS_LOW();
  spiwrite(c);
  CS_HIGH();*/

    dcPin.write(0);
    csPin.write(0);
    var buffer = spiBus.transceive(1, c);
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
    dcPin.write(1);
    csPin.write(0);
    var buffer = spiBus.transceive(1, c);
    csPin.write(1);  
}
//function setAddrWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
function setAddrWindow(x0, y0, x1, y1) {

  writecommand(ST7735_CASET); // Column addr set
  writedata(0x00);
  writedata(x0+xstart);     // XSTART 
  writedata(0x00);
  writedata(x1+xstart);     // XEND

  writecommand(ST7735_RASET); // Row addr set
  writedata(0x00);
  writedata(y0+ystart);     // YSTART
  writedata(0x00);
  writedata(y1+ystart);     // YEND

  writecommand(ST7735_RAMWR); // write to RAM
  
    
  
}

function drawPixel(x, y, color) {
//function drawPixel(int16_t x, int16_t y, uint16_t color) {

  if((x < 0) ||(x >= width) || (y < 0) || (y >= height)) return;

  setAddrWindow(x,y,x+1,y+1);

  dcPin.write(1);
  csPin.write(0);
  spiwrite(color >> 8);
  spiwrite(color);
  csPin.write(1);

}

console.log("SPI test starting..");
try {
    
    // Do I need direction to be set to write only?
    writecommand(ST7735_MADCTL);
    writedata(0xC0);

    //need a wait here?
    setTimeout(function(){  drawPixel(width/2, height/2, ST7735_GREEN);
    drawPixel(width/3, height/3, ST7735_GREEN);
    drawPixel(width/4, height/4, ST7735_GREEN);
    drawPixel(width/5, height/5, ST7735_GREEN);}, 3000);

} catch (err) {
  console.log("SPI error: " + err.message);
}



