#ifndef UABSTRACTTEXTDISPLAY_H
#define UABSTRACTTEXTDISPLAY_H

namespace sdds{
	namespace textDisplaySpike{

		template <int nRows, int nColumns>
		struct TdisplayBuffer{
			char Fbuffer[nRows][nColumns];
			char* operator[](int row) { return Fbuffer[row]; }
			const char* operator[](int row) const { return Fbuffer[row]; }
		};

		struct TrowChanges{
			const char* _buffer = nullptr;
			int row;
			int firstChangedIdx;
			int lastChangedIdx;
			int n = 0;
			bool hasChanges(){ return _buffer != nullptr; }
		};

		struct TcursorInterface{
			int x=0;
			int y=0;
		};

		template <int nRows, int nColumns>
		struct Tcursor : public TcursorInterface{
			public:
				void right(){ x++; }
				void left(){ x=x>0?x-1:0; }
				void up(){ y=y>0?y-1:0; }
				void down(){ y++; }
		};

		class TabstractTextDisplayInterface : public Tthread{
			protected:
				Tevent FupdateEvent;
				Tevent FevHandshake;

				//to be called by specialization if display is ready to receive new commands
				void onTaskDone() { FevHandshake.signal(); };

				//to be overridden by the deriving class
				virtual void doClear(){ }
				virtual void doSetCursor(const TcursorInterface _cursor){ }
				virtual void doUpdateRow(const TrowChanges _changes){}
			public:
				TabstractTextDisplayInterface()
					: FupdateEvent(this)
					, FevHandshake(this,1)
				{

				}
		};

		template <int nRows, int nColumns>
		class TabstractTextDisplay : public TabstractTextDisplayInterface{
			private:
				TdisplayBuffer<nRows,nColumns> FcurrContent;
				TdisplayBuffer<nRows,nColumns> FnextContent;
				bool FclearScreen = false; 
			protected:

			public:
				typedef Tcursor<nRows,nColumns> _Tcursor;
				constexpr static int N_LINES = nRows;
				constexpr static int N_COLUMNS = nColumns;

				TabstractTextDisplay()
				{
					FdisplayCursor.x = -1;
					FdisplayCursor.y = -1;
				}
				
				_Tcursor getCursor(){ return Fcursor; }

				void clear(){
					for (auto row=0; row< nRows; row++){
						for (auto col=0; col < nColumns; col++){
							FcurrContent[row][col] = ' ';
							FnextContent[row][col] = ' ';
						}
					}
					FclearScreen = true;
					FupdateEvent.signal();
				}

				void setCursor(const _Tcursor _cursor){ 
					Fcursor = _cursor;
					FupdateEvent.signal();
				}

				bool write(int _row, int _col, char c){
					if ((_row >= nRows) || (_col >= nColumns)) return false;
					FnextContent[_row][_col] = c;
					FupdateEvent.signal();
					return true;
				}

			protected:
				_Tcursor Fcursor;
				_Tcursor FdisplayCursor;

				TrowChanges getChangesInRow(int _row){
					TrowChanges c = {};
					if (_row >= nRows) return c;

					for (auto i = 0; i < nColumns; i++){
						if (FcurrContent[_row][i] != FnextContent[_row][i]){
							c._buffer = &FnextContent[_row][i];
							c.firstChangedIdx = i;
							break;
						} 
					}
					if (c._buffer){
						for (auto i = c.firstChangedIdx; i < nColumns; i++){
							if (FcurrContent[_row][i] != FnextContent[_row][i])
								c.lastChangedIdx = i;
						}
						c.n = c.lastChangedIdx-c.firstChangedIdx+1;
						c.row = _row;
					}
					return c;
				}	

			private:
				int FrowToUpdate = 0; 
				
				bool _handleUpdate(int _firstRow, int _lastRow){
					while (_firstRow <= _lastRow){
						auto c = getChangesInRow(_firstRow);
						if (c.hasChanges()){
							FrowToUpdate = _firstRow+1 < nRows? _firstRow+1 : 0;
							doUpdateRow(c);
							memcpy(&FcurrContent[c.row][c.firstChangedIdx],&FnextContent[c.row][c.firstChangedIdx],c.n);
							setPriority(1);
							return true;
						}
						_firstRow++;
					}
					return false;
				}

				void handleUpdate(){
					if (FclearScreen){
						FclearScreen = false;
						doClear();
						setPriority(1);
						return;
					}

					if ((Fcursor.x != FdisplayCursor.x) || (Fcursor.y != FdisplayCursor.y)){
						FdisplayCursor = Fcursor;
						doSetCursor(Fcursor);
						setPriority(1);
						return;
					}

					auto updateStart = FrowToUpdate;
					if (_handleUpdate(updateStart,nRows-1)) return;
					if (_handleUpdate(0,updateStart-1)) return;
					
					setPriority(0);
					FrowToUpdate = 0;
				}
				
				void execute(Tevent* _ev) override{
					if (_ev == &FupdateEvent || _ev == &FevHandshake)
						handleUpdate();
					if (isTaskEvent(_ev)){

					}
				}

			public:
				void begin(){}

		};

		/*
		template <int First, int... Rest>
		struct Sum {
			static constexpr int value = First + Sum<Rest...>::value;
		};

		template <int Last>
		struct Sum<Last> {
			static constexpr int value = Last;
		};

		template <int nRows, int... colWidths>
		class TtableTextDisplay : public TabstractTextDisplay<nRows,Sum<colWidths...>::value> {
			public:
			constexpr static int colWidthsArray[] = {colWidths...}; // Array mit Spaltenbreiten
			static constexpr int nColumns = sizeof...(colWidths);
			static constexpr int nTotalWidth = Sum<colWidths...>::value;

			void test(){
				int colW = colWidthsArray[0];
				std::cout<<colW<<std::endl;
			}
		};

		class TtestDisplay : public TtableTextDisplay<4,30,30>{

		} testDisp;

		*/

	}
}

#endif //UABSTRACTTEXTDISPLAY_H
