#include "ztkconfigs.h"
#include "ap_startup.h"

/* for debug */
#define DEBUG_AP_STARTUP	1
#if DEBUG_AP_STARTUP
    #include "gplib.h"
    #define _dmsg(x)		print_string x
#else
    #define _dmsg(x)
#endif

/* */
#if C_LOGO == CUSTOM_ON
	static INT32S startup_logo_img_ptr;
	static INT32U startup_logo_decode_buff;
#endif

/* */
extern INT8U		audio_done;
extern VIDEO_ARGUMENT	gvarg;		// global video argument

void ap_startup_init(void)
{
	IMAGE_DECODE_STRUCT img_info;
	INT32U	size;
	INT16U	logo_fd;

#if C_LOGO == CUSTOM_ON
	ap_music_effect_resource_init();		//wwj add
	audio_vol_set(ap_state_config_volume_get());	//wwj add

	size = ((INT32U) gvarg.DisplayBufferWidth) * ((INT32U) gvarg.DisplayBufferHeight) * 2UL;
	startup_logo_decode_buff = (INT32U) gp_malloc_align(size, 64);
	if (!startup_logo_decode_buff) {
		DBG_PRINT("State startup allocate jpeg output buffer fail.\r\n");
		return;
	}

	logo_fd = nv_open((INT8U *) "POWER_ON_LOGO.JPG");
	if (logo_fd != 0xFFFF) {
		size = nv_rs_size_get(logo_fd);
		startup_logo_img_ptr = (INT32S) gp_malloc(size);
		if (!startup_logo_img_ptr) {
			DBG_PRINT("State startup allocate jpeg input buffer fail.[%d]\r\n", size);
			gp_free((void *) startup_logo_decode_buff);
			return;
		}
		if (nv_read(logo_fd, (INT32U) startup_logo_img_ptr, size)) {
			DBG_PRINT("Failed to read resource_header in ap_startup_init\r\n");
			gp_free((void *) startup_logo_img_ptr);
			gp_free((void *) startup_logo_decode_buff);
			return;
		}
		img_info.image_source		= (INT32S) startup_logo_img_ptr;
		img_info.source_size		= size;
		img_info.source_type		= TK_IMAGE_SOURCE_TYPE_BUFFER;
		img_info.output_format		= C_SCALER_CTRL_OUT_RGB565;
		img_info.output_ratio		= 0;
		img_info.out_of_boundary_color	= 0x008080;
		img_info.output_buffer_width	= gvarg.DisplayBufferWidth;
		img_info.output_buffer_height	= gvarg.DisplayBufferHeight;
		img_info.output_image_width	= gvarg.DisplayBufferWidth;
		img_info.output_image_height	= gvarg.DisplayBufferHeight;
		img_info.output_buffer_pointer	= startup_logo_decode_buff;
		if (jpeg_buffer_decode_and_scale(&img_info) == STATUS_FAIL) {
			gp_free((void *) startup_logo_img_ptr);
			gp_free((void *) startup_logo_decode_buff);
			DBG_PRINT("State startup decode jpeg file fail.\r\n");
			return;
		}
		OSQPost(DisplayTaskQ, (void *) (startup_logo_decode_buff|MSG_DISPLAY_TASK_JPEG_DRAW));
    #if MINI_DVR_BOARD_VERSION == GPL32680_MINI_DVR_CAR_RECORD_V2
		OSTimeDly(5);
		tft_backlight_en_set(TRUE);		
    #endif
		if (audio_effect_play(EFFECT_BEEP)) {
			audio_done++;
		}
		OSTimeDly(20);
		if (audio_effect_play(EFFECT_POWER_ON)) {
			audio_done++;
		}
		gp_free((void *) startup_logo_img_ptr);
		OSTimeDly(100);
	} else {
    #if MINI_DVR_BOARD_VERSION == GPL32680_MINI_DVR_CAR_RECORD_V2
		tft_backlight_en_set(TRUE);
    #endif
	}
	gp_free((void *) startup_logo_decode_buff);
#endif
	if (vid_dec_entry() < 0) {
		DBG_PRINT("Failed to init motion jpeg task\r\n");
	}
}

void ap_startup_exit(void)
{
	video_encode_entrance();
	video_encode_preview_start(gvarg);

#if DYNAMIC_QVALUE==0  // dominant add
	video_encode_set_jpeg_quality(QUALITY_FACTOR);
#endif
}
