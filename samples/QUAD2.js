var quad2API = {};
quad2API.init  = function (screen, x, y, w, h) {
    this.screen = screen;
    this.x = x;
    this.y = y;
    this.width = w;
    this.height = h;
    this.screen.fillRect(this.x, this.y, this.width, this.height, YELLOW);
    this.pin = aio.open({ pin: 'A3' });
    this.checkAngle(); //Get the initial reading.
    //this.pin.on('change', this.checkAngle.bind(this));
    // this.id = setInterval(this.checkAngle.bind(this), 500);
    // console.log("QUAD1 3 + " + this.pin.read());
}
quad2API.checkAngle = function()
{
    var rawValue = this.pin.read();
    rawValue *= 10;
    rawValue += ' ';
    console.log(rawValue);
    this.screen.fillRect(this.x, this.y + this.height / 3, this.width,
                         this.height / 4, YELLOW);
    this.screen.drawString(this.x + this.width / 5, this.height / 5,
                           rawValue+'', BLACK, 2);
}
module.exports = quad2API;
