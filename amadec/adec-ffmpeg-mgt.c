#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <dlfcn.h>

#include <audio-dec.h>
#include <adec-pts-mgt.h>
#include <adec_write.h>

#if 1//************Macro Definitions**************
#define DECODE_ERR_PATH "/sys/class/audiodsp/codec_fatal_err"
#define DECODE_NONE_ERR 0
#define DECODE_INIT_ERR 1
#define DECODE_FATAL_ERR 2

int exit_decode_thread=0;
static int exit_decode_thread_success=0;
static unsigned long decode_offset=0;
static int nDecodeErrCount=0;
static buffer_stream_t *g_bst=NULL;
static int fd_uio=-1;
static AudioInfo g_AudioInfo;
static int sn_threadid=-1;
static int sn_getpackage_threadid=-1;

static int aout_stop_mutex=0;//aout stop mutex flag
static int last_valid_pts=0;
static int out_len_after_last_valid_pts=0;
static int pcm_cache_size=0;

struct package{
    char *data;//buf ptr
    int size;               //package size
    struct package * next;//next ptr
};
typedef struct {
    struct package *first;
    int pack_num;
    struct package *current;
    int mutex; //  0 idle 1 inuse
}Package_List;
Package_List pack_list;


void *audio_decode_loop(void *args);
void *audio_getpackage_loop(void *args);

static int set_sysfs_int(const char *path, int val);
static void stop_decode_thread(aml_audio_dec_t *audec);

#endif//**********Macro Definitions end************

#if 1//***************Arm Decoder******************
/*audio decoder list structure*/
typedef struct 
{
	//enum CodecID codec_id;
	int codec_id;
	char    name[64];
} audio_lib_t;


//    CODEC_ID_AAC
audio_lib_t audio_lib_list[] =
{
	{ACODEC_FMT_AAC, "libfaad.so"},
	{ACODEC_FMT_AAC_LATM, "libfaad.so"},
	{ACODEC_FMT_APE, "libape.so"},
	{ACODEC_FMT_MPEG, "libmad.so"},
	{ACODEC_FMT_FLAC, "libflac.so"},
	NULL
} ;

int find_audio_lib(aml_audio_dec_t *audec)
{    
	int i;
	int num;
	audio_lib_t *f;
	int fd = 0;

	num = ARRAY_SIZE(audio_lib_list);   
	audio_decoder_operations_t *adec_ops=audec->adec_ops;
	
	for (i = 0; i < num; i++) {        
		f = &audio_lib_list[i];        
		if (f->codec_id == audec->format) 
		{            
			fd = dlopen(audio_lib_list[i].name,RTLD_NOW);
			//adec_print("dlopen failed, fd = %d,\n %s",fd,dlerror());
			if (fd != 0)
			{
			       adec_ops->init = dlsym(fd, "audio_dec_init");
				adec_ops->decode = dlsym(fd, "audio_dec_decode");
				adec_ops->release = dlsym(fd, "audio_dec_release");
				adec_ops->getinfo = dlsym(fd, "audio_dec_getinfo");
			}
			else 
			{
				adec_print("cant find decoder lib\n");
				return -1;
			}			
			return 0;
		}
	}    
	return -1;
}

audio_decoder_operations_t AudioArmDecoder=
{
    "FFmpegDecoder",
    AUDIO_ARM_DECODER,
    0,
};

#endif//***************Arm Decoder******************

#if 1//***************ffmpeg Decoder******************
static int FFmpegDecoderInit(audio_decoder_operations_t *adec_ops)
{
#if 0
       aml_audio_dec_t *audec=(aml_audio_dec_t *)(adec_ops->priv_data);
       AVCodecContext *ctxCodec = NULL;
	AVCodec *acodec = NULL;
       ctxCodec = avcodec_alloc_context();
	if(!ctxCodec) {
		adec_print("AVCodecContext allocate error!\n");
		ctxCodec = NULL;
	}
	ctxCodec=audec->pcodec->ctxCodec;
	ctxCodec->codec_type = CODEC_TYPE_AUDIO;
	acodec = avcodec_find_decoder(ctxCodec->codec_id);
	if (!acodec) {
		adec_print("acodec not found\n");
		set_sysfs_int(DECODE_ERR_PATH,DECODE_INIT_ERR);
		return  -1;
	}
	if (avcodec_open(ctxCodec, acodec) < 0) {
		adec_print("Could not open acodec = %d\n", acodec);
		set_sysfs_int(DECODE_ERR_PATH,DECODE_INIT_ERR);
		return  -1;
	}
	 //int data_width=audec->pcodec->ctxCodec->sample_fmt;
        //int channels=audec->channels=audec->pcodec->ctxCodec->channels;
        //int samplerate=audec->samplerate=audec->pcodec->ctxCodec->sample_rate;
	adec_ops->nInBufSize=READ_ABUFFER_SIZE;
	adec_ops->nOutBufSize=DEFAULT_PCM_BUFFER_SIZE;
	#endif
	return 0;
}
static int FFmpegDecode(audio_decoder_operations_t *adec_ops, char *outbuf, int *outlen, char *inbuf, int inlen){
#if 0
        aml_audio_dec_t *audec=(aml_audio_dec_t *)(adec_ops->priv_data);
        AVCodecContext *ctxCodec=audec->pcodec->ctxCodec;
        AVCodecContext *ctxCodec=NULL;
	AVPacket avpkt;
	
	av_init_packet(&avpkt);
	avpkt.data = inbuf;
	avpkt.size = inlen;
	int  i;
	if (ctxCodec->codec_id == CODEC_ID_FLAC){
		static int end = 0; 
		static int pkt_end_data = 0;
		if (inlen < 10240)	{
			end++;
			if (1==i)		
				pkt_end_data = inlen;	
			if (pkt_end_data == inlen){		
				end++;	
			}
			else{
				end=0;		
			}
			if ((end/2)<5)	
				return -1;	
		}
		end=0;	
		pkt_end_data = 0;
	}
	  ret = avcodec_decode_audio3(ctxCodec, (int16_t *)outbuf, outlen, &avpkt);
	  #endif
	  int ret;
	  return ret;
}

static int FFmpegDecoderRelease(audio_decoder_operations_t *adec_ops)
{
    aml_audio_dec_t *audec=(aml_audio_dec_t *)(adec_ops->priv_data);
    //avcodec_close(audec->pcodec->ctxCodec);
    return 0;
}

audio_decoder_operations_t AudioFFmpegDecoder=
{
    .name="FFmpegDecoder",
    .nAudioDecoderType=AUDIO_FFMPEG_DECODER,
    .init=FFmpegDecoderInit,
    .decode=FFmpegDecode,
    .release=FFmpegDecoderRelease,
    .getinfo=NULL,
};

#endif//***************Arm Decoder end***************

#if 1 /*package list ops*/
int package_list_free()
{
    while((pack_list.mutex))
        usleep(100);
        pack_list.mutex=1;
    while(pack_list.pack_num)
    {
        struct package * p=pack_list.first;
        pack_list.first=pack_list.first->next;
        free(p->data);
        free(p);
        pack_list.pack_num--;
    }
    pack_list.mutex=0;
    return 0;
}

int package_list_init()
{
    pack_list.first=NULL;
    pack_list.pack_num=0;
    pack_list.current=NULL;
    pack_list.mutex=0;
    return 0;
}

int package_add(char * data,int size)
{
    if(pack_list.mutex)
        return -3;
    pack_list.mutex=1;  
    if(pack_list.pack_num==4)//enough
    {
         pack_list.mutex=0;
        return -2;
    }
    struct package *p=malloc(sizeof(struct package));
    if(!p) //malloc failed
        return -1;
    p->data=data;
    p->size=size;
    if(pack_list.pack_num==0)//first package
    {
        pack_list.first=p;
        pack_list.current=p;
        pack_list.pack_num=1;
    }
    else
    {
        pack_list.current->next=p;
        pack_list.current=p;
        pack_list.pack_num++;
    }
    pack_list.mutex=0;
    return 0;
}

struct package * package_get()
{
    if(pack_list.mutex)
        return NULL;
    pack_list.mutex=1;  
    if(pack_list.pack_num==0)
    {
        pack_list.mutex=0;
        return NULL;
    }
    struct package *p=pack_list.first;
    if(pack_list.pack_num==1)
    {
        pack_list.first=NULL;
        pack_list.pack_num=0;
        pack_list.current=NULL;
    }
    else
    {
        pack_list.first=pack_list.first->next;
        pack_list.pack_num--;
    }
    pack_list.mutex=0;
    return p;
}

int get_package_size()
{
    int nTotalSize=0;
    int num=pack_list.pack_num;
    if(num==0)
        return 0;
    struct package *p=pack_list.first;
    while(num)
    {
        nTotalSize+=p->size;
        p=p->next;
        num--;
    }
    return num;
}
#endif

#if 1//now in use but need to replace
int armdec_stream_read(dsp_operations_t *dsp_ops, char *buffer, int size)
{
    out_len_after_last_valid_pts+=size;
     return read_pcm_buffer(buffer,g_bst, size);
}

unsigned long  armdec_get_pts(dsp_operations_t *dsp_ops)
{
    unsigned long val,offset;
    unsigned long pts;
    int data_width,channels,samplerate;
    unsigned long long frame_nums ;
    unsigned long delay_pts;
    switch(g_bst->data_width)
    {
        case AV_SAMPLE_FMT_U8:
            data_width=8;
            break;
        case AV_SAMPLE_FMT_S16:
            data_width=16;
            break;
            case AV_SAMPLE_FMT_S32:
            data_width=32;
            break;
        default:
            data_width=16;
    }
    channels=g_bst->channels;
    samplerate=g_bst->samplerate;
    offset=decode_offset;
    if(dsp_ops->dsp_file_fd)
        ioctl(dsp_ops->dsp_file_fd,AMSTREAM_IOC_APTS_LOOKUP,&offset);
    else
        adec_print("====abuf have not open!\n",val);
    pts=offset;
   
    if(pts==0)
    {
        if (last_valid_pts)
        pts = last_valid_pts;
        frame_nums = (out_len_after_last_valid_pts * 8 / (data_width * channels));
        pts+= (frame_nums*90000/samplerate);
        //adec_print("==decode_offset:%d out_pcm:%d   pts:%d \n",decode_offset,out_len_after_last_valid_pts,pts);
        return pts; 
        //return -1; 
    }

    int len = g_bst->buf_level+pcm_cache_size;
    frame_nums = (len * 8 / (data_width * channels));
    delay_pts = (frame_nums*90000/samplerate);
    if (pts > delay_pts) {
        pts -= delay_pts;
    } else {
        pts = 0;
    }
    val=pts;
    last_valid_pts=pts;
    out_len_after_last_valid_pts=0;
    //adec_print("====get pts:%ld offset:%ld frame_num:%lld delay:%ld \n",val,decode_offset,frame_nums,delay_pts);
    return val;
}

//set decoder err condition

static int set_sysfs_int(const char *path, int val)
{
    int fd;
    int bytes;
    char  bcmd[16];
    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0664);
    if (fd >= 0) {
        sprintf(bcmd, "%d", val);
        bytes = write(fd, bcmd, strlen(bcmd));
        close(fd);
        return 0;
    }
    return -1;
}
int get_decoder_status(void *p,struct adec_status *adec)
{
    aml_audio_dec_t *audec=(aml_audio_dec_t *)p;
    if(g_bst&&audec)
    {
        adec->channels=g_bst->channels;
        adec->sample_rate=g_bst->samplerate;
        adec->resolution=g_bst->data_width;
        adec->error_count=nDecodeErrCount;//need count
        adec->status=(audec->state> INITTED)?1:0;
        return 0;
    }
    else 
        return -1;
}

#endif


#if 1//*************General Func***************

/**
 * \brief register audio decoder
 * \param audec pointer to audec ,codec_type
 * \return 0 on success otherwise -1 if an error occurred
 */
 int RegisterDecode(aml_audio_dec_t *audec,int type)
{
    switch(type)
    {
        case AUDIO_ARM_DECODER:
            audec->adec_ops=&AudioArmDecoder;
            find_audio_lib(audec);
            audec->adec_ops->priv_data=audec;
            break;
        case AUDIO_FFMPEG_DECODER:
            audec->adec_ops=&AudioFFmpegDecoder;
            audec->adec_ops->priv_data=audec;
            break;
        default:
            audec->adec_ops=&AudioFFmpegDecoder;
            audec->adec_ops->priv_data=audec;
            break;
    }
    return 0;
}

static int InBufferInit(aml_audio_dec_t *audec)
{
       int ret = uio_init(fd_uio);
	if (ret < 0){
		adec_print("uio init error! \n");
		return -1;
	}
	return 0;
}
static int InBufferRelease(aml_audio_dec_t *audec)
{
        close(fd_uio);
        fd_uio=-1;
        return 0;
}


static int OutBufferInit(aml_audio_dec_t *audec)
{
    g_bst=malloc(sizeof(buffer_stream_t));
    if(!g_bst)
    {
        adec_print("===g_bst malloc failed! \n");
        g_bst=NULL;
        return -1;
    }
    if(audec->adec_ops->nOutBufSize<=0) //set default if not set
        audec->adec_ops->nOutBufSize=DEFAULT_PCM_BUFFER_SIZE;
    int ret=init_buff(g_bst,audec->adec_ops->nOutBufSize);
    if(ret==-1)
    {
        adec_print("=====pcm buffer init failed !\n");
        return -1;
    }

    g_bst->data_width=audec->data_width=AV_SAMPLE_FMT_S16;
     if(audec->channels>0)
        g_bst->channels=audec->channels;
    else
        g_bst->channels=audec->channels=2;
    if(audec->samplerate>0)
        g_bst->samplerate=audec->samplerate;
    else
        g_bst->samplerate=audec->samplerate=48000;
    adec_print("=====pcm buffer init ok buf_size:%d\n",g_bst->buf_length);
    
    return 0;
}
static int OutBufferRelease(aml_audio_dec_t *audec)
{
    if(g_bst)
        release_buffer(g_bst);
    return 0;
}

static int audio_codec_init(aml_audio_dec_t *audec)
{
        //reset static&global
        exit_decode_thread=0;
        exit_decode_thread_success=0;
        decode_offset=0;
        nDecodeErrCount=0;
        g_bst=NULL;
        fd_uio=-1;
        aout_stop_mutex=0;
        last_valid_pts=0;
        out_len_after_last_valid_pts=0;
        pcm_cache_size=0;
        package_list_init();
        while(0!=set_sysfs_int(DECODE_ERR_PATH,DECODE_NONE_ERR))
        {
            adec_print("====set codec fatal   failed ! \n");
            usleep(100000);
        }
       
	audec->data_width=AV_SAMPLE_FMT_S16;
        if(audec->channels>0)
            audec->adec_ops->channels=audec->channels;
        else
            audec->adec_ops->channels=audec->channels=2;
        if(audec->samplerate>0)
            audec->adec_ops->samplerate=audec->samplerate;
        else
            audec->adec_ops->samplerate=audec->samplerate=48000;
        switch(audec->data_width)
        {
            case AV_SAMPLE_FMT_U8:
                audec->adec_ops->bps=8;
                break;
            case AV_SAMPLE_FMT_S16:
                audec->adec_ops->bps=16;
                break;
                case AV_SAMPLE_FMT_S32:
                audec->adec_ops->bps=32;
                break;
            default:
                audec->adec_ops->bps=16;
        }
        adec_print("==param= bps:%d samplerate:%d channel:%d \n",audec->adec_ops->bps,audec->adec_ops->samplerate,audec->adec_ops->channels);
        audec->adec_ops->extradata_size=audec->extradata_size;
        if(audec->extradata_size>0)
            memcpy(audec->adec_ops->extradata,audec->extradata,audec->extradata_size);
        int ret=0;
        //1-decoder init
        ret=audec->adec_ops->init(audec->adec_ops);
        if(ret==-1)
        {
            adec_print("====adec_ops init err \n");
            goto err1;
        }
        //2-pcm_buffer init
        ret=OutBufferInit(audec);
        if(ret==-1)
        {
            adec_print("====out buffer  init err \n");
            goto err2;
        }
	//3-init uio
	ret=InBufferInit(audec);
	 if(ret==-1)
        {
            adec_print("====in buffer  init err \n");
            goto err3;
        }
	   //4-other init
	audec->adsp_ops.dsp_on = 1;
       audec->adsp_ops.dsp_read = armdec_stream_read;
       audec->adsp_ops.get_cur_pts = armdec_get_pts;
       return 0;

err1:
        audec->adec_ops->release(audec->adec_ops);
        return -1;
err2:
        audec->adec_ops->release(audec->adec_ops);
        OutBufferRelease(audec);
        return -1;
err3:
        audec->adec_ops->release(audec->adec_ops);
        OutBufferRelease(audec);
        InBufferRelease(audec);
        return -1;
       
}
static int audio_codec_release(aml_audio_dec_t *audec)
{
    //1-decode thread quit
    //adec_print("====adec_ffmpeg_release start release ! \n");
    stop_decode_thread(audec);
    //adec_print("====adec_ffmpeg_release quit decode ok ! \n");
     //2-decoder release
    audec->adec_ops->release(audec->adec_ops);
    //3-uio uninit
    InBufferRelease(audec);
    //4-outbufferrelease
    OutBufferRelease(audec);
    //5-other release
    audec->adsp_ops.dsp_on = -1;
    audec->adsp_ops.dsp_read = NULL;
    audec->adsp_ops.get_cur_pts = NULL;
    audec->adsp_ops.dsp_file_fd= NULL;
    
    return 0;
}

#endif//***********General Func end*************

#if 1//**************Message Func***************

static int audio_hardware_ctrl(hw_command_t cmd)
{
    int fd;

    fd = open(AUDIO_CTRL_DEVICE, O_RDONLY);
    if (fd < 0) {
        adec_print("Open Device %s Failed!", AUDIO_CTRL_DEVICE);
        return -1;
    }

    switch (cmd) {
    case HW_CHANNELS_SWAP:
        ioctl(fd, AMAUDIO_IOC_SET_CHANNEL_SWAP, 0);
        break;

    case HW_LEFT_CHANNEL_MONO:
        ioctl(fd, AMAUDIO_IOC_SET_LEFT_MONO, 0);
        break;

    case HW_RIGHT_CHANNEL_MONO:
        ioctl(fd, AMAUDIO_IOC_SET_RIGHT_MONO, 0);
        break;

    case HW_STEREO_MODE:
        ioctl(fd, AMAUDIO_IOC_SET_STEREO, 0);
        break;

    default:
        adec_print("Unknow Command %d!", cmd);
        break;

    };

    close(fd);

    return 0;

}

static int get_first_apts_flag(dsp_operations_t *dsp_ops)
{
    int val;
    if (dsp_ops->dsp_file_fd < 0) {
        adec_print("read error!! audiodsp have not opened\n");
        return -1;
    }
    ioctl(dsp_ops->dsp_file_fd, GET_FIRST_APTS_FLAG, &val);
    return val;
}


/**
 * \brief start audio dec when receive START command.
 * \param audec pointer to audec
 */
static void start_adec(aml_audio_dec_t *audec)
{
    int ret;
    audio_out_operations_t *aout_ops = &audec->aout_ops;
    dsp_operations_t *dsp_ops = &audec->adsp_ops;

    if (audec->state == INITTED) {
        audec->state = ACTIVE;
#if 1
        //get info from the audiodsp == can get from amstreamer
        while ((!get_first_apts_flag(dsp_ops)) && (!audec->need_stop)) {
        
            adec_print("wait first pts checkin complete !");
            usleep(100000);
        }
        
        /*start  the  the pts scr,...*/
        ret = adec_pts_start(audec);

        if (audec->auto_mute) {
            avsync_en(0);
            adec_pts_pause();

            while ((!audec->need_stop) && track_switch_pts(audec)) {
                usleep(1000);
            }

            avsync_en(1);
            adec_pts_resume();

            audec->auto_mute = 0;
        }
#endif
        aout_ops->start(audec);

    }
}

/**
 * \brief pause audio dec when receive PAUSE command.
 * \param audec pointer to audec
 */
static void pause_adec(aml_audio_dec_t *audec)
{
    audio_out_operations_t *aout_ops = &audec->aout_ops;

    if (audec->state == ACTIVE) {
        audec->state = PAUSED;
        adec_pts_pause();
        aout_ops->pause(audec);
    }
}

/**
 * \brief resume audio dec when receive RESUME command.
 * \param audec pointer to audec
 */
static void resume_adec(aml_audio_dec_t *audec)
{
    audio_out_operations_t *aout_ops = &audec->aout_ops;

    if (audec->state == PAUSED) {
        audec->state = ACTIVE;
        aout_ops->resume(audec);
        adec_pts_resume();
    }
}

/**
 * \brief stop audio dec when receive STOP command.
 * \param audec pointer to audec
 */
static void stop_adec(aml_audio_dec_t *audec)
{
    audio_out_operations_t *aout_ops = &audec->aout_ops;

    if (audec->state > INITING) {
        audec->state = STOPPED;
	aout_ops->mute(audec, 1); //mute output, some repeat sound in audioflinger after stop
        aout_ops->stop(audec);
        audio_codec_release(audec);
    }
}

/**
 * \brief release audio dec when receive RELEASE command.
 * \param audec pointer to audec
 */
static void release_adec(aml_audio_dec_t *audec)
{
    audec->state = TERMINATED;
}

/**
 * \brief mute audio dec when receive MUTE command.
 * \param audec pointer to audec
 * \param en 1 = mute, 0 = unmute
 */
static void mute_adec(aml_audio_dec_t *audec, int en)
{
    audio_out_operations_t *aout_ops = &audec->aout_ops;

    if (aout_ops->mute) {
        adec_print("%s the output !\n", (en ? "mute" : "unmute"));
        aout_ops->mute(audec, en);
        audec->muted = en;
    }
}

/**
 * \brief set volume to audio dec when receive SET_VOL command.
 * \param audec pointer to audec
 * \param vol volume value
 */
static void adec_set_volume(aml_audio_dec_t *audec, float vol)
{
    audio_out_operations_t *aout_ops = &audec->aout_ops;

    if (aout_ops->set_volume) {
        adec_print("set audio volume! vol = %f\n", vol);
        aout_ops->set_volume(audec, vol);
    }
}

/**
 * \brief set volume to audio dec when receive SET_LRVOL command.
 * \param audec pointer to audec
 * \param lvol left channel volume value
 * \param rvol right channel volume value
 */
static void adec_set_lrvolume(aml_audio_dec_t *audec, float lvol,float rvol)
{
    audio_out_operations_t *aout_ops = &audec->aout_ops;

    if (aout_ops->set_lrvolume) {
        adec_print("set audio volume! left vol = %f,right vol:%f\n", lvol,rvol);
        aout_ops->set_lrvolume(audec, lvol,rvol);
    }
}
static void adec_flag_check(aml_audio_dec_t *audec)
{
    audio_out_operations_t *aout_ops = &audec->aout_ops;

    if (audec->auto_mute && (audec->state > INITTED)) {
        aout_ops->pause(audec);
        while ((!audec->need_stop) && track_switch_pts(audec)) {
            usleep(1000);
        }
        aout_ops->resume(audec);
        audec->auto_mute = 0;
    }
}

#endif//************Message Func end*************

#if 1 //main loop
static void start_decode_thread(aml_audio_dec_t *audec)
{
    if(audec->state != INITTED)
    {
        adec_print("decode not inited quit \n");
        return -1;
    }
    pthread_t    tid;
    int ret = pthread_create(&tid, NULL, (void *)audio_getpackage_loop, (void *)audec);
    sn_getpackage_threadid=tid;
    adec_print("Create get package thread success! tid = %d\n", tid);
    //thread need to sync
    ret = pthread_create(&tid, NULL, (void *)audio_decode_loop, (void *)audec);
    if (ret != 0) {
        adec_print("Create ffmpeg decode thread failed!\n");
        return ret;
    }
    sn_threadid=tid;
    adec_print("Create ffmpeg decode thread success! tid = %d\n", tid);
    
}
static void stop_decode_thread(aml_audio_dec_t *audec)
{
    exit_decode_thread=1;
    int ret = pthread_join(sn_threadid, NULL);
    adec_print("decode thread exit success \n");
    ret = pthread_join(sn_getpackage_threadid, NULL);
    adec_print("get package thread exit success \n");
    exit_decode_thread=0;
    sn_threadid=-1;
    sn_getpackage_threadid=-1;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  getNextFrameSize  Get next frame size
*
* @param[in]   format     audio format
*
* @return      -1: no frame_size  use default   0: need get again  non_zero: success
*/
/* --------------------------------------------------------------------------*/

typedef struct {
    char buff[10];
    int size;
    int status;//0 init 1 finding sync word 2 finding framesize 3 frame size found
}StartCode;
static StartCode start_code;
static int get_frame_size(int format)
{
    int frame_szie=0;
    int ret=0;
    int extra_data=8;//?
    if(start_code.status==0||start_code.status==3)
        memset(&start_code,0,sizeof(StartCode));
    /*ape case*/
    if(format==ACODEC_FMT_APE){	
        if(start_code.status==0)//have not get the sync data
        {
            ret=read_buffer(start_code.buff,4);
            if(ret<=0)
                return 0;
            start_code.size=4;
            start_code.status=1;          
        }
        if(start_code.status==1) {//start find sync word
            if(start_code.size<4){
                ret=read_buffer(start_code.buff+start_code.size,4-start_code.size);
                if(ret<=0)
                    return 0;
                start_code.size=4;
            }
            if(start_code.size==4){
                if((start_code.buff[0]=='A')&&(start_code.buff[1]=='P')&&(start_code.buff[2]=='T')&&(start_code.buff[3]=='S')){
                   
                    start_code.size=0;
                    start_code.status=2;  //sync word found ,start find frame size
                }
                else{
                    start_code.size=3;
                    //memcpy(start_code.buff,&start_code.buff[1],3);
                    start_code.buff[0] = start_code.buff[1];
                    start_code.buff[1] = start_code.buff[2];
                    start_code.buff[2] = start_code.buff[3];
                    return 0;
                }
              }
              
            }
            if(start_code.status==2)
            {
                ret=read_buffer(start_code.buff,4);
                if(ret<=0)
                    return 0;
                start_code.size=4;
                frame_szie  = start_code.buff[3]<<24|start_code.buff[2]<<16|start_code.buff[1]<<8|start_code.buff[0]+extra_data;		
	         frame_szie  = (frame_szie+3)&(~3);	
	         start_code.status=3;//found frame size
	         return frame_szie;
            }
        }
    return -1;
}

void *audio_getpackage_loop(void *args)
{
     int ret;
    aml_audio_dec_t *audec;
    audio_decoder_operations_t *adec_ops;
    int nNextFrameSize=0;//next read frame size
    int inlen = 0;//real data size in in_buf
    int nInBufferSize=0;//full buffer size
    char *inbuf = NULL;//real buffer
    int rlen = 0;//read buffer ret size
    int nAudioFormat;
    	
    adec_print("adec_getpackage_loop start!\n");
    audec = (aml_audio_dec_t *)args;
    adec_ops=audec->adec_ops;
    nAudioFormat=audec->format;
    inlen=0;
    nNextFrameSize=adec_ops->nInBufSize;    
    while (1){
exit_decode_loop:
          /*detect quit condition*/
          if(exit_decode_thread)
	      {
	            if(inbuf)
	                free(inbuf);
	            package_list_free();
        	     break;
	      }
	      /*step 2  get read buffer size*/
	      nNextFrameSize=get_frame_size(nAudioFormat);
	      //adec_print("==get frame size:%d \n",nNextFrameSize);
	      if(nNextFrameSize==-1)
	        nNextFrameSize=adec_ops->nInBufSize;
	      else if(nNextFrameSize==0)
	      {
	        usleep(1000);
	        continue;
	      }
	      /*step 3  read buffer*/
                nInBufferSize = nNextFrameSize;
		  if (inbuf != NULL) {
			  free(inbuf);
			  inbuf = NULL;
		  }
		  inbuf = malloc(nInBufferSize);
                int nNextReadSize=nInBufferSize;
                int nRet=0;
                int nReadErrCount=0;
                int nCurrentReadCount=0;
                int nReadSizePerTime=1*1024;
                rlen=0;
                while(nNextReadSize>0)
                {
                    if(exit_decode_thread)
	                goto exit_decode_loop;
                    if(nNextReadSize<=nReadSizePerTime)
                        nReadSizePerTime=nNextReadSize;
                    nRet = read_buffer(inbuf+rlen, nReadSizePerTime);//read 10K per time
                    if(nRet<=0)
                    {
                        usleep(1000);
                        continue;
                    }
                    rlen+=nRet;
                    nNextReadSize-=nRet;
                    
                }
                nCurrentReadCount=rlen;
                rlen += inlen;
                while(package_add(inbuf,rlen))
                {
                    //add failed
                    if(exit_decode_thread)
	                goto exit_decode_loop;
	             usleep(1000);
                }
                inbuf=NULL;
        }
QUIT:
        adec_print("Exit adec_getpackage_loop Thread!");
        pthread_exit(NULL);
        return NULL;
}

static char pcm_buf_tmp[AVCODEC_MAX_AUDIO_FRAME_SIZE];//max frame size out buf
void *audio_decode_loop(void *args)
{
    int ret;
    aml_audio_dec_t *audec;
    audio_out_operations_t *aout_ops;
    audio_decoder_operations_t *adec_ops;
    int nNextFrameSize=0;//next read frame size
    int inlen = 0;//real data size in in_buf
    int nRestLen=0;//left data after last decode 
    int nInBufferSize=0;//full buffer size
    //int nStartDecodePoint=0;//start decode point in in_buf
    char *inbuf = NULL;//real buffer
    int rlen = 0;//read buffer ret size
    char *pRestData=NULL;
    char *inbuf2;
    
    int dlen = 0;//decode size one time
    int declen = 0;//current decoded size
    int nCurrentReadCount=0;
    
    char startcode[5];	
    int extra_data = 8;
    int nCodecID;
    int nAudioFormat;
    char *outbuf=pcm_buf_tmp;
    int outlen = 0;
    	
    adec_print("adec_armdec_loop start!\n");
    audec = (aml_audio_dec_t *)args;
    aout_ops = &audec->aout_ops;
    adec_ops=audec->adec_ops;
    memset(outbuf, 0, AVCODEC_MAX_AUDIO_FRAME_SIZE);

    //nAudioFormat=audec->pcodec->audio_type;
    nAudioFormat=audec->format;
    inlen=0;
    //nNextFrameSize=READ_ABUFFER_SIZE;//default frame size
    nNextFrameSize=adec_ops->nInBufSize;    
    while (1){
exit_decode_loop:
          //detect quit condition
          if(exit_decode_thread)
	      {
        	        if (inbuf) 
        	        {
            	            free(inbuf);
            		     inbuf = NULL;
        		  }
        		  //if(pRestData)
        		  //{
        		  //  free(pRestData);
        		  //  pRestData=NULL;
        		  //}
        		  //adec_print("====exit decode thread\n");
        		  exit_decode_thread_success=1;
        		  break;
	      }
	      //detect audio info changed
	      memset(&g_AudioInfo,0,sizeof(AudioInfo));
	     adec_ops->getinfo(audec->adec_ops, &g_AudioInfo);
	      if(g_AudioInfo.channels!=0&&g_AudioInfo.samplerate!=0)
	      {
	        if((g_AudioInfo.channels !=g_bst->channels)||(g_AudioInfo.samplerate!=g_bst->samplerate))
	        {
	            adec_print("====Info Changed: src:sample:%d  channel:%d dest sample:%d  channel:%d \n",g_bst->samplerate,g_bst->channels,g_AudioInfo.samplerate,g_AudioInfo.channels);
				audec->format_changed_flag = 1;
				g_bst->channels=audec->channels=g_AudioInfo.channels;
				g_bst->samplerate=audec->samplerate=g_AudioInfo.samplerate;
				
#if 0
				if(aout_stop_mutex==0)
	            {
        	            aout_stop_mutex=1;
        	            //send Message
        	            //reset param
        	            
    	            	aout_ops->stop(audec);
						aout_ops->init(audec);
						aout_ops->start(audec);
        	            aout_stop_mutex=0;
	            }
#endif				
				
	        }
	      }
	      //step 2  get read buffer size
             struct package *p_Package;
             p_Package=package_get();
             if(!p_Package){
                usleep(1000);
                continue;
             }
             if (inbuf != NULL) {
        	    free(inbuf);
                  inbuf = NULL;
             }
             if(inlen&&pRestData)
    	      {
    	            rlen=p_Package->size+inlen;
    	            inbuf=malloc(rlen);
    	            memcpy(inbuf, pRestData, inlen);
    	            memcpy(inbuf+inlen,p_Package->data,p_Package->size);
    	            free(pRestData);
    	            free(p_Package->data);
    	      }
    	      else
    	      {
    	        rlen=p_Package->size;
    	        inbuf=p_Package->data;
    	        p_Package->data=NULL;
    	      }
    	       free(p_Package);
    	       nCurrentReadCount=rlen;
    	       inlen=rlen;
              declen  = 0;
              if (nCurrentReadCount > 0)
              {
    			  //inlen = rlen;
    			  //adec_print("declen=%d,rlen = %d--------------------------------------------------\n\n", declen,rlen);
    			  while (declen<rlen) {
				 if(exit_decode_thread)
    				  {
					 adec_print("exit decoder,-----------------------------\n");
    				        goto exit_decode_loop;
    				  }
    				  outlen = AVCODEC_MAX_AUDIO_FRAME_SIZE;
    				  //adec_print("decode_audio------------------------------------\n");
    				  //dlen = decode_audio(audec->pcodec->ctxCodec, outbuf, &outlen, inbuf+declen, inlen);
    				  dlen = adec_ops->decode(audec->adec_ops, outbuf, &outlen, inbuf+declen, inlen);
    				  //dlen = decodeAACfile(outbuf, &outlen, inbuf+declen, inlen);
    				  if (dlen <= 0)
    				  {
    				  	  //adec_print("dlen = %d error----\n",dlen);
    					  if (nAudioFormat==ACODEC_FMT_APE)
    					  {
    					        inlen=0;	  
    					  }
    					  else if(inlen>0)
    					  {
    					         // adec_print("packet end %d bytes----\n",inlen);
    						  //memcpy(apkt_end, (uint8_t *)(inbuf+declen), inlen);
    						  pRestData=malloc(inlen);
    						  if(pRestData)
    						    memcpy(pRestData, (uint8_t *)(inbuf+declen), inlen);
    					  }
    					  nDecodeErrCount++;//decode failed, add err_count
    					  break;
    				  }
    				  nDecodeErrCount=0;//decode success reset to 0
    				  declen += dlen;
    				  inlen -= dlen;
    				  //adec_print("after decode_audio rlen=%d,declen=%d,inlen=%d,dlen=%d,outlen=%d,-----------------------------\n",rlen,declen,inlen,dlen,outlen);

    				  //write to the pcm buffer
    				  decode_offset+=dlen;
    				  pcm_cache_size=outlen;
    				  if(g_bst)
    				  {
            				//while(g_bst->buf_level>=g_bst->buf_length*0.8)
            				while(g_bst->buf_length-g_bst->buf_level<outlen)
            				{
            				    if(exit_decode_thread)
            				    {
    						 adec_print("exit decoder,-----------------------------\n");
            				        goto exit_decode_loop;
            				        break;
            				    }
            				    usleep(100000);
            				}
            				int wlen=0;
            				while(outlen)
            				{
            				    wlen=write_pcm_buffer(outbuf, g_bst,outlen); 
            				    outlen-=wlen;
            				    pcm_cache_size-=wlen;
            				}
    				  }
    				//write end
    			  }
        	  }
              else
              {
                   //get data failed,maybe at the end
                   usleep(1000);
                   continue;
              }
	}
    
    adec_print("Exit adec_armdec_loop Thread!");
    pthread_exit(NULL);
error:	
    pthread_exit(NULL);
    return NULL;
}

void *adec_armdec_loop(void *args)
{
    int ret;
    aml_audio_dec_t *audec;
    audio_out_operations_t *aout_ops;
    adec_cmd_t *msg = NULL;

    audec = (aml_audio_dec_t *)args;
    aout_ops = &audec->aout_ops;

    while (!audec->need_stop) {
        audec->state = INITING;
        ret = audio_codec_init(audec);
        if (ret == 0) {
            ret = aout_ops->init(audec);
            if (ret) {
                adec_print("Audio out device init failed!");
                audio_codec_release(audec);
                continue;
            }
            audec->state = INITTED;
            start_decode_thread(audec);
            start_adec(audec);
            break;
        }

	if(!audec->need_stop){
            usleep(100000);
	}
    }

    do {
        //if(message_pool_empty(audec))
        //{
        //adec_print("there is no message !\n");
        //  usleep(100000);
        //  continue;
        //}
        
        adec_reset_track(audec);
        adec_flag_check(audec);

        msg = adec_get_message(audec);
        if (!msg) {
            usleep(100000);
            continue;
        }

        switch (msg->ctrl_cmd) {
        case CMD_START:

            adec_print("Receive START Command!\n");
            start_decode_thread(audec);
            start_adec(audec);
            break;

        case CMD_PAUSE:

            adec_print("Receive PAUSE Command!");
            pause_adec(audec);
            break;

        case CMD_RESUME:

            adec_print("Receive RESUME Command!");
            resume_adec(audec);
            break;

        case CMD_STOP:

            adec_print("Receive STOP Command!");
            while(aout_stop_mutex)
            {
                usleep(100000);
            }
            aout_stop_mutex=1;
            stop_adec(audec);
            aout_stop_mutex=0;
            break;

        case CMD_MUTE:

            adec_print("Receive Mute Command!");
            if (msg->has_arg) {
                mute_adec(audec, msg->value.en);
            }
            break;

        case CMD_SET_VOL:

            adec_print("Receive Set Vol Command!");
            if (msg->has_arg) {
                adec_set_volume(audec, msg->value.volume);
            }
            break;
	 case CMD_SET_LRVOL:

            adec_print("Receive Set LRVol Command!");
            if (msg->has_arg) {
                adec_set_lrvolume(audec, msg->value.volume,msg->value_ext.volume);
            }
            break;	 	
		
        case CMD_CHANL_SWAP:

            adec_print("Receive Channels Swap Command!");
            audio_hardware_ctrl(HW_CHANNELS_SWAP);
            break;

        case CMD_LEFT_MONO:

            adec_print("Receive Left Mono Command!");
            audio_hardware_ctrl(HW_LEFT_CHANNEL_MONO);
            break;

        case CMD_RIGHT_MONO:

            adec_print("Receive Right Mono Command!");
            audio_hardware_ctrl(HW_RIGHT_CHANNEL_MONO);
            break;

        case CMD_STEREO:

            adec_print("Receive Stereo Command!");
            audio_hardware_ctrl(HW_STEREO_MODE);
            break;

        case CMD_RELEASE:

            adec_print("Receive RELEASE Command!");
            release_adec(audec);
            break;

        default:
            adec_print("Unknow Command!");
            break;

        }

        if (msg) {
            adec_message_free(msg);
            msg = NULL;
        }
    } while (audec->state != TERMINATED);

    adec_print("Exit Message Loop Thread!");
    pthread_exit(NULL);
    return NULL;
}

#endif
