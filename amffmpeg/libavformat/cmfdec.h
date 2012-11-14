#ifndef AVFORMAT_CMFDEMUX_H
#define AVFORMAT_CMFDEMUX_H

#include "avio.h"
#include "internal.h"
#include "cmfvpb.h"




typedef struct cmf {
    struct cmfvpb *cmfvpb;
    AVFormatContext *sctx;
    AVPacket *pkt;
    int parsering_index;
    int chip_totalnum;
} cmf_t;




#endif






