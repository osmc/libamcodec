/**
 * \file audiodsp-ctl.c
 * \brief  Functions of Auduodsp control
 * \version 1.0.0
 * \date 2011-03-08
 */
/* Copyright (C) 2007-2011, Amlogic Inc.
 * All right reserved
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <audio-dec.h>
#include <audiodsp.h>
#include <log-print.h>

static int reset_track_enable=0;
void adec_reset_track_enable(int enable_flag)
{
    reset_track_enable=enable_flag;
	adec_print("reset_track_enable=%d\n", reset_track_enable);
}

static int get_sysfs_int(const char *path)
{
    int fd;
    int val = 0;
    char  bcmd[16];
    fd = open(path, O_RDONLY);
    if (fd >= 0) {
        read(fd, bcmd, sizeof(bcmd));
        val = strtol(bcmd, NULL, 10);
        close(fd);
    }
    return val;
}

static int set_sysfs_int(const char *path, int val)
{
    int fd;
    int bytes;
    char  bcmd[16];
    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd >= 0) {
        sprintf(bcmd, "%d", val);
        bytes = write(fd, bcmd, strlen(bcmd));
        close(fd);
        return 0;
    }
    return -1;
}

static int audiodsp_get_format_changed_flag()
{
    return get_sysfs_int("/sys/class/audiodsp/format_change_flag");

}

 void audiodsp_set_format_changed_flag( int val)
{
    set_sysfs_int("/sys/class/audiodsp/format_change_flag", val);

}

void adec_reset_track(aml_audio_dec_t *audec)
{
	if(audec->format_changed_flag){
		adec_print("reset audio_track: samplerate=%d channels=%d\n", audec->samplerate,audec->channels);
        audio_out_operations_t *out_ops = &audec->aout_ops;
		out_ops->mute(audec, 1);
		out_ops->pause(audec);
        out_ops->stop(audec);
		//audec->SessionID +=1;
        out_ops->init(audec);
		if(audec->state == ACTIVE)
        	out_ops->start(audec);
	    audec->format_changed_flag=0;
	}
}

int audiodsp_format_update(aml_audio_dec_t *audec)
{
    int m_fmt;
    int ret = -1;
    unsigned long val;
    dsp_operations_t *dsp_ops = &audec->adsp_ops;
	
    if (dsp_ops->dsp_file_fd < 0 || get_audio_decoder()!=AUDIO_ARC_DECODER) {
        return ret;
    }
	
	ret=0;
	if(1/*audiodsp_get_format_changed_flag()*/)
	{
         ioctl(dsp_ops->dsp_file_fd, AUDIODSP_GET_CHANNELS_NUM, &val);
         if (val != (unsigned long) - 1) {
		    if( audec->channels != val){
			   //adec_print("dsp_format_update: pre_channels=%d  cur_channels=%d\n", audec->channels,val);
               audec->channels = val;
		       ret=1;
		    }
         }

         ioctl(dsp_ops->dsp_file_fd, AUDIODSP_GET_SAMPLERATE, &val);
         if (val != (unsigned long) - 1) {
		     if(audec->samplerate != val){
			    //adec_print("dsp_format_update: pre_samplerate=%d  cur_samplerate=%d\n", audec->samplerate,val);
                audec->samplerate = val;
		        ret=2;
		     }
         }
         #if 1
         ioctl(dsp_ops->dsp_file_fd, AUDIODSP_GET_BITS_PER_SAMPLE, &val);
         if (val != (unsigned long) - 1) {
		     if(audec->data_width != val){
		        //adec_print("dsp_format_update: pre_data_width=%d  cur_data_width=%d\n", audec->data_width,val);
                audec->data_width = val;
		        ret=3;
		     }
         }
		 #endif
		//audiodsp_set_format_changed_flag(0);
	}
	if(ret>0){
	    audec->format_changed_flag=ret;
	    adec_print("dsp_format_update: audec->format_changed_flag = %d \n", audec->format_changed_flag); 
	}
    return ret;
}



int audiodsp_get_pcm_left_len()
{
    return get_sysfs_int("/sys/class/audiodsp/pcm_left_len");

}




