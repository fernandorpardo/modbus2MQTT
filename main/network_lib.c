/**
 *
 *  (c) Fernando R (iambobot.com)
 * 
 *  version
 *	1.0 May 2025
 *
**/

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>            // struct addrinfo
#include <arpa/inet.h>
#include "esp_netif.h"

#include "config.h"
#include "network_lib.h"

static const char *TAG = "NETWORK";
static int sockfd;
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
//   NETWORK

static int (*NetworkCallback) (int)= 0;

void network_init(int (*callbak) (int))
{
	sockfd= -1;
	NetworkCallback= callbak;
} // network_init()


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
//   WIFI

/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define ESP_WIFI_SSID      					WIFI_SSID
#define ESP_WIFI_PASS      					WIFI_PASSWORD
#define ESP_WIFI_MAXIMUM_RETRY  			5

#define	CONFIG_ESP_WIFI_PW_ID 		""

#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD 	WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SAE_MODE 					WPA3_SAE_PWE_BOTH
#define ESP_WIFI_H2E_IDENTIFIER 			CONFIG_ESP_WIFI_PW_ID

/* #if CONFIG_ESP_WPA3_SAE_PWE_HUNT_AND_PECK
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
#define EXAMPLE_H2E_IDENTIFIER ""
#elif CONFIG_ESP_WPA3_SAE_PWE_HASH_TO_ELEMENT
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HASH_TO_ELEMENT
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#elif CONFIG_ESP_WPA3_SAE_PWE_BOTH
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#endif
 */
/* 
#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif
 */
/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1


static int s_retry_num = 0;


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
//  WiFi

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
	printf("\n[WiFi event_handler] event_base= %s ", event_base);
	if(event_base == WIFI_EVENT) printf("WIFI_EVENT"); 
	else if(event_base == IP_EVENT) printf("IP_EVENT"); 
	else  printf("other EVENT"); 
	printf("\n");
	
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } 
	else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) 
	{
        if (s_retry_num < ESP_WIFI_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } 
	
	// Connected
	else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
	{
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
} // event_handler()

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
	
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, 	 &event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,	IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = ESP_WIFI_SSID,
            .password = ESP_WIFI_PASS,
             // * Authmode threshold resets to WPA2 as default if password matches WPA2 standards (password len => 8).
             // * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             // * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
             // * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
            .sae_h2e_identifier = ESP_WIFI_H2E_IDENTIFIER,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    // * Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
    // * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) 
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    // * xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
   //  * happened. 
    if (bits & WIFI_CONNECTED_BIT) 
	{
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", ESP_WIFI_SSID, ESP_WIFI_PASS);
		if(NetworkCallback) NetworkCallback (NETWORK_STATUS_WIFI_CONNECTED);
    } 
	else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", ESP_WIFI_SSID, ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
} // wifi_init_sta()




// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
//  TCP

// https://github.com/espressif/esp-idf/blob/master/examples/protocols/sockets/tcp_client/main/tcp_client_v4.c




char host_ip[] = MQTT_HOST_IP_ADDR;

int net_tcp_connect(void)
{
	sockfd =  socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if (sockfd < 0) {
		ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
		return -1;
	}
	ESP_LOGI(TAG, "Socket created, connecting to %s:%d", host_ip, MQTT_HOST_IP_PORT);
	// Set time-out before connect
	// RCV time-out milisec
	struct timeval timeout;      
	timeout.tv_sec = 0; 
	timeout.tv_usec = 500000;
	if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
	{
		fprintf(stdout, "\n[net_tcp_connect] setsockopt() ERROR "); 
		perror("setsockopt() failed\n");
		fflush(stdout);
		close(sockfd);
		sockfd= -1;
		return -1;	
	}	
	// SND time-out seconds
	timeout.tv_sec = 1; 
	timeout.tv_usec = 0;
	if (setsockopt (sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
	{
		fprintf(stdout, "\n[net_tcp_connect] setsockopt() ERROR "); 
		perror("setsockopt() failed\n");
		fflush(stdout);
		close(sockfd);
		sockfd= -1;
		return -1;	
	}
	// connect
	struct sockaddr_in dest_addr;
	inet_pton(AF_INET, host_ip, &dest_addr.sin_addr);
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(MQTT_HOST_IP_PORT);
	int err = connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
	if (err != 0) {
		ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
		close(sockfd);
		sockfd= -1;
		return -1;
	}
	ESP_LOGI(TAG, "Successfully connected");
	return 0;
} // net_tcp_connect()

void net_tcp_close(void)
{
	if (sockfd != -1) {
		ESP_LOGE(TAG, "Shutting down socket and restarting...");
		shutdown(sockfd, 0);
		close(sockfd);
		sockfd= -1;
		// network_tcp_connected= false;
		// network_mqtt_connected= false;
		// lcdPrint(&lcd_dev, f16x, "Socket closed\n");
		if(NetworkCallback) NetworkCallback (NETWORK_STATUS_SOCKET_CLOSED);
	}	
} // net_tcp_close()


int net_tcp_send(char *message, size_t len)
{
	fprintf(stdout,"net_tcp_send %d ....", len);
	fflush(stdout);
	
	// ssize_t send(int sockfd, const void buf[.len], size_t len, int flags);
	ssize_t err = send(sockfd, message, len, 0);
	
	fprintf(stdout," DONE %s %d \n", (len==err) ? "OK len=" : "*** ERROR *** err=", err);
	fflush(stdout);	
	
	if (err < 0) 
	{
		ESP_LOGE(TAG, "send failed: errno %d", errno);
		perror("send() failed\n");
		if(errno == 128)
		{
			printf("[net_tcp_send] TCP connection lost !!!\n"); 
			fflush(stdout);
			net_tcp_close();
		}
		return -1;
	}
	return 0;
} // net_tcp_send()


int net_tcp_receive(char *message, size_t max_sz)
{
	int n = recv(sockfd, message, max_sz, 0);
	// Error occurred during receiving
	if (n <= 0) {
		if(n == 0) 
		{
			ESP_LOGE(TAG, "[net_tcp_receive] DISCONNECTED\n");	
			net_tcp_close();
		}
		else if(errno == EAGAIN) // Resource temporarily unavailable		
		{
			// Not to worry, it is the time -out
			//fprintf(stdout,"+"); fflush(stdout);					
		}
		else if(errno == 128)
		{
			ESP_LOGE(TAG, "[net_tcp_receive] TCP connection lost !!!\n"); 
			net_tcp_close();
		}
		else
			ESP_LOGE(TAG, "[net_tcp_receive] recv failed: errno %d", errno);
		
		return -1;
	}
	// Data received	
	ESP_LOGI(TAG, "[net_tcp_receive] Received %d bytes from %s:", n, host_ip);
	
	return n;
} // net_tcp_receive


// END OF FILE