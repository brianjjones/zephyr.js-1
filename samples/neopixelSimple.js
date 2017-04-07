
/*
 This is an example of how simple driving a Neopixel can be
 This code is optimized for understandability and changability rather than raw speed
 More info at http://wp.josh.com/2014/05/11/ws2812-neopixels-made-easy/
*/

// Change this to be at least as long as your pixel string (too long will work fine, just be a little slower)

var gpio = require('gpio');
var pins = require('arduino101_pins');

var neoPix = gpio.open({
    pin: pins.IO6,
    direction: 'out'
});


var PIXELS=100 // Number of pixels in the string

// These values depend on which pin your string is connected to and what board you are using 
// More info on how to find these at http://www.arduino.cc/en/Reference/PortManipulation

// These values are for the pin that connects to the Data Input pin on the LED strip. They correspond to...

// Arduino Yun:     Digital Pin 8
// DueMilinove/UNO: Digital Pin 12
// Arduino MeagL    PWM Pin 4

// You'll need to look up the port/bit combination for other boards. 

// Note that you could also include the DigitalWriteFast header file to not need to to this lookup.

// These are the timing constraints taken mostly from the WS2812 datasheets 
// These are chosen to be conservative and avoid problems rather than for maximum throughput 

var T1H=900    // Width of a 1 bit in ns
var T1L=600    // Width of a 1 bit in ns

var T0H=400    // Width of a 0 bit in ns
var T0L=900    // Width of a 0 bit in ns

var RES=6000    // Width of the low gap between bits to cause a frame to latch

// Here are some convience defines for using nanoseconds specs to generate actual CPU delays

//var NS_PER_SEC (1000000000L)          // Note that this has to be SIGNED since we want to be able to check for negative values of derivatives

/*var F_CPU 3600 //BJONES
var CYCLES_PER_SEC (F_CPU)

var NS_PER_CYCLE ( NS_PER_SEC / CYCLES_PER_SEC )

var NS_TO_CYCLES(n) ( (n) / NS_PER_CYCLE )
*/
var waitLock = false;

function sendByte(byteVal) { 
console.log("SENDING BYTE " + byteVal);
    var b = [];

   for (var i = 7; i > -1; i--) {
      sendBit((byteVal >> i) & 1);
      }
}

//BJONES TODO - create a 'sleep' method for ZJS that is exposed globally. Specifically expose
// zjs_sleep to Javascript
// BIG NOTE!!! Currently k_sleep uses milliseconds, which is WAY too slow.  Look in deps/zephyr/kernel/sched.c
// to find a way to use duration of nanoseconds and convert to ticks.
// If that is still too slow, expose entire neopixel lib to javascript and do it all in C.  That should work.

function delayCycles(nSecs) {
    waitLock = true;
    setTimeout(function() {waitLock = false; console.log("nSeconds is up");}, nSecs);
    console.log("Setting Timeout for  " + nSecs);
    while (waitLock){
    //Chill here until time passed
    }
    console.log("DONE WITH TIMEOUT");
}

// Actually send a bit to the string. We must to drop to asm to enusre that the complier does
// not reorder things and make it so the delay happens in the wrong place.

function sendBit( bitVal ) { //bool bitval
  

    if ( bitVal ) {      // 1-bit
 
      neoPix.write(true);

      //BJONES TODO NEED TO MAKE MY OWN DELAY CYCLES METHOD.  Could do it in
      //C and return only once that amount of time had occured.

      delayCycles(T1H);
      //DELAY_CYCLES( NS_TO_CYCLES( T1H ) - 2 ); // 1-bit width less overhead for the actual bit setting
                                                     // Note that this delay could be longer and everything would still work
      neoPix.write(false);

        delayCycles(T1L);
      //DELAY_CYCLES( NS_TO_CYCLES( T1L ) - 10 ); // 1-bit gap less the overhead of the loop
 
    } else {             // 0-bit
 
      //cli();                                       // We need to protect this bit from being made wider by an interrupt 
 
      neoPix.write(true);

        delayCycles(T0H);
      //DELAY_CYCLES( NS_TO_CYCLES( T0H ) - 2 ); // 0-bit width less overhead
                                                    // **************************************************************************
                                                    // This line is really the only tight goldilocks timing in the whole program!
                                                    // **************************************************************************
      neoPix.write(false);
 
     // sei();

      delayCycles(T0L);
      //DELAY_CYCLES( NS_TO_CYCLES( T0L ) - 10 ); // 0-bit gap less overhead of the loop
 
    }
 
    // Note that the inter-bit gap can be as long as you want as long as it doesn't exceed the 5us reset timeout (which is A long time)
    // Here I have been generous and not tried to squeeze the gap tight but instead erred on the side of lots of extra time.
    // This has thenice side effect of avoid glitches 
}  

  
//function sendByte( byte ) {  //char* byte
    
    //for(  bit = 0 ; bit &lt; 8 ; bit++ ) {
 
      //sendBit( bitRead( byte , 7 ) ); // Neopixel wants bit in highest-to-lowest order
                                                     // so send highest bit (bit #7 in an 8-bit byte since they start at 0)
      //byte &lt;&lt;= 1; // and then shift left so bit 6 moves into 7, 5 moves into 6, etc
 
  //  }         
//} 

/*

  The following three functions are the public API:
  
  ledSetup() - set up the pin that is connected to the string. Call once at the begining of the program.  
  sendPixel( r g , b ) - send a single pixel to the string. Call this once for each pixel in a frame.
  show() - show the recently sent pixel on the LEDs . Call once per frame. 
  
*/


// Set the specified pin up as digital out

function ledsetup() {
  
  //BJONES Need ZJS EQUIV for arduino call
    neoPix.write(false);
  //bitSet( PIXEL_DDR , PIXEL_BIT );
  
}

function sendPixel( r,  g , b )  {  //unsigned char rgb
  
  sendByte(g);          // Neopixel wants colors in green then red then blue order
  sendByte(r);
  sendByte(b);
  
}


// Just wait long enough without sending any bots to cause the pixels to latch and display the last sent frame

function show() {
	//BJONES this is from the avr-libc, can I just includ <util/delay.h>? Or is this not supported
	//and I need to sleep or something similar.  Look up what we use for wait in arc etc
    delayCycles(RES);
    //DELAY_CYCLES( NS_TO_CYCLES(RES) );
}


// Display a single color on the whole string

function showColor( r ,  g ,  b ) {   //unsigned char  rgb
  
  //cli();   BJONES REMOVING INTERUPT STUFF ARDUINO ONLY
  for( p=0; p<PIXELS; p++ ) {
    sendPixel( r , g , b );
  }
  //sei();
  show();
  
}

// Fill the dots one after the other with a color
// rewrite to lift the compare out of the loop
function colorWipe(r , g, b, wait ) { //unsigned char 
  for( i=0; i<PIXELS; i+= (PIXELS/60) ) {
    
//    cli();
    var p=0;
    
    while (p++<=i) {
        sendPixel(r,g,b);
    } 
     
    while (p++<=PIXELS) {
        sendPixel(0,0,0);  
      
    }
    
    //sei();
    show();
    //BJONES Need ZJS EQUIV for arduino call. Likely sleep
    //delay(wait);
  }
}


  ledsetup();
  // Some example procedures showing how to display to the pixels:
  colorWipe(255, 0, 0, 0); // Red
  show();
  //colorWipe(0, 255, 0, 0); // Green
  //colorWipe(0, 0, 255, 0); // Blue






