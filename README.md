# SDDS Text Display Spike

## 🔗 Overview  
This extension builds on [SDDS](https://github.com/mLamneck/SDDS) and enables you to create a generic user interface for an LCD text display with just a few lines of code.
The UI structures variables and their values in an intuitive way, allowing easy navigation and value editing in various formats.
If you're not familiar with SDDS, check out the [documentation](https://github.com/mLamneck/SDDS?tab=readme-ov-file#documentation) first.  

Currently, we provide our own implementation for `CrystalFontz635` displays. Additionally, we support all displays compatible with the [LiquidCrystal](https://github.com/arduino-libraries/LiquidCrystal) library.  

> **Note:** LiquidCrystal is licensed under LGPL. Due to its more restrictive licensing model, you must install it manually. Make sure to comply with the license requirements.  

## ✨ Features
- **Hierarchical SDDS data structure** with intuitive navigation
- **Live updates** of variable values
- **Value editing** in hex, binary, or decimal format
- **Storage of cursor positions and display states** for each menu
- **Abstract display interface**, allowing support for various displays
- **Currently available implementations**:
  - Console application
  - CrystalFontz LCD
  - all displays working with LiquidCrystal lib
- **Optimized performance** through screen mirroring: Only changes are transmitted

## 🛠 Installation

### Arduino IDE

This library uses the SDDS Core library, which has to be installed first when using Arduino IDE:

 1. Clone the [SDDS Core Library](https://github.com/mLamneck/SDDS) into your library folder.
 2. Clone this repository into your library folder.
 3. Install [LiquidCrystal](https://github.com/arduino-libraries/LiquidCrystal).

### PlatformIO
[PlatformIO](http://platformio.org) is an open source ecosystem for IoT development with cross platform build system, library manager and full support for Espressif ESP8266/ESP32 development. It works on the popular host OS: Mac OS X, Windows, Linux 32/64, Linux ARM (like Raspberry Pi, BeagleBone, CubieBoard).

1. Install [PlatformIO IDE](http://platformio.org/platformio-ide)
2. Create new project using "PlatformIO Home > New Project"
3. Add "SDDS_textDisplaySpike" to project using [Project Configuration File `platformio.ini`](http://docs.platformio.org/page/projectconf.html) and [lib_deps](http://docs.platformio.org/page/projectconf/section_env_library.html#lib-deps) option:

```ini
[env:d1_mini_lite]
platform = espressif8266
board = d1_mini_lite
framework = arduino
lib_deps = https://github.com/mLamneck/SDDS_textDisplaySpike
lib_deps = https://github.com/arduino-libraries/LiquidCrystal
```

Happy coding with PlatformIO!

## ⚖️ Getting Started

The following example code is taken from [SDDS](https://github.com/mLamneck/SDDS). It can turn the bord led on/off, let it blink with adjustable intervals. The state can be saved into non-volatile memory. You can find a lot of information about the example and the concepts of sdds on the original page. Here we assume you are already familiar with the concept. You should have a display properly connected to your board. This examples uses a `CrystalFontz CFA635` in UART configuration. It needs a power supply of 5V and RX, TX of an UART, so a total of 4 wires. In this example we use Serial2 for the display and the default Serial for our serialSpike. But this is up to you. 

```cpp
#include "uTypedef.h"
#include "uMultask.h"
#include "uParamSave.h"

sdds_enum(OFF,ON) TonOffState;

class Tled : public TmenuHandle{
    Ttimer timer;
    public:
		sdds_var(TonOffState,ledSwitch,sdds::opt::saveval)
		sdds_var(TonOffState,blinkSwitch,sdds::opt::saveval)
		sdds_var(Tuint16,onTime,sdds::opt::saveval,500)
		sdds_var(Tuint16,offTime,sdds::opt::saveval,500)

		Tled(){
			pinMode(LED_BUILTIN, OUTPUT);

			on(ledSwitch){
				if (ledSwitch == TonOffState::e::ON) digitalWrite(LED_BUILTIN,0);
				else digitalWrite(LED_BUILTIN,1);
			};

			on(blinkSwitch){
				if (blinkSwitch == TonOffState::e::ON) timer.start(0);
				else timer.stop();
			};

			on(timer){
				if (ledSwitch == TonOffState::e::ON){
					ledSwitch = TonOffState::e::OFF;
					timer.start(offTime);
				} 
				else {
					ledSwitch = TonOffState::e::ON;
					timer.start(onTime);
				}
			};
		}
};

class TuserStruct : public TmenuHandle{
  public:
    sdds_struct(
        sdds_var(Tled,led)
        sdds_var(TparamSaveMenu,params)
    )
    TuserStruct(){
        //you application code goes here... 
    }
} userStruct;

#include "uSerialSpike.h"
TserialSpike serialHandler(userStruct,115200);

//***************************************************
// this code is new to add the display functionality
#include "uTextDisplaySpike.h"
#include "uCrystalFontzCFA635.h"
typedef sdds::textDisplaySpike::TcrystalFontzCFA635<4, 20, Stream> Tdisplay;
Tdisplay disp(&Serial2);		//use the seial your display is connceted to
TtextDisplaySpike<Tdisplay> tds(userStruct,disp);
//***************************************************

void setup(){

}

void loop(){
  TtaskHandler::handleEvents();
}

```

## 📚 How to use the code with other displays

There are a few levels ob abstraction that should make it fairly easy to support new types of displays, especially if you have already a driver for it. 

The functions we need from this drivers are.

* setCursorPosition
* writeStrToPosition
* clearScreen

Additionally we need a function to read the input (pressed keys or rotary encoder). This is done by implementing a function `readKey` and by providing a mapping for the keys. We will see how that looks like in a minute.Let's start with an examples.

### Console Display for windows

For the developement we have used a virtual display in the terminal of our developement environment (The programm was running as a console application on windows). So we need to implement the methods mentioned above to write to the console. Let's aks the KI of your choice to implement these 3 functions for us. "Please provide 3 methods ... to set the cursor in a console application on windows using c++,...". In my case, after some back and forth, it came up with the following. I aksed about reading the keyboard as well, because we need it for the navigation anyway. Specifically we need 6 Keys: UP, DOWN, LEFT, RIGHT, ENTER, ESC.

```C++

#include <conio.h>
#include <windows.h>

void clearScreen(){ std::cout << "\033[H\033[J"; }
void gotoxy(int x, int y) { std::cout << "\033[" << y+1 << ";" << x+1 << "H"; }
void writeToConsole(int x, int y, const char* text, int length) {
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD position = {static_cast<SHORT>(x), static_cast<SHORT>(y)};
	DWORD written;
	WriteConsoleOutputCharacter(hConsole, text, length, position, &written);
}

constexpr static int SDDS_TDS_KEY_LEFT = 75;
constexpr static int SDDS_TDS_KEY_RIGHT = 77;
constexpr static int SDDS_TDS_KEY_UP = 72;
constexpr static int SDDS_TDS_KEY_DOWN = 80;
constexpr static int SDDS_TDS_KEY_ESC = 27;
constexpr static int SDDS_TDS_KEY_ENTER = 13;

int readKey(){
	if (!_kbhit()) return 0;
	return  _getch();
}
```

We give it a try in a simple hello world application and it works. No we have everything we need to implement the new class for our `textDisplaySpike`. We create a new file `uConsoleDisplay.h` with the typical include guards and the includes provided from the AI. We put everything in a namespace sdds::textDisplaySpike. You don't have to do it, but it helps to avoid naming conflicts when having a lot of files. We create a class TconsoleDisplay which inherits from TabstractDisplay. TabstractDisplay is a template class and expects 2 parameters.
* the number of rows the display has
* the number of characters in one row.

In our case we are not really limited but we want to emulate some real display and maybe use it to debug different displays. That's why we make our TconsoleDisplay a template class itself. This way we can decide later how many rows and columns we have. We copy the functions provided by the AI into the class. Now we have to implement the 3 functions mentioned above:

```C++

//within the class
//...

void doSetCursor(const TcursorInterface _cursor) override{ 
	gotoxy(_cursor.x,_cursor.y);
	this->onTaskDone();
}

void doClear() override{
	clearScreen();
	this->onTaskDone();
}

void doUpdateRow(TrowChanges _changes) override {
	writeToConsole(_changes.firstChangedIdx, _changes.row, _changes._buffer, _changes.n);
	this->onTaskDone();
}
```

The implementation of these funtions is now very easy, we just call the functions provided by the AI (for other displays provided by a library for that display). After the function has been called, we have to call this->onTaskDone() in order to handshake the operation with the base class. The `readKey` method just returns the key pressed on the keyboard. We provide constants for the keys the `textDisplaySpike` is interested in. The final class looks like this.


```C++
#ifndef UCONSOLEDISPLAY_H
#define UCONSOLEDISPLAY_H

#include "uAbstractTextDisplay.h"
#include <conio.h>
#include <windows.h>

namespace sdds{
	namespace textDisplaySpike{

		template <int nRows, int nColumns>
		class TconsoleDisplay : public TabstractTextDisplay<nRows,nColumns>{
			public:
				constexpr static int KEY_LEFT = 75;
				constexpr static int KEY_RIGHT = 77;
				constexpr static int KEY_UP = 72;
				constexpr static int KEY_DOWN = 80;
				constexpr static int KEY_ESC = 27;
				constexpr static int KEY_ENTER = 13;

				int readKey(){
					if (!_kbhit()) return 0;
					return  _getch();
				}
			private:
				/**
				 * functions to be implemented for a specific os, this works only for windows
				 * - clearScreen
				 * - writeToConsole
				 * - gotoxy
				 */
				void clearScreen(){ std::cout << "\033[H\033[J"; }
				void gotoxy(int x, int y) { std::cout << "\033[" << y+1 << ";" << x+1 << "H"; }
				void writeToConsole(int x, int y, const char* text, int length) {
					HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
					COORD position = {static_cast<SHORT>(x), static_cast<SHORT>(y)};
					DWORD written;
					WriteConsoleOutputCharacter(hConsole, text, length, position, &written);
				}

				void doSetCursor(const TcursorInterface _cursor) override{ 
					gotoxy(_cursor.x,_cursor.y);
					this->onTaskDone();
				}

				void doClear() override{
					clearScreen();
					this->onTaskDone();
				}
				
				void doUpdateRow(TrowChanges _changes) override {
					writeToConsole(_changes.firstChangedIdx, _changes.row, _changes._buffer, _changes.n);
					this->onTaskDone();
				}

		};


	}
}

#endif //UCONSOLEDISPLAY_H
```

Now we can use it like so:

```C++
//some coding with sdds providing a TuserStruct...

TuserStruct userStruct;


//adding the dislay funtionality to our existing code
typedef sdds::textDisplaySpike::TconsoleDisplay<4, 20> TconsoleDisplay;
TconsoleDisplay display;
TtextDisplaySpike<TconsoleDisplay> tds(userStruct,display);
```

First of all we provide a specific definition of our TconsoleDisplay (with a specific number of rows and columns) by the typedef in the first line. Next we create an instance of this display. And last but not least we create our TtextDisplaySpike with this specific display to work on our userStruct.

### TextDisplaySpike using LyquidCrystal library for Arduino

The LiquidCrystal library allows you to control LCD displays that are compatible with the Hitachi HD44780 driver. There are many of them out there, and you can usually tell them by the 16-pin interface.

> **Note**:
> The following is not tested! It has not even been compiled. So there might be some issues with it. 
> But it should be easy to make it work.

Like said, the libray can be used for a lot of displays, so in fact if we provide a wrapper for our `TextDisplaySpike`, this can be used for a lot of displays as well. 
We start again with a new file `uLiquidCrystal4TDS.h` (LiquidCrystal for Text Display Spike).

```C++
//include guard...

#include "uAbstractTextDisplay.h"
#include "LiquidCrystal.h"

namespace sdds{
	namespace textDisplaySpike{
	
		/**
		 * read the keys from GPIO
		 */
		template <int LEFT, int RIGHT, int UP, int DOWN, int ENTER, int ESCAPE>
		class TgpioKeyPad{
			public:
				//numbers randomly chosen
				constexpr static int SDDS_TDS_KEY_LEFT = 75;
				constexpr static int SDDS_TDS_KEY_RIGHT = 77;
				constexpr static int SDDS_TDS_KEY_UP = 72;
				constexpr static int SDDS_TDS_KEY_DOWN = 80;
				constexpr static int SDDS_TDS_KEY_ESC = 27;
				constexpr static int SDDS_TDS_KEY_ENTER = 13;

				dtypes::uint8 FkeyStates[6];
				bool keyPressed(int _keyIdx, int _pin){
					if (digitalRead(_pin)){
						if (FkeyStates[_keyIdx] == 0){
							FkeyStates[_keyIdx] = 1;
							return true;
						}
					} else
						FkeyStates[_keyIdx] = 0;
					return false;
				}

				void setupKeys(){
					pinMode(LEFT,INPUT);
					pinMode(RIGHT,INPUT);
					pinMode(UP,INPUT);
					pinMode(DOWN,INPUT);
					pinMode(ESCAPE,INPUT);
					pinMode(ENTER,INPUT);
				}
				
				int readKey(){
					if (keyPressed(0,LEFT)) 	return SDDS_TDS_KEY_LEFT;
					if (keyPressed(1,RIGHT)) 	return SDDS_TDS_KEY_RIGHT;
					if (keyPressed(2,UP)) 		return SDDS_TDS_KEY_UP;
					if (keyPressed(3,DOWN)) 	return SDDS_TDS_KEY_DOWN;
					if (keyPressed(4,ENTER)) 	return SDDS_TDS_KEY_ENTER;
					if (keyPressed(5,ESCAPE)) 	return SDDS_TDS_KEY_ESC;

					return 0;
				}
		};

		/*
		 * Wrapper around LiquidCrystal
		 */
		template <
			int nRows, int nColumns
			,int rs, int en, int d4, int d5, int d6, int d7 
			,int LEFT, int RIGHT, int UP, int DOWN, int ENTER, int ESCAPE 
		>
		class TliquidCrystal4TDS : public TabstractTextDisplay<nRows,nColumns>, public TgpioKeyPad<LEFT,RIGHT,UP,DOWN,ENTER,ESCAPE>{
			public:
				TliquidCrystal4TDS() : Flcd(rs,en,d4,d5,d6,d7)
				{
				}

				void begin(){
					Flcd.begin(nColumns,nRows);
					this->setupKeys();
				}

			private:
				LiquidCrystal Flcd;
				

				void doSetCursor(const TcursorInterface _cursor) override{ 
					Flcd.setCursor(_cursor.x, _cursor.y);
					this->onTaskDone();
				}

				void doClear() override{
					Flcd.clear();
					this->onTaskDone();
				}
				
				void doUpdateRow(TrowChanges _changes) override {
					Flcd.setCursor(_changes.firstChangedIdx,_changes.row);
					for (int i = 0; i < _changes.n; i++)
						Flcd.write(_changes._buffer[i]);
					Flcd.setCursor(this->Fcursor.x,this->Fcursor.y);
					this->onTaskDone();
				}

		};


	}
}
```

The main thing we are doing here is to implement the 3 methods
* doSetCursor
* doClear
* doUpdateRow

by forwarding it to the LiquidCrystal library. To update a row at a specific column with a substring, we set the cursor
to the first character that need to be changed, write all the characters that have been changed and set the
cursor back to the original position. There might be a better way of doint this but according to the
documentation it seems like it is not.

The reading of the GPIO Pins for the Keypad, we created a seperate class to make this code reusable (we might want to use this for other displays as well).

The usage of our new created class would look like this:

```C++
//... provide a TuserStruct according to sdds
//...

TuserStruct userStruct;

// some more spikes interacting with userStruct...
//...

#include "uLiquidCrystal4TDS"
typedef sdds::textDisplaySpike::TliquidCrystal4TDS<
	4,20						//rows, columns
	,1,2,3,4,5,6				//pins for liquid crystal lib (rs, en, ...)
	,10,11,12,13,14,15			//gpio pins
> TlcDisplay;
TlcDisplay display;
TtextDisplaySpike<TlcDisplay> tds(us,display);


void setup(){
	diplay.begin();
}
```

## 📖 Further Information
- [SDDS Repository](https://github.com/your-sdds-repo)
- [CrystalFontz LCDs](https://www.crystalfontz.com/)
- [LiquidCrystal library](https://github.com/arduino-libraries/LiquidCrystal)
