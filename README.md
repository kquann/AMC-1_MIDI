# MC1-MIDI
Arduino-based MIDI controller

General:  
-This is a general purpose MIDI controller based around an Arduino Uno microcontroller.  
-Please see associated parts list and schematic for details.  
-Makes use of ShiftPWM (from elcojacobs), MIDI (from Forty Seven Effects) and LiquidCrystal595 (from tehniq3) libraries. Make sure these are placed appropriately in your Ardunio IDE library directory.  
-The controller itself has 13 switches (10 multi-use, 1 page up, 1 page down and 1 tap-tempo).  
-Support for 3 additional external switches also supported.  
-Each of 13-switches + Ext switching jacks has a multi-colored RGB LED.  
-Supports 6 expression pedals (TRS-style).  
-Interface is set up in "Pages", whereby you can scroll through different pages of presets/IAs, etc.
Each page consists of 10 different buttons, scrolling the page number increments/decrements through pages.
For instance page 1 = effects 1-10, page 2 = 11-20, etc. Note indexing starts at 0.  
-Current set-up is here as an example, change as you see fit!  

Issues:  
-Controller settings currently only changeable by modifying this code and re-flashing.  
-Make sure to disconnect MIDI out from any devices while flashing over USB!  
-MIDI over USB not tested.  
-Currently no expression calibration function.  
-MIDI in not extensively tested.  
-External switches not implemented in code.  
-While unit can support up to 6 expression pedals, code is currently set up for 1.  
-Code can be optimized (it's old), particularly with presets, but it gets the job done.  

