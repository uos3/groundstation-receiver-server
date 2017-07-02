// FCD Control: https://github.com/csete/fcdctl

// RTLTcp:
// * http://www.rtl-sdr.com/forum/viewtopic.php?f=1&t=1653#p4018
// * https://cwne88.wordpress.com/sdr/rtl_tcp-protocol/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>

#include "main.h"
#include "sdr.h"
#include "buffer.h"
#include "socket.h"
#include "socket_feed.h"

buffer_t tcp_samples_buffer;
buffer_t rtltcp_samples_buffer;

netclient_t tcp_clients[TCP_CLIENTS_MAX];
netclient_t rtltcp_clients[RTLTCP_CLIENTS_MAX];

/* See main.h for this macro to define and declare the threads */
DECLARE_THREADS();

int main (void)
{
  /* Start all declared threads */
  for(int i=0; i<THREADS_NUMBER; i++)
  {
    if(pthread_create(&threads[i].thread, NULL, threads[i].function, threads[i].arg))
    {
      fprintf(stderr, "Error creating %s pthread\n", threads[i].name);
      return 1;
    }
  }

  fprintf(stdout, "rx server running. Standing by for acquisition.\n");
	
	while(1)
	{
    sleep(1);
	}

  return 0;
}