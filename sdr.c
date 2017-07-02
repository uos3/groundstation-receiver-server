#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <alsa/asoundlib.h>
#include <math.h>

#include "main.h"
#include "buffer.h"
#include "sdr.h"

extern buffer_t tcp_samples_buffer;
extern buffer_t rtltcp_samples_buffer;

void *rx_sdr(void* arg)
{
  (void) arg;
  int ret;
  snd_pcm_t *handle;
  snd_pcm_hw_params_t *hw_params;

	/* Open the device */
  ret = snd_pcm_open (&handle, "plughw:1,0", SND_PCM_STREAM_CAPTURE, 0);
  //ret = snd_pcm_open (&handle, "default", SND_PCM_STREAM_CAPTURE, 0);
  if (ret < 0) {
    fprintf(stderr
          , "unable to open pcm device: %s\n"
          , snd_strerror(ret)
    );
    return NULL;
  }

  unsigned int sample_rate = 192000;
  snd_pcm_uframes_t frame_samples = 192;
  int frame_size = frame_samples * 2 * 2; /* 2 bytes/sample, 2 channels */

  /* Use a buffer large enough to hold one period */
  char *framebuffer_current = (char *) malloc(frame_size);
  char *framebuffer_previous = (char *) malloc(frame_size);
  char *framebuffer_swaptemp;

  tcp_samples_t *tcp_samples;
  rtltcp_samples_t *rtltcp_samples = malloc(sizeof(rtltcp_samples_t) / sizeof(char));

  /* Allocate Hardware Parameters structures and fills it with config space for PCM */
  snd_pcm_hw_params_malloc (&hw_params);
  snd_pcm_hw_params_any (handle, hw_params);

  /* Set parameters : interleaved channels, 16 bits unsigned little endian, 192KHz, 2 channels */
  snd_pcm_hw_params_set_access (handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
  snd_pcm_hw_params_set_format (handle, hw_params, SND_PCM_FORMAT_U16_LE);
  snd_pcm_hw_params_set_rate_near (handle, hw_params, &sample_rate, NULL);
  snd_pcm_hw_params_set_channels (handle, hw_params, 2);

  snd_pcm_hw_params_set_period_size_near(handle, hw_params, &frame_samples, NULL);
  snd_pcm_hw_params_get_period_size(hw_params, &frame_samples, NULL);
  fprintf(stdout, "Info: SDR frame samples: %ld\n", frame_samples);

  unsigned int time_period;
  snd_pcm_hw_params_get_period_time(hw_params, &time_period, NULL);
  fprintf(stdout, "Info: SDR frame time period: %d ms\n", time_period);

  /* Assign them to the playback handle and free the parameters structure */
  ret = snd_pcm_hw_params (handle, hw_params);
  if (ret < 0) {
    fprintf(stderr
          , "Error: SDR unable to open pcm device: %s\n"
          , snd_strerror(ret)
    );
    free(framebuffer_current);
    free(framebuffer_previous);
    return NULL;
  }
  snd_pcm_hw_params_free (hw_params);

  /* Pre-warm buffer */
  snd_pcm_readi(handle, framebuffer_previous, frame_samples);

  while(1)
  {
    ret = snd_pcm_readi(handle, framebuffer_current, frame_samples);
    if (ret == -EPIPE)
    {
      /* EPIPE means overrun */
      fprintf(stderr, "Error: SDR buffer overrun.\n");
      snd_pcm_prepare(handle);
    }
    else if (ret < 0)
    {
      fprintf(stderr, "Error: SDR read error: %s\n", snd_strerror(ret));
    }
    else if (ret != (int)frame_samples)
    {
      fprintf(stderr, "Error: SDR short read.\n");
    }
    else
    {


      /* Good read */
      tcp_samples = (tcp_samples_t *)framebuffer_previous;

      /* DEBUG print of samples */ /*
      fprintf(stdout, "\n192:\n");
      for(int i=0; i<192; i++)
      {
        fprintf(stdout, "%d|%d,", tcp_samples->s[i][0], tcp_samples->s[i][1]);
      } */

      /* Push 16-bit 192KS/s Buffer (academic) */
      if(!Buffer_Push(&tcp_samples_buffer, (void *)tcp_samples))
      {
        fprintf(stderr, "Error: SDR TCP Samples Buffer push failed!\n");
      }

      /** Resample into 8-bit 250KS/s Buffer (rtl_tcp) **/

      /* Linear Interpolation */
      /* TODO: Better interpolation (Cosine/Cubic) */
      double index_interpolated;
      for(int i=0; i<=248; i++)
      {
        index_interpolated = (double)i*(192.0/250.0);

        rtltcp_samples->s[i][0] = 
          (uint8_t)
          (
            (uint16_t)
            (
              tcp_samples->s[(int)floor(index_interpolated)][0]
              + (
                  ( 
                    tcp_samples->s[(int)ceil(index_interpolated)][0]
                    - tcp_samples->s[(int)floor(index_interpolated)][0]
                  )
                  * ( index_interpolated - floor(index_interpolated) )
              )
            ) >> 3
          );

        rtltcp_samples->s[i][1] = 
          (uint8_t)
          (
            (uint16_t)
            (
              tcp_samples->s[(int)floor(index_interpolated)][1]
              + (
                  ( 
                    tcp_samples->s[(int)ceil(index_interpolated)][1]
                    - tcp_samples->s[(int)floor(index_interpolated)][1]
                  )
                  * ( index_interpolated - floor(index_interpolated) )
              )
            ) >> 3
          );
      }
      rtltcp_samples->s[249][0] = 
        (uint8_t)
        (
          (uint16_t)
          (
            tcp_samples->s[191][0]
            + (
                ( 
                  ((uint16_t *)framebuffer_current)[0]
                  - tcp_samples->s[191][0]
                )
                * ( 0.232 )
            )
          ) >> 3
        );

      rtltcp_samples->s[249][1] = 
        (uint8_t)
        (
          (uint16_t)
          (
            tcp_samples->s[191][1]
            + (
                ( 
                  ((uint16_t *)framebuffer_current)[1]
                  - tcp_samples->s[191][1]
                )
                * ( 0.232 )
            )
          ) >> 3
        );


      /* DEBUG print of samples */ /*
      fprintf(stdout, "\nSDR 250:\n");
      int max=0;
      for(int i=0; i<250; i++)
      {
        //fprintf(stdout, "%d|%d,", rtltcp_samples->s[i][0], rtltcp_samples->s[i][1]);
        if(rtltcp_samples->s[i][0] < max) max = rtltcp_samples->s[i][0];
        if(rtltcp_samples->s[i][1] < max) max = rtltcp_samples->s[i][1];
      } 
      fprintf(stdout, "max: %d\n", max); */

      /* Push 16-bit 250KS/s Buffer (academic) */
      if(!Buffer_Push(&rtltcp_samples_buffer, (void *)rtltcp_samples))
      {
      	fprintf(stderr, "Error: SDR RTL_TCP Samples Buffer push failed!\n");
      }

      framebuffer_swaptemp = framebuffer_previous;
      framebuffer_previous = framebuffer_current;
      framebuffer_current = framebuffer_swaptemp;
    }
  }

  fprintf(stderr, "Error: SDR loop exited.\n");

  /* Close the handle and exit */
  snd_pcm_drain(handle);
  snd_pcm_close(handle);
  free(framebuffer_current);
  free(framebuffer_previous);
  return NULL;
}