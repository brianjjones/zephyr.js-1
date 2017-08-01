// Copyright (c) 2017, Intel Corporation.
// JavaScript library for the BMP280 sensor

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


function ST7735() {

    // API object
    var st7735API = {};

    var gpio = require('gpio');
    var spi = require("spi");
    var spiBus = spi.open({bus:1, polarity:0, phase:0, bits:8});
    st7735API.dcPin = gpio.open(8);   // Command / Data select pin
    st7735API.csPin = gpio.open(4);   // SPI slave pin
    st7735API.rstPin = gpio.open(7);  // Reset pin
    st7735API.spiBus = spi.open({bus:1, polarity:0, phase:0, bits:8});

    st7735API.writecommand = function(comm) {  
        this.dcPin.write(0);
        this.csPin.write(0);
        this.spiBus.transceive(1, comm, "write");
        this.csPin.write(1);
    }

    // Send data over SPI
    st7735API.writedata = function(data) {
        this.dcPin.write(1);
        this.csPin.write(0);    
        this.spiBus.transceive(1, data, "write");
        this.csPin.write(1);  
    }

    // Sets which pixels we are going to send data for
    st7735API.setAddrWindow = function(x0, y0, x1, y1) {
      this.writecommand(ST7735_CASET); // Column addr set
      this.writedata([0,x0,0,x1]);

      this.writecommand(ST7735_RASET); // Row addr set
      this.writedata([0,y0,0,y1]);
      
      this.writecommand(ST7735_RAMWR); // Save values to RAM  
    }

    st7735API.fillRect = function(x0, y0, x1, y1, color) {
        // Check for invalid pixel location
        if((x0 < 0 || x0 > x1) || (x1 >= width) ||
           (y0 < 0 || y0 > y1) || (y1 >= height)) {
            console.log("Invalid pixel location ");
            return;
        }
                
        this.setAddrWindow(x0, y0, x1, y1);
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
            this.writedata(buf);
    }

    st7735API.drawPixel = function(x, y, color) {
      // Check for invalid pixel location
      if((x < 0) || (x >= width) || (y < 0) || (y >= height)) {
        console.log("Invalid pixel location");
        return;
      }

      this.setAddrWindow(x,y,x+1,y+1);
      this.writedata(color);
    }

    st7735API.initScreen = function() {
        this.csPin.write(0);
        this.rstPin.write(1);        
        this.writecommand(ST7735_SWRESET);
        this.writecommand(ST7735_SLPOUT);  // exit sleep        
        this.writecommand([0x26]);
        this.writedata([0x04]);
        this.writecommand(ST7735_FRMCTR1);  
        this.writedata([0x0e]);  
        this.writedata([0x10]); 
        this.writecommand(ST7735_PWCTR1);  // Power control
        this.writedata([0x08]);
        this.writedata([0]);
        this.writecommand(ST7735_PWCTR2);  // Power control
        this.writedata([0x05]);
        this.writecommand(ST7735_VMCTR1);  // Power control
        this.writedata([0x38]);
        this.writedata([0x40]);
        this.writecommand(ST7735_INVOFF);  // Don't invert display
        this.writecommand(ST7735_COLMOD);  // Set color mode
        this.writedata([0x05]);            // 16-bit color    
        this.writecommand(ST7735_MADCTL);  // If not set the colors will be inverted
        this.writedata([0xC0]);    
        this.writecommand(ST7735_CASET);   // Collumn addr set
        this.writedata([0x00]);
        this.writedata([0x00]);
        this.writedata([0x00]);
        this.writedata([0x7F]);    
        this.writecommand(ST7735_RASET);  // Row addr set
        this.writedata([0x00]);
        this.writedata([0x00]);
        this.writedata([0x00]);
        this.writedata([0x9F]); 
        this.writecommand(ST7735_INVCTR);  // Display inversion control
        this.writedata([0x00]);
        this.writecommand([0xF2]);
        this.writedata([1]);    
        this.writecommand(ST7735_GMCTRP1);
        this.writedata([0x3f,0x22,0x20,0x30,0x29,0x0c,0x4e,0xb7,0x3c,0x19,0x22,0x1e,0x02,0x01,0x00]);
        this.writecommand(ST7735_GMCTRN1);
        this.writedata([0x00,0x1b,0x1f,0x0f,0x16,0x13,0x31,0x84,0x43,0x06,0x1d,0x21,0x3d,0x3e,0x3f]);    
        this.writecommand(ST7735_DISPON);
        this.writecommand(ST7735_RAMWR);
    }

    return st7735API;
};

module.exports.ST7735 = new ST7735();
