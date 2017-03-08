var d3jsGraph = (function(args) {
    var D3JSGRAPH = {};

    if (!args)
            var args = {};
            
    D3JSGRAPH.Graph = function(args) {
        this.height = args.height ? args.height : 960;
        this.width = args.width ? args.width : 500;
        this.left = args.left ? args.left : 0;
        this.top = args.top ? args.top : 0;
        this.dataSize = args.dataSize ? args.dataSize : 100;
        this.data = new Array(this.dataSize).fill(0);
        this.dataMaxVal = args.dataMaxVal ? args.dataMaxVal : 100;
        this.dataMinVal = args.dataMinVal ? args.dataMinVal : 0;
        this.lineColor = args.lineColor ? args.lineColor : "steelblue";
        var margin = {top: 30, right: 20, bottom: 30, left: 50};    
        this.transitionTime = this.transitionTime ? args.transitionTime : 90;
        this.text = args.text ? args.text : "Value";
        //this.updateData = args.updateData;

        var svg = d3.select("body")
                    .append("svg")
                    .attr("width", this.width + margin.left + margin.right)
                    .attr("height", this.height + margin.top + margin.bottom)
                    .attr("style", "position:absolute;left:" + this.left + "px; top:" + this.top +"px" );

        var g = svg.append("g").attr("transform", "translate(" + margin.left + "," + margin.top + ")");


        this.x = d3.scale.linear()
                    .domain([1,this.dataSize-4])
                    .range([0, this.width]);

        var y = d3.scale.linear()
                    .domain([this.dataMinVal, this.dataMaxVal])
                    .range([this.height, 0]);

        var yAxis = d3.svg.axis().scale(y)
                        .orient("left").ticks(5);
              
        this.line = d3.svg.line()
                        .interpolate("basis")
                        .x(function(d, i) { return this.x(i); }.bind(this))
                        .y(function(d, i) { return y(d); });

        g.append("defs").append("clipPath")
            .attr("id", "clip")
        .append("rect")
            .attr("width", this.width)
            .attr("height", this.height);

        g.append("g")
            .attr("class", "y axis")
            .call(yAxis)
        .append("text")
            .attr("fill", "#000")
            .attr("transform", "rotate(-90)")
            .attr("y", 6)
            .attr("dy", "0.71em")
            .attr("text-anchor", "end")
            .text(this.text);

        this.path = g.append("g")
                .attr("clip-path", "url(#clip)")
            .append("path")
                .datum(this.data)
            .attr("class", "line")
            .attr("fill", "none")
            .attr("stroke", this.lineColor)
            .attr("stroke-linejoin", "round")
            .attr("stroke-linecap", "round")
            .attr("stroke-width", 1.5)
            .attr("d", this.line);
    }

    D3JSGRAPH.Graph.prototype.pushData = function (newVal) {
        this.data.push(newVal);    
        this.data.shift();

        this.path
            .attr("d", this.line(this.data))
            .attr("transform", null)            
            .transition()
            .attr("transform", "translate(" + this.x(0) + ")")
            .duration(this.transitionTime - 30)
            .ease("linear");
    }
    
    return D3JSGRAPH;
}());
