#ifndef __RX_SOCKET_H_
#define __RX_SOCKET_H_

#define TCP_CLIENTS_MAX		16
#define RTLTCP_CLIENTS_MAX	16

typedef struct netclient_t {
	
	/* Socket for this viewer */
	int sock;
	
} netclient_t;

void *rx_tcp_socket(void* arg);
void *rx_rtltcp_socket(void* arg);

void Socket_close(netclient_t *client);

#endif