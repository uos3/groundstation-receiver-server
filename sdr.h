#ifndef __RX_SDR_H_
#define __RX_SDR_H_

typedef struct tcp_samples_t {
   uint16_t s[192][2];
} tcp_samples_t;

typedef struct rtltcp_samples_t {
   uint8_t s[250][2];
} rtltcp_samples_t;

void *rx_sdr(void* arg);

#endif