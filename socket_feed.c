
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "main.h"
#include "sdr.h"
#include "socket.h"
#include "buffer.h"
#include "socket_feed.h"

extern buffer_t tcp_samples_buffer;
extern buffer_t rtltcp_samples_buffer;

extern netclient_t tcp_clients[TCP_CLIENTS_MAX];
extern netclient_t rtltcp_clients[RTLTCP_CLIENTS_MAX];

void *rx_tcp_feed(void* arg)
{
  (void) arg;
  int i, r;

  tcp_samples_t *tcp_samples;
  int tcp_samples_size = ( sizeof(tcp_samples_t)/sizeof(char) );
  
	while(1)
	{
		Buffer_WaitPop(&tcp_samples_buffer, (void **)&tcp_samples);
		
		for(i = 0; i < TCP_CLIENTS_MAX; i++)
		{
			if(tcp_clients[i].sock <= 0) continue;

			r = send(tcp_clients[i].sock, tcp_samples, tcp_samples_size, 0);
			if(r < 0)
			{
				if(errno == EAGAIN || errno == EWOULDBLOCK)
				{
					/* The socket is busy, we should try again later */
          /* but for now, drop the connection i

netclient_t tcp_clients[TCP_CLIENTS_MAX];
netclient_t rtltcp_clients[RTLTCP_CLIENTS_MAX];mmediately */
          fprintf(stderr, "TCP Client Socket blocked, dropped.\n");
          Socket_close(&tcp_clients[i]);
					break;
				}
				
				/* An error has occured. Drop the connection */
        fprintf(stderr, "TCP Client Socket errored, dropped.\n");
        Socket_close(&tcp_clients[i]);
				break;
			}
		}
	}
}

void *rx_rtltcp_feed(void* arg)
{
  (void) arg;
  int i, r;

  rtltcp_samples_t *rtltcp_samples;
  int rtltcp_samples_size = ( sizeof(rtltcp_samples_t)/sizeof(char) );
  
  //int j=0;
  while(1)
  {
    Buffer_WaitPop(&rtltcp_samples_buffer, (void **)&rtltcp_samples);

    //fprintf(stdout, "%d\n", j++);
    /*fprintf(stdout, "%d | %d (%d)\n"
      , Buffer_Head(&rtltcp_samples_buffer)
      , Buffer_Tail(&rtltcp_samples_buffer)
      , Buffer_Loss(&rtltcp_samples_buffer)
      );*/
    
    for(i = 0; i < RTLTCP_CLIENTS_MAX; i++)
    {
      if(rtltcp_clients[i].sock <= 0) continue;

      /* DEBUG print of samples */ /*
      fprintf(stdout, "\nNET 500:\n");
      for(int i=0; i<500; i++)
      {
        fprintf(stdout, "%d,", ((uint8_t *)rtltcp_samples)[i]);
      } */

      r = send(rtltcp_clients[i].sock, (void *)rtltcp_samples, rtltcp_samples_size, 0);
      if(r < 0)
      {
        if(errno == EAGAIN || errno == EWOULDBLOCK)
        {
          /* The socket is busy, we should try again later */
          /* but for now, drop the connection immediately */
          fprintf(stderr, "RTLTCP Client Socket blocked, dropped.\n");
          Socket_close(&rtltcp_clients[i]);
          break;
        }
        
        /* An error has occured. Drop the connection */
        fprintf(stderr, "RTLTCP Client Socket errored, dropped.\n");
        Socket_close(&rtltcp_clients[i]);
        break;
      }
    }
  }
}