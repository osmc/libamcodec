/*
 * cmfvpb  io for ffmpeg system
 * Copyright (c) 2001 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 *cmf:combine file,mutifiles extractor;
 *cmfvpb:virtual pb for cmf dec
 * amlogic:2012.6.29
 */


#include "libavutil/intreadwrite.h"
#include "libavutil/avstring.h"
#include "libavutil/dict.h"
#include "avformat.h"
#include "avio_internal.h"
#include "riff.h"
#include "isom.h"
#include "libavcodec/get_bits.h"
#include "avio.h"

#include "cmfvpb.h"

static int cmfvpb_open(struct cmfvpb *cv , int index)
{
    int ret = 0;
    if (!cv ||  !cv->input || !cv->input->prot->url_seek) {
        av_log(NULL, AV_LOG_INFO, "cmfvpb_open is NULL\n");
        return -1;
    }
    ret = cv->input->prot->url_seek(cv->input, (int64_t)index, AVSEEK_SLICE_INDEX);//base open;
    ret = cv->input->prot->url_getinfo(cv->input, AVCMD_SLICE_START_OFFSET, 0, &cv->start_offset); //base open;
    ret = cv->input->prot->url_getinfo(cv->input, AVCMD_SLICE_SIZE, 0 , &cv->size);
    cv->end_offset = cv->start_offset + cv->size;
    cv->seg_pos = 0;
    cv->cur_index = index;
    av_log(NULL, AV_LOG_INFO, "cmfvpb_open [%d] start_offset [%llx] end_offset [%llx] slice_size[%llx] \n", cv->cur_index, cv->start_offset, cv->end_offset, cv->size);

    return ret;
}

static int cmfvpb_read(void *v, uint8_t *buf, int buf_size)
{
    int len, rsize;
    struct cmfvpb *cv=v;	
	
    rsize = FFMIN(buf_size, cv->size -cv->seg_pos);
    len = ffurl_read(cv->input , buf, buf_size);
    if (len < 0) {
        av_log(NULL, AV_LOG_INFO, "cmfvpb_read failed\n");
        return -1;
    }
    cv->seg_pos += len;
    return len;
}

static int64_t cmfvpb_seek(void*v, int64_t offset, int whence)
{
    int64_t lowoffset=0;
    struct cmfvpb *cv=v;	
	
    if (!cv || ! cv->input   || !cv->input->prot->url_seek) {
        return -1;
    }
    av_log(NULL, AV_LOG_INFO, "cmfvpb_seek cv->index [%d] offset[%lld] whence[%d] cv->size[%lld]\n", cv->cur_index, offset, whence,cv->size);
    if (whence == AVSEEK_SIZE) {
        if (cv->size <= 0) {
            av_log(NULL, AV_LOG_INFO, " cv->size error sieze=%lld\n", cv->size);
        }
        return cv->size;
    } else if (whence == SEEK_END) {
    		lowoffset=cv->end_offset+offset;
    } else if (whence == SEEK_SET) {
        	lowoffset=cv->start_offset+offset;
    } else if (whence == SEEK_CUR) {
       	lowoffset=cv->start_offset+cv->seg_pos+offset;
    } else {
        av_log(NULL, AV_LOG_INFO, " un support seek cmd else whence  [%x]\n",whence);
	 return -1;
    }
    lowoffset=cv->input->prot->url_seek(cv->input,lowoffset, SEEK_SET);
    cv->seg_pos=lowoffset-cv->start_offset;
    return cv->seg_pos;
}

int cmfvpb_dup_pb(AVIOContext *pb, struct cmfvpb **cmfvpb, int index)
{
    struct cmfvpb *cv ;
    AVIOContext *vpb;

    cv = av_mallocz(sizeof(struct cmfvpb));
    cv->input = pb->opaque;
    cmfvpb_open(cv, index);
    cv->read_buffer = av_malloc(INITIAL_BUFFER_SIZE);
    if (!cv->read_buffer) {
        av_free(cv);
        return AVERROR(ENOMEM);
    }
    vpb  = av_mallocz(sizeof(AVIOContext));
    if (!vpb) {
        av_free(cv->read_buffer);
        av_free(cv);
        return AVERROR(ENOMEM);
    }
    memcpy(vpb, pb, sizeof(*pb));

    ffio_init_context(vpb, cv->read_buffer, INITIAL_BUFFER_SIZE, 0, cv,
                      cmfvpb_read, NULL, cmfvpb_seek);
    if (pb->read_packet || pb->seek) {
        ffio_init_context(pb, pb->buffer, pb->buffer_size, 0, pb->opaque,
                          NULL, NULL, NULL);/*reset old pb*/
    }
    cv->pb = vpb;
    *cmfvpb = cv;
    return 0;
}

int cmfvpb_pb_free(struct cmfvpb *cv)
{
    if (!cv->read_buffer) {
        av_free(cv->read_buffer);
    }
    av_free(cv->pb);
    av_free(cv);
    return 0;
}

