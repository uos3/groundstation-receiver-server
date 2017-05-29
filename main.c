// FCD Control: https://github.com/csete/fcdctl

// RTLTcp:
// * http://www.rtl-sdr.com/forum/viewtopic.php?f=1&t=1653#p4018
// * https://cwne88.wordpress.com/sdr/rtl_tcp-protocol/

#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>

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

	snd_pcm_uframes_t frames = 32;
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
		if (ret == -EPIPE) {
		  /* EPIPE means overrun */
		  fprintf(stderr, "overrun occurred\n");
		  snd_pcm_prepare(handle);
		} else if (ret < 0) {
		  fprintf(stderr,
		          "error from read: %s\n",
		          snd_strerror(ret));
		} else if (ret != (int)frames) {
		  fprintf(stderr, "short read, read %d frames\n", ret);
		}
		/* Write buffer to stdout, todo: put into circular buffer for TCP TX */
		ret = write(1, buffer, size);
		if (ret != size)
		  fprintf(stderr,
		          "short write: wrote %d bytes\n", ret);
	}


	/* Close the handle and exit */
	snd_pcm_drain(handle);
	snd_pcm_close(handle);
	free(buffer);
	exit (0);
}