// JS for using the SPI screen

var timer = false;
var block = false;
var width = 128;
var height = 160;
var maxPixels = width * height;

//var ST7735_NOP = [0x0];
var ST7735_SWRESET = [0x01];
//var ST7735_RDDID = [0x04];
//var ST7735_RDDST = [0x09];

//var ST7735_SLPIN  = [0x10];
var ST7735_SLPOUT  = [0x11];
//var ST7735_PTLON  = [0x12];
//var ST7735_NORON  = [0x13];

var ST7735_INVOFF = [0x20];
//var ST7735_INVON = [0x21];
//var ST7735_DISPOFF = [0x28];
var ST7735_DISPON = [0x29];
var ST7735_CASET = [0x2A];  // column addr set
var ST7735_RASET = [0x2B];  // row addr set
var ST7735_RAMWR = [0x2C];
//var ST7735_RAMRD = [0x2E];

var ST7735_COLMOD = [0x3A];
var ST7735_MADCTL = [0x36];

var ST7735_FRMCTR1 = [0xB1];
//var ST7735_FRMCTR2 = [0xB2];
//var ST7735_FRMCTR3 = [0xB3];
var ST7735_INVCTR = [0xB4];
//var ST7735_DISSET5 = [0xB6];

var ST7735_PWCTR1 = [0xC0];
var ST7735_PWCTR2 = [0xC1];
//var ST7735_PWCTR3 = [0xC2];
//var ST7735_PWCTR4 = [0xC3];
//var ST7735_PWCTR5 = [0xC4];
var ST7735_VMCTR1 = [0xC5];

/*var ST7735_RDID1 = [0xDA];
var ST7735_RDID2 = [0xDB];
var ST7735_RDID3 = [0xDC];
var ST7735_RDID4 = [0xDD];
*/
//var ST7735_PWCTR6 = [0xFC];

var ST7735_GMCTRP1 = [0xE0];
var ST7735_GMCTRN1 = [0xE1];

// Color definitions
var	BLACK =  [0x00,0x00];
var	BLUE  =  [0x00,0x1F];
var	RED   =  [0xF8,0x00];
var	GREEN =  [0x07,0xE0];
var CYAN  =  [0x07,0xFF];
var MAGENTA = [0xF8,0x1F];
var YELLOW =  [0xFF,0xE0];
var WHITE =  [0xFF,0xFF];

var gpio = require('gpio');
var spi = require("spi");
var spiBus = spi.open({bus:1, polarity:0, phase:0, bits:8});
var dcPin = gpio.open(8);   // Command / Data select pin
var csPin = gpio.open(4);   // SPI slave pin
var rstPin = gpio.open(7);  // Reset pin

// Send a command over SPI
function writecommand(command) {
    dcPin.write(0);
    csPin.write(0);
    spiBus.transceive(1, command, "write");
    csPin.write(1);
}

// Send data over SPI
function writedata(data) {
    dcPin.write(1);
    csPin.write(0);    
    spiBus.transceive(1, data, "write");
    csPin.write(1);  
}

// Sets which pixels we are going to send data for
function setAddrWindow(x0, y0, x1, y1)
{  
  writecommand(ST7735_CASET); // Column addr set
  writedata([0,x0,0,x1]);

  writecommand(ST7735_RASET); // Row addr set
  writedata([0,y0,0,y1]);
  
  writecommand(ST7735_RAMWR); // Save values to RAM  
}

function fillRect(x0, y0, x1, y1, color){
    // Check for invalid pixel location
    if((x0 < 0 || x0 > x1) || (x1 >= width) ||
       (y0 < 0 || y0 > y1) || (y1 >= height)) {
        console.log("Invalid pixel location ");
        return;
    }
            
    setAddrWindow(x0, y0, x1, y1);
    var w = x1 - x0;
    var h = y1 - y0;
    var pixels = w * h * 2; // Each pixel has two bytes of data
    var passes = 1;     // Number of times the data buffer needs to be sent

    if (pixels > maxPixels) {
        var tmpPass = pixels / maxPixels;
        var whole = parseInt(tmpPass);
        var dec = tmpPass - whole;
        if (dec > 0.0) {            
            passes += 1;            
        }
        pixels = maxPixels; // Set pixels to the maximum size
    }
    
    var buf = Buffer(pixels);
    var bufColor = Buffer([color[0], color[1], color[0], color[1]]);       
    buf.fill(bufColor);
        
    for ( var i = 0; i < passes ; i++)      
        writedata(buf);
}

function drawPixel(x, y, color) {
  // Check for invalid pixel location
  if((x < 0) || (x >= width) || (y < 0) || (y >= height)) {
    console.log("Invalid pixel location");
    return;
  }

  setAddrWindow(x,y,x+1,y+1);
  writedata(color);
}

function initScreen()
{
    csPin.write(0);
    rstPin.write(1);
    
    writecommand(ST7735_SWRESET);
    
    //spiBus.ZJSwait(100);

    writecommand(ST7735_SLPOUT);  // exit sleep
    //spiBus.ZJSwait(50);
    writecommand([0x26]);
    writedata([0x04]);
    writecommand(ST7735_FRMCTR1);  
    writedata([0x0e]);  
    writedata([0x10]); 
    writecommand(ST7735_PWCTR1);  // Power control
    writedata([0x08]);
    writedata([0]);
    writecommand(ST7735_PWCTR2);  // Power control
    writedata([0x05]);
    writecommand(ST7735_VMCTR1);  // Power control
    writedata([0x38]);
    writedata([0x40]);
    writecommand(ST7735_INVOFF);  // Don't invert display
    writecommand(ST7735_COLMOD);  // Set color mode
    writedata([0x05]);            // 16-bit color    
    writecommand(ST7735_MADCTL);  // If not set the colors will be inverted
    writedata([0xC0]);    
    writecommand(ST7735_CASET);   // Collumn addr set
    writedata([0x00]);
    writedata([0x00]);
    writedata([0x00]);
    writedata([0x7F]);    
    writecommand(ST7735_RASET);  // Row addr set
    writedata([0x00]);
    writedata([0x00]);
    writedata([0x00]);
    writedata([0x9F]); 
    writecommand(ST7735_INVCTR);  // Display inversion control
    writedata([0x00]);
    writecommand([0xF2]);
    writedata([1]);    
    writecommand(ST7735_GMCTRP1);
    writedata([0x3f,0x22,0x20,0x30,0x29,0x0c,0x4e,0xb7,0x3c,0x19,0x22,0x1e,0x02,0x01,0x00]);
    writecommand(ST7735_GMCTRN1);
    writedata([0x00,0x1b,0x1f,0x0f,0x16,0x13,0x31,0x84,0x43,0x06,0x1d,0x21,0x3d,0x3e,0x3f]);    
    writecommand(ST7735_DISPON);
    writecommand(ST7735_RAMWR);

    //spiBus.ZJSwait(100);
}
console.log("SPI test starting..");
try {

    initScreen();

    fillRect(0, 0, width - 1, height - 1, BLACK);
    fillRect(70, 70, 100, 100, RED);
    
    drawPixel(64, 80, RED);
    drawPixel(42, 53, CYAN);
    drawPixel(32, 40, MAGENTA);
    drawPixel(25, 32, YELLOW);

} catch (err) {
  console.log("SPI error: " + err.message);
}



