/** ************************************************************************************************
 *	DDSU666H 
 *  (c) Fernando R (iambobot.com)
 * 	1.0.0 - June 2025 - created
 *
 ** ************************************************************************************************
**/

#include <stdio.h>
#include <string.h>			// memset
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"		// ESP_LOGW

#include "config.h"
#include "modbus.h"
#include "DDSU666H.h"


/**
---------------------------------------------------------------------------------------------------
MODBUS Protocol 

MODBUS© Protocol is a messaging structure, widely used to establish master-slave communication 
between intelligent devices. A MODBUS message sent from a master to a slave contains the address 
of the slave, the 'command' (e.g. 'read register' or 'write register'), the data, 
and a check sum (LRC or CRC).

Request for a single floating point parameter i.e. two 16-bit Modbus Protocol Registers
First Byte                                                      Last Byte 
	0			1		2		3		4		5			6		7
Slave 	| Function | Start Address | Number of Points |  Error Check    |
Address	| Code 	   | (Hi) 	| (Lo) | (Hi)  |   (Lo)   |  (Hi) 	| (Lo)  |


Normal response to a request for a single floating point parameter i.e. two 16-bit Modbus Protocol Registers. 
First Byte                                                              Last Byte 
	0			1		2		3		4		  5			6		7		8
Slave 	| Function | Byte   | First Register | Second Register |  Error Check    |
Address	| Code 	   | Count 	| (Hi)  |  (Lo)  | (Hi)  |   (Lo)  |  (Hi) 	| (Lo)  |


---------------------------------------------------------------------------------------------------
Inverter <--> DDSU666H message exchange (sniffed traffic)

DDSU666H is modbus address 0x0B
The inverter sends periodically 3 types of requests to slave address 0x0B 

(1) Single register 2006
0B 03 20 06 00 02 2F 60 0B 03 04 BE 0D 6A 16 4A B6
Every 250-300ms the inverter requests reading register 2006 (active power)
Inverter request 	0B 03 20 06 00 02 2F 60
DDSU666H response	0B 03 04 BE 0D 6A 16 4A B6

(2) Multiple register 2000 .. 2020
0B 03 20 00 00 22 CE B9 0B 03 44 43 68 33 33 3F 16 45 A2 00 00 00 00 BE 0B C6 A8 BE 0B C6 A8 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 3E 0B C6 A8 3E 0B C6 A8 00 00 00 00 BF 80 00 00 BF 80 00 00 BF 80 00 00 00 00 00 00 42 47 F5 C3 63 F0
Every 1-5 seconds the inverter sends a multiple register read from starting address 2000 for 34 (0022hex) positions = 17 registers
Inverter request 	0B 03 20 00 00 22 CE B9
DDSU666H response	0B 03 44 
					43 68 33 33 3F 16 45 A2 00 00 00 00 BE 0B C6 A8 
					BE 0B C6 A8 00 00 00 00 00 00 00 00 00 00 00 00 
					00 00 00 00 3E 0B C6 A8 3E 0B C6 A8 00 00 00 00 
					BF 80 00 00 BF 80 00 00 BF 80 00 00 00 00 00 00 
					42 47 F5 C3 
					63 F0
				
			byte	value			
			00		0B	slave address
			01		03	function code 	0x03 = Read Multiple Holding Registers
			02		44  byte count = 44 hex (68 dec) bytes =>  68 / 4 bytes per register = 17 registers 2000 .. 2020
					value (4 bytes)		register
			03  	43 68 33 33  		2000
					3F 16 45 A2 		2002
					00 00 00 00 		2004
					BE 0B C6 A8 		2006
					BE 0B C6 A8 		2008
					00 00 00 00 		200A
					00 00 00 00 		200C
					00 00 00 00 		200E
					00 00 00 00 		2010
					3E 0B C6 A8 		2012
					3E 0B C6 A8 		2014
					00 00 00 00 		2016
					BF 80 00 00 		2018
					BF 80 00 00 		201A
					BF 80 00 00 		201C
					00 00 00 00 		201E
					42 47 F5 C3 		2020
					63 F0				CRC

(3) Multiple register 4000 .. 401E
0B 03 40 00 00 20 51 78 0B 03 40 C4 EA DB 33 C4 EA DB 33 00 00 00 00 00 00 00 00 00 00 00 00 44 E5 D5 71 44 E5 D5 71 00 00 00 00 00 00 00 00 00 00 00 00 45 68 58 52 45 68 58 52 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 CE 89
Every 1-10 seconds the inverter sends a multiple register read from starting address 4000 for 32 (0020hex) positions = 16 registers
Inverter request 	0B 03 40 00 00 20 51 78
DDSU666H response	0B 03 40 
					C4 EA DB 33 C4 EA DB 33 00 00 00 00 00 00 00 00 
					00 00 00 00 44 E5 D5 71 44 E5 D5 71 00 00 00 00 
					00 00 00 00 00 00 00 00 45 68 58 52 45 68 58 52 
					00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
					CE 89 
					
			byte	value			
			00		0B	slave address
			01		03	function code 	0x03 = Read Multiple Holding Registers
			02		40  byte count = 40 hex (64 dec) bytes => 64 / 4 bytes = 16 registers 4000 .. 401E
					value (4 bytes)		register
			03  	C4 EA DB 33 		4000
					C4 EA DB 33 		4002
					00 00 00 00 		4004
					00 00 00 00 		4006
					00 00 00 00 		4008
					44 E5 D5 71 		400A
					44 E5 D5 71 		400C
					00 00 00 00 		400E
					00 00 00 00 		4010
					00 00 00 00 		4012
					45 68 58 52 		4014
					45 68 58 52 		4016
					00 00 00 00 		4018
					00 00 00 00 		401A
					00 00 00 00 		401C
					00 00 00 00 		401E
					CE 89 				CRC
					

---------------------------------------------------------------------------------------------------
**/

/**
---------------------------------------------------------------------------------------------------
		
								   UART

---------------------------------------------------------------------------------------------------
**/
static const int RX_BUF_SIZE = 128;
void DDSU666H_uart_init(void) 
{
	// DDSU666-H
	// Huawei inverters communicate in the two-line RS485 mode at baud rates of 4800 bps, 9600 
	// bps, or 19200 bps 
	// Data is transferred in asynchronous RTU mode. Each frame consists of one start bit, eight 
	// payload data bits, one CRC bit, and one stop bit (11 bits in total). 
	
	// UART 2
    uart_config_t uart_config_2 = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_2, RX_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_2, &uart_config_2));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2, GPIO_NUM_17, GPIO_NUM_16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
} // DDSU666H_uart_init()




/**
---------------------------------------------------------------------------------------------------
		
								   TASKS

---------------------------------------------------------------------------------------------------
**/
DDSU666H_data_type DDSU666H_data;
uint16_t  DDSU666H_reg_request;

int DDSU666H_rxdata_process( uint8_t* data, int rxBytes, int ix0)
{
	int ix= ix0;
	for(; ix<rxBytes && data[ix]!=DDSU666H_MODBUS_ADDRESS; ix++);
	if(ix<rxBytes)
	{
		if(rxBytes - ix >= 8)
		{
			// Function Code
			if(data[ix + 1]== 0x03)
			{
				uint16_t reg= ENDIAN(*(uint16_t*)(data + ix + 2));
				// is a request
				// sent by the INVENTER
				// do nothing
				if((reg>=0x2000 && reg<=0x20FF) || (reg>=0x4000 && reg<=0x400F))
				{
					DDSU666H_reg_request= reg;
					uint16_t ErrorCheck= data[ix + 7]<<8 | data[ix + 6];
					uint16_t crc= CRC16( &data[ix], 8 - 2);
					if(ErrorCheck == crc)
					{
						if(_VERBOSE_ && reg != 0x2006) fprintf(stdout, "\nRequest register= %04X bytes= %04X", reg, ENDIAN(*(uint16_t *)(data+ix+4)));
					}
					return ix + 8;
				}
				// is a response
				// sent by the DDSU666H in response to the INVERTER's request
				// then do record data into DDSU666H data structure
				else
				{
					uint8_t bytecount= *(data + ix + 2);
					if(ix + 3 + bytecount + 2 <=rxBytes)
					{
						uint16_t ErrorCheck= data[ix + 3 + bytecount + 1]<<8 | data[ix + 3 + bytecount];
						uint16_t crc= CRC16( &data[ix], 3 + bytecount);
						if(ErrorCheck == crc)
						{
							if(_VERBOSE_ &&  DDSU666H_reg_request != 0x2006) fprintf(stdout, "\nResponse= %d bytes %d %04X %04X %d", bytecount, bytecount/4, ErrorCheck, crc, rxBytes - (ix + 3 + bytecount + 2) );
							
							float value, v=0, a=0;
							for(int i=0; i<(bytecount/4) && i<32; i++)
							{
								value= record2float( ix + data + 3 + 4*i);
								if( DDSU666H_reg_request == 0x2000)
								{
									uint16_t reg= 0x2000 + 2 * i;
									switch(reg)
									{
										case DDSU666H_REG_VOLTAGE: 			DDSU666H_data.Voltage= v= value; break;
										case DDSU666H_REG_CURRENT: 			DDSU666H_data.Current= a= value; break;
										case DDSU666H_REG_ACTIVE_POWER: 	DDSU666H_data.ActivePower= value * 1000.0; break;
										case DDSU666H_REG_REACTIVE_POWER: 	DDSU666H_data.ReactivePower= value * 1000.0; break;
										case DDSU666H_REG_APPARENT_POWER: 	DDSU666H_data.ApparentPower= value * 1000.0; break;
										case DDSU666H_REG_POWER_FACTOR: 	DDSU666H_data.PowerFactor= value * 1000.0; break;
										case DDSU666H_REG_FRECUENCY: 		DDSU666H_data.Frecuency= value;	break;
									}
									if(_VERBOSE_) fprintf(stdout, "\n0x%4X= %3.2f", reg, value); 
								}
								else if( DDSU666H_reg_request == 0x4000) 
								{
									uint16_t reg= 0x4000 + 2 * i;
									switch(reg)
									{
										case DDSU666H_REG_ACTIVE_IN_ELECTRICITY: DDSU666H_data.ActiveInElectricity= value; break;
										case DDSU666H_REG_NEGATIVE_ACTIVE_ENERGY: DDSU666H_data.NegativeActiveEnergy= value; break;
										case DDSU666H_REG_POSITIVE_ACTIVE_ENERGY: DDSU666H_data.PositiveActiveEnergy= value; break;
									}
									if(_VERBOSE_) fprintf(stdout, "\n0x%4X= %3.2f", reg, value);
								}
							}
//							if(_VERBOSE_ &&  DDSU666H_reg_request != 0x2006) fprintf(stdout, "\nRegister %04X",  DDSU666H_reg_request);
							return (ix + 3 + bytecount + 2);
						}
					}				
				}
			}
		}
	}
	return 0;
} // DDSU666H_rxdata_process




void DDSU666H_RX_task(void *arg)
{
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE);
	
	// int64_t t0, t1;
	// t1= esp_timer_get_time();
    while (1) 
	{
		// t0=t1;
        int rxBytes = uart_read_bytes(UART_NUM_2, data, RX_BUF_SIZE, 200 / portTICK_PERIOD_MS);
		// t1= esp_timer_get_time();
        if (rxBytes > 0) 
		{
			// DUMP
			// fprintf(stdout,"\nDDSU666H %lld ms %d bytes\n",  (t1 - t0) / 1000, rxBytes);
			
			// for(int i=0; i<	rxBytes; i++)	
				// fprintf(stdout,"%02X ", data[i]);
			// fprintf(stdout,"\n");
			// fflush(stdout);
			
			int ix=0;
			do {
				ix= DDSU666H_rxdata_process( data, rxBytes, ix);
			} while( ix != 0 && ix <rxBytes);
        }
    }
    free(data);
} // DDSU666H_RX_task


void DDSU666H_create(UBaseType_t uxPriority)
{
	//DDSU666H_data_init();
	// Data init
	memset(&DDSU666H_data, 0, sizeof(struct DDSU666H_data_s));
	DDSU666H_reg_request= 0;
	// UART init
	DDSU666H_uart_init();
	// Task create
    xTaskCreate(DDSU666H_RX_task, "DSU666H_rx_task", 4*1024, NULL, uxPriority, NULL);
} // DDSU666H_create


// END OF FILE