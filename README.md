# SkyCOM

SkyCOM is an advanced serial communications protocol for communication between up to 1023 microcontrollers on a network.

## origins
I started this project at the start of 2021, building a custom serial protocol + library since working with RS232 on industrial CNC machines at my job gave me PTSD.

## features
* checksun
* up to 1023 devices
* int, byte and floating point values send true binary values
* build in system commands

## datatypes
* byte
* ascii strings
* integer
* floating point

# how to use the SkyCOM library

## hardware
---
SkyCOM is designed to work with ttl RX/TX lines or RS485 and the library directly supports and manages RE (Reciever Enable) and DE (Drive Enable) outputs for use with RS485 IC's

## transmitting
---
```
COM_START(addr, version)
```
This sets a static 10 bit device address to be send along with messages from the device. messages which do not include this static address will be ignored.

Protocol version control is also included in the protocol, this can be used to make devices incompatible if you want to exclude legacy devices on your network.
```
COM_DATA_VAL(value)
```
Accepts positive and negative integers and floating point values and adds them to the message.
```
COM_DATA_STRING(string)
```
Adds a regular ASCII string, to the message.
```
COM_DATA_STRUCT(code)
```
Adds the message structure, this value **can** be send along with the data to communicate its structure to the reciever(s). The value *(0-255)* can be interpreted by the users firmware and bound to the according message processing and data interpretation code.
```
COM_DATA_REQ(code)
```
Adds a data request command *(0-255)*to the message. This code can be interpreted by the reciever and may be used to prompt it to return certain data.
```
COM_ERR(A, B)
```
Add an error code to the message. *(0-1023)*
```
COM_DATA_ID(code)
```
Can be interpreted as *(0-255)* COM_DATA_STRUCT but is meant to be inserted before a single package. (inspired by GCODE)
```
int addrs[16] = {5,8,32,75,0,0,0,0,0,0,0,0,0,0,0,0};
COM_TRANSMIT(addrs, BpS)
```
COM_TRANSMIT function generates the bits to be transmitted. The function takes an array of integers *(0-1023)*for reciever addresses and adds them to the message.

The array must be initialized to 0 and have 16 entities

## recieving
---
```
MSG_RECIEVING()
```
Checks if a message is being recieved, if so it will store the bits and process the message.
```
MSG_DATA_TYPE(DataAddress)
```
Returns the datatype/ command code that's send with the data package.

| Code  | Package type |
|-------|-----------|
| 0     | |
| 1     | String |
| 2     | Value|
| 3     | |
| 4     | |
| 5     | Struct |
| 6     | ERROR |
| 7     | Data ID |
| 8     |  |
| 9     | Data request |
| 10    |  |
| 11    | |
| 12    | Byte |
| 13    | Checksum |
| 14    | |
| 15    | |

**Importand!**

Checksums are only visible to SkyCOM.c and are handled by the library thus ```MSG_DATA_TYPE()``` will never return 13.

```
MSG_VAL_DATA_TYPE()
```
Since floats and string are send in the same package type, this function returns if the package contains an int or float.

```0 = int, 1 = float ```

```
MSG_DATA_CNT()
```
Returns the ammount of data packages.
```
MSG_GET_STRUCT(DataAddress)
```
If there is only one struct in the message, data address is ignored, else the function will return the byte of the address.
```
MSG_GET_BYTE(DataAddress)
```
Returns the byte value of the package at set address
```
MSG_VAL_GET_FLT(DataAddress)
MSG_VAL_GET_INT(DataAddress)
```
These functions return an int or float from a value package.
```
MSG_RX_RDY()
```
This re-initializes some variables so a new message can be recieved.

*```MSG_RX_RDY()``` MUST BE RUN AFTER EVERY RECIEVED MESSAGE IS DONE PROCCESING!*
# Examples

```
void COM_SET_BIT(){
	HAL_GPIO_WritePin(GPIOB, RS485_TX_Pin, GPIO_PIN_SET);
}
void COM_RESET_BIT(){
	HAL_GPIO_WritePin(GPIOB, RS485_TX_Pin, GPIO_PIN_RESET);
}

int COM_GET_BIT(){
	if(HAL_GPIO_ReadPin(GPIOB, RS485_RX_Pin) == GPIO_PIN_SET){
		return 1;
	} else if(HAL_GPIO_ReadPin(GPIOB, RS485_RX_Pin) == GPIO_PIN_RESET){
		return 0;
	}
}
uint16_t COM_GET_TIM(){
	return __HAL_TIM_GET_COUNTER(&htim6);
}
```
Since the library is universal and works on anything running C, the hardware commands can be set somewhere else outside of the SkyCOM.c file.
```
COM_SET_BIT()       //set TX pin HIGH
```
```
COM_RESET_BIT()     //set TX pin LOW
```
```
COM_GET_BIT()       //get RX pin value
```
```
COM_GET_TIM()       //get uS timer value
```
**These functions are neccesairy for recieving and transmitting!**

## transmission example
---
```
int main(void){

    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM6_Init();

    //start timer
    HAL_TIM_Base_Start(&htim6);
    
    //setup SkyCOM library
    COM_START(1, 2);        //address 1, protocol 2

    int Addrs[16] = {2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  
    while(1){
        COM_DATA_STRUCT(12);        //set data structure to 12
        COM_DATA_BYTE(1);           //add byte
        COM_DATA_BYTE(0);           //..
        COM_TRANSMIT(Addrs,1000);   //transmit message at 1000 bits per second

        //delay
        HAL_Delay(500);

        //repeat with bytes flipped
        COM_DATA_STRUCT(12);
        COM_DATA_BYTE(0);
        COM_DATA_BYTE(1);
        COM_TRANSMIT(Addrs,1000);

        HAL_Delay(500);
    }
}
```

Example of the user functions for STM32 usign the Hardware Abstraction Layer (HAL) and timer 6.

## recieving example
---
```
int main(void){

    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM6_Init();

    //start timer
    HAL_TIM_Base_Start(&htim6);

    //setup SkyCOM library
    COM_START(2, 2);        //address 2, protocol 2

    while(1){
        //check if recieving and if so store and process the bits
        MSG_RECIEVING();

        if(MSG_NEW() == 1){
            //check structure of message so correct data proccessing is used
            if(MSG_GET_STRUCT(0) == 12){
                //check value of byte 1 and toggle LED based on value
                if(MSG_GET_BYTE(1) == 1){
                    HAL_GPIO_WritePin(GPIOA, LED1_Pin, GPIO_PIN_SET);
                } else if(MSG_GET_BYTE(1) == 0){
                    HAL_GPIO_WritePin(GPIOA, LED1_Pin, GPIO_PIN_RESET);
                }

                //check value of byte 2 and toggle LED based on value
                if(MSG_GET_BYTE(2) == 1){
                    HAL_GPIO_WritePin(GPIOA, LED2_Pin, GPIO_PIN_SET);
                } else if(MSG_GET_BYTE(2) == 0){
                    HAL_GPIO_WritePin(GPIOA, LED2_Pin, GPIO_PIN_RESET);
                }
            }
            //clear message so new message can be recieved
            MSG_RX_RDY();
        }
    }
}
```
# License
Copyright [2021] [Milos de Wit]

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
