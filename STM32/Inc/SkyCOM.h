#ifndef SkyCOM_H_   /* Include guard */
#define SkyCOM_H_

#include "stm32l1xx_hal.h"

void COM_START(int addr, int protocol);

void COM_DATA_ID(int val);
void COM_DATA_STRUCT(int val);
void COM_DATA_REQ(int val);
void COM_DATA_BYTE(int val);

void COM_ERR(int A, int B);

void COM_DATA_STRING(char Msg[]);
void COM_DATA_VAL(float val);

void COM_TRANSMIT(int RX_addrs[16], int BpS);


unsigned char DataBits[1500];    //holds the DataBits/ receive buffer

unsigned char MSG_RECIEVING();
int MSG_PROCESS(int MsgLen);
int MSG_DATA_CNT();
int MSG_DATA_TYPE(int DtaAddr);
unsigned char MSG_GET_STRUCT(int DtaAddr);
unsigned char MSG_GET_BYTE(int DtaAddr);
int MSG_GET_DATA_TYPE(int DtaAddr);
int MSG_VAL_GET_INT(int DtaAddr);
float MSG_VAL_GET_FLT(int DtaAddr);

void MSG_RX_RDY();

#endif
