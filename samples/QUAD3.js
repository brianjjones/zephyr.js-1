function QUAD3() {
    var quad3API = {};
    console.log("QUAD3 1");
    quad3API.init  = function (screen, x, y, w, h) {
        console.log("QUAD3 2");
        this.screen = screen;
        this.x = x;
        this.y = y;
        this.width = w;
        this.height = h;
        this.screen.fillRect(this.x, this.y, this.width, this.height, RED);
    }
    return quad3API;
}

module.exports.QUAD3 = new QUAD3();
