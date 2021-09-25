#ifndef SkyCOM_H_
#define SkyCOM_H_

//fundamental functions
void COM_START(int addr, int protocol);
void COM_TRANSMIT(int RX_addrs[16], int BpS);
unsigned char MSG_RECIEVING();
void MSG_RX_RDY();

//data functions
void COM_DATA_ID(int val);
void COM_DATA_STRUCT(int val);
void COM_DATA_REQ(int val);
void COM_DATA_BYTE(int val);

void COM_ERR(int Error);

void COM_DATA_STRING(char Msg[]);
void COM_DATA_VAL(float val);

//user hardware functions
void uC_TX_PIN(unsigned char bit);
void uC_EN_PIN(unsigned char bit);


void uC_DelayMs(uint32_t MsDelay);
uint16_t uC_GET_TIM();
int uC_GET_BIT();

unsigned char MSG_STATE();

//data functions
int MSG_VAL_DATA_TYPE(int DtaAddr);
int MSG_GET_DATA_TYPE(int DtaAddr);

unsigned int MSG_ERROR(int DtaAddr);

int MSG_DATA_CNT();
unsigned char MSG_GET_STRUCT(int DtaAddr);
unsigned char MSG_GET_BYTE(int DtaAddr);

const char* MSG_GET_STRING(int DtaAddr);

int MSG_VAL_GET_INT(int DtaAddr);
float MSG_VAL_GET_FLT(int DtaAddr);

#endif
