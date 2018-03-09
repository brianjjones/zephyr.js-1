function QUAD1() {
    var quad1API = {};
    console.log("QUAD1 1");
    quad1API.init  = function (screen, x, y, w, h) {
        console.log("QUAD1 2");
        this.screen = screen;
        this.x = x;
        this.y = y;
        this.width = w;
        this.height = h;
        this.screen.fillRect(this.x, this.y, this.width, this.height, GREEN);
        console.log("QUAD1 3");
    }

    // var aio = require("aio");
    // // pins
    // var pinA = aio.open({ pin: 'A0' });
    //
    // setInterval(function () {
    //     var rawValue = pinA.read();
    //     if (rawValue == 0) {
    //         console.log("PinA: invalid temperature value");
    //     } else {
    //         var voltage = (rawValue / 4096.0) * 3.3;
    //         var celsius = (voltage - 0.5) * 100 + 0.5;
    //         celsius = celsius | 0;
    //         console.log("PinA: temperature in Celsius is: " + celsius);
    //     }
    // }, 1000);
    console.log("QUAD1 4");
    return quad1API;
}
console.log("QUAD1 5");
module.exports.QUAD1 = new QUAD1();
