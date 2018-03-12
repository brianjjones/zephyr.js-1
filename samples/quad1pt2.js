function QUAD1() {
    var quad1API = {};
        quad1API.screen = null;
        quad1API.x = 0;
        quad1API.y = 0;
        quad1API.width = 0;
        quad1API.height = 0;
        quad1API.id = null;
    quad1API.init  = function (screen, x, y, w, h) {
        this.screen = screen;
        this.x = x;
        this.y = y;
        this.width = w;
        this.height = h;
        this.screen.fillRect(this.x, this.y, this.width, this.height, GREEN);
        this.pin = aio.open({ pin: 'A2' });
        this.checkTemp(); //Get the initial reading.
        // this.id = setInterval(this.checkAngle.bind(this), 2000);
        // this.pin.on('change', this.checkTemp.bind(this));
        this.id = setInterval(this.checkTemp.bind(this), 5000);
    }
    quad1API.checkTemp = function()
    {
        var rawValue = this.pin.read();
        if (rawValue == 0) {
            console.log("Invalid temperature value");
        } else {
            var voltage = (rawValue * 3.3) / 1024;
            var celsius = (voltage - 0.5) * 10;
            celsius = celsius | 0;
            console.log(celsius + "C");
            this.screen.fillRect(this.x, this.height / 3, this.width,
                                 this.height / 3, GREEN);
            this.screen.drawString(this.width / 5, this.height / 5,
                                   celsius + "C" , BLUE, 2);
        }
    }
    return quad1API;
}
module.exports.QUAD1 = new QUAD1();
console.log("QUAD1 LOADED");
