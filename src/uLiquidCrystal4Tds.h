#include "LiquidCrystal.h"

namespace sdds{
	namespace textDisplaySpike{
		template <int LEFT, int RIGHT, int UP, int DOWN, int ENTER, int ESCAPE>
		class TgpioKeyPad{
			public:
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