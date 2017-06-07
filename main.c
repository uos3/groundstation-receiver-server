// FCD Control: https://github.com/csete/fcdctl

// RTLTcp:
// * http://www.rtl-sdr.com/forum/viewtopic.php?f=1&t=1653#p4018
// * https://cwne88.wordpress.com/sdr/rtl_tcp-protocol/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <alsa/asoundlib.h>
#include <math.h>

typedef struct native_sample_buffer_t {
   uint16_t s[192][2];
} native_sample_buffer_t;

typedef struct upsampled_sample_buffer_t {
   uint16_t s[250][2];
} upsampled_sample_buffer_t;

typedef struct upsampled_rtlsdr_sample_buffer_t {
   uint8_t s[250][2];
} upsampled_rtlsdr_sample_buffer_t;

int main (void)
{
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
      return 1;
    }

  /* Allocate Hardware Parameters structures and fills it with config space for PCM */
  snd_pcm_hw_params_malloc (&hw_params);
  snd_pcm_hw_params_any (handle, hw_params);

  /* Set parameters : interleaved channels, 16 bits little endian, 44100Hz, 2 channels */
  snd_pcm_hw_params_set_access (handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
  snd_pcm_hw_params_set_format (handle, hw_params, SND_PCM_FORMAT_S16_LE);
  unsigned int val = 44100;
  snd_pcm_hw_params_set_rate_near (handle, hw_params, &val, NULL);
  snd_pcm_hw_params_set_channels (handle, hw_params, 2);

  snd_pcm_uframes_t frames = 192;
    snd_pcm_hw_params_set_period_size_near(handle, hw_params, &frames, NULL);

  /* Assign them to the playback handle and free the parameters structure */
  ret = snd_pcm_hw_params (handle, hw_params);
  if (ret < 0) {
      fprintf(stderr
            , "unable to open pcm device: %s\n"
            , snd_strerror(ret)
        );
      return 1;
    }
  snd_pcm_hw_params_free (hw_params);

  /* Use a buffer large enough to hold one period */
  snd_pcm_hw_params_get_period_size(hw_params, &frames, NULL);
  int size = frames * 4; /* 2 bytes/sample, 2 channels */
  char *buffer = (char *) malloc(size);

  unsigned int time_period;
  snd_pcm_hw_params_get_period_time(hw_params, &time_period, NULL);
    long loops = 5000000 / time_period;

  while (loops > 0)
  {
    loops--;
    ret = snd_pcm_readi(handle, buffer, frames);
    if (ret == -EPIPE)
    {
      /* EPIPE means overrun */
      fprintf(stderr, "overrun occurred\n");
      snd_pcm_prepare(handle);
    }
    else if (ret < 0)
    {
      fprintf(stderr, "error from read: %s\n", snd_strerror(ret));
    }
    else if (ret != (int)frames)
    {
      fprintf(stderr, "short read, read %d frames\n", ret);
    }
    else
    {
      /* Good read */
      native_sample_buffer_t *native_sample_buffer = (native_sample_buffer_t *)buffer;
      fprintf(stdout, "\n192:\n");
      for(int i=0; i<192; i++)
      {
        fprintf(stdout, "%d|%d,", native_sample_buffer->s[i][0], native_sample_buffer->s[i][1]);
      }

      upsampled_sample_buffer_t *upsampled_sample_buffer = malloc(sizeof(upsampled_sample_buffer_t));

      for(int i=0; i<250; i++)
      {
        /* Linear Interpolation */
        double index_interpolated = (double)i*(192.0/250.0);
        upsampled_sample_buffer->s[i][0] = 
            native_sample_buffer->s[(int)floor(index_interpolated)][0]
          + (
              ( 
                native_sample_buffer->s[(int)ceil(index_interpolated)][0]
                - native_sample_buffer->s[(int)floor(index_interpolated)][0]
              )
            * ( index_interpolated - floor(index_interpolated) )
            );
        upsampled_sample_buffer->s[i][1] = 
            native_sample_buffer->s[(int)floor(index_interpolated)][1]
          + (
              ( 
                native_sample_buffer->s[(int)ceil(index_interpolated)][1]
                - native_sample_buffer->s[(int)floor(index_interpolated)][1]
              )
            * ( index_interpolated - floor(index_interpolated) )
            );
        /* TODO: Better interpolation (Cosine/Cubic) */
      }

      fprintf(stdout, "\n250:\n");
      for(int i=0; i<250; i++)
      {
        fprintf(stdout, "%d|%d,", upsampled_sample_buffer->s[i][0], upsampled_sample_buffer->s[i][1]);
      }

      upsampled_rtlsdr_sample_buffer_t *upsampled_rtlsdr_sample_buffer = malloc(sizeof(upsampled_rtlsdr_sample_buffer_t));

      for(int i=0; i<250; i++)
      {
        upsampled_rtlsdr_sample_buffer->s[i][0] = (uint8_t)((upsampled_sample_buffer->s[i][0] >> 8) & 0xFF);
        upsampled_rtlsdr_sample_buffer->s[i][1] = (uint8_t)((upsampled_sample_buffer->s[i][1] >> 8) & 0xFF);
      }

    }
    /* Write buffer to stdout, todo: put into circular buffer for TCP TX */
    //ret = write(1, buffer, size);
    //if (ret != size)
    //{
    //  fprintf(stderr, "short write: wrote %d bytes\n", ret);
    //}
  }


  /* Close the handle and exit */
  snd_pcm_drain(handle);
  snd_pcm_close(handle);
  free(buffer);
  exit (0);
}