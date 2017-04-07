
/*
 This is an example of how simple driving a Neopixel can be
 This code is optimized for understandability and changability rather than raw speed
 More info at http://wp.josh.com/2014/05/11/ws2812-neopixels-made-easy/
*/

// Change this to be at least as long as your pixel string (too long will work fine, just be a little slower)

var PIXELS 100 // Number of pixels in the string

// These values depend on which pin your string is connected to and what board you are using 
// More info on how to find these at http://www.arduino.cc/en/Reference/PortManipulation

// These values are for the pin that connects to the Data Input pin on the LED strip. They correspond to...

// Arduino Yun:     Digital Pin 8
// DueMilinove/UNO: Digital Pin 12
// Arduino MeagL    PWM Pin 4

// You'll need to look up the port/bit combination for other boards. 

// Note that you could also include the DigitalWriteFast header file to not need to to this lookup.

var PORTB 3 //BJONES
var DDRB 3
var PIXEL_PORT  PORTB  // Port of the pin the pixels are connected to
var PIXEL_DDR   DDRB   // Port of the pin the pixels are connected to
var PIXEL_BIT   4      // Bit of the pin the pixels are connected to

// These are the timing constraints taken mostly from the WS2812 datasheets 
// These are chosen to be conservative and avoid problems rather than for maximum throughput 

var T1H  900    // Width of a 1 bit in ns
var T1L  600    // Width of a 1 bit in ns

var T0H  400    // Width of a 0 bit in ns
var T0L  900    // Width of a 0 bit in ns

var RES 6000    // Width of the low gap between bits to cause a frame to latch

// Here are some convience defines for using nanoseconds specs to generate actual CPU delays

var NS_PER_SEC (1000000000L)          // Note that this has to be SIGNED since we want to be able to check for negative values of derivatives

var F_CPU 3600 //BJONES
var CYCLES_PER_SEC (F_CPU)

var NS_PER_CYCLE ( NS_PER_SEC / CYCLES_PER_SEC )

var NS_TO_CYCLES(n) ( (n) / NS_PER_CYCLE )

// Actually send a bit to the string. We must to drop to asm to enusre that the complier does
// not reorder things and make it so the delay happens in the wrong place.

function sendBit( bitVal ) { //bool bitval
  

    if ( bitVal ) {      // 1-bit
 
      bitSet( PIXEL_PORT , PIXEL_BIT );
 
      DELAY_CYCLES( NS_TO_CYCLES( T1H ) - 2 ); // 1-bit width less overhead for the actual bit setting
                                                     // Note that this delay could be longer and everything would still work
      bitClear( PIXEL_PORT , PIXEL_BIT );
 
      DELAY_CYCLES( NS_TO_CYCLES( T1L ) - 10 ); // 1-bit gap less the overhead of the loop
 
    } else {             // 0-bit
 
      cli();                                       // We need to protect this bit from being made wider by an interrupt 
 
      bitSet( PIXEL_PORT , PIXEL_BIT );
 
      DELAY_CYCLES( NS_TO_CYCLES( T0H ) - 2 ); // 0-bit width less overhead
                                                    // **************************************************************************
                                                    // This line is really the only tight goldilocks timing in the whole program!
                                                    // **************************************************************************
      bitClear( PIXEL_PORT , PIXEL_BIT );
 
      sei();
 
      DELAY_CYCLES( NS_TO_CYCLES( T0L ) - 10 ); // 0-bit gap less overhead of the loop
 
    }
 
    // Note that the inter-bit gap can be as long as you want as long as it doesn't exceed the 5us reset timeout (which is A long time)
    // Here I have been generous and not tried to squeeze the gap tight but instead erred on the side of lots of extra time.
    // This has thenice side effect of avoid glitches 
}  

  
function sendByte( byte ) {  //char* byte
    
    for( unsigned char bit = 0 ; bit &lt; 8 ; bit++ ) {
 
      sendBit( bitRead( byte , 7 ) ); // Neopixel wants bit in highest-to-lowest order
                                                     // so send highest bit (bit #7 in an 8-bit byte since they start at 0)
      byte &lt;&lt;= 1; // and then shift left so bit 6 moves into 7, 5 moves into 6, etc
 
    }         
} 

/*

  The following three functions are the public API:
  
  ledSetup() - set up the pin that is connected to the string. Call once at the begining of the program.  
  sendPixel( r g , b ) - send a single pixel to the string. Call this once for each pixel in a frame.
  show() - show the recently sent pixel on the LEDs . Call once per frame. 
  
*/


// Set the specified pin up as digital out

function ledsetup() {
  
  //BJONES Need ZJS EQUIV for arduino call
  bitSet( PIXEL_DDR , PIXEL_BIT );
  
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
    DELAY_CYCLES( NS_TO_CYCLES(RES) );
}


/*

  That is the whole API. What follows are some demo functions rewriten from the AdaFruit strandtest code...
  
  https://github.com/adafruit/Adafruit_NeoPixel/blob/master/examples/strandtest/strandtest.ino
  
  Note that we always turn off interrupts while we are sending pixels becuase an interupt
  could happen just when we were in the middle of somehting time sensitive.
  
  If we wanted to minimize the time interrupts were off, we could instead 
  could get away with only turning off interrupts just for the very brief moment 
  when we are actually sending a 0 bit (~1us), as long as we were sure that the total time 
  taken by any interrupts + the time in our pixel generation code never exceeded the reset time (5us).
  
*/


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

// Theatre-style crawling lights.
// Changes spacing to be dynmaic based on string size

var THEATER_SPACING (PIXELS/20)

function theaterChase(  r ,  g,  b,  wait ) {
  
  for (j=0; j< 3 ; j++) {  
  
    for (q=0; q < THEATER_SPACING ; q++) {
      
      var step=0;
      
      //cli();
      
      for (i=0; i < PIXELS ; i++) {
        
        if (step==q) {
          
          sendPixel( r , g , b );
          
        } else {
          
          sendPixel( 0 , 0 , 0 );
          
        }
        
        step++;
        
        if (step==THEATER_SPACING) step =0;
        
      }
      
      //sei();
      
      show();
      //BJONES Need ZJS EQUIV for arduino call likely sleep
      //delay(wait);
      
    }
    
  }
  
}
        


// I rewrite this one from scrtach to use high resolution for the color wheel to look nicer on a *much* bigger string
                                                                            
function rainbowCycle( frames ,  frameAdvance,  pixelAdvance ) { //unsigned char, uint, uint
  
  // Hue is a number between 0 and 3*256 than defines a mix of r->g->b where
  // hue of 0 = Full red
  // hue of 128 = 1/2 red and 1/2 green
  // hue of 256 = Full Green
  // hue of 384 = 1/2 green and 1/2 blue
  // ...
  
  var firstPixelHue = 0;     // Color for the first pixel in the string
  
  for(j=0; j<frames; j++) {                                  
    
    var currentPixelHue = firstPixelHue;
       
    //cli();    
        
    for(i=0; i< PIXELS; i++) {
      
      if (currentPixelHue>=(3*256)) {                  // Normalize back down incase we incremented and overflowed
        currentPixelHue -= (3*256);
      }
            
      var phase = currentPixelHue >> 8;
      var step = currentPixelHue & 0xff;
                 
      switch (phase) {
        
        case 0: 
          sendPixel( ~step , step ,  0 );
          break;
          
        case 1: 
          sendPixel( 0 , ~step , step );
          break;

        case 2: 
          sendPixel(  step ,0 , ~step );
          break;
          
      }
      
      currentPixelHue+=pixelAdvance;                                      
      
                          
    } 
    
    //sei();
    
    show();
    
    firstPixelHue += frameAdvance;
           
  }
}

  
// I added this one just to demonstrate how quickly you can flash the string.
// Flashes get faster and faster until *boom* and fade to black.

function detonate(  r ,  g ,  b ,  startdelayms) { //uchar,uchar,uchar, uint
  while (startdelayms) {
    
    showColor( r , g , b );      // Flash the color 
    showColor( 0 , 0 , 0 );
    
    //BJONES Need ZJS EQUIV for arduino call likely sleep
    //delay( startdelayms );      
    
    startdelayms =  ( startdelayms * 4 ) / 5 ;           // delay between flashes is halved each time until zero
    
  }
  
  // Then we fade to black....
  
  for(fade=256; fade>0; fade-- ) {
    
    showColor( (r * fade) / 256 ,(g*fade) /256 , (b*fade)/256 );
        
  }
  
  showColor( 0 , 0 , 0 );
  
    
}


function loop() {

  ledsetup();
  // Some example procedures showing how to display to the pixels:
  colorWipe(255, 0, 0, 0); // Red
  colorWipe(0, 255, 0, 0); // Green
  colorWipe(0, 0, 255, 0); // Blue
  
  // Send a theater pixel chase in...
  theaterChase(127, 127, 127, 0); // White
  theaterChase(127,   0,   0, 0); // Red
  theaterChase(  0,   0, 127, 0); // Blue
  
  rainbowCycle(1000 , 20 , 5 );
  detonate( 255 , 255 , 255 , 1000);
  
  return;  
}





