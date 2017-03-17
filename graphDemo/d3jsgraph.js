var d3jsGraph = (function(args) {
    var D3JSGRAPH = {};

    if (!args)
            var args = {};

        function why(d,i,y,h) {
            return h - y(d);
        }
    D3JSGRAPH.Graph = function(args) {
        this.height = args.height ? args.height : 960;
        this.width = args.width ? args.width : 500;
        this.left = args.left ? args.left : 0;
        this.top = args.top ? args.top : 0;
        this.dataSize = args.dataSize ? args.dataSize : 100;
        this.data = new Array(this.dataSize).fill(10);
        this.dataMaxVal = args.dataMaxVal ? args.dataMaxVal : 100;
        this.dataMinVal = args.dataMinVal ? args.dataMinVal : 0;
        this.lineColor = args.lineColor ? args.lineColor : "steelblue";
        var margin = {top: 30, right: 20, bottom: 30, left: 50};    
        this.transitionTime = this.transitionTime ? args.transitionTime : 90;
        this.text = args.text ? args.text : "";
        this.clipSize = args.clipSize ? args.clipSize : 4;      //BJONES ClipSize must be > 1 as it is removing from the size of the array, otherwise array overflow.
        this.type = args.type ? args.type : "line";
        this.bars;
        //this.svg;
        //this.updateData = args.updateData;

        var svg = d3.select("body")
                    .append("svg")
                    .attr("width", this.width + margin.left + margin.right)
                    .attr("height", this.height + margin.top + margin.bottom)
                    .attr("style", "position:absolute;left:" + this.left + "px; top:" + this.top +"px" );

        var g = svg.append("g").attr("transform", "translate(" + margin.left + "," + margin.top + ")");


       
        

        if (this.type === "line") {

            this.x = d3.scale.linear()
                        .domain([1,this.dataSize-this.clipSize])
                        //.domain([0,this.dataSize-this.clipSize])
                        .range([0, this.width]);

            var y = d3.scale.linear()
                        .domain([this.dataMinVal, this.dataMaxVal])
                        .range([this.height, 0]);

            //BJONES need to make this.yAxis to allow it to come after the line vs bar stuff
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
        else if (this.type === "bar") {


          //BJONES figure out how to get the bars to clip properly
          //Could I possibly remove the clip stuff all together for the bars?
          // If I do that do I need to recalculate the size based on the 'margin' values?
this.x = d3.scale.linear().range([0, this.width]);//d3.scale.ordinal().rangeRoundBands([0, this.width], .05);

this.y = d3.scale.linear().range([this.height, 0]);

            //x.domain(data.map(function(d,i) { return i; }));
            this.x.domain([0,this.dataSize-1]);    //BJONES not sure I need the map since doing array
            this.y.domain([this.dataMinVal, this.dataMaxVal]);

            g.append("defs").append("clipPath")
                .attr("id", "clip")
            .append("rect")
                .attr("width", this.width)
                .attr("height", this.height);
                
            this.bars = g//svg.selectAll("bar")
                .attr("clip-path", "url(#clip)")
                  .data(this.data)
                .enter().append("rect")
                  .attr("class", "bar")
                  .style("fill", "steelblue")
                  .attr("x", function(d,i) { return this.x(i); }.bind(this))
                  .attr("width", this.width / this.dataSize) //this.x.rangeBand())
                  .attr("y", function(d,i) { return this.y(d); }.bind(this))
                  .attr("height", function(d,i) { return why(d,i,this.y,this.height);}.bind(this));
                 // .attr("height", function(d,i) {return this.height - this.y(d)}.bind(this));
                  //.attr("height", function(d,i) { return this.height - y(d); }.bind(this));
            
        }
    }

    D3JSGRAPH.Graph.prototype.pushData = function (newVal) {
        this.data.push(newVal);    
        this.data.shift();

        if (this.type === "line") {
        this.path
            .attr("d", this.line(this.data))
            .attr("transform", null)            
            .transition()
            .attr("transform", "translate(" + this.x(0) + ")")
            .duration(this.transitionTime - 30)
            .ease("linear");

        }
        else if (this.type === "bar") {
/*            var bars = this.selector.selectAll(".bar").data(thisGraph.visibleData);

            bars.attr("x", function(d) {
                    return xFunc(d[thisGraph.xCoordName]);
                })
                .attr("height", function(d) {
                    return thisGraph.height - yFunc(d[yCoord]);
                })
                .attr("width", function(d) {
                    return barSize;
                })
                .attr("y", function(d) {
                    return yFunc(d[yCoord]);
                });

            //Draw new bars being created
            bars.enter()
                .append("rect")
                .attr("x", function(d) {
                    return xFunc(d[thisGraph.xCoordName]);
                })
                .attr("height", function(d) {
                    return thisGraph.height - yFunc(d[yCoord]);
                })
                .classed(yCoord, true)
                .attr("y", function(d) {
                    return yFunc(d[yCoord]);
                })
                .attr("width", function(d) {
                    return barSize;
                });
*/
            
    
             //this.svg.selectAll(".bar")
                this.bars
                  .data(this.data)
//                .enter().append("rect")
                
                  .style("fill", "red")
                  .attr("x", function(d,i) { return this.x(i); }.bind(this))
                  //.attr("width", this.x.rangeBand())
                  .attr("width", this.width / this.dataSize)
                  .attr("y", function(d,i) { return this.y(d); }.bind(this))
                  //.attr("height", this.height - 60);
                  .attr("height", function(d,i) { return this.height - d}.bind(this))
                  .exit().remove();
        }
    }
    
    return D3JSGRAPH;
}());
