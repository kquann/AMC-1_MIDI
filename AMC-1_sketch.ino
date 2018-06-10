/************************************************************************************************************************************
6/8/18
AMC-1 MIDI CONTROLLER v1.0.0
Kevin Quann
kevin.quann@gmail.com

General:
-This is a general purpose MIDI foot controller based around an Arduino Uno microcontroller.
-Please see associated parts list and schematic for details.
-Makes use of ShiftPWM, MIDI and LiquidCrystal595 libraries.
-Controller itself has 13 switches (10 multi-use, 1 page up, 1 page down and 1 tap-tempo).
-Support for 3 additional external switches also supported
-Each of 13-switches + Ext switching jacks has a  multi-colored RGB LED.
-Supports 6 expression pedals (TRS-style)
-Interface is set up in "Pages", whereby you can scroll through different pages of presets/IAs, etc.
Each page consists of 10 different buttons, scrolling the page number increments/decrements through pages.
For instance page 1 = effects 1-10, page 2 = 11-20, etc. Note indexing starts at 0.
-Current set-up is here for example, change as you see fit!

Issues:
-Controller settings currently only changeable by modifying this code and re-flashing.
-Make sure to disconnect MIDI out from any devices while flashing over USB!
-MIDI over USB not tested.
-Currently no expression calibration function.
-MIDI in not extensively tested.
-External switches not implemented in code
-While unit can support up to 6 expression pedals, code is currently set up for 1.
-Code can be optimized (it's old), particularly with presets, but it gets the job done.

Changes:
-Posted to GitHub

 ************************************************************************************************************************************/

//Set up for PISO shift-in:
//How many 74HC165N shift register chips are daisy-chained:
#define NUMBER_OF_SHIFT_CHIPS   2

//Width of data (how many ext lines).
#define DATA_WIDTH   NUMBER_OF_SHIFT_CHIPS * 8

//Width of pulse to trigger the shift register to read and latch.
#define PULSE_WIDTH_USEC   5

//Optional delay between shift register reads.
#define POLL_DELAY_MSEC   1

//Since only 2 chips are used, we may use int, >2 requires long
#define BYTES_VAL_T unsigned int

//Setting up ShiftPWM for LEDs:
#define SHIFTPWM_USE_TIMER2  // for Arduino Uno and earlier (Atmega328)

// Clock and data pins are pins from the hardware SPI, you cannot choose them yourself if you use the hardware SPI.
// Data pin is MOSI (Uno and earlier: 11)
// Clock pin is SCK (Uno and earlier: 13)
// You can choose the latch pin yourself.
const int ShiftPWM_latchPin=10;

// If your LED's turn on if the pin is low, set this to true, otherwise set it to false.
const bool ShiftPWM_invertOutputs = false; 

const bool ShiftPWM_balanceLoad = false;

const int MIDIOutPin = 1; //MIDI Out pin (UART Tx)

//Including all libraries:
#include <ShiftPWM.h>   // include ShiftPWM.h after setting the pins
#include <LiquidCrystal595.h> //included for LCD display
#include <MIDI.h> //midi library
#include <avr/pgmspace.h> //PROGMEM to store stuff in flash memory

unsigned char maxBrightness = 255;
unsigned char pwmFrequency = 75;
int numRegisters = 6; //Number of 595 shift registers used
int numRGBleds = 16;

// Defining pins for ShiftLCD
LiquidCrystal595 lcd(7,8,9);     // datapin, latchpin, clockpin

MIDI_CREATE_DEFAULT_INSTANCE(); //needed for midi library

int ploadPin        = 3;  // Connects to Parallel load pin the 165
int clockEnablePin  = 4;  // Connects to Clock Enable pin the 165
int dataPin         = 5; // Connects to the Q7 pin the 165
int clockPin        = 6; // Connects to the Clock pin the 165

boolean ExpState = 0; //Are any expression pedals in use? 0=no, 1=yes
int RawExpVals[6] = {0, 0, 0, 0, 0, 0}; //vector to store raw expression pedal values

unsigned long time; //Needed for tap tempo and blinking LEDs
unsigned long timer = 0; //used in conjunction with time
unsigned long tap_interval = 300; //defaults to 300ms
unsigned long tap_time; //Used when tap tempo is pushed 

char inChan, inDB1, inDB2; //These are variables to store incoming CC message channel, Data Byte 1 and Data Byte 2

int CurrentPage = 0;

//Define page names in flash. 8 pages is arbitrary, may be expanded or shortened.
//Names may not exceed 16 characters owing to 16x2 LCD
const PROGMEM char PageName_1[] = "Presets";
const PROGMEM char PageName_2[] = "POD HD";
const PROGMEM char PageName_3[] = "M13 Part 1";
const PROGMEM char PageName_4[] = "M13 Part 2";
const PROGMEM char PageName_5[] = "Hex 1"; 
const PROGMEM char PageName_6[] = "Hex 2";
const PROGMEM char PageName_7[] = "Unused";
const PROGMEM char PageName_8[] = "Looper";

//PageName Lookup table
const char* const PageNameTable[] PROGMEM = {PageName_1, PageName_2, PageName_3, PageName_4, PageName_5, PageName_6, PageName_7, PageName_8};

//Store effect names in Flash, number of names must be # of pages * 10 = 80 in this case.
//Names should not exceed 16 characters
const PROGMEM char EffectName_1[] = "Clean";
const PROGMEM char EffectName_2[] = "Drive";
const PROGMEM char EffectName_3[] = "Guyatone";
const PROGMEM char EffectName_4[] = "Muff";
const PROGMEM char EffectName_5[] = "Pickle";
const PROGMEM char EffectName_6[] = "Fuzz";
const PROGMEM char EffectName_7[] = "Geiger";
const PROGMEM char EffectName_8[] = "Empty";
const PROGMEM char EffectName_9[] = "Empty";
const PROGMEM char EffectName_10[] = "ALL MODS OFF";
const PROGMEM char EffectName_11[] = "FS1";
const PROGMEM char EffectName_12[] = "FS2";
const PROGMEM char EffectName_13[] = "FS3";
const PROGMEM char EffectName_14[] = "FS4";
const PROGMEM char EffectName_15[] = "FS5";
const PROGMEM char EffectName_16[] = "FS6";
const PROGMEM char EffectName_17[] = "FS7";
const PROGMEM char EffectName_18[] = "FS8";
const PROGMEM char EffectName_19[] = "Exp 1";
const PROGMEM char EffectName_20[] = "Tuner";
const PROGMEM char EffectName_21[] = "M13 2A";
const PROGMEM char EffectName_22[] = "M13 2B";
const PROGMEM char EffectName_23[] = "M13 2C";
const PROGMEM char EffectName_24[] = "Empty";
const PROGMEM char EffectName_25[] = "M13 Half time";
const PROGMEM char EffectName_26[] = "M13 1A";
const PROGMEM char EffectName_27[] = "M13 1B";
const PROGMEM char EffectName_28[] = "M13 1C";
const PROGMEM char EffectName_29[] = "Empty";
const PROGMEM char EffectName_30[] = "M13 Fwd/Rev";
const PROGMEM char EffectName_31[] = "M13 4A";
const PROGMEM char EffectName_32[] = "M13 4B";
const PROGMEM char EffectName_33[] = "M13 4C";
const PROGMEM char EffectName_34[] = "Record";
const PROGMEM char EffectName_35[] = "Play";
const PROGMEM char EffectName_36[] = "M13 3A";
const PROGMEM char EffectName_37[] = "M13 3B";
const PROGMEM char EffectName_38[] = "M13 3C";
const PROGMEM char EffectName_39[] = "Overdub";
const PROGMEM char EffectName_40[] = "Play Once";
const PROGMEM char EffectName_41[] = "Hex 1-1";
const PROGMEM char EffectName_42[] = "Hex 1-2";
const PROGMEM char EffectName_43[] = "Hex 1-3";
const PROGMEM char EffectName_44[] = "Hex 1-4";
const PROGMEM char EffectName_45[] = "Hex 1-5";
const PROGMEM char EffectName_46[] = "Hex 1-6";
const PROGMEM char EffectName_47[] = "Empty";
const PROGMEM char EffectName_48[] = "Empty";
const PROGMEM char EffectName_49[] = "Empty";
const PROGMEM char EffectName_50[] = "Empty";
const PROGMEM char EffectName_51[] = "Hex 2-1";
const PROGMEM char EffectName_52[] = "Hex 2-2";
const PROGMEM char EffectName_53[] = "Hex 2-3";
const PROGMEM char EffectName_54[] = "Hex 2-4";
const PROGMEM char EffectName_55[] = "Hex 2-5";
const PROGMEM char EffectName_56[] = "Hex 2-6";
const PROGMEM char EffectName_57[] = "Empty";
const PROGMEM char EffectName_58[] = "Empty";
const PROGMEM char EffectName_59[] = "Empty";
const PROGMEM char EffectName_60[] = "Empty";
const PROGMEM char EffectName_61[] = "Effect 61";
const PROGMEM char EffectName_62[] = "Effect 62";
const PROGMEM char EffectName_63[] = "Effect 63";
const PROGMEM char EffectName_64[] = "Effect 64";
const PROGMEM char EffectName_65[] = "Effect 65";
const PROGMEM char EffectName_66[] = "Effect 66";
const PROGMEM char EffectName_67[] = "Effect 67";
const PROGMEM char EffectName_68[] = "Effect 68";
const PROGMEM char EffectName_69[] = "Effect 69";
const PROGMEM char EffectName_70[] = "Effect 70";
const PROGMEM char EffectName_71[] = "Record";
const PROGMEM char EffectName_72[] = "Overdub";
const PROGMEM char EffectName_73[] = "Play";
const PROGMEM char EffectName_74[] = "Play Once";
const PROGMEM char EffectName_75[] = "Empty";
const PROGMEM char EffectName_76[] = "Reverse";
const PROGMEM char EffectName_77[] = "Half Time";
const PROGMEM char EffectName_78[] = "Empty";
const PROGMEM char EffectName_79[] = "Empty";
const PROGMEM char EffectName_80[] = "Empty";

const char* const EffectNameTable[] PROGMEM = //Essentially a lookup table
{
  EffectName_1, EffectName_2, EffectName_3, EffectName_4, EffectName_5,
  EffectName_6, EffectName_7, EffectName_8, EffectName_9, EffectName_10,
  EffectName_11, EffectName_12, EffectName_13, EffectName_14, EffectName_15,
  EffectName_16, EffectName_17, EffectName_18, EffectName_19, EffectName_20,
  EffectName_21, EffectName_22, EffectName_23, EffectName_24, EffectName_25,
  EffectName_26, EffectName_27, EffectName_28, EffectName_29, EffectName_30,
  EffectName_31, EffectName_32, EffectName_33, EffectName_34, EffectName_35,
  EffectName_36, EffectName_37, EffectName_38, EffectName_39, EffectName_40,
  EffectName_41, EffectName_42, EffectName_43, EffectName_44, EffectName_45,
  EffectName_46, EffectName_47, EffectName_48, EffectName_49, EffectName_50,
  EffectName_51, EffectName_52, EffectName_53, EffectName_54, EffectName_55,
  EffectName_56, EffectName_57, EffectName_58, EffectName_59, EffectName_60,
  EffectName_61, EffectName_62, EffectName_63, EffectName_64, EffectName_65,
  EffectName_66, EffectName_67, EffectName_68, EffectName_69, EffectName_70,
  EffectName_71, EffectName_72, EffectName_73, EffectName_74, EffectName_75,
  EffectName_76, EffectName_77, EffectName_78, EffectName_79, EffectName_80
};
 
//LED values for each Effect/button, must match length of effects (pages * 10) = 80, as above
//Each LED has Red, Green and Blue value associated with it, may range from 0 (off) - 255 (max brightness)
//For initialization, set LEDs to dim state (color value of 2), to be amplifed later if on  
//Assigning more than one color to "2", will result in purple, yellow, white, etc.
const PROGMEM char LEDValues[80][3] = //R, G, B
{
  {0, 0, 2}, //1
  {2, 2, 0},
  {2, 0, 0},
  {2, 0, 0},
  {2, 0, 0},
  {2, 0, 0},
  {2, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 2},
  {2, 0, 0}, //11
  {2, 0, 0},
  {2, 0, 0},
  {2, 0, 0},
  {2, 0, 0},
  {2, 0, 0},
  {2, 0, 0},
  {2, 0, 0},
  {0, 0, 2},
  {0, 0, 2},
  {0, 2, 0}, //21
  {0, 0, 2},
  {0, 0, 2},
  {0, 0, 0},
  {0, 0, 2},
  {2, 0, 2},
  {2, 0, 2},
  {2, 0, 2},
  {0, 0, 0},
  {0, 0, 2},
  {2, 0, 0}, //31
  {2, 0, 0},
  {2, 0, 0},
  {2, 0, 0},
  {0, 2, 0},
  {0, 2, 0},
  {0, 2, 0},
  {0, 2, 0},
  {2, 0, 0},
  {0, 2, 0},
  {0, 2, 2}, //41
  {0, 0, 2},
  {2, 0, 0},
  {2, 0, 0},
  {0, 0, 2},
  {0, 2, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {2, 0, 0}, //51
  {2, 0, 0},
  {2, 0, 0},
  {2, 0, 0},
  {2, 0, 0},
  {2, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0}, //61
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {2, 0, 0}, //71
  {2, 0, 0},
  {0, 2, 0},
  {0, 2, 0},
  {0, 0, 0},
  {0, 0, 2},
  {0, 0, 2},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0}
};

//MIDI values (in char values, right now nothing on chan 16, so these are effectively empty)
//PLEASE SEE: http://www.indiana.edu/~emusic/etext/MIDI/chapter3_MIDI4.shtml
//Col1: Status Byte, i.e. CC on chan 16 = 10111111 = BF = 191
//Col2: Data Byte 1, i.e. CC number
//Col3: Data Byte 2, i.e. CC value (in this case CC OFF). 
//Col4: Alternate Data Byte 2, i.e. CC value ON. 
const PROGMEM char MIDIValues[80][4] =
{
  {191, 1, 0, 127}, //SLOT 1, Clean
  {191, 11, 0, 127}, //Blues driver
  {191, 12, 0, 127}, //Big muff
  {191, 51, 0, 127}, //fuzz factory
  {191, 52, 0, 127}, //pickle
  {191, 13, 0, 127}, //geiger
  {191, 7, 0, 127},
  {191, 80, 0, 127},
  {191, 81, 0, 127}, 
  {191, 80, 0, 127}, //Empty but kills all mod effects
  {176, 51, 0, 127},  //SLOT 11, POD FS1 on Chan 1
  {176, 52, 0, 127}, //FS2
  {176, 53, 0, 127}, //FS3
  {176, 54, 0, 127}, //FS4
  {176, 55, 0, 127}, //FS5
  {176, 56, 0, 127}, //FS6
  {176, 57, 0, 127}, //FS7
  {176, 58, 0, 127}, //FS8
  {191, 59, 0, 127}, //Exp 1 toggle
  {176, 69, 0, 127}, //POD Tuner
  {177, 14, 0, 127}, //21, M13 on Chan2, Effect 2A
  {177, 15, 0, 127}, //M13 2B
  {177, 16, 0, 127}, //M13 2C
  {191, 24, 0, 127}, 
  {177, 36, 0, 127}, //M13 Looper Half time
  {177, 11, 0, 127}, //M13 1A
  {177, 12, 0, 127}, //M13 1B
  {177, 13, 0, 127}, //M13 1C
  {191, 29, 0, 127},
  {177, 85, 0, 127}, //M13 Looper Forward/Rev
  {177, 20, 0, 127}, //31, M13 4A
  {177, 21, 0, 127}, //M13 4B
  {177, 22, 0, 127}, //M13 4C
  {177, 50, 64, 127}, //M13 Record
  {177, 28, 0, 127},  //M13 Play
  {177, 17, 0, 127}, //M13 3A
  {177, 18, 0, 127}, //M13 3B
  {177, 19, 0, 127}, //M13 3C
  {177, 50, 0, 63}, //M13 Overdub
  {177, 80, 0, 127}, //M13 Play once
  {178, 80, 0, 127}, //41, Hex #1-1 on Chan 3, cc80, Mooer
  {178, 81, 0, 127}, //Hex 1-2, Shaker
  {178, 82, 0, 127}, //Hex 1-3, Fuzz factory
  {178, 83, 0, 127}, //Hex 1-4, Geiger
  {178, 84, 0, 127}, //Hex 1-5, Phaser
  {178, 85, 0, 127}, //Hex 1-6, 616
  {191, 47, 0, 127},
  {191, 48, 0, 127},
  {191, 49, 0, 127},
  {191, 50, 0, 127},
  {178, 88, 0, 127}, //51 Hex #2-1 on Chan 3, cc88, Delay?
  {178, 89, 0, 127}, //Hex 2-2, pickle
  {178, 90, 0, 127}, //Hex 2-3, big muff
  {178, 91, 0, 127}, //Hex 2-4, ??
  {178, 92, 0, 127}, //Hex 2-5, ??
  {178, 93, 0, 127}, //Hex 2-6, ??
  {191, 57, 0, 127},
  {191, 58, 0, 127},
  {191, 59, 0, 127},
  {191, 60, 0, 127},
  {191, 41, 0, 127}, //61
  {191, 42, 0, 127},
  {191, 43, 0, 127},
  {191, 44, 0, 127},
  {191, 45, 0, 127},
  {191, 46, 0, 127},
  {191, 47, 0, 127},
  {191, 48, 0, 127},
  {191, 49, 0, 127},
  {191, 50, 0, 127},
  {176, 60, 64, 127}, //71, POD Looper rec
  {176, 60, 0, 63}, //overdub
  {176, 61, 0, 127}, //POD looper play
  {176, 62, 0, 127}, //POD play once
  {191, 63, 65, 127}, //empty
  {176, 65, 0, 127}, //POD forward /rev
  {176, 68, 0, 127}, //POD half /full
  {191, 58, 0, 127},
  {191, 59, 0, 127},
  {191, 60, 0, 127}
};

//Expression values (in char values)
//This should be lengthed as more effects utilize expression values
//Col1: Effect # (starting at 0)
//Col2: Exp # (starting at 0)
//Col3: Status Byte, i.e. CC on chan 16 = 10111111 = BF = 191
//Col4: Data Byte 1, i.e. CC number
//Col5: Min Value
//Col6: Max Value
const PROGMEM char ExpValues[4][6] =
{
  {25, 0, 177, 1, 0, 127}, //Test for M13 effect in effect position 26
  {26, 0, 177, 1, 0, 127},
  {27, 0, 177, 1, 0, 127}, 
  {18, 0, 176, 1, 0, 127} //Testing for POD FS1, will send exp 1 value to POD HD exp 1
};

//Linkage table for linked effects / presets
//This table should be lengthened, shortened with addition/removal of preset switches
//Col1: Driver Effect # (Starting at 0), this is the button you press to initiate the link
//Col2: Passenger Effect # (Starting at 0), this will be the recipient of the on / off function
//Col3: Toggle, this will either turn off (0) or on (1) the passenger effect
const PROGMEM char LinkTable[140][3] =
{
  {20, 21, 0}, //IE effect in pos 20 will turn off effect in pos 21 when switched on
  {20, 22, 0}, //It will also turn off effect in pos 22
  {21, 20, 0},
  {21, 22, 0},
  {22, 20, 0},
  {22, 21, 0},
  {25, 26, 0},
  {25, 27, 0},
  {26, 25, 0},
  {26, 27, 0},
  {27, 25, 0},
  {27, 26, 0},
  {30, 31, 0}, 
  {30, 32, 0},
  {31, 30, 0},
  {31, 32, 0},
  {32, 30, 0},
  {32, 31, 0},
  {35, 36, 0}, 
  {35, 37, 0},
  {36, 35, 0},
  {36, 37, 0},
  {37, 35, 0},
  {37, 36, 0},
  {0, 42, 0}, //clean button turns off all Hex Distortion pedals
  {0, 43, 0},
  {0, 50, 0},
  {0, 51, 0},
  {0, 52, 0},
  {0, 53, 0},
  {0, 1, 0}, //also turn off any other drive preset on first page
  {0, 2, 0},
  {0, 3, 0},
  {0, 4, 0},
  {0, 5, 0}, 
  {0, 6, 0},
  {1, 53, 1}, //drive button turns on blues driver but turns off everything else
  {1, 42, 0}, 
  {1, 43, 0},
  {1, 50, 0},
  {1, 51, 0},
  {1, 52, 0},
  {1, 0, 0}, //also turn off any other drive preset on first page
  {1, 2, 0},
  {1, 3, 0},
  {1, 4, 0},
  {1, 5, 0},
  {1, 6, 0}, 
  {2, 50, 1}, //guyatone button turns on guyatone but turns off everything else
  {2, 42, 0}, 
  {2, 43, 0},
  {2, 51, 0},
  {2, 52, 0},
  {2, 53, 0},
  {2, 0, 0}, //also turn off any other drive preset on first page
  {2, 1, 0},
  {2, 3, 0},
  {2, 4, 0},
  {2, 5, 0}, 
  {2, 6, 0},
  {3, 52, 1}, //Muff button turns on muff but turns off everything else
  {3, 53, 0}, 
  {3, 42, 0},
  {3, 43, 0},
  {3, 50, 0},
  {3, 51, 0},
  {3, 0, 0}, //also turn off any other drive preset on first page
  {3, 1, 0},
  {3, 2, 0},
  {3, 4, 0},
  {3, 5, 0},
  {3, 6, 0}, 
  {4, 51, 1}, //pickle button turns on pickle but turns off everything else
  {4, 52, 0}, 
  {4, 53, 0},
  {4, 42, 0},
  {4, 43, 0},
  {4, 50, 0},
  {4, 0, 0}, //also turn off any other drive preset on first page
  {4, 1, 0},
  {4, 2, 0},
  {4, 3, 0},
  {4, 5, 0}, 
  {4, 6, 0},
  {5, 42, 1}, //FF button turns on FF but turns off everything else
  {5, 43, 0}, 
  {5, 50, 0},
  {5, 51, 0},
  {5, 52, 0},
  {5, 53, 0},
  {5, 0, 0}, //also turn off any other drive preset on first page
  {5, 1, 0},
  {5, 2, 0},
  {5, 3, 0},
  {5, 4, 0}, 
  {5, 6, 0},
  {6, 43, 1}, //geiger button turns on geiger but turns off everything else
  {6, 42, 0}, 
  {6, 50, 0},
  {6, 51, 0},
  {6, 52, 0},
  {6, 53, 0},
  {6, 0, 0}, //also turn off any other drive preset on first page
  {6, 1, 0},
  {6, 2, 0},
  {6, 3, 0},
  {6, 4, 0}, 
  {6, 5, 0},
  {72, 70, 0}, //When play on POD looper is hit, stop recording
  {72, 73, 0}, //When also stop play once
  {73, 70, 0}, //likewise, when play once is hit, stop recording
  {73, 72, 0}, //and also stop normal play
  {34, 33, 0}, //When play on M13 looper is hit, stop recording
  {34, 39, 0}, //When also stop play once
  {39, 33, 0}, //likewise, when play once is hit, stop recording
  {39, 34, 0}, //and also stop normal play
  {9, 10, 0}, //Master kill FS1-8
  {9, 11, 0},
  {9, 12, 0},
  {9, 13, 0},
  {9, 14, 0},
  {9, 15, 0},
  {9, 16, 0},
  {9, 17, 0},
  {9, 20, 0}, //Master Kill all M13 except keep FX loop on
  {9, 21, 0},
  {9, 22, 0},
  {9, 25, 0},
  {9, 26, 0},
  {9, 27, 0},
  {9, 30, 1},//Keep effects loop on
  {9, 31, 0},
  {9, 32, 0}, 
  {9, 35, 0},
  {9, 36, 0},
  {9, 37, 0}, 
  {9, 40, 0}, //Kill Hex mods / delays (all mods)
  {9, 41, 0},
  {9, 44, 0},
  {9, 45, 0}
};

//Effect states (must be # pages * 10 = 80 as well)
boolean EffectState[80] = //Is effect engaged? This is dynamic and cannot be stored in FLASH
{
  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, //Clean preset on by default
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, //M13 effects loop is on by default
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

BYTES_VAL_T pinValues;
BYTES_VAL_T oldPinValues;

/* This function is essentially a "shift-in" routine reading the
 * serial Data from the 165N shift register chips and representing
 * the state of those pins in an unsigned integer (or long).
*/
BYTES_VAL_T read_shift_regs()
{
    long bitVal;
    BYTES_VAL_T bytesVal = 0;

    /* Trigger a parallel Load to latch the state of the data lines,
    */
    digitalWrite(clockEnablePin, HIGH);
    digitalWrite(ploadPin, LOW);
    delayMicroseconds(PULSE_WIDTH_USEC);
    digitalWrite(ploadPin, HIGH);
    digitalWrite(clockEnablePin, LOW);

    /* Loop to read each bit value from the serial out line
     * of the SN74HC165N.
    */
    for(int i = 0; i < DATA_WIDTH; i++)
    {
        bitVal = digitalRead(dataPin);

        /* Set the corresponding bit in bytesVal.
         *  x |= y;   // equivalent to x = x | y (bitwise OR)
         *  (bitVal << ((DATA_WIDTH-1) - i)); writes MSB to LSB
        */
        bytesVal |= (bitVal << ((DATA_WIDTH-1) - i)); 

        /* Pulse the Clock (rising edge shifts the next bit).
        */
        digitalWrite(clockPin, HIGH);
        delayMicroseconds(PULSE_WIDTH_USEC);
        digitalWrite(clockPin, LOW);
    }

    return(bytesVal);
}

//send MIDI message, not using the MIDI library for this.
void MIDImessage(char StatusByte, char DataByte1, char DataByte2) //Trying out Char instead of INT
{
  Serial.write(StatusByte); //Issues MIDI byte in the correct order. Status byte = CC + channel #
  Serial.write(DataByte1); //ie send CC #
  Serial.write(DataByte2); //ie send CC value
}

void SendMIDI(int EffectNumber) //Gets Effect MIDI info and passes it to MIDImessage (above)
{
  char StatusByte, DataByte1, DataByte2;
  StatusByte = pgm_read_byte(&(MIDIValues[EffectNumber][0]));
  DataByte1 = pgm_read_byte(&(MIDIValues[EffectNumber][1]));
  if (EffectState[EffectNumber] == 0) //If effect is off, get "off" value
  {
    DataByte2 = pgm_read_byte(&(MIDIValues[EffectNumber][2]));
  }
  else //Otherwise get "on" value 
  {
    DataByte2 = pgm_read_byte(&(MIDIValues[EffectNumber][3]));
  }
  MIDImessage(StatusByte, DataByte1, DataByte2);
}

//Display LED status for a given page accordingly
void ChangeLEDs()
{
  int y;
  char red, green, blue;
  for (char x = 0; x < 10; x++)
  { 
    y = x + CurrentPage * 10;
    red = pgm_read_byte(&(LEDValues[y][0])); //Needed for PROGMEM reads, ampersand specifies memory address
    green = pgm_read_byte(&(LEDValues[y][1])); //Lots of other PROGMEM read functions for different types
    blue = pgm_read_byte(&(LEDValues[y][2]));
    if(EffectState[y] == 1) //If the effect is ON, multiply LED values by 10 to make it brighter
    {
      red = red * 10;
      green = green * 10;
      blue = blue * 10;
    }
    ShiftPWM.SetRGB(x, red, green, blue); //Update LED value
  }
}


//Handle Linked effects
void LinkToggle(int EffectNumber)
{
  char driver, passenger;
  boolean toggle_val;
  for (int x = 0; x < 140; x++) //THIS MUST BE THE SAME SIZE AS THE LENGTH OF LinkTable
  { 
    driver = pgm_read_byte(&(LinkTable[x][0])); //Needed for PROGMEM reads, ampersand specifies memory address
    passenger = pgm_read_byte(&(LinkTable[x][1])); //Lots of other PROGMEM read functions for different types
    toggle_val = pgm_read_byte(&(LinkTable[x][2]));
    if((driver == EffectNumber) && EffectState[EffectNumber] == 1) //If the effectnumber is found in the linktable and just switched on:
    {
      if(EffectState[passenger] != toggle_val) //only toggle if needed
      {
        EffectState[passenger] = toggle_val; // sets on/off to appropriate value
        SendMIDI(passenger); //sends a midi signal 
      }
    }
  }
}

void DisplayPageName()
{
  char NameBuffer[16];    // make sure this is large enough for the largest string it must hold (16 for LCD row)
  strcpy_P(NameBuffer, (char*)pgm_read_word(&(PageNameTable[CurrentPage]))); // Necessary casts and dereferencing
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(NameBuffer);
}



void DisplayEffectName(int EffectNumber)
{
  char NameBuffer[16];    // make sure this is large enough for the largest string it must hold (16 for LCD row)
  strcpy_P(NameBuffer, (char*)pgm_read_word(&(EffectNameTable[EffectNumber]))); // Necessary casts and dereferencing
  lcd.setCursor(0,1);
  lcd.print("                "); //Clears bottom line
  lcd.setCursor(0,1);
  lcd.print(NameBuffer);
}

void GetExpStatus() //General Exp pedal function
{
  char effectnumber;
  char expnumber;
  char StatusByte;
  char DataByte1;
  char Min;
  char Max;
  int NewValue;
  char counter = 0;
  for (char x = 0; x < 4; x++) //This MUST be the same length as the ExpValues array
  { 
    effectnumber = pgm_read_byte(&(ExpValues[x][0])); //Gets the effect number from the ExpValue array for lookup in EffectState
    if(EffectState[effectnumber] == 1) //If the effect is ON, take appropriate steps
    {
      expnumber = pgm_read_byte(&(ExpValues[x][1])); //get the appropriate expression number
      StatusByte = pgm_read_byte(&(ExpValues[x][2]));
      DataByte1 = pgm_read_byte(&(ExpValues[x][3]));
      Min = pgm_read_byte(&(ExpValues[x][4]));
      Max = pgm_read_byte(&(ExpValues[x][5]));
      NewValue = map(analogRead(expnumber), 0, 1023, (Min - 2),  (Max + 2)); // maps 0 to 1023 to between min and max. Padding of 2 ensures full range.
      NewValue = min(NewValue, Max); //Constrain to maximum
      NewValue = max(NewValue, Min); //Constrain to minimum
      if (RawExpVals[expnumber] != NewValue) //Only send message if we're changing values
      {
        RawExpVals[expnumber] = NewValue; //update the pre-existing value
        MIDImessage(StatusByte, DataByte1, NewValue); //Send MIDI
      }
      counter = counter + 1; 
    }
  }

  if (counter > 0) // if counter is greater than zero, at least one effect is using expression, turn on ExpState
  {
    ExpState = 1;
  }
  else
  {
    ExpState = 0;
  }
}

void TapTempo()
{
  tap_time = time;
  MIDImessage(176, 64, 127); //Tap tempo for POD
  MIDImessage(177, 64, 127); //Tap tempo for M13
}

void CheckMIDIIn() //checks if MIDI in signal does anything, note this will not send any midi signals, only update status, may require MIDI thru to work in practice
{
  char StatusByte, DataByte1, offByte, onByte;
  for (int x = 0; x < 80; x ++) //MUST BE SAME LENGTH AS ARRAY
  {
    StatusByte = pgm_read_byte(&(MIDIValues[x][0])); //look through table and get each status byte
    DataByte1 = pgm_read_byte(&(MIDIValues[x][1])); //also get DataByte1
    offByte = pgm_read_byte(&(MIDIValues[x][2])); //get off byte (usually 0)
    onByte = pgm_read_byte(&(MIDIValues[x][3])); //get on byte (usually 127)
    if (inChan == StatusByte && inDB1 == DataByte1) //if MIDI in signal matches those in the array, evaluate
    {
      if (inDB2 == offByte) // toggle effect off
      {
        EffectState[x] = 0;
        ChangeLEDs();
      }
      if (inDB2 == onByte) // toggle effect on
      {
        EffectState[x] = 1;
        ChangeLEDs();
      }
    }
  }
}

void setup()
{
  // Sets the number of 8-bit registers that are used.
  ShiftPWM.SetAmountOfRegisters(numRegisters);

  ShiftPWM.SetPinGrouping(1); //This is the default
  
  ShiftPWM.Start(pwmFrequency,maxBrightness);

  //LCD stuff, not currently implemented in loop
  lcd.begin(16,2);             // 16 characters, 2 rows
  lcd.clear();
  lcd.setCursor(0,0); //Write to top row
  lcd.print("Page INIT");
  lcd.setCursor(0,1); //Write to bottom row
  lcd.print("Effect INIT");

  //PISO stuff:
    /* Initialize digital pins...
    */
    pinMode(ploadPin, OUTPUT);
    pinMode(clockEnablePin, OUTPUT);
    pinMode(clockPin, OUTPUT);
    pinMode(dataPin, INPUT);

    digitalWrite(clockPin, LOW);
    digitalWrite(ploadPin, HIGH);

    /* Read in and display the pin states at startup.
    */
    pinValues = read_shift_regs();
    //display_pin_values();
    oldPinValues = pinValues;


// Turn all LED's off.
  ShiftPWM.SetAll(0);

//Turn on just these 3:
  ShiftPWM.SetRGB(10, 5, 0, 0); //Page down LED
  ShiftPWM.SetRGB(11, 0, 5, 0); //Page up LED
  //ShiftPWM.SetRGB(12, 0, 0, 5); //Tap tempo LED

  MIDI.begin(MIDI_CHANNEL_OMNI); //needed for midi library, set to listen to all channels
  MIDI.turnThruOff(); //Turns off MIDI thru
}



void loop()
{    
  time = millis(); //gets current time
  
//Read the state of all buttons:
    pinValues = read_shift_regs();

//Only do stuff if a button was pushed:
//Should probably put most code in here
    if(pinValues != oldPinValues) // Note there is not currently any reassigning of oldPinValues, so only reacting to any button presses
    {
      for (int x = 0; x < 10; x++) //Check each button 1-10 for a change and toggle its effect state
      {
        if((pinValues >> x) & 1) 
        {
          EffectState[x + CurrentPage * 10] = !EffectState[x + CurrentPage * 10]; //Toggle effect state
          LinkToggle(x + CurrentPage * 10); //Ensures linked effects are handled
          SendMIDI(x + CurrentPage * 10); // Sends the appropriate MIDI message
          ChangeLEDs();
          DisplayEffectName(x + CurrentPage * 10); //This is slow and thus after LED update
          delay(250); //Crude debouncing 250ms
        }
      }
      
      if((pinValues >> 10) & 1) //Page Down
      {
        if(CurrentPage == 0)
        {
          CurrentPage = 7; //Goes to page 6 if down is pressed from page 0
        }
        else
        {
          CurrentPage = CurrentPage - 1; //Otherwise just decrement
        }
        DisplayPageName();
        ChangeLEDs();
        delay(250); //Crude debouncing 250ms
      }
      
      if((pinValues >> 11) & 1) //Page Up
      {
        if(CurrentPage == 7)
        {
          CurrentPage = 0; //Goes to page 0 if up is pressed from page 6
        }
        else
        {
          CurrentPage = CurrentPage + 1; //Otherwise just increment
        }
        DisplayPageName();
        ChangeLEDs();
        delay(250); //Crude debouncing 250ms
      }

      if((pinValues >> 12) & 1) //Tap Tempo
      {
        if((time - tap_time > 100) && (time - tap_time < 2000)) // must be between 100ms and 2s
        {
          tap_interval = time - tap_time; //reassign interval
        }
        TapTempo(); //send MIDI and get new tap_time
        delay(200); //Crude debouncing 200ms, little more conservative since it's tap tempo
      }
      
      //Evaluate if any of the Expression effects are now on:
      GetExpStatus();
      
    }

   //if any expression pedals are in use, execute the exp function 
   if (ExpState == 1)
  {
    GetExpStatus();
    delay(5); //Expression works better with a slight delay to prevent excessive sampling and serial port slowdown
  }

  //Testing Blinking LEDs:
  if (time - timer > tap_interval)
  {
    ShiftPWM.SetRGB(12, 0, 0, 5); //Tap tempo LED on (blue)
    timer = time; //reset timer
  }
  if (time - timer > 50) //wait 50ms
  {
    ShiftPWM.SetRGB(12, 0, 0, 2); //Tap tempo LED off (blue)
  }
   
//READ MIDI IN
if (MIDI.read())                // Is there a MIDI message incoming ?
    {
        switch(MIDI.getType())      // Get the type of the message we caught
        {
            case midi::ControlChange:       // If it is a Control Change,
                inChan = MIDI.getChannel(); //assign channel
                inChan = inChan + 175; //add this to get the total byte value
                inDB1 = MIDI.getData1(); //assign databyte 1
                inDB2 = MIDI.getData2(); //assign databyte 2
                CheckMIDIIn(); //Check if this applies to any effect state
                break;
            // See the online reference for other message types
            default:
                break;
        }
    }

    delay(POLL_DELAY_MSEC);
}
