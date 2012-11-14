#ifndef AVFORMAT_CMFDEMUXVPB_H
#define AVFORMAT_CMFDEMUXVPB_H

#include "avio.h"
#include "internal.h"
#include "dv.h"

typedef struct cmfvpb {
    uint8_t* read_buffer;
    int reading_flag;
    URLContext *input;
    AVIOContext *pb;
    int64_t start_offset;
    int64_t end_offset;
    int64_t size;
    int64_t seg_pos;
    uint32_t cur_index;
} cmfvpb;

#define INITIAL_BUFFER_SIZE 32768
int cmfvpb_dup_pb(AVIOContext *pb, struct cmfvpb **cmfvpb, int index);
int cmfvpb_pb_free(struct cmfvpb *ci);

#endif
