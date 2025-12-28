#ifndef _PROJECT_CONFIG_H_
#define _PROJECT_CONFIG_H_

#define	PIN_GPIO_ONBOARD_LED	2	// Normally blue led is GPIO 2 on ESP32-WROOM (30 pins)

// DEBUGGING OPTIONS
#define	_VERBOSE_				0
#define	_SELF_SUBSCRIBE_		0	// if _SELF_SUBSCRIBE_ then the device sends the subscribe request to retrieve their own published messages (meant for testing pourposes)

// WiFi
#define	WIFI_SSID	    		"YOUR_SSID"
#define	WIFI_PASSWORD			"THE_PASSWORD_OF_YOUR_SSID"

// 	MQTT Server (Mosquitto)		
#define MQTT_HOST_IP_ADDR 		"192.168.1.103"
#define MQTT_HOST_IP_PORT 		1883


// Endianness (little-endian, big-endian) 
#define ENDIANNESS LITTLE_ENDIAN

#if ENDIANNESS == LITTLE_ENDIAN
#define ENDIAN(a) ( (((uint16_t)a<<8) & 0xFF00) | (((uint16_t)a>>8) & 0x00FF) )
#else
#define ENDIAN(a) (a)	
#endif

// MQTT PINGREQ period (seconds)
#define	MQTT_PINGREQ_TIME		60

// THIS DEVICE MQTT ID  
#define DEVICE_MQTT_NAME		"modbus2mqtt"

#endif
// END OF FILE