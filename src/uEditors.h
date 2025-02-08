#ifndef UEDITOR_H
#define UEDITOR_H

#include <uTypedef.h>
#include <uMemoryUtils.h>

#include <cmath>		//std::pow
#include <cstring>		//strlen
#include <algorithm>	//std::max, std::min

namespace sdds{
	namespace textDisplaySpike{

		/**
		 * @brief TeditorBase base class for all editors
		 */
		class TeditorBase{
			protected:
				int FdisplayWidth;    // width available for us
				Tdescr* Fdescr;
				bool FeditDone = false;
			public:
				TeditorBase(Tdescr* _descr, int _displayWidth){
					Fdescr = _descr;
					FdisplayWidth = _displayWidth;
				}
				virtual void keyLeft() {}; 
				virtual void keyRight() {}; 
				virtual void keyUp() = 0; 
				virtual void keyDown() = 0; 
				virtual void keyEnter(){ }; 
				virtual void keyEsc(){ FeditDone = true; }; 
				virtual int displayCursorPos() = 0;
				virtual void getDisplayString(dtypes::string& _out) = 0;
				bool editDone(){ return FeditDone; }
		};

		/**
		 * @brief TenumEditor editor for enum types
		 */
		class TenumEditor : public TeditorBase{
			void keyUp() override {
				if (FordVal <= 0) return;
				FordVal--;
			}; 

			void keyDown() override { 
				if (FordVal+1 >= FenCnt) return;
				FordVal++;
			}

			void keyEnter() override {
				memcpy(Fdescr->pValue(),&FordVal,Fdescr->valSize()); 
				Fdescr->signalEvents();
			}

			int displayCursorPos() override { return FdisplayWidth-1; }; 

			void getDisplayString(dtypes::string& _out) override {
				auto it = static_cast<TenumBase*>(Fdescr)->enumInfo().iterator;
				const char* enStr = "";
				for (dtypes::uint32 i=0;i<=FordVal;i++)
					enStr = it.next();
				_out = enStr;
			};

			dtypes::uint32 FordVal;
			dtypes::uint32 FenCnt;
			public:
				TenumEditor(Tdescr* _enum, int _dispWidth) : TeditorBase(_enum,_dispWidth){
					FordVal = 0;
					memcpy(&FordVal,_enum->pValue(),_enum->valSize());
					FenCnt = 0;
					auto it = static_cast<TenumBase*>(Fdescr)->enumInfo().iterator;
					while (it.hasNext()){
						FenCnt++;
						it.next();
					}
				}
		};

		template <typename T>
		constexpr int countDigits(T num, int digits = 0) {
			return (num > 0) ? countDigits(num / 10, digits + 1) : digits;
		}

		template <typename T>
		constexpr int maxDecimalDigits() {
			return countDigits(dtypes::high<T>());
		}

		/**
		 * @brief TintEditor Editor for integer Types
		 * 
		 * @tparam TdescrType
		 */
		template <class TdescrType>
		class TintEditor : public TeditorBase{

			typedef typename TdescrType::dtype TworkInteger;
			constexpr static auto MAX_VALUE = dtypes::high<TworkInteger>();
			constexpr static auto MIN_VALUE = dtypes::low<TworkInteger>();
			constexpr static auto MAX_DIGITS = maxDecimalDigits<TworkInteger>();

			void keyLeft() override { moveCursorLeft(); }; 
			void keyRight() override { moveCursorRight(); }; 
			void keyUp() override { increaseDigit(); }; 
			void keyDown() override { decreaseDigit(); }; 
			void keyEnter() override{
				*static_cast<TdescrType*>(Fdescr) = Fint;
			}
			
			int displayCursorPos() override{
				return FdisplayWidth - FcursorPos - 1;
			};

			void getDisplayString(dtypes::string& _out) override{
				if (Fdescr->showOption() == sdds::opt::showHex)
					return sdds::to_string_hex(_out,&Fint,sizeof(Fint));
				else if (Fdescr->showOption() == sdds::opt::showBin)
					return sdds::to_string_bin(_out,&Fint,sizeof(Fint));
				else{
					return sdds::to_string(_out,Fint);
				}
				_out = "";
				//return "";
			};

			TworkInteger Fint;
			int FcursorPos = 0;
			int FmaxCursorPos;

			public:
				TintEditor(Tdescr* _d, const int _displayWith) : TeditorBase(_d,_displayWith){
					FdisplayWidth = _displayWith;
					Fdescr = _d;
					Fint = *static_cast<TdescrType*>(_d);
					if (Fdescr->showOption() == sdds::opt::showHex)
						FmaxCursorPos = Fdescr->valSize()*2-1;
					else if (Fdescr->showOption() == sdds::opt::showBin)
						FmaxCursorPos = Fdescr->valSize()*8-1;
					else
						FmaxCursorPos = MAX_DIGITS-1;
				}

			private:

				TworkInteger getWeight(){
					switch(Fdescr->showOption()){
						case sdds::opt::showHex:
							return static_cast<TworkInteger>(std::pow(16, FcursorPos));
						case sdds::opt::showBin:
							return static_cast<TworkInteger>(std::pow(2, FcursorPos));
					}
					return static_cast<TworkInteger>(std::pow(10, FcursorPos));
				}

				void increaseDigit() {
					if (Fint <= MAX_VALUE - getWeight()) {
        				Fint += getWeight();
					} else {
						Fint = MAX_VALUE;
					}
				}

				void decreaseDigit() {
					if (Fint >= MIN_VALUE + getWeight()) {
        				Fint -= getWeight();
					} else {
						Fint = MIN_VALUE;
					}
				}

				void moveCursorLeft() {
					if (FcursorPos < FmaxCursorPos)
						FcursorPos++;
				}

				void moveCursorRight() {
					if (FcursorPos > 0)
						FcursorPos--;
				}

		};

		class TtimeEditor : public TeditorBase{
			int FcursorPos = 0;
			int FsecInc = 1;
			int FuSecInc = 0;
			Ttime::dtype Ftime;

			void keyLeft() override { moveCursorLeft(); }; 
			void keyRight() override { moveCursorRight(); }; 
			void keyUp() override { increaseDigit(); }; 
			void keyDown() override { decreaseDigit(); }; 
			void keyEnter() override{
				*static_cast<Ttime*>(Fdescr) = Ftime;
			}
			
			int displayCursorPos() override{
				auto cp = FcursorPos + FcursorPos/2;
				return FdisplayWidth - cp - 1;
			};

			void calcWeight(){
				switch (FcursorPos){
					case 0 :
						FsecInc = 1;
						break;
					case 1 :
						FsecInc = 10;
						break;
					case 2 :
						FsecInc = 60;
						break;
					case 3 :
						FsecInc = 600;
						break;
					case 4 :
						FsecInc = 3600;
						break;
					case 5 :
						FsecInc = 36000;
						break;
					default:
						break;
				}
			}

			void moveCursorLeft(){
				//if (FcursorPos >= 5) return;
				FcursorPos++;
				calcWeight();
			}

			void moveCursorRight(){
				if (FcursorPos <= 0)
					return;
				FcursorPos--;
				calcWeight();
			}
			
			void increaseDigit(){
				Ftime.tv_sec += FsecInc;
				Ftime.tv_usec += FuSecInc;
			}

			void decreaseDigit(){
				Ftime.tv_sec -= FsecInc;
				Ftime.tv_usec -= FuSecInc;
			}

			void getDisplayString(dtypes::string& _out) override{
				sdds::to_string(_out,*static_cast<Ttime*>(Fdescr),Ftime);
			};

			public:
				TtimeEditor(Tdescr* _d, int _dispWidth) : TeditorBase(_d,_dispWidth){
					Ftime = *static_cast<Ttime*>(_d);
				}
		};

		/**
		 * @brief TeditorContainer SingletonContainer for all possible editors
		 * 
		 */
		using sdds::memUtils::TsingletonContainer;
		class TeditorContainer: public TsingletonContainer<TeditorBase
			,TenumEditor
			,TintEditor<Tuint8>
			,TintEditor<Tuint16>
			,TintEditor<Tuint32>
			,TintEditor<Tint8>
			,TintEditor<Tint16>
			,TintEditor<Tint32>
			,TintEditor<Tfloat32>
			,TtimeEditor
			> 
		{
			public:
				TeditorBase* create(Tdescr* _d, const int _displayWidth){
					switch(_d->type()){
						case sdds::Ttype::INT8:
							return TsingletonContainer::create<TintEditor<Tint8>>(_d,_displayWidth);
						case sdds::Ttype::INT16:
							return TsingletonContainer::create<TintEditor<Tint16>>(_d,_displayWidth);
						case sdds::Ttype::INT32:
							return TsingletonContainer::create<TintEditor<Tint32>>(_d,_displayWidth);
						case sdds::Ttype::UINT8:
							return TsingletonContainer::create<TintEditor<Tuint8>>(_d,_displayWidth);
						case sdds::Ttype::UINT16:
							return TsingletonContainer::create<TintEditor<Tuint16>>(_d,_displayWidth);
						case sdds::Ttype::UINT32:
							return TsingletonContainer::create<TintEditor<Tuint32>>(_d,_displayWidth);
						case sdds::Ttype::ENUM:
							return TsingletonContainer::create<TenumEditor>(_d,_displayWidth);
						case sdds::Ttype::FLOAT32:
							return TsingletonContainer::create<TintEditor<Tfloat32>>(_d,_displayWidth);
						case sdds::Ttype::TIME:
							return TsingletonContainer::create<TtimeEditor>(_d,_displayWidth);
						default:
							return nullptr;
					}
				}
		};

	}	
}

#endif //UEDITOR_H
