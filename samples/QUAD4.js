var quad4API = {};
quad4API.init  = function (screen, x, y, w, h) {
    this.screen = screen;
    this.x = x;
    this.y = y;
    this.width = w;
    this.height = h;
    this.screen.fillRect(this.x, this.y, this.width, this.height, CYAN);
    this.screen.drawString(this.x + 4,
                         this.y + 2,
                         "Hello", RED, 3);
    this.screen.drawString(this.x + 4,
                        this.y + 25, "PDX!", MAGENTA, 3);
}
module.exports = quad4API;
