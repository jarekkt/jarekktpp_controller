/*!
    \file burst_rcv.h


*/


#ifndef BURST_RCV_H
#define BURST_RCV_H


#include "system.h"

typedef enum
{
	CH_DEBUG,
	CH_RS485_1,
	CH_RS485_2,
	CH_USB,
	CH_CNT
}ch_idx_e;

typedef enum
{
	RCV_FRAME_DIRECT = 0,
	RCV_FRAME_ENCAPSULATED
}rcv_frame_e;


typedef struct
{
	ch_idx_e 		channel;
	rcv_frame_e		frame_format;
}burst_rcv_ctx_t;

void burst_rcv_init();
void burst_rcv_once();
void burst_rcv_send_response(const burst_rcv_ctx_t * rcv_ctx,char * response, int length);


#endif //BURST_RCV_H
