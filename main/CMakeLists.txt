set(srcs 
	"main.c"
	"sdm120ct"
	"mqtt.c"
	"mqtt_client.c"
	"cstr.c"
	"network_wifi.c"
	"network_tcpclient.c"
	"network_webserver"
	"modbus.c"
	"DDSU666H.c"
	)


idf_component_register(SRCS ${srcs} INCLUDE_DIRS ".")

