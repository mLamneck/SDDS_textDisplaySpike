#ifndef UTEXTDISPLAYSPIKE_H
#define UTEXTDISPLAYSPIKE_H

#include "uAbstractTextDisplay.h"
#include "uMultask.h"
#include "uTypedef.h"
#include "uSddsToString.h"
#include "uEditors.h"

template<class TdisplayType>
class TtextDisplaySpike : Tthread{
	private:
		using TeditorBase = sdds::textDisplaySpike::TeditorBase;

		constexpr static int N_LINES = TdisplayType::N_LINES;
		constexpr static int N_COLUMNS = TdisplayType::N_COLUMNS;
		constexpr static int LAST_COLUMN = TdisplayType::N_COLUMNS - 1;
		constexpr static int NAME_COL_WIDTH = 10;
		constexpr static int VAL_COL_START = 11;
		constexpr static int VAL_COL_WIDTH = N_COLUMNS-VAL_COL_START;

		constexpr static int N_VIEWS = 20;

		TdisplayType* Fdisplay;
		TmenuHandle* Froot;
		Tevent FevReadKey;

		TobjectEvent FmenuEvent;

		sdds::textDisplaySpike::TeditorContainer FeditorContainer;
		Tdescr* FvalueInEditor = nullptr;
		Tdescr* getValueInEditor() { return FvalueInEditor; }
		void setValueInEditor( Tdescr* _d ) { FvalueInEditor = _d; }

		class Tview{
			public:
				Tstruct* menu = nullptr;
				dtypes::uint8 firstVisible = 0;		//first line of struct visible in display window
				dtypes::uint8 cursorY = 0;			//cursor pos in display window
				TmenuHandle* menuHandle() { return menu->value(); }
		};
		Tview FspareView;
		Tview Fviews[N_VIEWS];
		Tview* FcurrView;
		TmenuHandle* currMenu() { return FcurrView->menuHandle(); }

		dtypes::string FworkStr;

		Tview* findView(Tstruct* _menu){
			for (auto i = 0; i<N_VIEWS; i++)
				if (Fviews[i].menu == _menu) return &Fviews[i];
			for (auto i = 0; i<N_VIEWS; i++){
				if (Fviews[i].menu == nullptr){
					Fviews[i].menu = _menu;
					return &Fviews[i];
				} 
			}
			return &FspareView;
		}

		/*****************************************
		 * Helper functions for cursor
		******************************************/
		
		template <class Tcursor>
		void setCursor(Tcursor& _cursor){
			FcurrView->cursorY = _cursor.y;
			Fdisplay->setCursor(_cursor);
		}

		void setCursorX(int _x){
			auto c = Fdisplay->getCursor();
			c.x = _x;
			Fdisplay->setCursor(c);
		}


		/*****************************************
		 * Helper functions for columns
		******************************************/

		void nameColumnToDisplay(int _dispRow, Tdescr* d){
			const char* name = d->name();
			int colIdx = 0;
			while (*name != '\0'){
				Fdisplay->write(_dispRow,colIdx,*name++);
				if (colIdx++ >= NAME_COL_WIDTH-1) break;
			}
			while (colIdx <= NAME_COL_WIDTH-1)
				Fdisplay->write(_dispRow,colIdx++,' ');
		}

		void valueColumnToDisplay(int _dispRow, const char* _valStr){
			int fill = VAL_COL_WIDTH - strlen(_valStr);
			int colIdx = VAL_COL_START;
			if (fill > 0){
				while (fill-- > 0)
					Fdisplay->write(_dispRow, colIdx++, ' ');
			}
			else{
				while (fill++ < 0)
					_valStr++;
			}

			while (*_valStr != '\0')
				if (!Fdisplay->write(_dispRow,colIdx++,*_valStr++)) return;

		}

		void valueColumnToDisplay(int _dispRow, Tdescr* _d){
			sdds::to_string(FworkStr,_d);
			valueColumnToDisplay(_dispRow,FworkStr.c_str());
		}

		/**
		 * @brief displays the active menu
		 * 
		 * starting with the first visible row, it writes all available
		 * rows to the screen.
		 * 
		 * @param _clear clears the display before update
		 */
		void displayMenu(bool _clear = true){
			if (_clear) 
				Fdisplay->clear();
			auto it = currMenu()->iterator(FcurrView->firstVisible);
			for (int row = 0; row < N_LINES; row++){
				if (!it.hasCurrent()) break;
				Tdescr* d = it.current();
				if (d != getValueInEditor()){
					nameColumnToDisplay(row,d);
					valueColumnToDisplay(row,d);
				};
				it.jumpToNext();
			}
			auto cursor = Fdisplay->getCursor();
			cursor.y = FcurrView->cursorY;
			Fdisplay->setCursor(cursor);
		}

		Tdescr* itemUnderCursor(){
			auto cursor = Fdisplay->getCursor();
			return currMenu()->get(cursor.y+FcurrView->firstVisible);
		}

		void enterMenu(Tstruct* _menu){
			if (!_menu) return;

			FcurrView->menuHandle()->events()->remove(&FmenuEvent);
			FcurrView = findView(_menu);
			FcurrView->menu = _menu;
			FmenuEvent.setObservedRange(0,255);
			FcurrView->menuHandle()->events()->push_first(&FmenuEvent);
			displayMenu();
		}

		void leaveMenu(){
			enterMenu(FcurrView->menu->parent());
		}

		void editDone(){
			//workaround to restore the current value if the item was edited but
			//the editing has been canceled... we should change this by 
			//only updating the row. SignalEvents will trigger eventHandlers even
			//if the value has not been written
			if (getValueInEditor())
				getValueInEditor()->signalEvents();

			setValueInEditor(nullptr);
			setCursorX(0);
			FeditorContainer.destroy();
		}

		/*****************************************************
		 * key handling
		 *	up, down, left, right, enter, escape
		 *****************************************************/

		/**
		 * @brief Handles a key event by forwarding it to a type-specific editor class.
		 *
		 * This method is responsible for forwarding a key-related action to the 
		 * currently active editor instance. It invokes the provided method on 
		 * the active editor and updates the display accordingly.
		 *
		 * The method ensures that an editor is active before attempting to call
		 * the provided method. If no editor is active, the function returns `nullptr`.
		 *
		 * @param method Pointer to the member function of `TeditorBase` to be invoked.
		 *               This should be a method with no arguments and no return value.
		 * @return TeditorBase* Returns a pointer to the active editor if a value is 
		 *         currently being edited; returns `nullptr` otherwise.
		 *
		 * @note Ensure that the method passed as a parameter is valid and corresponds 
		 *       to the active editor's capabilities.
		 * 
		 * @example Usage:
		 * ```cpp
		 * TeditorBase* editor = handleKey(TeditorBase::keyUp);
		 * if (editor) {
		 *     // Perform additional actions on the active editor
		 * }
		 * ```
		 */
		bool handleKey(void (TeditorBase::*method)()) {
			auto editor = FeditorContainer.getInstance();
			if (!editor) return false;
			(editor->*method)();
			if (editor->editDone()){
				editDone();
				return true;
			}
			editor->getDisplayString(FworkStr);
			valueColumnToDisplay(Fdisplay->getCursor().y,FworkStr.c_str());
			setCursorX(editor->displayCursorPos() + VAL_COL_START);
			return true;
		}

		/**
		 * @brief handles a keyUp event
		 * 
		 * Forwards the event a type-specific editor class if one is active.
		 * Otherwise move the cursor or scroll the view if neccessary
		 * 
		 */
		void doOnkeyUp(){
			if (handleKey(&TeditorBase::keyUp)) return;

			auto cursor = Fdisplay->getCursor();
			if (cursor.y == 0 && FcurrView->firstVisible > 0){
				FcurrView->firstVisible--;
				displayMenu(false);
				return;
			}
			cursor.up();
			setCursor(cursor);			
		}

		/**
		 * @brief handles a keyDown event
		 * 
		 * Forwards the event a type-specific editor class if one is active.
		 * Otherwise move the cursor or scroll the view if neccessary
		 * 
		 */
		void doOnkeyDown(){
			if (handleKey(&TeditorBase::keyDown)) return;
			
			auto cursor = Fdisplay->getCursor();
			if (cursor.y >= N_LINES - 1){
				if (FcurrView->firstVisible + N_LINES + 1 > currMenu()->childCount()) return;
				FcurrView->firstVisible++;
				displayMenu(false);
				return;
			}

			else if (FcurrView->firstVisible + cursor.y + 1 >= currMenu()->childCount()) return;
			cursor.down();
			setCursor(cursor);		
		}

		/**
		 * @brief handles a keyLeft event
		 * 
		 * Forwards the event a type-specific editor class if one is active.
		 * Otherwise return to parent menu if not in root level
		 * 
		 */
		void doOnkeyLeft(){
			if (handleKey(&TeditorBase::keyLeft)) return;
			leaveMenu();
		}

		/**
		 * @brief handles a keyRight event
		 * 
		 * Forwards the event a type-specific editor class if one is active.
		 *
		 * if no editor is active and the item under cursor is a struct, enter this struct
		 * otherwise start edit mode for this value
		 * 
		 */
		void doOnkeyRight(){
			if (handleKey(&TeditorBase::keyRight)) return;
			
			auto itemUnderC = itemUnderCursor();
			if (!itemUnderC) return;

			if (itemUnderC->isStruct()){
				enterMenu(static_cast<Tstruct*>(itemUnderC));
			}
			else{
				FvalueInEditor = itemUnderC;
				auto editor = FeditorContainer.create(itemUnderC,VAL_COL_WIDTH);
				if(!editor) return;
				setCursorX(editor->displayCursorPos() + VAL_COL_START);
			}
		}

		/**
		 * @brief handles a keyEnter event
		 * 
		 * Forwards the event a type-specific editor class if one is active and stops the editing process 
		 */
		void doOnkeyEnter(){
			if (FeditorContainer.getInstance()){
				FeditorContainer.getInstance()->keyEnter();
				editDone();
			}
		}

		/**
		 * @brief handles a keyEscape event
		 */
		void doOnkeyEsc(){
			if (handleKey(&TeditorBase::keyEsc)) return;
		}
	
		void readKey(){
			switch (Fdisplay->readKey()) {
				case TdisplayType::SDDS_TDS_KEY_UP: return doOnkeyUp();
				case TdisplayType::SDDS_TDS_KEY_DOWN: return doOnkeyDown();
				case TdisplayType::SDDS_TDS_KEY_LEFT: return doOnkeyLeft();
				case TdisplayType::SDDS_TDS_KEY_RIGHT: return doOnkeyRight();
				case TdisplayType::SDDS_TDS_KEY_ESC: return doOnkeyEsc();
				case TdisplayType::SDDS_TDS_KEY_ENTER: return doOnkeyEnter();
			}
		}
	
		void execute(Tevent* _ev) override{
			if (_ev == &FevReadKey){
				readKey();
				FevReadKey.setTimeEvent(10);
			} else if (_ev == FmenuEvent.event()){
				if (FmenuEvent.getChangedItemCount() > 0)
					displayMenu(false);
				_ev->setTimeEvent(250);		//limit update rate to 4Hz
			} else if (isTaskEvent(_ev)){
				static bool init = true;
				if (init){
					_ev->setTimeEvent(1000);
					init = false;
					Fdisplay->begin();
					return;
				}
				FcurrView = findView(Froot);
				enterMenu(Froot);
			}
		}

	public:
		TtextDisplaySpike(TmenuHandle& _root, TdisplayType& _display)
			: FevReadKey(this)
			, FmenuEvent(this)
		{
			Fdisplay = &_display;
			Froot = &_root;
			FcurrView = findView(_root);
			FevReadKey.setTimeEvent(10);
		}
};

#endif //UTEXTDISPLAYSPIKE_H
