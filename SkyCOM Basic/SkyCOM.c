#include "SkyCOM.h"
#include <string.h>
//stdio only neccesairy for debugging
//#include <stdio.h>

char StrBits[255];				//all characters from message are in this array
char StrResv[127];
unsigned char DataBits[1500];   //holds the DataBits/ receive buffer
unsigned char SetpBits[200];   	//holds all non data values except checksum
unsigned char sum[16];     		//holds added binary values(checksum)
unsigned char StrCnt = 0;
int BitAddr = 0;          		//variable used for bit location in bit arrays
int DtaCnt = 0;        			//count of data's in message
int dVer = 1;           		//device protocol
int dTXaddr = 0;      			//device address on network

int mTXaddr = 0;			//received transmitter address
int mSctCnt = 0;			//struct count of recieved message

unsigned char ErrCode = 0;
unsigned int  mErrorCode = 0;

struct Str {
	unsigned char ArrAddr;
	unsigned char MsgAddr;
	unsigned char StrLen;
};
struct Str StrInfo[32];

struct Dta {
	unsigned char Tpe;       //stores data type, see table for values
	unsigned char BytVal;	 //store 8 bit byte
	unsigned char IoF;     //stores if value is int or float, 0 = int, 1 = float
	float FltVal;    //stores the possible float value
	int IntVal;    //stores the possible int value
};
struct Dta Data[32];

//to the power funtion bc I can't be arsed to include math.h
//I googled if it s assed or arsed
double power(int base, int exp){
	double result = 1.0;
	while(exp != 0){
		result *= base;
		--exp;
	}
	return result;
}
//add two binary array values
void BinAdd(unsigned char A[16], unsigned char B[16]){

	unsigned char remainder = 0;
	for(int i = 15; i >= 0; i--){
		unsigned rem = 0;
		if(remainder){
			++rem;
		}
		if(A[i])
			++rem;
		if(B[i])
			++rem;
		if(rem % 2 == 1){
			sum[i] = 1;
		} else {
			sum[i] = 0;
		}
		if(rem > 1){
			remainder = 1;
		} else {
			remainder = 0;
		}
	}
}

//convert value to bits in correct byte size and add to specific BitAddr in DataBits or SetpBits array
void DECtoBIN(int val, int size, int cursor, unsigned char arr){ //decimal value, byte size, location in array, array(0 = DataBits, 1 = SetpBits)
	unsigned char BIN[size];     //holds local bits

	//initialize bits array because otherwise output will be fucky-wucky
	for(int i = 0; i < size; i++){
		BIN[i] = 0;
	}

	int j = size - 1;   //since counting from 0, counter = size -1

	//sore remainder in local bits array
	while(val > 0){
		BIN[j] = val % 2;
		val = val / 2;
		j--;
	}

	//store bits in global DataBits/SetpBits array
	if(arr == 0){
		for(int k = 0; k < size; k++){
			DataBits[k + cursor] = BIN[k];
		}
	} else {
		for(int k = 0; k < size; k++){
			SetpBits[k + cursor] = BIN[k];
		}
	}
}

//convert part of binary array to decimal integer
int ConvertToDec(int curs, int size){
	int result = 0;
	unsigned char nBitByt[size];

	for(int i = 0; i < size; i++){
		nBitByt[i] = DataBits[i + curs];
	}

	for(int i = 0; i < size; i++){
		result <<= 1;
		result |= nBitByt[i];
	}

	return result;
}

//glorified for loop
void ArrayArray(unsigned char Byt[], int size){
	for(int i = 0; i < size; i++){
		DataBits[i + BitAddr] = Byt[i];
	}
	BitAddr = BitAddr + size;
}
//check if value is an int or float
unsigned char CheckDtaTpe(float val){

	int IntVal = val;               //store int of the float val
	float IntFloat = val - IntVal;  //convert the int val back to a float

	//if the value is equal to 0, it's an int, otherwise it's a float
	if(IntFloat == 0){
		return 0;
	} else if(IntFloat != 0){
		return 1;
	}
}
//return the ammount of 8bytes a value uses
int ValBytCntr(float val){
	int Len = 1;

	//calculate the ammount of bytes the value uses
	for(int i = 0; i < 8; i++){
		if(val < power(2.0, 8 *(i + 1))){
			break;
		}
		Len++;
	}
	return Len;
}
//add 4 bit command value and 8 bit value to array
void AddCmnd(unsigned char Byt[4], int var){
	DtaCnt++;

	//store it in global DataBits array
	ArrayArray(Byt, 4);

	DECtoBIN(var, 8, BitAddr, 0);     //add actual bit value

	BitAddr = BitAddr + 8;            //move cursor 8byte
}

void COM_START(int addr, int protocol){
	//set RS485 driver enable pins LOW
	uC_EN_PIN(0);

	dVer = protocol;
	dTXaddr = addr;
}

//add an ID to the message
void COM_DATA_ID(int val){
	unsigned char CmdCode[4] = {0, 1, 1, 1}; //baked Data ID 4byte(7, Data ID)
	AddCmnd(CmdCode, val);
}
//add a struct to the message
void COM_DATA_STRUCT(int val){           //structure ID
	unsigned char CmdCode[4] = {0, 1, 0, 1}; //baked struct 4byte(5, Structure set)
	AddCmnd(CmdCode, val);
}
//add a data request to the message
void COM_DATA_REQ(int val){           //data ID
	unsigned char CmdCode[4] = {1, 0, 0, 1}; //baked data request 4byte(9, DataRequest)
	AddCmnd(CmdCode, val);
}
//add a byte(0-255)to teh message
void COM_DATA_BYTE(int val){           //data ID
	unsigned char CmdCode[4] = {1, 1, 0, 0}; //baked data request 4byte(9, DataRequest)
	AddCmnd(CmdCode, val);
}
//add error code
void COM_ERR(int Error){
	DtaCnt++;

	unsigned char Byt[4] = {0, 1, 1, 0};    //baked error 4byte(6, error)
	ArrayArray(Byt, 4);

	DECtoBIN(Error, 10, BitAddr, 0);
	BitAddr += 10;
}

//add string to message
void COM_DATA_STRING(char Msg[]){
	unsigned char SoP[4] = {0, 0, 0, 1};  //baked start of string package byte
	unsigned char EoP[4] = {1, 1, 1, 0};    //baked end of package byte

	int len = strlen(Msg);     //get string length

	AddCmnd(SoP, len); //use AddCmnd function bc adding a 4bit and 8bit value anyways

	//put ASCII values of the string characters bits array
	for(int i = 0; i < len; i++){
		DECtoBIN(Msg[i], 8, BitAddr, 0); //covert and add character to global data array
		BitAddr = BitAddr + 8;            //move cursor 8byte
	}

	//put end of package byte in bits array
	for(int i = 0; i < 4; i++){
		DataBits[i + BitAddr] = EoP[i];
	}

	BitAddr += 4;    //move cursor half-byte
	DtaCnt++;           //add to package count
}
//add value to message
void COM_DATA_VAL(float val){

	unsigned char DtaTpe;                    //1 = float, 0 = int
	unsigned char Pol;

	if(val < 0){               //store, correct and check if value is negative
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
	if(DtaTpe == 0){
		int DtaLen = ValBytCntr(val); //stores the ammount of bytes the value takes up
		int Len = 8 * DtaLen;

		DECtoBIN(DtaLen, 4, BitAddr, 0);  //store size of value in 4 bytes
		BitAddr = BitAddr + 4;
		int IntVal = val;
		DECtoBIN(IntVal, Len, BitAddr, 0);
		BitAddr = BitAddr + Len;
	}
	//-----float---------------
	else {
		unsigned char Int = 0;
		int Pnt = 0;

		//make value integer and store the decimal point location
		while(Int == 0){
			val = val * 10;     //move decimal point(until val is an integer)

			int IntegerVal = val;               //store int of the float val
			float IntFlt = val - IntegerVal; //convert the int val back to a float

			//if the value is equal to 0, it's an int, otherwise it's a float
			if(IntFlt == 0){
				Int = 1;
			} else if(IntFlt != 0){
				Int = 0;
			}
			Pnt++;
		}

		int DtaLen = ValBytCntr(val); //stores the ammount of bytes the value takes up
		int Len = 8 * DtaLen;

		DECtoBIN(DtaLen, 6, BitAddr, 0);  //store size of value in a 4byte
		BitAddr += 6;

		DECtoBIN(Pnt, 6, BitAddr, 0);     //store the decimal point BitAddr in a 6byte
		BitAddr += 6;

		DECtoBIN(val, Len, BitAddr, 0);
		BitAddr += Len;
	}
	//--------------------

	//put end of package 4byte in bits array
	ArrayArray(EoP, 4);
	DtaCnt++;
}

//puts it all together
void COM_TRANSMIT(int RX_addrs[16], int BpS){ //address array, MUST be an initialized 16 enttity array
	int addrs = 0;

	for(int i = 0; i < 16; i++){
		if(RX_addrs[i] == 0){
			addrs = i;
			break;
		} else {
			DECtoBIN(RX_addrs[i], 10, 16 +(10 * i), 1);
		}
	}

	//calculate setup bit length(start bits + transmitter adress + reciever count + reciever addresses + version + package count)=(2 + 10 + 4 + 10*reciever_count)+ 6 + 8)
	int SetpLen = 16 + (10 * addrs);

	//add transmitter address, protovol version and message package count
	DECtoBIN(dTXaddr, 10, 2, 1); //place 2 bc at start of setup array, only start bits prior(2)
	DECtoBIN(dVer, 6, SetpLen, 1);
	DECtoBIN(DtaCnt, 8, SetpLen + 6, 1);

	SetpLen += 14;     //add 14 bits to SetpLen

	DECtoBIN(addrs, 4, 12, 1);     //add reciever count

	unsigned char SoM[2] = { 1, 0 };        //start of message bits
	unsigned char CKS[4] = { 1, 1, 0, 1 };    //baked checksum bits

	//add checksum 4byte to array
	for(int i = 0; i < 4; i++){
		DataBits[i + BitAddr] = CKS[i];
	}
	BitAddr = BitAddr + 4;

	for(int i = 0; i < 2; i++){
		SetpBits[i] = SoM[i];
	}

	int MsgLenght = SetpLen + BitAddr;

	//merge setup and data arrays
	unsigned char FullMessage[MsgLenght + 20]; //Length of both arrays, + 16 bit checksum + 4 bit end of message
	for(int i = 0; i < MsgLenght; i++){
		if(i < SetpLen){
			FullMessage[i] = SetpBits[i];
		} else {
			FullMessage[i] = DataBits[i - SetpLen];
		}
	}

	//re initialize sum array to 0
	for(int i = 0; i < 16; i++){
		sum[i] = 0;
	}

	int MsgByteItr;     //amount of iterations to divide message in 8 bit parts
	int BytCnt = 0;		//separate cursor only for 8 bit chop

	//message is divide-able by eight only if 4 is added OR subtracted
	if(CheckDtaTpe((MsgLenght + 4)/ 8)== 0){
		MsgByteItr =(MsgLenght + 4)/ 8;
	} else if(CheckDtaTpe((MsgLenght - 4)/ 8)== 0){
		MsgByteItr =(MsgLenght - 4)/ 8;
	}

	for(int j = 0; j < MsgByteItr; j++){
		unsigned char BinVal[16] = { 0 };		//store byte from message
		for(int i = 0; i < 8; i++){	//copy byte from message to buffer array
			BinVal[i + 8] = FullMessage[i + BytCnt];
		}
		BinAdd(sum, BinVal);			//add buffer to total checksSUM value
		BytCnt = BytCnt + 8;				//move cursor
	}

	//add checksum to full-message array
	for(int i = 0; i < 16; i++){
		FullMessage[i + MsgLenght] = sum[i];
	}

	//set RS485 driver enable pins HIGH
	//uC_EN_PIN(1);

	//go through bits and set to value
	for(int i = 0; i < MsgLenght + 16; i++){
		if(FullMessage[i] == 0){
			//set TX pin LOW
			uC_TX_PIN(0);
		} else if(FullMessage[i] == 1){
			//set TX pin HIGH
			uC_TX_PIN(1);
		}
		uC_DelayMs(1000/BpS);
		//bits per second for transmission(max 1000 is using "HAL_Delay()" function
		//uC_DELAY(1000/BpS);
	}

	//set RS485 driver enable pins LOW
	//uC_EN_PIN(0);
	//set TX pin LOW after transmission
	uC_TX_PIN(0);

	BitAddr = 0;
	DtaCnt = 0;
}


unsigned char NewMsg = 0;
int MsgCnt = 0;

//process all data in the message
void MSG_PROCESS(){

	//printf("\n------------------------------------------------\n");

	//cursor
	int curs = 0;
	//bit address StrBits
	int StrAddr = 0;
	//checksum bit address
	int CksSpot;
	//stores other receivers of message
	int mRXaddr[16];
	//message protocol version
	int mVer;
	//amount of addresses send along with message
	unsigned char AddrCnt = 0;



	/*store the device's role as address.
	 *
	 * pending address		0
	 * general address	 	1
	 * address included		2
	 * address excluded		3
	 */

	unsigned char DeviceAddrState = 0;

	//set error code to busy
	ErrCode = 3;

	//check start bits
	if(DataBits[0] != 1 || DataBits[1] != 0){
		ErrCode = 4;
	}

	curs = 2;

	//store transmitter addresses
	mTXaddr = ConvertToDec(curs, 10);
	curs = 12;

	//store receiver address count
	AddrCnt = ConvertToDec(curs, 4);
	curs += 4;

	ErrCode = 5;
	//printf("address: %d");

	//store receiver addresses
	for(int i = 0; i < AddrCnt; i++){
		mRXaddr[i] = ConvertToDec(curs, 10);
		//printf("%d, ", mRXaddr[i]);
		if(mRXaddr[i] == dTXaddr){
			DeviceAddrState = 2;
			ErrCode = 6;
		}
		curs = curs + 10;
	}

	//printf("\n");

	if(DeviceAddrState == 2){
		//store protocol version
		mVer = ConvertToDec(curs, 6);
		curs += 6; //curs 7 bc bit 7 from curs holds if packagecount is it or datacount(non string/ value package messages)

		//store package/data count
		DtaCnt = ConvertToDec(curs, 8);
		curs += 8;

		//process the actual data, loop repeats DtaCnt times
		for(int k = 0; k < DtaCnt + 1; k++){
			int DtaTpe;
			//store datatype
			DtaTpe = ConvertToDec(curs, 4);
			curs += 4;
			Data[k].Tpe = DtaTpe;

			//convert and store string message
			if(DtaTpe == 1){
				//convert and store package length/ character count
				unsigned char Len = ConvertToDec(curs, 8);
				curs += 8;

				//store location in StrBits array
				StrInfo[StrCnt].ArrAddr = StrAddr;
				//stores string length
				StrInfo[StrCnt].StrLen = Len;
				//store data address
				StrInfo[StrCnt].MsgAddr = k;

				//convert and store package character data
				for(int j = 0; j < Len; j++){
					StrBits[j + StrInfo[StrCnt].ArrAddr] = ConvertToDec(curs, 8);
					StrAddr++;
					curs += 8;
				}

				StrCnt++;

				//check for end of package
				if(ConvertToDec(curs, 4) != 14){
					ErrCode = 4;
				}
				curs += 4;
			}

			//convert and store value message
			else if(DtaTpe == 2){
				//store datatype(int or float)and value polarity
				unsigned char DtTp = DataBits[curs];
				unsigned char Pol = DataBits[curs + 1];
				curs += 2;

				//store datatype(0 int, 1 float)in IntOrFloat
				Data[k].IoF = DtTp;

				if(DtTp == 0){
					int Len = ConvertToDec(curs, 4);
					curs += 4;
					//len value is send in bytes, so x8 is neccesairy
					int Val = ConvertToDec(curs, Len * 8);
					//check and adjust polarity
					if(Pol == 0){
						Val = Val * -1;
					}
					curs += (Len * 8);
					Data[k].IntVal = Val;
				} else {
					int Len = ConvertToDec(curs, 6);
					curs += 6;
					//point value stores amount of divisions/decimal places the point is from least significant bit
					int Pnt = ConvertToDec(curs, 6);
					curs += 6;
					//len value is send in bytes, so x8 is needed
					float Val = ConvertToDec(curs, Len * 8);
					//divide amount of times decimal is shifted to create a float
					for(int i = 0; i < Pnt; i++){
						Val = Val / 10;
					}
					if(Pol == 0){
						Val = Val * -1;
					}
					Data[k].FltVal = Val;
					curs += (Len * 8);
				}

				//check for end of package
				if(ConvertToDec(curs, 4) != 14){
					ErrCode = 4;
				}
				curs += 4;
			}

			//store struct id
			else if(DtaTpe == 5){
				mSctCnt++;
				Data[k].BytVal = ConvertToDec(curs, 8);
				curs += 8;
			}

			//store error code)
			else if(DtaTpe == 6){
				mErrorCode  = ConvertToDec(curs, 10);
				curs += 10;
			}

			//store data id
			else if(DtaTpe == 7){
				Data[k].BytVal = ConvertToDec(curs, 8);
				curs += 8;
			}

			//store data request
			else if(DtaTpe == 9){
				Data[k].BytVal = ConvertToDec(curs, 8);
				curs += 8;
			}

			//store byte
			else if(DtaTpe == 12){
				Data[k].BytVal = ConvertToDec(curs, 8);
				curs += 8;
			}

			//check checksum/ end of data
			else if(DtaTpe == 13){
				//store checksum start position
				CksSpot = curs;
			}
		}

		unsigned char checksum[16];		//recieved checksum
		int MsgByteItr;     			//message byte itterations

		//store checksum
		for(int i = 0; i < 16; i++){
			checksum[i] = DataBits[i + CksSpot];
		}

		//re initialize sum array to 0
		for(int i = 0; i < 16; i++){
			sum[i] = 0;
		}

		//bc reasons array will never be divide-able by 8 so 4 has to be added OR subtracted
		if(CheckDtaTpe((CksSpot + 4)/ 8)== 0){
			MsgByteItr =(CksSpot + 4)/ 8;
		} else if(CheckDtaTpe((CksSpot - 4)/ 8)== 0){
			MsgByteItr =(CksSpot - 4)/ 8;
		}

		int BytCnt = 0;     //curs val but for 8bit chop

		for(int i = 0; i < 16; i++){
			sum[i] = 0;
		}

		//chop bit array in 8 bit bytes and add them together to create a checksum
		for(int j = 0; j < MsgByteItr; j++){
			unsigned char BinVal[16] = { 0 };         //chopped 8bit stored here
			for(int i = 0; i < 8; i++){
				BinVal[i + 8] = DataBits[i + BytCnt]; //i+8 since 8 bit value stored in 16 bit array
			}
			BinAdd(sum, BinVal);               //add the 8 bit value to total
			BytCnt += 8;
		}

		//printf("\n gott. > ");

		for(int i = 0; i < 16; i++){
			//printf("%d", checksum[i]);
		}

		//printf("\n calc. > ");

		//check if checksum from message is equal to calculated checksum bit by bit bc why bother
		for(int i = 0; i < 16; i++){
			//printf("%d", sum[i]);
			if(sum[i] != checksum[i]){
				ErrCode = 5;
			}
		}

		if(ErrCode == 5){
			//printf("\nCHECKSUM ERROR!\n");
			MSG_RX_RDY();
		} else{
			ErrCode = 2;
			//printf("\nCHECKSUM OK!\n");
		}

	} else {
		//printf("errcode: %d\n", ErrCode);
	}
}

unsigned char MSG_STATE(){
	return ErrCode;
}

uint16_t BitStart;
uint16_t BitTim;

unsigned char MSG_RECIEVING(){

//	uint16_t BitStart;
//	uint16_t BitTim;

	unsigned char ZeroCnt = 0;
	unsigned char BitOne = 1;
	unsigned char Recieving = 0;
	int BitCnt = 1;

	//mark transition to high for first bit
	if(uC_GET_BIT() == 1){
		BitStart = uC_GET_TIM();		//save counter time
		Recieving = 1;									//mark first bit and go into the receiving loop
	}

	while(Recieving == 1){

		if(uC_GET_BIT() == 0 && BitOne == 1){
			BitTim = uC_GET_TIM() - BitStart;		//calculate bit time
			BitOne = 0;
			//////printf("New message! #%d\n", MsgCnt);
			uC_DelayMs((BitTim / 1000)/ 2);				//delay exact time it took transmitter to send a bit
														//since at the end of a bit, only delay half a bit-time
			//////printf("BitOneTime: %d\n", BitTim);
			DataBits[0] = 1;
		} else if(uC_GET_BIT() == 1 && BitOne == 0){
			DataBits[BitCnt] = 1;
			BitCnt++;
			ZeroCnt = 0;
			uC_DelayMs(BitTim / 1000);
		} else if(uC_GET_BIT() == 0 && BitOne == 0){
			DataBits[BitCnt] = 0;
			BitCnt++;
			ZeroCnt++;
			uC_DelayMs(BitTim / 1000);
		}

		//if more then 10 0's are received, assume end of message
		if(ZeroCnt > 10 && Recieving == 1){
			ZeroCnt = 0;
			Recieving = 0;
			ErrCode = 2;
			MSG_PROCESS();
			MsgCnt++;
		}
	}
}

int MSG_DATA_CNT(){
	return DtaCnt;
}
int MSG_DATA_TYPE(int DtaAddr){
	return Data[DtaAddr].Tpe;
}
//if there is ony one struct, address is ignored, else the byte of the corresponding address is returned
unsigned char MSG_GET_STRUCT(int DtaAddr){
	if(mSctCnt == 1){
		for(int i = 0; i < 32; i++){
			if(Data[i].Tpe == 5){
				return Data[i].BytVal;
				break;
			}
		}
	} else {
		return Data[DtaAddr].BytVal;
	}
}

unsigned char MSG_GET_BYTE(int DtaAddr){
	return Data[DtaAddr].BytVal;
}

int MSG_GET_TRANSMITTER(){
	return mTXaddr;
}

//reserved memory space for MSG_GET_STRING() function


const char* MSG_GET_STRING(int DtaAddr){

	for(int i = 0; i < 127; i++){
		StrResv[i] = 0;
	}

	for(int i = 0; i < 32; i++){
		if(StrInfo[i].MsgAddr == DtaAddr){
			for(int j = 0; j < StrInfo[i].StrLen; j++){
				StrResv[j] = StrBits[j + StrInfo[i].ArrAddr];
			}
			break;
		}
	}

	return StrResv;
}

int MSG_VAL_DATA_TYPE(int DtaAddr){
	return Data[DtaAddr].IoF;
}

int MSG_VAL_GET_INT(int DtaAddr){
	return Data[DtaAddr].IntVal;
}

unsigned int MSG_ERROR(int DtaAddr){
	return mErrorCode;
}

float MSG_VAL_GET_FLT(int DtaAddr){
	return Data[DtaAddr].FltVal;
}
void MSG_RX_RDY(){
	StrCnt = 0;
	NewMsg = 0;
	DtaCnt = 0;
	ErrCode = 0;
	mErrorCode = 0;
	mSctCnt = 0;
}
