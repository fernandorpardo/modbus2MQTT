set(srcs 
	"main.c"
	"sdm120ct"
	"mqtt.c"
	"cstr.c"
	"network_lib.c"
	"modbus.c"
	"DDSU666H.c"
	)


idf_component_register(SRCS ${srcs} INCLUDE_DIRS ".")

