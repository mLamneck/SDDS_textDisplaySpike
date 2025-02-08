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
					gotoxy(this->getCursor().x,this->getCursor().y);
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