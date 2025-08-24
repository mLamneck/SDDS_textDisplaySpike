#ifndef UEDITOR_H
#define UEDITOR_H

#include <uTypedef.h>

//Dear AVR-GCC, thanks for keeping C++ interesting: every line of portable code becomes a new adventure here.
#include "uMmath.h"
//#include <cstring>	//strlen	not available on some platforms (Arduino i.e. Uno)
#include <string.h>		//strlen
#include <new>			//required for AVR-GCC

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
				virtual void init(Tdescr* _descr, int _displayWidth){
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
				void init(Tdescr* _enum, int _dispWidth) override{
					TeditorBase::init(_enum,_dispWidth);
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
				void init(Tdescr* _d, const int _displayWith) override{
					TeditorBase::init(_d,_displayWith);
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
							return static_cast<TworkInteger>(mmath::pow(16, FcursorPos));
						case sdds::opt::showBin:
							return static_cast<TworkInteger>(mmath::pow(2, FcursorPos));
					}
					return static_cast<TworkInteger>(mmath::pow(10, FcursorPos));
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
				void init(Tdescr* _d, int _dispWidth) override{
					TeditorBase::init(_d,_dispWidth);
					Ftime = *static_cast<Ttime*>(_d);
				}
		};

		/**
		 * @brief TeditorContainer SingletonContainer for all possible editors
		 * 
		 */
		class TeditorContainer {
			private:
				union Tcontainer{
					Tcontainer(){};
  					~Tcontainer(){};

					TenumEditor FenumEditor;
					TintEditor<Tuint8> Fuint8Editor;
					TintEditor<Tuint16> Fuint16Editor;
					TintEditor<Tuint32> Fuint32Editor;
					TintEditor<Tint8> Fint8Editor;
					TintEditor<Tint16> Fint16Editor;
					TintEditor<Tint32> Fint32Editor;
					TintEditor<Tfloat32> Ffloat32Editor;
					TtimeEditor FtimeEditor;
				};
				Tcontainer Fcontainer;
				TeditorBase* Finstance = nullptr;
			public:
				TeditorBase* getInstance(){ return Finstance; }
				void destroy(){
					if (Finstance == nullptr) return;

					Finstance->~TeditorBase(); 
					Finstance = nullptr;
				}

				TeditorBase* create(Tdescr* _d, const int _displayWidth){
					destroy();

					auto t = _d->type();
					if (t == sdds::Ttype::INT8) 
						Finstance = new (&Fcontainer.Fint8Editor) TintEditor<Tint8>;
					else if (t == sdds::Ttype::INT16) 
						Finstance = new (&Fcontainer.Fint16Editor) TintEditor<Tint16>;
					else if (t == sdds::Ttype::INT32) 
						Finstance = new (&Fcontainer.Fint32Editor) TintEditor<Tint32>;
					else if (t == sdds::Ttype::UINT8) 
						Finstance = new (&Fcontainer.Fuint8Editor) TintEditor<Tuint8>;
					else if (t == sdds::Ttype::UINT16) 
						Finstance = new (&Fcontainer.Fuint16Editor) TintEditor<Tuint16>;
					else if (t == sdds::Ttype::UINT32) 
						Finstance = new (&Fcontainer.Fuint32Editor) TintEditor<Tuint32>;
					else if (t == sdds::Ttype::ENUM) 
						Finstance = new (&Fcontainer.FenumEditor) TenumEditor;
					else if (t == sdds::Ttype::FLOAT32) 
						Finstance = new (&Fcontainer.Ffloat32Editor) TintEditor<Tfloat32>;
					else
						return nullptr;
					
					Finstance->init(_d,_displayWidth);
					return Finstance;
				}
		};

	}	
}

#endif //UEDITOR_H
