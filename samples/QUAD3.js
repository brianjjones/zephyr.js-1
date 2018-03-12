var quad3API = {};
quad3API.init  = function (screen, x, y, w, h) {
    // var gpio = require("gpio");
    this.screen = screen;
    this.x = x;
    this.y = y;
    this.width = w;
    this.height = h;
    this.color = BLUE;
    this.screen.fillRect(this.x, this.y, this.width, this.height, RED);
    this.pin = gpio.open({pin: '2', mode: 'in', edge: 'any'});
    // this.checkTemp(); //Get the initial reading.
    this.pin.onchange = function(event) {

        quad3API.screen.fillRect(quad3API.x + (quad3API.width / 3),
            quad3API.y + (quad3API.height / 3), quad3API.width / 4,
            quad3API.width / 4, quad3API.color);

        quad3API.color = quad3API.color === RED ? BLUE : RED;
    };
}
module.exports = quad3API;
