/** ************************************************************************************************
 *	SDM120CT  
 *  (c) Fernando R (iambobot.com)
 * 	1.0.0 - April 2025
 *
 ** ************************************************************************************************
**/
/** ***********************************************************************************************
MODBUS Protocol 

MODBUS© Protocol is a messaging structure, widely used to establish master-slave communication 
between intelligent devices. A MODBUS message sent from a master to a slave contains the address 
of the slave, the 'command' (e.g. 'read register' or 'write register'), the data, 
and a check sum (LRC or CRC).


Eastron SDM120Modbus Smart Meter Modbus Protocol Implementation V2.4 
https://www.eastroneurope.com/images/uploads/products/protocol/SDM120-MODBUS_Protocol.pdf


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


*********************************************************************************************** **/
/** ***********************************************************************************************
Active Power (Real Power):
This is the power that's actually used by a device to do work. It's the power that results in a change 
in energy, like the energy used to turn a motor shaft or heat a resistor.

Reactive Power:
This type of power is not used for doing work but is necessary for the operation of certain electrical 
components like inductors and capacitors. It oscillates back and forth between the source and the load, 
storing energy and releasing it, but not actually consuming it. 

Apparent Power:
This is the total power in an AC circuit, including both active and reactive power. It's calculated by 
multiplying the voltage and current. 

Power Factor:
The power factor represents the ratio of active power to apparent power. It indicates how effectively 
the power is being used. A power factor of 1 means all the power is being used for work, while a lower 
power factor indicates that some of the power is reactive. 

*********************************************************************************************** **/

#include <stdio.h>		// fprintf 
#include <string.h>		// memcpy
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"		// esp_timer_get_time()

#include "config.h"
#include "modbus.h"
#include "sdm120ct.h"

static void (*SDM120CT_callback) (SDM120CT_sequence_phase_t SDM120CT_sequence_phase)= 0;

SDM120CT_sequence_phase_t SDM120CT_sequence_phase;
SDM120CT_device_info_type SDM120CT_device_info;
SDM120CT_data_type SDM120CT_data;

/**
---------------------------------------------------------------------------------------------------
		
								   UART

---------------------------------------------------------------------------------------------------
**/
static const int RX_BUF_SIZE = 128;
// SDM120CT
void SDM120CT_uart_init(void) 
{
	// UART 1
	uart_config_t uart_config_1 = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_1, &uart_config_1));
	// esp_err_t uart_set_pin(uart_port_t uart_num, int tx_io_num, int rx_io_num, int rts_io_num, int cts_io_num)
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, GPIO_NUM_14, GPIO_NUM_13, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
} // SDM120CT_uart_init()

/**
---------------------------------------------------------------------------------------------------
		
								   

---------------------------------------------------------------------------------------------------
**/
void SDM120CT_rxdata_process (uint16_t query, uint8_t* data)
{
	// device info
	if(SDM120CT_sequence_phase == INFO)
	{
		switch(query)
		{
			case SDM120CT_REG_METERID:	
				SDM120CT_device_info.MeterID= record2float(data+3); 
				break;		
			case SDM120CT_REG_BAUDRATE: 
				SDM120CT_device_info.Baudrate= record2float(data+3);
				break;
			// 4 bytes / unsigned int32
			// 01 03 04 01 CC 57 A6 85 BA
			case SDM120CT_REG_SERIALNUMBER: 
			{
				float value= record2float(data+3);
				SDM120CT_device_info.Serialnumber= *(uint32_t*) &value;
				break;
			}
			// 2 bytes Hex
			// 01 03 04 00 21 00 00 AA 39 
			case SDM120CT_REG_METERCODE: 
				SDM120CT_device_info.MeterCODE= ENDIAN( *(uint16_t *)&(data[3]) ); 
				break;
			// 2 bytes Hex
			//  01 03 04 00 00 00 00 FA 33
			case SDM120CT_REG_SOFTWAREVERSION: 
				SDM120CT_device_info.SoftwareVersion= ENDIAN( *(uint16_t *)&(data[3]) ); 
				break;
		}	
	}
	// Measurements
	else
	{
		float value= record2float(data+3);
		if(_VERBOSE_) fprintf(stdout, "\nvalue %3.2f", value);	
		switch(query)
		{
			case SDM120CT_REG_VOLTAGE: 			SDM120CT_data.Voltage= value; 		break;
			case SDM120CT_REG_CURRENT: 			SDM120CT_data.Current= value; 		break;
			case SDM120CT_REG_ACTIVEPOWER: 		SDM120CT_data.ActivePower= value; 	break;
			case SDM120CT_REG_APPARENTPOWER: 	SDM120CT_data.ApparentPower= value; break;
			case SDM120CT_REG_REACTIVEPOWER: 	SDM120CT_data.ReactivePower= value; break;
			case SDM120CT_REG_POWERFACTOR: 		SDM120CT_data.PowerFactor= value; 	break;
			case SDM120CT_REG_FRECUENCY: 		SDM120CT_data.Frecuency= value; 	break;
		}
	}
} // SDM120CT_rxdata_process

/**
---------------------------------------------------------------------------------------------------
		
								   TASKS

---------------------------------------------------------------------------------------------------
**/
int64_t t0;
int SDM120CT_query_index;
uint16_t *SDM120CT_query_list;
const uint16_t SDM120CT_data_query_list[]= {SDM120CT_REG_VOLTAGE, SDM120CT_REG_CURRENT, SDM120CT_REG_ACTIVEPOWER, SDM120CT_REG_APPARENTPOWER, SDM120CT_REG_REACTIVEPOWER, SDM120CT_REG_POWERFACTOR, SDM120CT_REG_FRECUENCY, 0xFFFF};
const uint16_t SDM120CT_deviceinfo_query_list[]= {SDM120CT_REG_METERID, SDM120CT_REG_BAUDRATE, SDM120CT_REG_SERIALNUMBER, SDM120CT_REG_METERCODE, SDM120CT_REG_SOFTWAREVERSION, 0xFFFF};
// int SDM120CT_send_query();

int SDM120CT_send_query()
{
	if(SDM120CT_query_index<0 || SDM120CT_query_list[SDM120CT_query_index] == 0xFFFF)
	{
		if(SDM120CT_query_list[SDM120CT_query_index] == 0xFFFF) 
			//SDM120CT_querylist_done();
			SDM120CT_callback(SDM120CT_sequence_phase);
		SDM120CT_query_index= -1;
		int r= (SDM120CT_sequence_phase == INFO) ? 1 : 0;
		SDM120CT_sequence_phase= DATA;
		SDM120CT_query_list= ( uint16_t *) &SDM120CT_data_query_list;
		return r;
	}
	if(SDM120CT_sequence_phase == INFO)
	{
		modbus_holding_parameter_query_type query = MODBUS_HOLDING_PARAMETER_DEFAULT(SDM120CT_deviceinfo_query_list[SDM120CT_query_index]);
		query.Error_Check= CRC16( (uint8_t *) &query, sizeof(query) - 2 );
		char *q= (char*)&query;
		size_t sz= sizeof(query);
		t0= esp_timer_get_time();
		int txBytes= uart_write_bytes(UART_NUM_1, q, sz);
		if(txBytes != (int) sz) printf("\n[ERROR] SDM120CT_send_queryTx uart_write_bytes %2d bytes", txBytes);	
	}
	else
	{
		modbus_master_query_type query = MODBUS_QUERY_DEFAULT(SDM120CT_data_query_list[SDM120CT_query_index]);
		query.Error_Check= CRC16( (uint8_t *) &query, sizeof(query) - 2 );
		char *q= (char*)&query;
		size_t sz= sizeof(query);
		t0= esp_timer_get_time();
		int txBytes= uart_write_bytes(UART_NUM_1, q, sz);
		if(txBytes != (int) sz) printf("\n[ERROR] SDM120CT_send_queryTx uart_write_bytes %2d bytes", txBytes);
	}
	return 0;	
} // SDM120CT_send_query


#define LOOP_DELAY_MS				1000
#define SEC2LOOPS(x)				(x * 1000/LOOP_DELAY_MS)
#define DATA_REFRESH_TIME			SEC2LOOPS(SDM120CT_DATA_REFRESH_SEC)	// Send the query list every x seconds


void SDM120CT_TX_task(void *arg)
{
	int request_time_sec= 0;
	while (1) 
	{
		if( -- request_time_sec <= 0 )
		{
			request_time_sec= DATA_REFRESH_TIME;
			SDM120CT_query_index= 0;
			SDM120CT_send_query();
		}
		// vTaskDelay() - Delays a task for a given number of ticks
		// portTICK_PERIOD_MS - one tick period in milliseconds. The unit of portTICK_PERIOD_MS is milliseconds/ticks
		vTaskDelay(1000 / portTICK_PERIOD_MS);	 // 1.000 ms = 1 sec 		
	}
} // SDM120CT_TX_task

void SDM120CT_RX_task(void *arg)
{
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE);
    while (1) {
        int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 200 / portTICK_PERIOD_MS);
        if (rxBytes > 0) 
		{		
			uint8_t Byte_Count= rxBytes>2? data[2] : 0;
			if(Byte_Count+2 == rxBytes-3)
			{
				int64_t t1= esp_timer_get_time();
				uint16_t ErrorCheck= data[8]<<8 | data[7];
				uint16_t crc= CRC16( data, rxBytes - 2);
				if(crc == ErrorCheck)
				{
					// SDM120CT_query_index == -1 means it is not a response to my request but traffic sniffed 
					if(SDM120CT_query_index >= 0)
					{
						SDM120CT_rxdata_process(SDM120CT_query_list[SDM120CT_query_index], data);
						if(_VERBOSE_) printf(" (elapsed %lld ms)", (t1 - t0) / 1000);	
						SDM120CT_query_index ++;
						if(SDM120CT_send_query() == 1) 
						{
							SDM120CT_query_index= 0; 
							SDM120CT_send_query();
						}
					}
				}
				else
					printf("\nERROR: CRC16 0x%4X  ErrorCheck 0x%4X", crc, ErrorCheck);
			}
			else
				printf("\nERROR: Byte_Count %d bytes", Byte_Count);
			fflush(stdout);
        }
    }
    free(data);
} // SDM120CT_RX_task

void SDM120CT_create(void (*callback) (SDM120CT_sequence_phase_t), UBaseType_t uxPriority)
{
	// Init data
	memset(&SDM120CT_data, 0, sizeof(struct SDM120CT_data_s));
	memset(&SDM120CT_device_info, 0, sizeof(struct SDM120CT_device_info_s));

	SDM120CT_callback= callback;
	
  	SDM120CT_query_index= -1,
	SDM120CT_sequence_phase= INFO;
 	SDM120CT_query_list= ( uint16_t *) &SDM120CT_deviceinfo_query_list;	

	// Init serial
	SDM120CT_uart_init();
	// TX/RX over serial
	xTaskCreate(SDM120CT_RX_task, "SDM120CT_rx_task", 4*1024, NULL, uxPriority, NULL);
	xTaskCreate(SDM120CT_TX_task, "SDM120CT_tx_task", 4*1024, NULL, uxPriority, NULL);
} // SDM120CT_create

// END OF FILE