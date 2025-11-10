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

#include "config.h"
#include "modbus.h"
#include "sdm120ct.h"

SDM120CT_sequence_phase_t SDM120CT_sequence_phase;
SDM120CT_device_info_type SDM120CT_device_info;
SDM120CT_data_type SDM120CT_data;

void SDM120CT_data_init(void)
{
	memset(&SDM120CT_data, 0, sizeof(struct SDM120CT_data_s));
	memset(&SDM120CT_device_info, 0, sizeof(struct SDM120CT_device_info_s));
}

void SDM120CT_printf(void)
{
	fprintf(stdout, "\nSolar (SDM120CT)");
	fprintf(stdout, "\nVoltage                %3.2f Volts", SDM120CT_data.Voltage);
	fprintf(stdout, "\nCurrent                %3.2f Amps",  SDM120CT_data.Current);
	fprintf(stdout, "\nActivePower            %3.2f Watts", SDM120CT_data.ActivePower);
	fprintf(stdout, "\nApparentPower          %3.2f Watts", SDM120CT_data.ApparentPower);
	fprintf(stdout, "\nReactivePower          %3.2f Var",   SDM120CT_data.ReactivePower);
	fprintf(stdout, "\nPowerFactor            %3.2f", 	   SDM120CT_data.PowerFactor*1000.0);
	fprintf(stdout, "\nFrecuency              %3.2f Hz",    SDM120CT_data.Frecuency);
	fprintf(stdout, "\n");
}

void SDM120CT_info_printf(void)
{
	fprintf(stdout, "\nMeterID           %3.2f", SDM120CT_device_info.MeterID);
	fprintf(stdout, "\nBaudrate          %3.2f", SDM120CT_device_info.Baudrate);
	fprintf(stdout, "\nSerialnumber      %08lX", SDM120CT_device_info.Serialnumber);
	fprintf(stdout, "\nMeterCODE         %04X",  SDM120CT_device_info.MeterCODE);
	fprintf(stdout, "\nSoftwareVersion   %04X",  SDM120CT_device_info.SoftwareVersion);	
	fprintf(stdout, "\n");	
}

char *SDM120CT_generate_json(char *json, size_t max_sz)
{
	snprintf(json, max_sz, "{\"SDM120CT\":{\"v\":\"%3.2f\",\"c\":\"%3.2f\",\"ap\":\"%3.2f\",\"rp\":\"%3.2f\"}}", 
	SDM120CT_data.Voltage, SDM120CT_data.Current, SDM120CT_data.ActivePower, SDM120CT_data.ReactivePower
	); 	
	return json;
} // SDM120CT_generate_json



void SDM120CT_rxdata_process (uint16_t query, uint8_t* data)
{
	if(SDM120CT_sequence_phase == INFO)
	{
		switch(query)
		{
			case SDM120CT_REG_METERID:	SDM120CT_device_info.MeterID= record2float(data+3); break;		
			case SDM120CT_REG_BAUDRATE: SDM120CT_device_info.Baudrate= record2float(data+3);break;
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
			case SDM120CT_REG_METERCODE: SDM120CT_device_info.MeterCODE= ENDIAN( *(uint16_t *)&(data[3]) ); break;
			// 2 bytes Hex
			//  01 03 04 00 00 00 00 FA 33
			case SDM120CT_REG_SOFTWAREVERSION: SDM120CT_device_info.SoftwareVersion= ENDIAN( *(uint16_t *)&(data[3]) ); break;
		}	
	}
	else
	{
		float value= record2float(data+3);
		if(_VERBOSE_) fprintf(stdout, "\nvalue %3.2f", value);	
		switch(query)
		{
			case SDM120CT_REG_VOLTAGE: 			SDM120CT_data.Voltage= value; break;
			case SDM120CT_REG_CURRENT: 			SDM120CT_data.Current= value; break;
			case SDM120CT_REG_ACTIVEPOWER: 		SDM120CT_data.ActivePower= value; break;
			case SDM120CT_REG_APPARENTPOWER: 	SDM120CT_data.ApparentPower= value; break;
			case SDM120CT_REG_REACTIVEPOWER: 	SDM120CT_data.ReactivePower= value; break;
			case SDM120CT_REG_POWERFACTOR: 		SDM120CT_data.PowerFactor= value; break;
			case SDM120CT_REG_FRECUENCY: 		SDM120CT_data.Frecuency= value; break;
		}
	}
} // SDM120CT_rxdata_process



// END OF FILE