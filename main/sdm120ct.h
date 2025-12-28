#ifndef _SDM120CT_H_
#define _SDM120CT_H_

#define SDM120CT_DATA_REFRESH_SEC	30	// Send the query list every x seconds

/*
samples

 1  4  0  0  0  2 71 CB  
 1  4  4 43 62  0  0 4F DE  
 1  4  0  6  0  2 91 CA  
 1  4  4  0  0  0  0 FB 84  
 1  4  0  C  0  2 B1 C8  
 1  4  4  0  0  0  0 FB 84  
 1  4  0 12  0  2 D1 CE  
 1  4  4  0  0  0  0 FB 84  
 1  4  0 18  0  2 F1 CC  
 1  4  4 80  0  0  0 D2 44  
 1  4  0 1E  0  2 11 CD  
 1  4  4 3F 80  0  0 F6 78  
 1  4  0 24  0  2 31 C0  
 1  4  4  0  0  0  0 FB 84  
 
 1  4  0 46  0  2 90 1E  
 1  4  4 42 48  0  0 6F EA  
 1  4  0 48  0  2 F1 DD  
 1  4  4  0  0  0  0 FB 84  
 1  4  0 4A  0  2 50 1D  
 1  4  4  0  0  0  0 FB 84  
 1  4  1 56  0  2 90 27  
 1  4  4  0  0  0  0 FB 84  
 1  4  0 4C  0  2 B0 1C  
 1  4  4  0  0  0  0 FB 84  
 1  4  0 4E  0  2 11 DC  
 1  4  4  0  0  0  0 FB 84  
 1  4  1 58  0  2 F1 E4  
 1  4  4  0  0  0  0 FB 84
*/


// START ADDRESS
// Register Map
// Source: Eastron SDM120Modbus Smart Meter Modbus Protocol Implementation V2.4 (https://www.eastroneurope.com/images/uploads/products/protocol/SDM120-MODBUS_Protocol.pdf)
// Function code 04 to read input parameters
#define SDM120CT_REG_VOLTAGE						0x0000		// Volts
#define SDM120CT_REG_CURRENT						0x0006		// Amps
#define SDM120CT_REG_ACTIVEPOWER					0x000C		// Watts
#define SDM120CT_REG_APPARENTPOWER					0x0012		// VA
#define SDM120CT_REG_REACTIVEPOWER					0x0018		// VAr
#define SDM120CT_REG_POWERFACTOR					0x001E		// None
#define SDM120CT_REG_FRECUENCY						0x0046		// Hz
#define SDM120CT_REG_IMPOERACTIVEENERGY				0x0048		// kWh
#define SDM120CT_REG_EXPORTACTIVEENERGY				0x004A		// kWh
#define SDM120CT_REG_IMPOERREACTIVEENERGY			0x004C		// kvarh
#define SDM120CT_REG_EXPORTREACTIVEENERGY			0x004E		// kvarh
#define SDM120CT_REG_TOTALSYSTEMPOWERDEMAND			0x0054		// W
#define SDM120CT_REG_MAXIMUMTOTALSYSTEMPOWERDEMAND	0x0056		// W
#define SDM120CT_REG_IMPORTSYSTEMPOWERDEMAND 		0x0058		// W
#define SDM120CT_REG_MAXIMUMIMPORTSYSTEMPOWERDEMAND 0x005A		// W
#define SDM120CT_REG_EXPORTSYSTEMPOWERDEMAND		0x005C		// W
#define SDM120CT_REG_MAXIMUMEXPORTSYSTEMPOWERDEMAND 0x005E		// W
#define SDM120CT_REG_CURRENTDEMAND					0x0102		// Amps
#define SDM120CT_REG_MAXIMUMCURRENTDEMAND			0x0108		// Amps
#define SDM120CT_REG_TOTALACTIVEENERGY				0x0156		// kWh
#define SDM120CT_REG_TOTALREACTIVEENERGY			0x0158		// Kvarh

// Function code 10 to set holding parameter ，function code 03 to read holding parameter 
// MODBUS Protocol code 03 reads the contents of the 4X registers. 
#define SDM120CT_REG_READYPULSEWIDTH				0x000C		// Write relay on period in milliseconds: 60, 100 or 200, default 100ms. ( 4 bytes float ) 
#define SDM120CT_REG_NETWORKPARITYSTOP				0x0012		// Write the network port parity/stop bits for MODBUS Protocol
#define SDM120CT_REG_METERID						0x0014		// Ranges from 1 to 247, Default ID is 1. ( 4 bytes float ) 
#define SDM120CT_REG_BAUDRATE						0x001C		// Write the network port baud rate for MODBUS Protocol ( 4 bytes float ) 
#define SDM120CT_REG_PULSE1OUTPUTMODE				0x0056		// Write MODBUS Protocol input parameter for pulse out 1 ( 4 bytes float )    

#define SDM120CT_REG_TIMEOFSCROLLDISPLAY 			0xF900		// Range：0-30s Default 0:does not display in turns (2 bytes Hex)
#define SDM120CT_REG_PULSE1OUTPUT					0xF910		// 0000:0.001kWh/imp(default) / 0001:0.01kWh/imp / 0002:0.1kWh/imp / 0003:1kWh/imp (2 bytes Hex)
#define SDM120CT_REG_MEASUREMENT_MODE 				0XF920		// 0001:mode 1(total = import) / 0002:mode 2 (total = import + export）(default) / 0003:mode 3 (total = import - export) (2 bytes Hex)

#define SDM120CT_REG_SERIALNUMBER					0xFC00		// Serial number ( 4 bytes unsigned int32 )
#define SDM120CT_REG_METERCODE						0xFC02		// Meter code = 00 20 ( 2 bytes Hex )
#define SDM120CT_REG_SOFTWAREVERSION				0xFC03		// Software version ( 2 bytes Hex ) 


#define MODBUS_QUERY_DEFAULT(address) 		\
    {                         				\
        .Slave_Adress =     1, 				\
        .Function_code =    4,  			\
        .Start_Address =    ENDIAN(address),\
		.Number_of_Points = ENDIAN(0x0002),	\
        .Error_Check =      0 				\
     }
	 
typedef struct modbus_master_query_s
{ 
	uint8_t Slave_Adress;			
	uint8_t Function_code;	
	uint16_t Start_Address;
	uint16_t Number_of_Points;
	uint16_t Error_Check;
} modbus_master_query_type;

#define MODBUS_HOLDING_PARAMETER_DEFAULT(address) 		\
    {                         				\
        .Slave_Adress =     1, 				\
        .Function_code =    3,  			\
        .Start_Address =    ENDIAN(address),\
		.Number_of_Points = ENDIAN(0x0002),	\
        .Error_Check =      0 				\
     }

typedef struct modbus_holding_parameter_query_s
{ 
	uint8_t Slave_Adress;			
	uint8_t Function_code;
	uint16_t Start_Address;
	uint16_t Number_of_Points;
	uint16_t Error_Check;
} modbus_holding_parameter_query_type;

// ------------------------------------------------------------------------------------------------
// Exposed interface
typedef enum  {INFO, DATA} SDM120CT_sequence_phase_t;

typedef struct SDM120CT_device_info_s
{
	float MeterID;
	float Baudrate;
	uint32_t Serialnumber;
	uint16_t MeterCODE;
	uint16_t SoftwareVersion;
} SDM120CT_device_info_type;

typedef struct SDM120CT_data_s
{
	float Voltage;
	float Current;
	float ApparentPower;
	float ActivePower;
	float ReactivePower;
	float PowerFactor;
	float Frecuency;
} SDM120CT_data_type;

extern SDM120CT_device_info_type SDM120CT_device_info;
extern SDM120CT_data_type SDM120CT_data;

void SDM120CT_create(void (*callback) (SDM120CT_sequence_phase_t SDM120CT_sequence_phase), UBaseType_t uxPriority);

#endif
// END OF FILE