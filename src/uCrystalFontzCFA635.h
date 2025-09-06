#ifndef UCRYSTALFONTZCFA635_H
#define UCRYSTALFONTZCFA635_H

#include "uAbstractTextDisplay.h"

template <typename T, int MAX_ELEMENTS>
class TringBuffer {
private:
    T Fbuffer[MAX_ELEMENTS];
    int Fhead = 0;  
    int Ftail = 0;

public:
    bool push(const T& value) {
        if (isFull()) {
            return false;
        }
        Fbuffer[Fhead] = value;
        Fhead = (Fhead + 1) % MAX_ELEMENTS;
        return true;
    }

    bool pop(T& value) {
        if (isEmpty()) {
            return false;
        }
        value = Fbuffer[Ftail];
        Ftail = (Ftail+1) < MAX_ELEMENTS ? (Ftail+1) : 0;
        return true;
    }

    T pop() {
        T value{};
        pop(value);
        return value;
    }

    void debugClear(){
    	Fhead = 0;
    	Ftail = 0;
    	for (int i = 0; i<MAX_ELEMENTS; i++){
    		Fbuffer[i] = 0;
    	}
    }

	void remove(int _count) {
		auto cnt = (_count > size()) ? size() : _count;

		if (Ftail + cnt < MAX_ELEMENTS) {
			Ftail = Ftail + cnt;
		} else {
			Ftail = (Ftail + cnt) - MAX_ELEMENTS;
		}
	}

	bool getIndexForOffset(int& _index, int offset) const {
		if (offset < 0 || offset >= size()) return false;

		if (Ftail + offset < MAX_ELEMENTS) {
			_index = Ftail + offset;
		} else {
			_index = (Ftail + offset) - MAX_ELEMENTS;
		}
		return true;
	}

	bool peek(T& value, int offset=0) const {
		int index = 0;
		if (!getIndexForOffset(index,offset)) return false;

		value = Fbuffer[index];
		return true;
	}

	bool peekBytes(T* _dst, int _n, int offset=0) const {
		if (_n < 0 || (_n+offset) > size()) return false;
		int index = 0;
		if (!getIndexForOffset(index,offset)) return false;
		while (index < MAX_ELEMENTS){ 
			*_dst++ = Fbuffer[index++];
			if (--_n == 0) return true; 
		}
		for (index = 0; index < _n; index++) 
			*_dst++ = Fbuffer[index++];
		return true;
	}

    bool isEmpty() const {
        return Fhead == Ftail;
    }

	bool isFull() const {
		if (Fhead + 1 == MAX_ELEMENTS) {
			return Ftail == 0;
		}
		return Fhead + 1 == Ftail;
	}

    int size() const {
		if (Fhead >= Ftail) 
			return Fhead - Ftail;
		else 
			return MAX_ELEMENTS - (Ftail - Fhead);
    }
};

namespace sdds{
	namespace textDisplaySpike{
		template <int nRows, int nColumns, class Tstream>
		class TcrystalFontzCFA635 : public TabstractTextDisplay<nRows,nColumns>{
			public:
				//from crystalFontz datasheet
				constexpr static int KEY_UP_PRESS        	= 1;
				constexpr static int KEY_DOWN_PRESS      	= 2;
				constexpr static int KEY_LEFT_PRESS      	= 3;
				constexpr static int KEY_RIGHT_PRESS     	= 4;
				constexpr static int KEY_ENTER_PRESS     	= 5;
				constexpr static int KEY_EXIT_PRESS      	= 6;
				constexpr static int KEY_UP_RELEASE      	= 7;
				constexpr static int KEY_DOWN_RELEASE    	= 8;
				constexpr static int KEY_LEFT_RELEASE    	= 9;
				constexpr static int KEY_RIGHT_RELEASE   	= 10;
				constexpr static int KEY_ENTER_RELEASE   	= 11;
				constexpr static int KEY_EXIT_RELEASE    	= 12;

				//translation for textDisplaySpike
				constexpr static int SDDS_TDS_KEY_LEFT 			 	= KEY_LEFT_PRESS;
				constexpr static int SDDS_TDS_KEY_RIGHT 				= KEY_RIGHT_PRESS;
				constexpr static int SDDS_TDS_KEY_UP 				= KEY_UP_PRESS;
				constexpr static int SDDS_TDS_KEY_DOWN 				= KEY_DOWN_PRESS;
				constexpr static int SDDS_TDS_KEY_ESC 				= KEY_EXIT_PRESS;
				constexpr static int SDDS_TDS_KEY_ENTER 				= KEY_ENTER_PRESS;

				TcrystalFontzCFA635(Tstream* _stream)
					: TabstractTextDisplay<nRows,nColumns>()
				{
					Fstream = _stream;

					FreadTimer.start(100);
					on(FreadTimer){ static_cast<TcrystalFontzCFA635*>(_self)->receive(); };
					on(FwriteTimer){ static_cast<TcrystalFontzCFA635*>(_self)->transmit(); };
					on(FresponseTimeout){ static_cast<TcrystalFontzCFA635*>(_self)->onResponseTimeout(); };
				}
			private:
				struct CMD{
					constexpr static int PING 				= 0x00;
					constexpr static int GET_VERSION 		= 0x01;
					constexpr static int CLS	 			= 0x06;
					constexpr static int SET_CURSOR			= 0x0B;
					constexpr static int SET_CURSOR_STYLE	= 0x0C;
					constexpr static int PLACE_TEXT			= 0x1F;
					constexpr static int MAX				= 37;
				};
				constexpr static int MAX_RECV_PAYLOAD = 16;

				Tstream* Fstream;

				Ttimer FreadTimer;
				Ttimer FwriteTimer;
				Ttimer FresponseTimeout;

				dtypes::uint8 FtxBuffer[nColumns+16];
				int FtxHead = 0;
				int FtxTail;

				/* we need to be able to cache a whole message in case we need to resync */
				TringBuffer<dtypes::uint8,2+MAX_RECV_PAYLOAD+2> FrxBuffer;
				int FrecState = 0;
				struct Tpacket{
					dtypes::uint8 type;
					dtypes::uint8 len;
					dtypes::uint8 payload[MAX_RECV_PAYLOAD];

					dtypes::uint16 getCrc(){
						return payload[len]+payload[len+1]*256;
					}
					dtypes::uint8 getCmd(){ return (type>>6); };
					dtypes::uint8 getType(){ return type; };				
				} FrecPack;				
				TringBuffer<dtypes::uint8,8> Fkeys;

				dtypes::uint16 get_crc(dtypes::uint8* bufptr,int len){
					static const dtypes::uint16 crcLookupTable[256] = {
						0x00000,0x01189,0x02312,0x0329B,0x04624,0x057AD,0x06536,0x074BF,
						0x08C48,0x09DC1,0x0AF5A,0x0BED3,0x0CA6C,0x0DBE5,0x0E97E,0x0F8F7,
						0x01081,0x00108,0x03393,0x0221A,0x056A5,0x0472C,0x075B7,0x0643E,
						0x09CC9,0x08D40,0x0BFDB,0x0AE52,0x0DAED,0x0CB64,0x0F9FF,0x0E876,
						0x02102,0x0308B,0x00210,0x01399,0x06726,0x076AF,0x04434,0x055BD,
						0x0AD4A,0x0BCC3,0x08E58,0x09FD1,0x0EB6E,0x0FAE7,0x0C87C,0x0D9F5,
						0x03183,0x0200A,0x01291,0x00318,0x077A7,0x0662E,0x054B5,0x0453C,
						0x0BDCB,0x0AC42,0x09ED9,0x08F50,0x0FBEF,0x0EA66,0x0D8FD,0x0C974,
						0x04204,0x0538D,0x06116,0x0709F,0x00420,0x015A9,0x02732,0x036BB,
						0x0CE4C,0x0DFC5,0x0ED5E,0x0FCD7,0x08868,0x099E1,0x0AB7A,0x0BAF3,
						0x05285,0x0430C,0x07197,0x0601E,0x014A1,0x00528,0x037B3,0x0263A,
						0x0DECD,0x0CF44,0x0FDDF,0x0EC56,0x098E9,0x08960,0x0BBFB,0x0AA72,
						0x06306,0x0728F,0x04014,0x0519D,0x02522,0x034AB,0x00630,0x017B9,
						0x0EF4E,0x0FEC7,0x0CC5C,0x0DDD5,0x0A96A,0x0B8E3,0x08A78,0x09BF1,
						0x07387,0x0620E,0x05095,0x0411C,0x035A3,0x0242A,0x016B1,0x00738,
						0x0FFCF,0x0EE46,0x0DCDD,0x0CD54,0x0B9EB,0x0A862,0x09AF9,0x08B70,
						0x08408,0x09581,0x0A71A,0x0B693,0x0C22C,0x0D3A5,0x0E13E,0x0F0B7,
						0x00840,0x019C9,0x02B52,0x03ADB,0x04E64,0x05FED,0x06D76,0x07CFF,
						0x09489,0x08500,0x0B79B,0x0A612,0x0D2AD,0x0C324,0x0F1BF,0x0E036,
						0x018C1,0x00948,0x03BD3,0x02A5A,0x05EE5,0x04F6C,0x07DF7,0x06C7E,
						0x0A50A,0x0B483,0x08618,0x09791,0x0E32E,0x0F2A7,0x0C03C,0x0D1B5,
						0x02942,0x038CB,0x00A50,0x01BD9,0x06F66,0x07EEF,0x04C74,0x05DFD,
						0x0B58B,0x0A402,0x09699,0x08710,0x0F3AF,0x0E226,0x0D0BD,0x0C134,
						0x039C3,0x0284A,0x01AD1,0x00B58,0x07FE7,0x06E6E,0x05CF5,0x04D7C,
						0x0C60C,0x0D785,0x0E51E,0x0F497,0x08028,0x091A1,0x0A33A,0x0B2B3,
						0x04A44,0x05BCD,0x06956,0x078DF,0x00C60,0x01DE9,0x02F72,0x03EFB,
						0x0D68D,0x0C704,0x0F59F,0x0E416,0x090A9,0x08120,0x0B3BB,0x0A232,
						0x05AC5,0x04B4C,0x079D7,0x0685E,0x01CE1,0x00D68,0x03FF3,0x02E7A,
						0x0E70E,0x0F687,0x0C41C,0x0D595,0x0A12A,0x0B0A3,0x08238,0x093B1,
						0x06B46,0x07ACF,0x04854,0x059DD,0x02D62,0x03CEB,0x00E70,0x01FF9,
						0x0F78F,0x0E606,0x0D49D,0x0C514,0x0B1AB,0x0A022,0x092B9,0x08330,
						0x07BC7,0x06A4E,0x058D5,0x0495C,0x03DE3,0x02C6A,0x01EF1,0x00F78
					};

					#if SDDS_ON_ARDUINO != 1
						register
					#endif
					dtypes::uint16 newCrc=0xFFFF;
					while(len--)
						newCrc = (newCrc >> 8) ^ crcLookupTable[(newCrc ^ *bufptr++) & 0xff];
					return(~newCrc);
				}

				void initSend(const dtypes::uint8 _type){
					FtxBuffer[0] = _type;
					FtxHead = 2;
				}

				void addData(dtypes::uint8 _data){
					FtxBuffer[FtxHead++]=_data;
				}
				
				void addData( const dtypes::uint8* _data, int _len){
					//memcpy(&FtxBuffer[FtxHead],_data,_len);
					for (int i=0; i<_len; i++){
						if (_data[i] == '_')
							FtxBuffer[FtxHead+i] = 196;
						else
							FtxBuffer[FtxHead+i] = _data[i];
					}
					FtxHead+=_len;
				}

				void transmit(){
					if (FtxTail >= FtxHead) return;
					
					int bytesToBeTrasmitted = FtxHead-FtxTail;
					FtxTail+=Fstream->write(&FtxBuffer[FtxTail],bytesToBeTrasmitted);;
					if (FtxTail < FtxHead)
						FwriteTimer.setTimeEvent(1);
					else
						FresponseTimeout.start(250);
				}

				int FretryCnt;
				void onResponseTimeout(){
					if (FretryCnt++ >= 0){
						handleResponse();
						return;
					}
					FtxTail = 0;
					transmit();
				}

				void handleResponse(){
					FtxHead = 0;
					FresponseTimeout.stop();
					this->onTaskDone();				
				}

				void handleReport(){
					//check for key reports
					if (FrecPack.getType() == 0x80){
						Fkeys.push(FrecPack.payload[0]);
					}
				}

				void handleError(){

				}

				void handlePacket(){
					switch(FrecPack.getCmd()){
						case 0x01: return handleResponse();
						case 0x02: return handleReport();
						case 0x03: return handleError();
						case 0x00: return;								//message to the display						
					}	
				}

				/**
				 * @brief reveives and handles packets from the incoming stream
				 * 
				 * @return true if it wants to be called again
				 * @return false call it later
				 */
				bool _receive(){
					switch(FrecState){

						//scanning for a valid type
						case 0:
							if (FrxBuffer.isEmpty()) return false;
							while (FrxBuffer.pop(FrecPack.type)){
								if ((FrecPack.type & 0x3F) < CMD::MAX){
									FrecState = 1;
									break;
								}
							}

						//scanning for a valid length
						case 1: 
							if (!FrxBuffer.peek(FrecPack.len)) return false;
							if (FrecPack.len > MAX_RECV_PAYLOAD){
								FrecState = 0;
								return true;
							}
							FrecState = 2;

						//wait for enough bytes to arrive
						case 2:
							//try to read len+2 bytes at offset 1 from the buffer and return 
							if (!FrxBuffer.peekBytes(&FrecPack.payload[0],FrecPack.len+2,1)) return false;
							auto crc = get_crc(&FrecPack.type,2+FrecPack.len);
							auto msgCrc = FrecPack.getCrc();
							FrecState=0;
							if (crc != msgCrc){
								FrxBuffer.pop();
								return true;
							}
							FrxBuffer.remove(FrecPack.len+3);
							handlePacket();
							return true;
					}

					/* in case something really went wrong reset state machine */
					FrecState = 0;
					return false;
				}

				void receive(){
					while (Fstream->available() && !FrxBuffer.isFull())
						FrxBuffer.push(Fstream->read());
					while (_receive()) ;
					FreadTimer.start(1);
				}

				void sendCmd(){
					FtxBuffer[1] = FtxHead-2;
					union {
						dtypes::uint16 value;
						dtypes::uint8 bytes[2];
					}crc;			
					crc.value = get_crc(&FtxBuffer[0],FtxHead);
					addData(crc.bytes[0]);
					addData(crc.bytes[1]);
					FtxTail = 0;
					FretryCnt = 0;
					transmit();
				}

			public:
				//"0x0B 0x02 0x00 0x00 0x73 0x89 " for 0,0
				void doSetCursor(const TcursorInterface _cursor) override{ 
					initSend(CMD::SET_CURSOR);
					addData(_cursor.x);
					addData(_cursor.y);
					sendCmd();
				}

				//"0x06 0x00 0x97 0x5B "
				void doClear() override{
					initSend(CMD::CLS);
					sendCmd();
					/**
					 * CLS sets the cursor to 0;0 and therefore we have to update our local copy.
					 */
					this->FdisplayCursor.x = 0;
					this->FdisplayCursor.y = 0;
				}
				
				void doUpdateRow(TrowChanges _changes) override {
					initSend(CMD::PLACE_TEXT);
					addData(_changes.firstChangedIdx);
					addData(_changes.row);
					addData(reinterpret_cast<const dtypes::uint8*>(_changes._buffer),_changes.n);
					sendCmd();
				}

				int readKey(){
					return Fkeys.pop();
				}

		};

	}
}

#endif //UCRYSTALFONTZCFA635_H
