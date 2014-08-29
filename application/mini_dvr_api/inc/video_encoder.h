#ifndef __VIDEO_ENCODER_H__
#define __VIDEO_ENCODER_H__

#include "avi_encoder_app.h"

extern PPU_REGISTER_SETS p_register_set;
extern INT32S gplib_ppu_text_rotate_zoom_set(PPU_REGISTER_SETS *p_register_set, INT32U text_index, INT16S angle, FP32 factor_k);
extern INT8U ap_state_config_voice_record_switch_get(void);	//wwj add

void video_encode_frame_rate_set(INT8U fps);
INT8U video_encode_frame_rate_get(void);
void video_encode_entrance(void);
void video_encode_exit(void);
CODEC_START_STATUS video_encode_preview_start(VIDEO_ARGUMENT arg);
CODEC_START_STATUS video_encode_preview_stop(void);
CODEC_START_STATUS video_encode_start(MEDIA_SOURCE src);
CODEC_START_STATUS video_encode_stop(void);
CODEC_START_STATUS video_encode_Info(VIDEO_INFO *info);
VIDEO_CODEC_STATUS video_encode_status(void);
CODEC_START_STATUS video_encode_auto_switch_csi_frame(void);
CODEC_START_STATUS video_encode_auto_switch_csi_fifo_end(void);
CODEC_START_STATUS video_encode_auto_switch_csi_fifo_frame_end(void);
CODEC_START_STATUS video_encode_set_zoom_scaler(FP32 zoom_ratio);
CODEC_START_STATUS video_encode_pause(void);
CODEC_START_STATUS video_encode_resume(void);
CODEC_START_STATUS video_encode_set_jpeg_quality(INT8U quality_value);
INT32U video_encode_get_jpeg_quality(void);
CODEC_START_STATUS video_encode_capture_picture(MEDIA_SOURCE src);
CODEC_START_STATUS video_encode_fast_switch_stop_and_start(MEDIA_SOURCE src);
#endif