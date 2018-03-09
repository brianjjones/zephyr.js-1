// Copyright (c) 2016-2017, Intel Corporation.

// Test code for Arduino 101 that uses two buttons on IO2 and IO4 to control
// the two onboard LEDs.
//
// Note: If you use a Grove touch sensor as one of the buttons, wire it to IO4:
// it doesn't work the other way and I haven't figured out why yet. It may
// relate to IO5 being connected to both the X86 and ARC processors.

console.log('GPIO test with two buttons controlling two LEDs...');

// import gpio module
var gpio = require('gpio');

// LED1 and LED2 are onboard LEDs on Arduino 101
// var led1 = gpio.open({pin: 'LED1', activeLow: true});
// var led2 = gpio.open({pin: 'LED2', activeLow: false});
var btn1 = gpio.open({pin: 2, mode: 'in', edge: 'falling'});
var btn2 = gpio.open({pin: 3, mode: 'in', edge: 'rising'});

// turn off LED #2 initially
// led1.write(0);

btn1.onchange = function (event) {
    console.log("BUTTON 2 + " + btn1.read());
//    led1.write(event.value);
};

btn2.onchange = function (event) {
    console.log("BUTTON 3 + " + btn2.read());
//    led2.write(event.value);
};
