/** ************************************************************************************************
 *	modbus2MQTT  
 *  (c) Fernando R (iambobot.com)
 * 	1.0.0 - April 2025
 *  2.0.0 - December 2025
 *			Network modules: WiFi, TCP and MQTT clietn
 *			WiFi recovery
 *			Rest API
 * 
 *
 ** ************************************************************************************************
**/

/**
To generate the code 

. $HOME/esp/esp-idf/export.sh
cd ~/esp/modbus2MQTT

idf.py set-target esp32

idf.py menuconfig
	
idf.py build


Execute
idf.py -p /dev/ttyUSB0 flash

idf.py -p /dev/ttyUSB0 monitor

idf.py -p /dev/ttyUSB0 flash monitor

Device info
esptool.py --port /dev/ttyUSB0 flash_id


Monitor

press Ctrl+a to print SW info
To exit IDF monitor use the shortcut Ctrl+]
**/

/*
SUBSCRIBE TO THE MESSAGES FROM THIS DEVICE
mosquitto_sub -d -t 'modbus2mqtt/set'
*/

#include <stdio.h>
#include <string.h>			// memset
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "nvs_flash.h"

#include "config.h"
#include "cstr.h"
#include "modbus.h"
#include "mqtt.h"
#include "mqtt_client.h"
#include "network_wifi.h"
#include "network_tcpclient.h"
#include "network_webserver.h"
#include "sdm120ct.h"
#include "DDSU666H.h"

#define PROJECT_NAME		"modbus2MQTT"
#define PROJECT_LOCATION 	"esp/modbus2MQTT"
#define PROJECT_VERSION		"02.00.00"

static const char *TAG = "modbus2MQTT";

/** ***********************************************************************************************
	This SW
	
	Tasks				
	"TCP"				2
	"MQTT"				1
	"RestAPI"			3							RestAPI server
	"SDM120CT_rx_task"	configMAX_PRIORITIES-1
	"SDM120CT_tx_task"	configMAX_PRIORITIES-1
	"DSU666H_rx_task"	configMAX_PRIORITIES-1

	Priority - a lower numerical value indicates a lower priority, and a higher number indicates a higher priority
	UART tasks have the highest priotity
	The priority goes from 0 to ( configMAX_PRIORITIES - 1 ), where configMAX_PRIORITIES is defined within FreeRTOSConfig.h
	The idle task has priority zero (tskIDLE_PRIORITY).
	
	What this SW does
	SDM120CT_tx_task - wake up every REQUEST_EVERY_SEC (30 secs) to send one by one the queries in SDM120CT_data_query_list
	through UART_1
	SDM120CT_rx_task - when last response is received then function SDM120CT_querylist_done() is called to generate a MQTT Publish with the data
	from both SDM120CT and DDSU666-H
	Data in the MQTT publish is json format
	DSU666H_rx_task - is the DSU666H sniffer that read the message exchanged between the inverter and the DSU666H

*********************************************************************************************** **/
/**
---------------------------------------------------------------------------------------------------
		
								   UART

---------------------------------------------------------------------------------------------------

	ESP-WROOM-32 (30 pins)
	
				GPIO
	
	UART_NUM_1	SDM120CT
	TX			14
	RX			13
	
	UART_NUM_2	DDSU666-H
	TX			17		U2_TXD
	RX			16		U2_RXD
	
---------------------------------------------------------------------------------------------------
**/
/**
---------------------------------------------------------------------------------------------------
		
								   CONSOLE INFO

---------------------------------------------------------------------------------------------------
**/
void PROJECTInfo(void)
{
	fprintf(stdout, "\n\n");
	fprintf(stdout, "[%s] SW     --------------------------------\n", TAG);
	fprintf(stdout, "   %s\n", PROJECT_NAME);
	fprintf(stdout, "   %s\n", PROJECT_LOCATION);
	fprintf(stdout, "   %s\n", PROJECT_VERSION);
	fprintf(stdout, "   configMAX_PRIORITIES = %d\n", configMAX_PRIORITIES);
	
} // PROJECTInfo()

void SystemInfo(void)
{
	fprintf(stdout, "\n\n");
	fprintf(stdout, "[%s] SYSTEM --------------------------------\n", TAG);
	// Print chip information
	esp_chip_info_t chip_info;
	uint32_t flash_size;
	esp_chip_info(&chip_info);
	fprintf(stdout, "   This is %s chip with %d CPU core(s), %s%s%s%s\n",
		   CONFIG_IDF_TARGET,
		   chip_info.cores,
		   (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
		   (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
		   (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
		   (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

	unsigned major_rev = chip_info.revision / 100;
	unsigned minor_rev = chip_info.revision % 100;
	fprintf(stdout, "   silicon revision v%d.%d\n", major_rev, minor_rev);
	if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
		fprintf(stdout, "Get flash size failed\n");
		return;
	}

	fprintf(stdout, "   %" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
		   (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

	fprintf(stdout, "   Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());
	fflush(stdout);
} // SystemInfo()


void DDSU666H_printf(void)
{
	fprintf(stdout, "\nGrid (DDSU666H)");
	fprintf(stdout, "\nVoltage                %3.2f Volts", 		DDSU666H_data.Voltage);
	fprintf(stdout, "\nCurrent                %3.2f Amperes", 		DDSU666H_data.Current);
	fprintf(stdout, "\nActivePower            %3.2f Watts", 		DDSU666H_data.ActivePower);
	fprintf(stdout, "\nReactivePower          %3.2f VARs",			DDSU666H_data.ReactivePower);
	fprintf(stdout, "\nApparentPower          %3.2f Volt-Amperes",	DDSU666H_data.ApparentPower);
	fprintf(stdout, "\nPowerFactor            %3.2f", 	   			DDSU666H_data.PowerFactor);
	fprintf(stdout, "\nFrecuency              %3.2f Hz", 	   		DDSU666H_data.Frecuency);

	fprintf(stdout, "\nActiveInElectricity    %3.2f", 				DDSU666H_data.ActiveInElectricity);
	fprintf(stdout, "\nNegativeActiveEnergy   %3.2f kWh",			DDSU666H_data.NegativeActiveEnergy);
	fprintf(stdout, "\nPositiveActiveEnergy   %3.2f kWh", 			DDSU666H_data.PositiveActiveEnergy);
	fprintf(stdout, "\n");
} // DDSU666H_printf

void SDM120CT_printf(void)
{
	fprintf(stdout, "\nSolar (SDM120CT)");
	fprintf(stdout, "\nVoltage                %3.2f Volts", SDM120CT_data.Voltage);
	fprintf(stdout, "\nCurrent                %3.2f Amps",  SDM120CT_data.Current);
	fprintf(stdout, "\nActivePower            %3.2f Watts", SDM120CT_data.ActivePower);
	fprintf(stdout, "\nApparentPower          %3.2f Watts", SDM120CT_data.ApparentPower);
	fprintf(stdout, "\nReactivePower          %3.2f Var",   SDM120CT_data.ReactivePower);
	fprintf(stdout, "\nPowerFactor            %3.2f", 	    SDM120CT_data.PowerFactor*1000.0);
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

/**
---------------------------------------------------------------------------------------------------
		
								   Network Callbacks

---------------------------------------------------------------------------------------------------
**/
char IPaddr[32];
char MACaddr[32];

typedef struct Network_status_s
{
	int WiFi_lost;
	int TCP_lost;
} Network_status_type;

Network_status_type Network_status;

int WiFiCallback (int status, void *d)
{
	switch(status)
	{
		// ----------------------------------------------------------------
		// WIFI
		case NETWORK_STATUS_WIFI_CONNECTED:
			{	
				cstr_copy(IPaddr, (char*)d, sizeof(IPaddr));
				ESP_LOGI(TAG,"WIFI CONNECTED - IP address %s", IPaddr);
			}
			break;
		case NETWORK_STATUS_WIFI_DISCONNECTED:
			{		
				ESP_LOGE(TAG,"WIFI DISCONNECTED");
			}
			break;			
		case NETWORK_STATUS_WIFI_LOST:
			{				
				Network_status.WiFi_lost ++;
				ESP_LOGE(TAG,"WIFI LOST");
			}
			break;
		default:
			fprintf(stdout, "[WiFiCallback] ERROR DEFAULT\n");	
			fflush(stdout);	
	}
	return 0;
} // WiFiCallback()

int TCPCallback (int status, void *d)
{
	switch(status)
	{
		case NETWORK_STATUS_CLIENT_CONNECTED:
			{
				ESP_LOGI(TAG,"CLIENT CONNECTED");			
			}
			break;
		case NETWORK_STATUS_CLIENT_CONNECTION_CLOSED:
			{
				Network_status.TCP_lost ++;
				ESP_LOGE(TAG,"CLIENT CONNECTION CLOSED");
			}
			break;
		default:
			fprintf(stdout, "[TCPCallback] ERROR DEFAULT\n");	
			fflush(stdout);	
	}
	return 0;
} // TCPCallback()

/**
---------------------------------------------------------------------------------------------------
		
								   Rest API Responses Callback

---------------------------------------------------------------------------------------------------
**/
// requescomes in the pay load of the HTTP message and 
// is a json message with "type" and "key"
// {"type":"....","key":"..."}'
// type is
// - data_request
// - device_info
int RestAPICallback(char * type, char *response, size_t sz_response)
{
	if(strcmp(type, "data_request")==0)
	{
/* 		snprintf(response, sz_response, "{");
		int len= strlen(response);
		DDSU666H_generate_json(&response[len], sz_response-len);
		len= strlen(response);
		snprintf(&response[len], sz_response-len, ",");
		len= strlen(response);
		SDM120CT_generate_json(&response[len], sz_response-len);
		len= strlen(response);
		snprintf(&response[len], sz_response-len, "}");
		 */
		snprintf(response, sz_response, 
			"{"
			"\"DDSU666H\":{\"v\":\"%3.2f\",\"c\":\"%3.2f\",\"ap\":\"%3.2f\",\"rp\":\"%3.2f\"},"
			"\"SDM120CT\":{\"v\":\"%3.2f\",\"c\":\"%3.2f\",\"ap\":\"%3.2f\",\"rp\":\"%3.2f\"}"
			"}",
			DDSU666H_data.Voltage, DDSU666H_data.Current, DDSU666H_data.ActivePower, DDSU666H_data.ReactivePower,
			SDM120CT_data.Voltage, SDM120CT_data.Current, SDM120CT_data.ActivePower, SDM120CT_data.ReactivePower
		); 
	}
	else if(strcmp(type, "device_info")==0)
	{
		snprintf(response, sz_response, 
			"{\"TCP\":\"%s\",\"MQTT\":\"%s\",\"WIFIlost\":\"%d\",\"TCPlost\":\"%d\"}", 
			network_tcp_is_connected()?  "ok":"-",
			MQTT_is_connected()? "ok":"-",
			Network_status.WiFi_lost,
			Network_status.TCP_lost
			);
	}
	else
	{
		snprintf(response, sz_response, "{\"type\":\"%s\",\"result\":\"%s\"}", type, "error");
	}	
	return 0;
} // RestAPICallback



/**
---------------------------------------------------------------------------------------------------
		
								   Report Callback

---------------------------------------------------------------------------------------------------
**/
static char publish_mess[256];

void SDM120CT_publish(void)
{
	if(MQTT_is_connected())
	{
		
		snprintf(publish_mess, sizeof(publish_mess),
			"{"
			"\"SDM120CT\":{\"v\":\"%3.2f\",\"c\":\"%3.2f\",\"ap\":\"%3.2f\",\"rp\":\"%3.2f\"}"
			"}",
			SDM120CT_data.Voltage, SDM120CT_data.Current, SDM120CT_data.ActivePower, SDM120CT_data.ReactivePower
			);
		mqtt_publish(network_tcp_send, DEVICE_MQTT_NAME"/set", publish_mess);	
		fprintf(stdout,"mqtt_publish %d bytes\n", strlen(publish_mess));
	}
} // SDM120CT_publish

void DDSU666H_publish(void)
{
	if(MQTT_is_connected())
	{
		snprintf(publish_mess, sizeof(publish_mess),
			"{"
			"\"DDSU666H\":{\"v\":\"%3.2f\",\"c\":\"%3.2f\",\"ap\":\"%3.2f\",\"rp\":\"%3.2f\"}"
			"}",
			DDSU666H_data.Voltage, DDSU666H_data.Current, DDSU666H_data.ActivePower, DDSU666H_data.ReactivePower
			); 		
		mqtt_publish(network_tcp_send, DEVICE_MQTT_NAME"/set", publish_mess);	
		fprintf(stdout,"mqtt_publish %d bytes\n", strlen(publish_mess));
	}
} // DDSU666H_publish

void SDM120CT_callback (SDM120CT_sequence_phase_t SDM120CT_sequence_phase)
{
	printf("\n");
	if(SDM120CT_sequence_phase == INFO)
	{
		SDM120CT_info_printf();
	}
	else
	{
		SDM120CT_printf();
		DDSU666H_printf();
		
		SDM120CT_publish();
		DDSU666H_publish();
	}
} // SDM120CT_callback


/**
---------------------------------------------------------------------------------------------------
		
								   ON-BOARD LED

---------------------------------------------------------------------------------------------------
**/
#define GPIO_OUTPUT_PIN_SEL_1		(1ULL<<PIN_GPIO_ONBOARD_LED)

bool led_is_on;

void gpiolib_output_init(void)
{
    //zero-initialize the config structure
    gpio_config_t io_conf_out = {};
    //disable interrupt
    io_conf_out.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf_out.mode = GPIO_MODE_OUTPUT; // GPIO_MODE_OUTPUT / GPIO_MODE_OUTPUT_OD
    //bit mask of the pins that you want to set
    io_conf_out.pin_bit_mask = GPIO_OUTPUT_PIN_SEL_1;
    //disable pull-down mode
    io_conf_out.pull_down_en = 0;
    //disable pull-up mode
    io_conf_out.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf_out);	

	// Set OUTPUT
	led_is_on= false;
	gpio_set_level(PIN_GPIO_ONBOARD_LED, 0);	// OFF
} // gpiolib_output_init

/**
---------------------------------------------------------------------------------------------------
		
								   MAIN

---------------------------------------------------------------------------------------------------
**/
void app_main(void)
{
	ESP_LOGI(TAG, "app_main");
	
	// IP address
	IPaddr[0]='\0';
	// MAC address
    unsigned char mac_base[6] = {0};
    ESP_ERROR_CHECK(esp_read_mac(mac_base, ESP_MAC_WIFI_STA));
	snprintf(MACaddr, sizeof(MACaddr), "%02X:%02X:%02X:%02X:%02X:%02X", mac_base[0],mac_base[1],mac_base[2],mac_base[3],mac_base[4],mac_base[5]);
	
	// Network statistics
	memset(&Network_status,0, sizeof(Network_status));
	
	// Console print SW project info
	PROJECTInfo();
	// Console print System info
 	SystemInfo();
	fprintf(stdout, "   Free heap size: %"PRIu32"\n", esp_get_free_heap_size());
	
 	// --------------------------------------------------------------------------------------------
	// WIFI
	// Initialize NVS
	// NVS is needed for the WiFi library	
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);	
	// Connect WiFi
	network_wifi_init(WiFiCallback);
		
	// --------------------------------------------------------------------------------------------
	// TASK
	// MQTT Mosquitto client	
	MQTT_client_create(TCPCallback, 1);
	
	// --------------------------------------------------------------------------------------------
	// TASK
	// REST API SERVER
	network_server_create(RestAPICallback, 3);

	// --------------------------------------------------------------------------------------------
	// TASK
	// SDM120CT serial
	SDM120CT_create(SDM120CT_callback, configMAX_PRIORITIES-1);
	// DSU666H serial
	DDSU666H_create(configMAX_PRIORITIES-1);

	// --------------------------------------------------------------------------------------------
	// On-board blue LED
	gpiolib_output_init();

	while(1)
	{
		int c= getchar();
		if(c>0)
		{
			// Ctrl + a
			if(c==0x01)
			{
				PROJECTInfo();
				SystemInfo();
				SDM120CT_info_printf();	
				fflush(stdout);
			}
			// Ctrl + w
			if(c==0x17)
			{
				fprintf(stdout, "\nWiFi try to reconnect\n");
				fflush(stdout);
				network_wifi_connect();
			}			
			// Ctrl + b
			if(c==0x02)
			{
				led_is_on = !led_is_on;
				fprintf(stdout, "\nLed set %s\n", led_is_on?"ON":"OFF");
				fflush(stdout);
				gpio_set_level(PIN_GPIO_ONBOARD_LED, led_is_on? 1:0);
			}	
			// Ctrl + m
			if(c==0x0a)
			{
				SDM120CT_printf();
				DDSU666H_printf();
			}
			else {
				fprintf(stdout, "\ncommmand is %c (0x%x)\n", c, c);
				fflush(stdout);
			}
		}
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
} // app_main

// END OF FILE