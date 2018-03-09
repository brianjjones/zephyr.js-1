function QUAD4() {
    console.log("QUAD4 1");
    var quad4API = {};
    quad4API.init  = function (screen, x, y, w, h) {
        console.log("QUAD4 2");
        this.screen = screen;
        this.x = x;
        this.y = y;
        this.width = w;
        this.height = h;
        this.screen.fillRect(this.x, this.y, this.width, this.height, BLUE);
        console.log("QUAD4 3");
    }
    return quad4API;
}
console.log("QUAD4 4");
module.exports.QUAD4 = new QUAD4();
