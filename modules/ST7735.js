// Copyright (c) 2017, Intel Corporation.
// JavaScript library for the ST7735 sensor
// Default setup:
// DC - Pin 8
// CS - Pin 4
// RST - Pin 7
// SCL - Pin 13
// SDA - Pin 11

function ST7735() {

    // API object
    var st7735API = {};
    st7735API.width = 128;
    st7735API.height = 160;
    st7735API.maxPixels = st7735API.width * st7735API.height;

    var gpio = require('gpio');
    var spi = require("spi");
    var board = require('board');
    var busSpeed = board.name === "arduino_101" ? 16000000 : 32000000;
    // You can reset these pins from your JS if you want
    st7735API.dcPin = gpio.open(8);   // Command / Data select pin
    st7735API.csPin = gpio.open(4);   // SPI slave pin
    st7735API.rstPin = gpio.open(7);  // Reset pin
    st7735API.spiBus = spi.open({bus:1, speed:busSpeed, polarity:0, phase:0, bits:8});

    st7735API.cmdAddrs = {
        SWRESET: [0x01],    // Software reset
        SLPOUT: [0x11],     // Exit sleep
        INVOFF: [0x20],     // Don't invert display
        DISPON: [0x29],     // Turn on display
        CASET: [0x2A],      // Set column addr
        RASET: [0x2B],      // Set row addr
        RAMWR: [0x2C],      // Save values to RAM
        COLMOD: [0x3A],     // Color mode
        MADCTL: [0x36],
        FRMCTR1: [0xB1],
        INVCTR: [0xB4],     // Display inversion control
        PWCTR1: [0xC0],     // Power control
        PWCTR2: [0xC1],     // Power control
        VMCTR1: [0xC5],     // Power control
        GMCTRP1: [0xE0],
        GMCTRN1: [0xE1]
    }

    st7735API.writeCommand = function(comm) {
        this.dcPin.write(0);    // Set DC pin low to indicate command
        this.csPin.write(0);    // Set CS pin low to start transmission
        this.spiBus.transceive(1, comm, "write");
        this.csPin.write(1);    // Set CS pin high to end transmission
    }

    // Send data over SPI
    st7735API.writeData = function(data) {
        this.dcPin.write(1);    // Set DC pin low to indicate data
        this.csPin.write(0);
        this.spiBus.transceive(1, data, "write");
        this.csPin.write(1);
    }

    // Sets which pixels we are going to send data for
    st7735API.setAddrWindow = function(x0, y0, x1, y1) {
      this.writeCommand(this.cmdAddrs.CASET); // Set column addrs
      this.writeData([0, x0, 0, x1]);
      this.writeCommand(this.cmdAddrs.RASET); // Set row addrs
      this.writeData([0, y0, 0, y1]);
      this.writeCommand(this.cmdAddrs.RAMWR); // Save values to RAM
    }

    st7735API.fillRect = function(x0, y0, x1, y1, color) {
        // Check for invalid pixel location
        if((x0 < 0 || x0 > x1) || (x0 >= this.width) ||
           (y0 < 0 || y0 > y1) || (y0 >= this.height)) {
            console.log("Invalid pixel location " + x0 + ", " + y0 + ", " + x1 + ", " + y1);
            return;
        }

        this.setAddrWindow(x0, y0, x1, y1);
        var w = x1 - x0 + 1;
        var h = y1 - y0 + 1;
        var pixels = w * h * 2; // Each pixel has two bytes of data
        var passes = 1;         // Number of times the data buffer needs to be sent

        if (pixels > this.maxPixels) {
            var tmpPass = pixels / this.maxPixels;
            var whole = parseInt(tmpPass);
            var dec = tmpPass - whole;
            if (dec > 0.0) {
                passes += 1;
            }
            pixels = this.maxPixels; // Set pixels to the maximum size
        }

        var buf = Buffer(pixels);
        var bufColor = Buffer([color[0], color[1], color[0], color[1]]);
        buf.fill(bufColor);

        for ( var i = 0; i < passes ; i++)
            this.writeData(buf);
    }

    st7735API.drawCircle = function(x0, y0, radius, color) {
        var f = 1 - radius;
        var ddF_x = 1;
        var ddF_y = -2 * radius;
        var x = 0;
        var y = radius;

        this.drawPixel(x0  , y0+radius, color);
        this.drawPixel(x0  , y0-radius, color);
        this.drawPixel(x0+radius, y0  , color);
        this.drawPixel(x0-radius, y0  , color);

        while (x<y) {
            if (f >= 0) {
                y--;
                ddF_y += 2;
                f += ddF_y;
            }
            x++;
            ddF_x += 2;
            f += ddF_x;

            this.drawPixel(x0 + x, y0 + y, color);
            this.drawPixel(x0 - x, y0 + y, color);
            this.drawPixel(x0 + x, y0 - y, color);
            this.drawPixel(x0 - x, y0 - y, color);
            this.drawPixel(x0 + y, y0 + x, color);
            this.drawPixel(x0 - y, y0 + x, color);
            this.drawPixel(x0 + y, y0 - x, color);
            this.drawPixel(x0 - y, y0 - x, color);
        }
    }

    st7735API.drawPixel = function(x, y, color) {
        // Check for invalid pixel location
        if((x < 0) || (x >= this.width) || (y < 0) || (y >= this.height)) {
            console.log("Invalid pixel location " + x + ", " + y);
            return;
        }

        this.setAddrWindow(x, y, x + 1, y + 1);
        this.writeData(color);
    }

    st7735API.drawLine = function(x0, y0, x1, y1, color) {
      // Check for invalid pixel location
        if((x0 < 0 || x1 < 0) || (x0 >= this.width || x1 >= this.width) ||
           (y0 < 0 || y1 < 0) || (y0 >= this.height || y1 >= this.height)) {
            console.log("Invalid pixel location " + x0 + ", " + y0 + ", " + x1 + ", " + y1);
            return;
        }

        var xLen = x1 > x0 ? x1 - x0 : x0 - x1;
        var yLen = y1 > y0 ? y1 - y0 : y0 - y1;
        xLen = xLen === 0 ? 1 : xLen; // Line width has to be at least a pixel
        yLen = yLen === 0 ? 1 : yLen;

        if (xLen > yLen) {
            if (x0 > x1) {  // Swap if x0 is greater
                var tmp = x0; x0 = x1; x1 = tmp;
                tmp = y0; y0 = y1; y1 = tmp;
            }
            var pos = y0;
            var step = yLen / xLen;

            for (var x = x0; x <= x1; x++) {
                this.drawPixel(x, parseInt(pos), color);
                pos += step;
            }
        }
        else {
            if (y0 > y1) {
                var tmp = x0; x0 = x1; x1 = tmp;
                tmp = y0; y0 = y1; y1 = tmp;
            }

            var pos = x0;
            var step = xLen / yLen;

            for (var y = y0; y <= y1; y++) {
                this.drawPixel(parseInt(pos), y, color);
                pos += step;
            }
        }
    }

    st7735API.initScreen = function() {
        this.csPin.write(0);
        this.rstPin.write(1);
        this.writeCommand(this.cmdAddrs.SWRESET);
        this.writeCommand(this.cmdAddrs.SLPOUT);  // exit sleep
        this.writeCommand([0x26]);
        this.writeData([0x04]);
        this.writeCommand(this.cmdAddrs.FRMCTR1);
        this.writeData([0x0e]);
        this.writeData([0x10]);
        this.writeCommand(this.cmdAddrs.PWCTR1);
        this.writeData([0x08]);
        this.writeData([0]);
        this.writeCommand(this.cmdAddrs.PWCTR2);
        this.writeData([0x05]);
        this.writeCommand(this.cmdAddrs.VMCTR1);
        this.writeData([0x38]);
        this.writeData([0x40]);
        this.writeCommand(this.cmdAddrs.INVOFF);
        this.writeCommand(this.cmdAddrs.COLMOD);
        this.writeData([0x05]);                   // Use 16 bit color mode
        this.writeCommand(this.cmdAddrs.MADCTL);  // If not set the colors will be inverted
        this.writeData([0xC0]);
        this.writeCommand(this.cmdAddrs.CASET);   // Set number of columns
        this.writeData([0x00]);
        this.writeData([0x00]);
        this.writeData([0x00]);
        this.writeData([0x7F]);
        this.writeCommand(this.cmdAddrs.RASET);  // Set number of rows
        this.writeData([0x00]);
        this.writeData([0x00]);
        this.writeData([0x00]);
        this.writeData([0x9F]);
        this.writeCommand(this.cmdAddrs.INVCTR);
        this.writeData([0x00]);
        this.writeCommand([0xF2]);
        this.writeData([1]);
        this.writeCommand(this.cmdAddrs.GMCTRP1);
        this.writeData([0x3f, 0x22, 0x20, 0x30, 0x29, 0x0c, 0x4e, 0xb7, 0x3c, 0x19, 0x22, 0x1e, 0x02, 0x01, 0x00]);
        this.writeCommand(this.cmdAddrs.GMCTRN1);
        this.writeData([0x00, 0x1b, 0x1f, 0x0f, 0x16, 0x13, 0x31, 0x84, 0x43, 0x06, 0x1d, 0x21, 0x3d, 0x3e, 0x3f]);
        this.writeCommand(this.cmdAddrs.DISPON);
        this.writeCommand(this.cmdAddrs.RAMWR);
    }

    return st7735API;
};

module.exports.ST7735 = new ST7735();
