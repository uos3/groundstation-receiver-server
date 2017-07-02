#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <signal.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "main.h"
#include "socket.h"

extern netclient_t tcp_clients[TCP_CLIENTS_MAX];
extern netclient_t rtltcp_clients[RTLTCP_CLIENTS_MAX];

int Socket_init(int port)
{
  int parentsock; /* parent socket */
  struct sockaddr_in serveraddr; /* server's addr */
  
  /* Prepare the network - ignore SIGPIPE on viewer disconnection */
  signal(SIGPIPE, SIG_IGN);

  /* Open TCP Socket */
  parentsock = socket(AF_INET, SOCK_STREAM, 0);
  if (parentsock < 0)
  {
    fprintf(stderr,"ERROR opening socket");
  }

  /* Add REUSEADDR Flag (TODO: Check if needed) */
  int optval = 1;
  setsockopt(parentsock, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

  /* Set up server address */
  memset(&serveraddr, 0, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons(port);

  /* Bind Socket to server address */
  if (bind(parentsock, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
  { 
    fprintf(stderr, "Output socket failed to bind\n");
  }

  /* Listen, and allow 5 requests in queue */
  if (listen(parentsock, 5) < 0)
  {
    fprintf(stderr, "Output socket failed to listen");
  }

  return parentsock;
}

int Socket_wait(int socket)
{
  /* Block here until we receive an incoming connection */
  int childsock; /* child socket */
  struct sockaddr_in clientaddr; /* client addr */
  socklen_t clientlen; /* byte size of client's address */
  char clientip[INET_ADDRSTRLEN]; /* dotted decimal host addr string */


  childsock = accept(socket, (struct sockaddr *) &clientaddr, &clientlen);
  if (childsock < 0)
  {
    fprintf(stderr, "Output socket failed to accept connection");
  }
  
  /* Process IP address of connection client */
  clientip[0] = '\0';
  inet_ntop(AF_INET, &clientaddr.sin_addr, clientip, INET_ADDRSTRLEN);
  printf("New output connection from %s\n", clientip);


  /* Attempt to set TCP_NODELAY on output socket */
  int i = 1;
  int n = setsockopt(childsock, IPPROTO_TCP, TCP_NODELAY, &i, sizeof(int));
  if(n < 0)
  {
    fprintf(stderr, "%d: Error setting TCP_NODELAY on output socket\n", childsock);
    perror("setsockopt");
    /* This is not a fatal error */
  }

  return childsock;
}

void Socket_add_tcp(int socket)
{
  for(int i=0; i < TCP_CLIENTS_MAX; i++)
  {
    if(tcp_clients[i].sock > 0) continue;
  
    tcp_clients[i].sock = socket;
    return;
  }

  /* No free slots, disconnect */
  fprintf(stderr, "Error, no tcp client slots remaining!\n");
  close(socket);
}

void Socket_add_rtltcp(int socket)
{
  for(int i=0; i < RTLTCP_CLIENTS_MAX; i++)
  {
    if(rtltcp_clients[i].sock > 0) continue;
  
    rtltcp_clients[i].sock = socket;
    return;
  }

  /* No free slots, disconnect */
  fprintf(stderr, "Error, no rtltcp client slots remaining!\n");
  close(socket);
}

void Socket_close(netclient_t *client)
{
  close(client->sock);
  client->sock = 0;
}

/* This function is run on a thread, started from main()
 *
 * This function listens on the TCP port for incoming connections from viewers
 * On connection, the viewer's socket file descriptor is added to the 'viewers' array
 *  to be serviced by 'merger_tx_feed'
 */
void *rx_tcp_socket(void* arg)
{
  (void) arg;

  int socket;
  int clientsocket;

  socket = Socket_init(1233);

  /* Infinite loop, blocks until incoming connection */
  while (1)
  {
    clientsocket = Socket_wait(socket);
    
    /* Add Client Socket to Viewer array */
    Socket_add_tcp(clientsocket);
  }
}

void *rx_rtltcp_socket(void* arg)
{
  (void) arg;
  
  int socket;
  int clientsocket;

  socket = Socket_init(1234);

  /* Infinite loop, blocks until incoming connection */
  while (1)
  {
    clientsocket = Socket_wait(socket);
    
    /* Add Client Socket to Viewer array */
    Socket_add_rtltcp(clientsocket);
  }
}