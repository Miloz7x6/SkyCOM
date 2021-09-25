# SkyCOM

SkyCOM is an advanced, easy to implement serial communications protocol for communication between up to 1023 microcontrollers on a network.

## Origins
I started this project at the start of 2021, building a custom serial protocol + library since working with RS232 on industrial CNC machines at my job gave me PTSD.

## features
* easy to process and implement in projects
* checksun
* up to 1023 devices
* int, byte and floating point values send as true binary values
* build in system commands

## datatypes
* byte
* ascii strings
* integer
* floating point

## this repository
* For simple projects implementation you can use the basic library, this doesn't use interrupts and only one hardware timer. Because of this it uses clock cycles when transmitting but is really easy to implement and fine for basic/ low speed projects.


* When implemented in more advanced projects, use of the advanced library is reccomended, this uses interrupts and timers and is harder to implement in a project. This does make the library more efficient and if you start a new project it is reccomended to use this version of the library.

# How to use the SkyCOM library

## Hardware
---
SkyCOM is designed to work with ttl RX/TX lines or RS485 and the library directly supports and manages RE (Reciever Enable) and DE (Drive Enable) outputs for use with RS485 IC's

## Transmitting
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

## Recieving
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
//uC set TX pin value
void uC_TX_PIN(unsigned char bit){
	if(bit == 1){
		HAL_GPIO_WritePin(GPIOB, RS485_TX_Pin, GPIO_PIN_SET);
	}
	else if(bit == 0){
		HAL_GPIO_WritePin(GPIOB, RS485_TX_Pin, GPIO_PIN_RESET);
	}

}

//uC return RX pin value
int uC_GET_BIT(){
	if(HAL_GPIO_ReadPin(GPIOB, RS485_RX_Pin) == GPIO_PIN_SET){
		return 1;
	} else if(HAL_GPIO_ReadPin(GPIOB, RS485_RX_Pin) == GPIO_PIN_RESET){
		return 0;
	}
}

//uC delay milliseconds
void uC_DelayMs(uint32_t MsDelay){
	HAL_Delay(MsDelay);
}

//uC return timer
uint16_t uC_GET_TIM(){
	return __HAL_TIM_GET_COUNTER(&htim6);
}
```
Since the library is universal and works on anything running C, the hardware commands can be set somewhere else outside of the SkyCOM.c file.
```
uC_TX_PIN(bit)      //set TX pin
```
```
uC_DelayMs(Delay)   //uc delay in milliseconds (1/1000 second)
```
```
uC_GET_BIT()        //get RX pin value
```
```
uC_GET_TIM()        //get uS timer value
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

  //start timer (STM32) 
  HAL_TIM_Base_Start(&htim6);

  //start library as device one, protovol version 2
  COM_START(1, 2);

  //reciever adress array
  int Addrs[16] = {2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

  while(1){

	  COM_DATA_STRUCT(15);
	  COM_DATA_VAL(42);
	  COM_DATA_STRING("hello there");
	  COM_TRANSMIT(Addrs,1000);

	  HAL_Delay(500);

	  COM_DATA_STRUCT(15);
	  COM_DATA_VAL(52.5645);
	  COM_DATA_STRING("made by miloz");
	  COM_TRANSMIT(Addrs,1000);

	  HAL_Delay(500);
  }
}
```

Example of the user functions for STM32 usign the Hardware Abstraction Layer (HAL) and timer 6. And ```printf()``` through debugger.

## recieving example
---
```
int main(void){

  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_TIM6_Init();
  
  //start timer (STM32) 
  HAL_TIM_Base_Start(&htim6);

  //start library as device 2, protovol version 2
  COM_START(2, 2);

  while(1){
      //this function checks if a message is being recieved, if so recieve and process message
	  MSG_RECIEVING();

      //if a new message is available, MSG_NEW() is 1
	  if(MSG_NEW() == 1){

		  printf("\nstruct: #%d\n", MSG_GET_STRUCT(0));

          //specific processing for message
		  if(MSG_GET_STRUCT(0) == 15){

			  if(MSG_VAL_DATA_TYPE(1) == 0){
				  printf("integral val: %d\n", MSG_VAL_GET_INT(1));
			  } else if(MSG_VAL_DATA_TYPE(1) == 1){
				  printf("floating val: %f\n", MSG_VAL_GET_FLT(1));
			  }

			  printf("string: %s\n", MSG_GET_STRING(2));
		  }

          //when done processing message, MSG_RX_RDY() enebles reciever again.
		  MSG_RX_RDY();
	  }
  }
}
```
## serial output
```
struct: #15
integral val: 42
string: hello there

struct: #15
floating val: 52.564503
string: made by miloz
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
