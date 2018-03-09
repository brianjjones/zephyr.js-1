function QUAD2() {
    var quad2API = {};
    quad2API.init  = function (screen, x, y, w, h) {
        this.screen = screen;
        this.x = x;
        this.y = y;
        this.width = w;
        this.height = h;
        this.screen.fillRect(this.x, this.y, this.width, this.height, YELLOW);
    }
    return quad2API;
}

module.exports.QUAD2 = new QUAD2();
