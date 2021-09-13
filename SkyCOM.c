#include "SkyCOM.h"
#include <string.h>

unsigned char DataBits[1500];    //holds the DataBits/ receive buffer
unsigned char SetpBits[200];     //holds all non data values except checksum
unsigned char sum[16];     		//holds added binary values (checksum)
int spot = 0;          	//cursor in data array
int PkgCnt = 0;        	//value and string package count
int DtaCnt = 0;        	//command count
int dVer = 1;           	//protocol network
int dTXaddr = 0;      		//device address on network

int mTXaddr = 0;			//received transmitter address
int mVer = 1;				//protocol version of received message
int mSctCnt = 0;			//struct count of recieved message
int mRXaddr[16];	//other network addresses which received the same message
unsigned char RXcnt = 0;	//store count ^

/*
 * 0: start bits
 * 1: recieving device
 * 2:
 * 3:
 * 4:
 * 5:
 * 6: checksum correct
 */

unsigned char ErrFlag[32] = { 0 };
int CksSpot;

//error values
int ErrA;
int ErrB;

struct Dta {
	unsigned char Tpe;       //stores data type, see table for values
	unsigned char BytVal;	 //store 8 bit byte
	unsigned char PkgID;
	unsigned char IoF;     //stores if value is int or float, 0 = int, 1 = float
	float FltVal;    //stores the possible float value
	char Str[255];  //stores possible string value
	int IntVal;    //stores the possible int value
};
struct Dta Data[32];

//to the power funtion bc I can't be arsed to include math.h
//I googled if it s assed or arsed
double power(int base, int exp) {
	double result = 1.0;
	while (exp != 0) {
		result *= base;
		--exp;
	}
	return result;
}
//add two binary array values
void BinAdd(unsigned char A[16], unsigned char B[16]) {

	unsigned char remainder = 0;
	for (int i = 15; i >= 0; i--) {
		unsigned rem = 0;
		if (remainder) {
			++rem;
		}
		if (A[i])
			++rem;
		if (B[i])
			++rem;
		if (rem % 2 == 1) {
			sum[i] = 1;
		} else {
			sum[i] = 0;
		}
		if (rem > 1) {
			remainder = 1;
		} else {
			remainder = 0;
		}
	}
}

//convert value to bits in correct byte size and add to specific spot in DataBits or SetpBits array
void DECtoBIN(int val, int size, int cursor, unsigned char arr) { //decimal value, byte size, location in array, array (0 = DataBits, 1 = SetpBits)
	unsigned char BIN[size];     //holds local bits

	//initialize bits array because otherwise output will be fucky-wucky
	for (int i = 0; i < size; i++) {
		BIN[i] = 0;
	}

	int j = size - 1;   //since counting from 0, counter = size -1

	//sore remainder in local bits array
	while (val > 0) {
		BIN[j] = val % 2;
		val = val / 2;
		j--;
	}

	//store bits in global DataBits/SetpBits array
	if (arr == 0) {
		for (int k = 0; k < size; k++) {
			DataBits[k + cursor] = BIN[k];
		}
	} else {
		for (int k = 0; k < size; k++) {
			SetpBits[k + cursor] = BIN[k];
		}
	}
}

//convert part of binary array to decimal integer
int ConvertToDec(int curs, int size) {
	int result = 0;
	unsigned char nBitByt[size];

	for (int i = 0; i < size; i++) {
		nBitByt[i] = DataBits[i + curs];
	}

	for (int i = 0; i < size; i++) {
		result <<= 1;
		result |= nBitByt[i];
	}

	return result;
}

//glorified for loop
void ArrayArray(unsigned char Byt[], int size) {
	for (int i = 0; i < size; i++) {
		DataBits[i + spot] = Byt[i];
	}
	spot = spot + size;
}
//check if value is an int or float
unsigned char CheckDtaTpe(float val) {

	int IntVal = val;               //store int of the float val
	float IntFloat = val - IntVal;  //convert the int val back to a float

	//if the value is equal to 0, it's an int, otherwise it's a float
	if (IntFloat == 0) {
		return 0;
	} else if (IntFloat != 0) {
		return 1;
	}
}
//return the ammount of 8bytes a value uses
int ValBytCntr(float val) {
	int Len = 1;

	//calculate the ammount of bytes the value uses
	for (int i = 0; i < 8; i++) {
		if (val < power(2.0, 8 * (i + 1))) {
			break;
		}
		Len++;
	}
	return Len;
}
//add 4 bit command value and 8 bit value to array
void AddCmnd(unsigned char Byt[4], int var) {
	DtaCnt++;

	//store it in global DataBits array
	ArrayArray(Byt, 4);

	DECtoBIN(var, 8, spot, 0);     //add actual bit value

	spot = spot + 8;            //move cursor 8byte
}

void COM_START(int addr, int protocol) {
	dVer = protocol;
	dTXaddr = addr;
}

//add an ID to the message
void COM_DATA_ID(int val) {
	unsigned char CmdCode[4] = { 0, 1, 1, 1 }; //baked Data ID 4byte (7, Data ID)
	AddCmnd(CmdCode, val);
}
//add a struct to the message
void COM_DATA_STRUCT(int val) {           //structure ID
	unsigned char CmdCode[4] = { 0, 1, 0, 1 }; //baked struct 4byte (5, Structure set)
	AddCmnd(CmdCode, val);
}
//add a data request to the message
void COM_DATA_REQ(int val) {           //data ID
	unsigned char CmdCode[4] = { 1, 0, 0, 1 }; //baked data request 4byte (9, DataRequest)
	AddCmnd(CmdCode, val);
}
//add a byte (0-255) to teh message
void COM_DATA_BYTE(int val) {           //data ID
	unsigned char CmdCode[4] = { 1, 1, 0, 0 }; //baked data request 4byte (9, DataRequest)
	AddCmnd(CmdCode, val);
}
//add error code
void COM_ERR(int A, int B) {
	DtaCnt++;

	unsigned char Byt[4] = { 0, 1, 1, 0 };    //baked error 4byte (6, error)
	ArrayArray(Byt, 4);

	if (B == 0) {
		DECtoBIN(A, 9, spot, 0);
		spot = spot + 10;
	}
}

//add string to message
void COM_DATA_STRING(char Msg[]) {
	unsigned char SoP[4] = { 0, 0, 0, 1 };  //baked start of string package byte
	unsigned char EoP[4] = { 1, 1, 1, 0 };    //baked end of package byte

	int len = strlen(Msg);     //get string length

	AddCmnd(SoP, len); //use AddCmnd function bc adding a 4bit and 8bit value anyways

	//put ASCII values of the string characters bits array
	for (int i = 0; i < len; i++) {
		DECtoBIN(Msg[i], 8, spot, 0); //covert and add character to global data array
		spot = spot + 8;            //move cursor 8byte
	}

	//put end of package byte in bits array
	for (int i = 0; i < 4; i++) {
		DataBits[i + spot] = EoP[i];
	}

	spot = spot + 4;    //move cursor half-byte
	PkgCnt++;           //add to package count
}
//add value to message
void COM_DATA_VAL(float val) {

	unsigned char DtaTpe;                    //1 = float, 0 = int
	unsigned char Pol;

	if (val < 0) {               //store, correct and check if value is negative
		Pol = 0;
		val = val * -1;
	} else {
		Pol = 1;
	}

	//if the value is equal to 0, it's an int, otherwise it's a float
	DtaTpe = CheckDtaTpe(val);

	unsigned char SoP[6] = { 0, 0, 1, 0, DtaTpe, Pol }; //baked start of Val package byte + datatype bit + value polarity type
	unsigned char EoP[4] = { 1, 1, 1, 0 };           //baked end of package byte

	//put start of value package byte in bits array
	ArrayArray(SoP, 6);

	//-----Integer---------------
	if (DtaTpe == 0) {
		int DtaLen = ValBytCntr(val); //stores the ammount of bytes the value takes up
		int Len = 8 * DtaLen;

		DECtoBIN(DtaLen, 4, spot, 0);  //store size of value in 4 bytes
		spot = spot + 4;
		int IntVal = val;
		DECtoBIN(IntVal, Len, spot, 0);
		spot = spot + Len;
	}
	//-----float---------------
	else {
		unsigned char Int = 0;
		int Pnt = 0;

		//make value integer and store the decimal point location
		while (Int == 0) {
			val = val * 10;     //move decimal point (until val is an integer)

			int IntegerVal = val;               //store int of the float val
			float IntFlt = val - IntegerVal; //convert the int val back to a float

			//if the value is equal to 0, it's an int, otherwise it's a float
			if (IntFlt == 0) {
				Int = 1;
			} else if (IntFlt != 0) {
				Int = 0;
			}
			Pnt++;
		}

		int DtaLen = ValBytCntr(val); //stores the ammount of bytes the value takes up
		int Len = 8 * DtaLen;

		DECtoBIN(DtaLen, 4, spot, 0);  //store size of value in a 4byte
		spot = spot + 4;

		DECtoBIN(Pnt, 6, spot, 0);     //store the decimal point spot in a 6byte
		spot = spot + 6;

		DECtoBIN(val, Len, spot, 0);
		spot = spot + Len;
	}
	//--------------------

	//put end of package 4byte in bits array
	ArrayArray(EoP, 4);
	PkgCnt++;
}

//puts it all together
void COM_TRANSMIT(int RX_addrs[16], int BpS) { //address array, MUST be an initialized 16 enttity array
	int addrs;

	for (int i = 0; i < 16; i++) {
		if (RX_addrs[i] == 0) {
			addrs = i;
			break;
		} else {
			DECtoBIN(RX_addrs[i], 10, 16 + (10 * i), 1);
		}
	}

	//calculate setup bit length (start bits + transmitter adress + reciever count + reciever addresses + version + package count) = (2 + 10 + 4 + 10*reciever_count) + 6 + 8)
	int SetpLen = 16 + (10 * addrs);

	//add transmitter address, protovol version and message package count
	DECtoBIN(dTXaddr, 10, 2, 1); //place 2 bc at start of setup array, only start bits prior (2)
	DECtoBIN(dVer, 6, SetpLen, 1);

	if (PkgCnt == 0) {
		SetpBits[SetpLen + 6] = 1;
		DECtoBIN(DtaCnt, 7, SetpLen + 7, 1);
	} else {
		SetpBits[SetpLen + 6] = 0;
		DECtoBIN(PkgCnt, 7, SetpLen + 7, 1);
	}
	SetpLen = SetpLen + 14;     //add 14 bits to SetpLen

	DECtoBIN(addrs, 4, 12, 1);     //add reciever count

	unsigned char SoM[2] = { 1, 0 };        //start of message bits
	unsigned char CKS[4] = { 1, 1, 0, 1 };    //baked checksum bits

	//add checksum 4byte to array
	for (int i = 0; i < 4; i++) {
		DataBits[i + spot] = CKS[i];
	}
	spot = spot + 4;

	for (int i = 0; i < 2; i++) {
		SetpBits[i] = SoM[i];
	}

	int MsgLenght = SetpLen + spot;

	//merge setup and data arrays
	unsigned char FullMessage[MsgLenght + 20]; //Length of both arrays, + 16 bit checksum + 4 bit end of message
	for (int i = 0; i < MsgLenght; i++) {
		if (i < SetpLen) {
			FullMessage[i] = SetpBits[i];
		} else {
			FullMessage[i] = DataBits[i - SetpLen];
		}
	}

	//re initialize sum array to 0
	for (int i = 0; i < 16; i++) {
		sum[i] = 0;
	}

	int MsgByteItr;     //amount of iterations to divide message in 8 bit parts
	int BytCnt = 0;		//separate cursor only for 8 bit chop

	//message is divide-able by eight only if 4 is added OR subtracted
	if (CheckDtaTpe((MsgLenght + 4) / 8) == 0) {
		MsgByteItr = (MsgLenght + 4) / 8;
	} else if (CheckDtaTpe((MsgLenght - 4) / 8) == 0) {
		MsgByteItr = (MsgLenght - 4) / 8;
	}

	for (int j = 0; j < MsgByteItr; j++) {
		unsigned char BinVal[16] = { 0 };		//store byte from message
		for (int i = 0; i < 8; i++) {	//copy byte from message to buffer array
			BinVal[i + 8] = FullMessage[i + BytCnt];
		}
		BinAdd(sum, BinVal);			//add buffer to total checksSUM value
		BytCnt = BytCnt + 8;				//move cursor
	}

	//add checksum to full-message array
	for (int i = 0; i < 16; i++) {
		FullMessage[i + MsgLenght] = sum[i];
	}

	//go through bits and set to value
	for (int i = 0; i < MsgLenght + 16; i++) {
		if (FullMessage[i] == 0) {
			//set TX pin LOW
			COM_RESET_BIT();
		} else if (FullMessage[i] == 1) {
			//set TX pin HIGH
			COM_SET_BIT();
		}
		//bits per second for transmission (max 1000 is using "HAL_Delay()" function
		uC_DELAY(1000/BpS);
	}

	//reset TX to make sure line is LOW after transmission
	COM_RESET_BIT();

	spot = 0;
	PkgCnt = 0;
	DtaCnt = 0;
}


unsigned char NewMsg = 0;
int MsgCnt = 0;

unsigned char MSG_NEW(){
	return NewMsg;
}

//process all data in the message
int MSG_PROCESS(int MsgLen) {

	int curs = 0;          //cursor
	unsigned char PkgID = 0;
	//message state, 1 is correct, 0 notCorrect
	int state;

	//check start bits
	if (DataBits[0] == 1 && DataBits[1] == 0) {
		ErrFlag[0] = 1;
		state = 1;
	} else {
		ErrFlag[0] = 0;
		state = 0;
	}
	curs = 2;

	//store transmitter adrress
	mTXaddr = ConvertToDec(curs, 10);
	curs = 12;

	//store reciever address count
	RXcnt = ConvertToDec(curs, 4);
	curs = curs + 4;

	//store and add reciever addresses
	for (int i = 0; i < RXcnt; i++) {
		mRXaddr[i] = ConvertToDec(curs, 10);
		if (mRXaddr[i] == dTXaddr) {
			ErrFlag[1] = 1;
		}
		curs = curs + 10;
	}

	if (ErrFlag[1] == 1) {
		//store protocol version
		mVer = ConvertToDec(curs, 6);
		curs = curs + 7; //curs 7 bc bit 7 from curs holds if packagecount is it or datacount (non string/ value package messages)

		//store package/data count
		DtaCnt = ConvertToDec(curs, 7);

		if (DataBits[curs - 1] != 1) {
			PkgCnt = DtaCnt;
		}

		//here datacount is still equal to packagecount, since packagecount is the
		//value in the message and datacount will change in next loop
		curs = curs + 7;

		//process the actual data, loop repeats DtaCnt times
		for (int k = 0; k < DtaCnt + 1; k++) {
			int DtaTpe;
			//store datatype
			DtaTpe = ConvertToDec(curs, 4);
			curs = curs + 4;
			Data[k].Tpe = DtaTpe;

			//convert and store string message
			if (DtaTpe == 1) {
				//store package ID
				Data[k].PkgID = PkgID;
				PkgID++;

				//convert and store package length/ character count
				int Len = ConvertToDec(curs, 8);
				curs = curs + 8;

				//convert and store package character data
				for (int j = 0; j < Len; j++) {
					Data[k].Str[j] = ConvertToDec(curs, 8);
					curs = curs + 8;
				}

				//check for end of package
				if (ConvertToDec(curs, 4) == 14) {
					state = 1;
				}
				curs = curs + 4;
			}

			//convert and store value message
			else if (DtaTpe == 2) {
				//store datatype (int or float) and value polarity
				unsigned char DtTp = DataBits[curs];
				unsigned char Pol = DataBits[curs + 1];
				curs = curs + 2;

				//store datatype (0 int, 1 float) in IntOrFloat
				Data[k].IoF = DtTp;

				if (DtTp == 0) {
					int Len = ConvertToDec(curs, 4);
					curs = curs + 4;
					//len value is send in bytes, so x8 is neccesairy
					int Val = ConvertToDec(curs, Len * 8);
					//check and adjust polarity
					if (Pol == 0) {
						Val = Val * -1;
					}
					curs = curs + (Len * 8);
					Data[k].IntVal = Val;
				} else {
					int Len = ConvertToDec(curs, 4);
					curs = curs + 4;
					//point value stores ammount of divisions/decimal places the point is from least significant bit
					int Pnt = ConvertToDec(curs, 6);
					curs = curs + 6;
					//len value is send in bytes, so x8 is neccesairy
					float Val = ConvertToDec(curs, Len * 8);
					//divide ammount of times decimal is shifted to created float
					for (int i = 0; i < Pnt; i++) {
						Val = Val / 10;
					}
					if (Pol == 0) {
						Val = Val * -1;
					}
					Data[k].FltVal = Val;
					curs = curs + (Len * 8);
				}

				//check for end of package
				if (ConvertToDec(curs, 4) == 14) {
					state = 1;
				}
				curs = curs + 4;
			}

			//store struct id
			else if (DtaTpe == 5) {
				DtaCnt++;
				mSctCnt++;
				Data[k].BytVal = ConvertToDec(curs, 8);
				curs = curs + 8;
			}

			//store error ID(s)
			else if (DtaTpe == 6) {
				DtaCnt++;
				//first bit stores if it is a 9 bit value or 5 and 4 bit (0 = 9bit, 1 = 5+4bit)
				if (DataBits[curs] == 0) {
					curs++;
					ErrA = ConvertToDec(curs, 9);
					ErrB = 255; //255 reserved value meaning ErrA represents 9 bit value
					curs = curs + 9;
				} else {
					curs++;
					ErrA = ConvertToDec(curs, 5);
					curs = curs + 5;
					ErrB = ConvertToDec(curs, 4);
					curs = curs + 4;
				}
			}

			//store data id
			else if (DtaTpe == 7) {
				DtaCnt++;
				Data[k].BytVal = ConvertToDec(curs, 8);
				curs = curs + 8;
			}

			//store data request
			else if (DtaTpe == 9) {
				DtaCnt++;
				Data[k].BytVal = ConvertToDec(curs, 8);
				curs = curs + 8;
			}

			//store byte
			else if (DtaTpe == 12) {
				DtaCnt++;
				Data[k].BytVal = ConvertToDec(curs, 8);
				curs = curs + 8;
			}

			//check checksum/ end of data
			else if (DtaTpe == 13) {
				state = 1;
			}
		}

		unsigned char checksum[16];		//recieved checksum
		int MsgByteItr;     			//message byte itterations

		//store checksum location for later printf
		CksSpot = curs;
		//store checksum
		for (int i = 0; i < 16; i++) {
			checksum[i] = DataBits[i + curs];
		}

		//re initialize sum array to 0
		for (int i = 0; i < 16; i++) {
			sum[i] = 0;
		}

		//-16 bc checksum is 16 bits and not included in... the checksum
		MsgLen = MsgLen - 16;

		//bc reasons array will never be divide-able by 8 so 4 has to be added OR subtracted
		if (CheckDtaTpe((MsgLen + 4) / 8) == 0) {
			MsgByteItr = (MsgLen + 4) / 8;
		} else if (CheckDtaTpe((MsgLen - 4) / 8) == 0) {
			MsgByteItr = (MsgLen - 4) / 8;
		}

		int BytCnt = 0;     //curs val but for 8bit chop

		//chop bit array in 8 bit parts and add them together to create checksum
		for (int j = 0; j < MsgByteItr; j++) {
			unsigned char BinVal[16] = { 0 };         //chopped 8bit stored here
			for (int i = 0; i < 8; i++) {
				BinVal[i + 8] = DataBits[i + BytCnt]; //i+8 since 8 bit value stored in 16 bit array
			}
			BinAdd(sum, BinVal);               //add the 8 bit value to total
			BytCnt = BytCnt + 8;
		}

		//check if checksum from message is equal to calculated checksum bit by bit bc why bother
		for (int i = 0; i < 16; i++) {
			if (sum[i] != checksum[i]) {
				ErrFlag[6] = 1;
				state = 0;
				break;
			}
		}
	} else {
		state = 0;
	}
	return state;
}

unsigned char MSG_RECIEVING() {

	uint16_t BitStart;
	uint16_t BitTim;

	unsigned char ZeroCnt = 0;
	unsigned char BitOne = 1;
	unsigned char Recieving = 0;
	int BitCnt = 1;

	//mark transition to high for first bit
	if (COM_GET_BIT() == 1 && Recieving == 0) {
		BitStart = COM_GET_TIM();		//save counter time
		Recieving = 1;									//mark first bit and go into the receiving loop
	}

	while(Recieving == 1){

		if (COM_GET_BIT() == 0 && BitOne == 1) {
			BitTim = COM_GET_TIM() - BitStart;		//calculate bit time
			BitOne = 0;
			////printf("New message! #%d\n", MsgCnt);
			uC_DELAY((BitTim / 1000) / 2);				//delay exact time it took transmitter to send a bit
														//since at the end of a bit, only delay half a bit-time
			////printf("BitOneTime: %d\n", BitTim);
			DataBits[0] = 1;
		}
		else if (COM_GET_BIT() == 1 && BitOne == 0) {
			DataBits[BitCnt] = 1;
			BitCnt++;
			ZeroCnt = 0;
			uC_DELAY(BitTim / 1000);
		} else if (COM_GET_BIT() == 0 && BitOne == 0) {
			DataBits[BitCnt] = 0;
			BitCnt++;
			ZeroCnt++;
			uC_DELAY(BitTim / 1000);
		}

		//if more then 10 0's are received, assume end of message
		if (ZeroCnt > 10 && Recieving == 1) {
			ZeroCnt = 0;
			Recieving = 0;
			NewMsg = 1;
			MSG_PROCESS(BitCnt-10);
			MsgCnt++;
		}
	}
}

int MSG_DATA_CNT() {
	return DtaCnt;
}
int MSG_DATA_TYPE(int DtaAddr) {
	return Data[DtaAddr].Tpe;
}
//if there is ony one struct, address is ignored, else the byte of the corresponding address is returned
unsigned char MSG_GET_STRUCT(int DtaAddr) {
	if (mSctCnt == 1) {
		for (int i = 0; i < 32; i++) {
			if (Data[i].Tpe == 5) {
				return Data[i].BytVal;
				break;
			}
		}
	} else {
		return Data[DtaAddr].BytVal;
	}
}

unsigned char MSG_GET_BYTE(int DtaAddr) {
	return Data[DtaAddr].BytVal;
}

int MSG_GET_TRANSMITTER() {
	return mTXaddr;
}

const char* MSG_GET_STRING(int DtaAddr){
	return Data[DtaAddr].Str;
}

int MSG_VAL_DATA_TYPE(int DtaAddr) {
	return Data[DtaAddr].IoF;
}

int MSG_VAL_GET_INT(int DtaAddr) {
	return Data[DtaAddr].IntVal;
}

float MSG_VAL_GET_FLT(int DtaAddr) {
	return Data[DtaAddr].FltVal;
}
void MSG_RX_RDY() {
	NewMsg = 0;
	DtaCnt = 0;
	PkgCnt = 0;
	mSctCnt = 0;
}
