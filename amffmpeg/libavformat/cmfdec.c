/*
 * cmfdec io for ffmpeg system
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
 * amlogic:2012.6.29
 */

#include <limits.h>


#include "libavutil/intreadwrite.h"
#include "libavutil/avstring.h"
#include "libavutil/dict.h"
#include "avformat.h"
#include "avio_internal.h"
#include "riff.h"
#include "isom.h"
#include "libavcodec/get_bits.h"

#if CONFIG_ZLIB
#include <zlib.h>
#endif

#include "qtpalette.h"

#undef NDEBUG
#include <assert.h>

#include "cmfdec.h"
#ifdef DEBUG
#define TRACE()     av_log(NULL, AV_LOG_INFO, "%s---%d \n",__FUNCTION__,__LINE__);
#else
#define TRACE()
#endif

static int nestedprobe = 0;
static int cmf_probe(AVProbeData *p)
{
    if (nestedprobe) {
        return 0;
    }
    if (p && p->s  && p->s->opaque) {
        URLContext *s = p->s->opaque;
        if (!strncmp(s->prot->name, "cmf", 3)) {
            nestedprobe++;
            return 101;
        }
    }
    return 0;
}
static void cmf_reset_packet(AVPacket *pkt)
{
    av_init_packet(pkt);
    pkt->data = NULL;
}

static void cmf_flush_packet_queue(AVFormatContext *s)
{
    AVPacketList *pktl;
    for (;;) {
        pktl = s->packet_buffer;
        if (!pktl) { 
            break;
        }
        s->packet_buffer = pktl->next;
        av_free_packet(&pktl->pkt);
        av_free(pktl);
    }
    while (s->raw_packet_buffer) {
        pktl = s->raw_packet_buffer;
        s->raw_packet_buffer = pktl->next;
        av_free_packet(&pktl->pkt);
        av_free(pktl);
    }
    s->packet_buffer_end =
        s->raw_packet_buffer_end = NULL;
    s->raw_packet_buffer_remaining_size = RAW_PACKET_BUFFER_SIZE;
}
static int cmf_parser_next_slice(AVFormatContext *s, int index, int first)
{
    int i, j, ret;
    struct cmf *bs = s->priv_data;
    struct cmfvpb *oldvpb = bs->cmfvpb;
    struct cmfvpb *newvpb = NULL;
    AVInputFormat *in_fmt = NULL;
    av_log(s, AV_LOG_INFO, "cmf_parser_next_slice:%d\n", index);
    ret = cmfvpb_dup_pb(s->pb, &newvpb, index);
    if (ret) {
        return ret;
    }
    if (bs->sctx) {
        avformat_free_context(bs->sctx);
    }
    bs->sctx = NULL;
    if (!(bs->sctx = avformat_alloc_context())) {
        av_log(s, AV_LOG_INFO, "cmf_parser_next_slice avformat_alloc_context failed!\n");
        return AVERROR(ENOMEM);
    }
    ret = av_probe_input_buffer(newvpb->pb, &in_fmt, "", NULL, 0, 0);
    bs->sctx->pb = newvpb->pb;
    ret = avformat_open_input(&bs->sctx, "", NULL, NULL);
    if (ret < 0) {
        av_log(s, AV_LOG_INFO, "cmf_parser_next_slice:avformat_open_input failed \n");
        goto fail;
    }
    if (first) {
        /* Create new AVStreams for each stream in this chip
             Add into the Best format ;
        */
        for (j = 0; j < (int)bs->sctx->nb_streams ; j++)   {
            AVStream *st = av_new_stream(s, 0);
            if (!st) {
                ret = AVERROR(ENOMEM);
                goto fail;
            }
            avcodec_copy_context(st->codec, bs->sctx->streams[j]->codec);

        }

        for (i = 0; i < (int)s->nb_streams ; i++)   {
            AVStream *st = s->streams[i];
            AVStream *sst = bs->sctx->streams[i];
            if (st->codec->codec_type == CODEC_TYPE_AUDIO) {
                st->time_base.den =   sst->time_base.den  ;
                st->time_base.num = sst->time_base.num ;

            }
            if (st->codec->codec_type == CODEC_TYPE_VIDEO) {
                st->time_base.den = sst->time_base.den;
                st->time_base.num = sst->time_base.num;
            }
        }
    }
    av_log(s, AV_LOG_INFO, "seek ci->ctx->media_data_offset [%llx]\n", bs->sctx->media_dataoffset);
    avio_seek(bs->sctx->pb, bs->sctx->media_dataoffset, SEEK_SET);
    ret = 0;
    bs->cmfvpb = newvpb;
fail:
    if (oldvpb) {
        cmfvpb_pb_free(oldvpb);
    }
    return ret;
}
static int cmf_read_header(AVFormatContext *s, AVFormatParameters *ap)
{
    struct cmf *bs = s->priv_data;
    memset(bs, 0, sizeof(*bs));
    ap=ap;
    return cmf_parser_next_slice(s, 0, 1);

}
static int cmf_read_packet(AVFormatContext *s, AVPacket *mpkt)
{

    struct cmf *cmf = s->priv_data;
    int ret;
    struct cmfvpb *ci ;
    AVPacket *pkt = mpkt;
    AVStream *st_parent;
retry_read:
    ci = cmf->cmfvpb;
    ret = av_read_frame(cmf->sctx, pkt);
    if (ret < 0) {
        av_log(s, AV_LOG_INFO, " next slice parser  [%d]\n", ret);
        cmf_flush_packet_queue(cmf->sctx);
        cmf_reset_packet(pkt);
        cmf->parsering_index = cmf->parsering_index + 1;
        ret = cmf_parser_next_slice(s, cmf->parsering_index, 0);
        if (!ret) {
            goto retry_read;    //
        }
    }
    st_parent = s->streams[pkt->stream_index];
    if (st_parent->start_time == AV_NOPTS_VALUE) {
        st_parent->start_time  = pkt->pts;
        av_log(s, AV_LOG_INFO, "first packet st->start_time [0x%llx] [0x%llx]\n", st_parent->start_time, pkt->stream_index);
    }
    return 0;
}
static
int cmf_read_seek(AVFormatContext *s, int stream_index, int64_t sample_time, int flags)
{
    av_log(NULL, AV_LOG_INFO, " cmf_read_seek index=%d ,time=%lld,flags=%d\n", stream_index, sample_time, flags);
    struct cmf *bs = s->priv_data;
    struct cmfvpb *ci ;
    int ret;
    ci = bs->cmfvpb;
    ret = av_seek_frame(bs->sctx, stream_index, sample_time, flags);
    if (ret < 0) {
        av_log(s, AV_LOG_INFO, "cmf_read_seek:av_seek_frame Failed\n");
    }
    return 0;
}
static
int cmf_read_close(AVFormatContext *s)
{
    struct cmf *bs = s->priv_data;
    av_log(NULL, AV_LOG_INFO, " cmf_read_close \n");	
    nestedprobe = 0;
    if (bs && bs->sctx) {
        avformat_free_context(bs->sctx);
    }	
     cmfvpb_pb_free(bs->cmfvpb);
    return 0;
}

AVInputFormat ff_cmf_demuxer = {
    "cmf",
    NULL_IF_CONFIG_SMALL("cmf demux"),
    sizeof(struct cmf),
    cmf_probe,
    cmf_read_header,
    cmf_read_packet,
    cmf_read_close,
    cmf_read_seek,
};

