#ifndef __VIDEO_CODEC_CALLBACK_H__
#define __VIDEO_CODEC_CALLBACK_H__
#include "application.h"
#include "drv_l1_sfr.h"

//video decode
extern void video_decode_end(void);
extern void video_decode_FrameReady(INT8U *FrameBufPtr);

//video encode
extern INT32U video_encode_sensor_start(INT32U csi_frame1, INT32U csi_frame2);
extern INT32U video_encode_sensor_stop(void);
extern INT32S video_encode_display_frame_ready(INT32U frame_buffer);
extern INT32S  video_encode_frame_ready(void *workmem, unsigned long fourcc, long cbLen, const void *ptr, int nSamples, int ChunkFlag);
extern void video_encode_end(void *workmem);

extern INT32U video_encode_audio_sfx(INT16U *PCM_Buf, INT32U cbLen);
extern INT32U video_encode_video_sfx(INT32U buf_addr, INT32U cbLen);

//display 
extern void user_defined_video_codec_entrance(void);
extern void user_defined_video_codec_isr(void);
extern void TFT_TD025THED1_Init(void);
extern void video_codec_show_image(INT8U TV_TFT, INT32U BUF,INT32U DISPLAY_MODE ,INT32U SHOW_TYPE);
extern void video_codec_ppu_configure(void);
extern void tft_vblank_isr_register(void (*user_isr)(void));
extern void tv_vblank_isr_register(void (*user_isr)(void));
#if C_MOTION_DETECTION == CUSTOM_ON
	extern void motion_detect_isr_register(void (*user_isr)(void));
#endif
#endif
