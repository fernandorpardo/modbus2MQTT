#ifndef _NETWORKLIB_H_
#define _NETWORKLIB_H_

#define NETWORK_STATUS_WIFI_CONNECTED	1
#define NETWORK_STATUS_SOCKET_CLOSED	2

void network_init(int (*callback) (int));
void wifi_init_sta(void);


int net_tcp_connect(void);
int net_tcp_send(char *message, size_t n);
int net_tcp_receive(char *message, size_t message_sz);
void net_tcp_close(void);

#endif
// END OF FILE