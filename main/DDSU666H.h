#ifndef _DDSU666H_H_
#define _DDSU666H_H_

#define	DDSU666H_MODBUS_ADDRESS				0x0B		// 0x0B == default factory address

// Function Codes
#define DDSU666H_FC_READREGISTER			0x03
#define DDSU666H_FC_WRITESINGLEREGISTER		0x06
#define DDSU666H_FC_WRITEMULTIPLEREGISTERS	0x10
#define DDSU666H_FC_READDEVICEIDENTIFIER	0x2B


// DDSU666-H register map
// source: reverse engineering
// Register offset is 2 bytes although values are 4 bytes (float) in the response
#define DDSU666H_REG_VOLTAGE				0x2000		// Volts (4-bytes floating decimal)
#define DDSU666H_REG_CURRENT				0x2002		// Amps (4-bytes floating decimal)
//											0x2004
#define DDSU666H_REG_ACTIVE_POWER			0x2006		// kW (4-bytes floating decimal)
//		DDSU666H_REG_ACTIVE_POWER			0x2008
//											0x200A
#define DDSU666H_REG_REACTIVE_POWER			0x200C		// Kvar (4-bytes floating decimal)
//		DDSU666H_REG_REACTIVE_POWER			0x200E
//											0x2010
#define DDSU666H_REG_APPARENT_POWER			0x2012		// kW (4-bytes floating decimal)
//		DDSU666H_REG_APPARENT_POWER			0x2014
//											0x2016
#define DDSU666H_REG_POWER_FACTOR			0x2018		// (4-bytes floating decimal)
//		DDSU666H_REG_POWER_FACTOR			0x201A
//		DDSU666H_REG_POWER_FACTOR			0x201C
//											0x201E
#define DDSU666H_REG_FRECUENCY				0x2020		// Hz (4-bytes floating decimal)


// 0x4000
#define DDSU666H_REG_ACTIVE_IN_ELECTRICITY	0x4000		// 
//		DDSU666H_REG_ACTIVE_IN_ELECTRICITY	0x4002
//		0x0000								0x4004
//		0x0000								0x4006
//		0x0000						    	0x4008
#define DDSU666H_REG_NEGATIVE_ACTIVE_ENERGY	0x400A		// Negative active energy
//		DDSU666H_REG_NEGATIVE_ACTIVE_ENERGY	0x400C
//		0x0000								0x400E
//		0x0000								0x4010
//		0x0000						    	0x4012
#define DDSU666H_REG_POSITIVE_ACTIVE_ENERGY	0x4014		// Positive active energy
//		DDSU666H_REG_POSITIVE_ACTIVE_ENERGY	0x4016	

typedef struct DDSU666H_data_s
{
	float Voltage;
	float Current;
	float ActivePower;
	float ReactivePower;
	float PowerFactor;
	// 0x4000
	float ActiveInElectricity;
	float NegativeActiveEnergy;
	float PositiveActiveEnergy;	
} DDSU666H_data_type;

// extern DDSU666H_data_type DDSU666H_data;


void DDSU666H_data_init(void);
void DDSU666H_printf(void);
char *DDSU666H_generate_json(char *json, size_t max_sz);
int DDSU666H_rxdata_process(uint8_t* data, int rxBytes, int ix0);


#endif
// END OF FILE