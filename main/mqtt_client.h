#ifndef _MQTT_CLIENT_H_
#define _MQTT_CLIENT_H_


void MQTT_client_create( int (*callback) (int, void*), UBaseType_t uxPriority);
bool MQTT_is_connected(void);

#endif
// END OF FILE