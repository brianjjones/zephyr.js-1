function ST7735(){var t={width:128,height:160};t.maxPixels=this.width*this.height;var i=require("gpio"),s=require("spi"),r=require("board"),d="arduino_101"===r.name?16e6:32e6,e="arduino_101"===r.name?1:0,a="arduino_101"===r.name?8:9;return t.dcPin=i.open(a),t.csPin=i.open(4),t.rstPin=i.open(7),t.spiBus=s.open({bus:e,speed:d,polarity:0,phase:0,bits:8}),t.cmdAddrs={SWRESET:[1],SLPOUT:[17],INVOFF:[32],DISPON:[41],CASET:[42],RASET:[43],RAMWR:[44],COLMOD:[58],MADCTL:[54],FRMCTR1:[177],INVCTR:[180],PWCTR1:[192],PWCTR2:[193],VMCTR1:[197],GMCTRP1:[224],GMCTRN1:[225]},t.writeCommand=function(t){this.dcPin.write(0),this.csPin.write(0),this.spiBus.transceive(1,t,"write"),this.csPin.write(1)},t.writeData=function(t){this.dcPin.write(1),this.csPin.write(0),this.spiBus.transceive(1,t,"write"),this.csPin.write(1)},t.drawCB=function(t,i,s,r,d){this.setAddrWindow(t,i,t+s-1,i+r-1),this.dcPin.write(1),this.csPin.write(0),this.spiBus.transceive(1,d,"write"),this.csPin.write(1)},t.setAddrWindow=function(t,i,s,r){this.writeCommand(this.cmdAddrs.CASET),this.writeData([0,t,0,s]),this.writeCommand(this.cmdAddrs.RASET),this.writeData([0,i,0,r]),this.writeCommand(this.cmdAddrs.RAMWR)},t.initScreen=function(){this.csPin.write(0),this.rstPin.write(1),this.writeCommand(this.cmdAddrs.SWRESET),this.writeCommand(this.cmdAddrs.SLPOUT),this.writeCommand([38]),this.writeData([4]),this.writeCommand(this.cmdAddrs.FRMCTR1),this.writeData([14]),this.writeData([16]),this.writeCommand(this.cmdAddrs.PWCTR1),this.writeData([8]),this.writeData([0]),this.writeCommand(this.cmdAddrs.PWCTR2),this.writeData([5]),this.writeCommand(this.cmdAddrs.VMCTR1),this.writeData([56]),this.writeData([64]),this.writeCommand(this.cmdAddrs.INVOFF),this.writeCommand(this.cmdAddrs.COLMOD),this.writeData([5]),this.writeCommand(this.cmdAddrs.MADCTL),this.writeData([192]),this.writeCommand(this.cmdAddrs.CASET),this.writeData([0]),this.writeData([0]),this.writeData([0]),this.writeData([127]),this.writeCommand(this.cmdAddrs.RASET),this.writeData([0]),this.writeData([0]),this.writeData([0]),this.writeData([159]),this.writeCommand(this.cmdAddrs.INVCTR),this.writeData([0]),this.writeCommand([242]),this.writeData([1]),this.writeCommand(this.cmdAddrs.GMCTRP1),this.writeData([63,34,32,48,41,12,78,183,60,25,34,30,2,1,0]),this.writeCommand(this.cmdAddrs.GMCTRN1),this.writeData([0,27,31,15,22,19,49,132,67,6,29,33,61,62,63]),this.writeCommand(this.cmdAddrs.DISPON),this.writeCommand(this.cmdAddrs.RAMWR)},t}module.exports.ST7735=new ST7735;
