#include "ztkconfigs.h"
#include "ap_display.h"

/* for debug */
#define DEBUG_AP_DISPLAY	0
#if DEBUG_AP_DISPLAY
    #include "gplib.h"
    #define _dmsg(x)		print_string x
#else
    #define _dmsg(x)
#endif

/* variables */
static INT8U *display_frame[TASK_DISPALY_BUFFER_NO];
static INT32U display_dma_queue[TASK_DISPALY_BUFFER_NO];
static INT32U display_isr_queue[TASK_DISPALY_BUFFER_NO];
static INT32U display_icon_sts[(ICOM_MAX_NUM >> 5) + 1];

static INT32U display_os_time;
static INT32U last_os_time;				//wwj add
static INT32U display_os_time_browser;	//wwj add

static INT32U display_effect_buff;
static INT32U display_setting_buff;
static STR_ICON display_str;

static DISPLAY_REC_TIME display_rec_time;
static DISPLAY_REC_TIME display_total_time;	//wwj add

static INT16U display_effect_offset;
static INT8U display_effect_sts;
static INT8U video_preview_timerid;
static INT8U display_mp3_index_sts;
static INT8U display_mp3_input_num_sts;

static INT8U timer_force_en = 0;
static INT8U timer_force_en_browser = 0;

static INT32U display_left_rec_time = 0;
static INT32U browse_curr_time;
static INT32S display_browser_total_time;

INT8U display_mp3_volume;
INT16U display_mp3_play_index;
INT16U display_mp3_total_index;
INT16U display_mp3_FM_channel;
INT16U display_mp3_input_num;

extern PPU_REGISTER_SETS p_register_set;

#define PLAY_INDEX_X		 56
#define PLAY_TOTAL_INDEX_X	120
#define PLAY_INDEX_Y		 200//34
#define MP3_VOLUME_X		 74+32+6
#define MP3_VOLUME_Y		 200-32//34+32
#define FM_CHANNEL_X		 10+14+14+14+6+14-4
#define FM_CHANNEL_Y		 168-28//66+32
#define MP3_INPUT_NUM_X		 260
#define MP3_INPUT_NUM_Y		 140
void ap_display_mp3_index_sts_set(INT8U type);
void ap_display_mp3_input_num_sts_set(INT8U type);
void ap_display_left_rec_time_draw(INT32U time, INT8U flag);

static DISPLAY_ICONSHOW display_icon_item[ICOM_MAX_NUM] = {
	{NULL, NULL, NULL, NULL, NULL},
	{ 32,  32, TRANSPARENT_COLOR,  /*10*/8,  1},	// ICON_BATTERY_RED	//wwj modify
	{ 32,  32, TRANSPARENT_COLOR,  /*10*/8,  1},	// ICON_BATTERY_1	//wwj modify
	{ 32,  32, TRANSPARENT_COLOR,  /*10*/8,  1},	// ICON_BATTERY_2	//wwj modify
	{ 32,  32, TRANSPARENT_COLOR,  /*10*/8,  1},	// ICON_BATTERY_3	//wwj modify
	{ 32,  32, TRANSPARENT_COLOR, 280,   0},	// ICON_CAPTURE
	{ 32,  32, TRANSPARENT_COLOR, 280,   0},	// ICON_VIDEO_RECORD
	{ 32,  32, TRANSPARENT_COLOR, 280,   0},	// ICON_PLAYBACK
	{ 32,  32, TRANSPARENT_COLOR,  /*52*/48,   0},	// ICON_MOTION_DETECT	//wwj modify
	{ 32,  32, TRANSPARENT_COLOR, 204,   0},	// ICON_REC
	{ 32,  32, TRANSPARENT_COLOR,  /*10*/0, 188/*210*/},	// ICON_PLAY	//wwj modify
	{ 32,  32, TRANSPARENT_COLOR,  /*10*/0, 188/*210*/},	// ICON_PAUSE	//wwj modify
	{ 32,  32, TRANSPARENT_COLOR,  128, 2/*10, 210*/},	// ICON_MIC_OFF		//wwj modify
	{ 32,  32, TRANSPARENT_COLOR, 130,  10},	// ICON_IR_ON
	{ 64,  64, TRANSPARENT_COLOR, 128,  88},	// ICON_FOCUS
	{ 32,  32, TRANSPARENT_COLOR, 280,   0},	// ICON_VIDEO
	{ 32,  32, TRANSPARENT_COLOR, /*166*/168,  0},	// ICON_NO_SD_CARD	//wwj modify
	{ 32,  32, TRANSPARENT_COLOR,  /*52*/48,   0},	// ICON_MOTION_DETECT_START	//wwj modify
	{256, 128, TRANSPARENT_COLOR,  32, 	40},	// ICON_USB_CONNECT
	{ 16,  96, TRANSPARENT_COLOR,  10, 	72},	// ICON_SCROLL_BAR
	{  8,   8, TRANSPARENT_COLOR,  14, 149},	// ICON_SCROLL_BAR_IDX
	{  8,  88, TRANSPARENT_COLOR, 290,  80},	// ICON_MD_STS_0
	{  8,  88, TRANSPARENT_COLOR, 290,  80},	// ICON_MD_STS_1
	{  8,  88, TRANSPARENT_COLOR, 290,  80},	// ICON_MD_STS_2
	{  8,  88, TRANSPARENT_COLOR, 290,  80},	// ICON_MD_STS_3
	{  8,  88, TRANSPARENT_COLOR, 290,  80},	// ICON_MD_STS_4
	{  8,  88, TRANSPARENT_COLOR, 290,  80},	// ICON_MD_STS_5
	{ 32,  32, TRANSPARENT_COLOR,  10,  168},	// ICON_MP3_PLAY
	{ 32,  32, TRANSPARENT_COLOR,  10,  168},	// ICON_MP3_PAUSE
	{ 32,  32, TRANSPARENT_COLOR,  42,  168},	// ICON_MP3_PLAY_ONE
	{ 32,  32, TRANSPARENT_COLOR,  42,  168},	// ICON_MP3_PLAY_ALL
	{128,  32, TRANSPARENT_COLOR,  10,  200},	// ICON_MP3_INDEX
	{ 32,  32, TRANSPARENT_COLOR,  74,  168},	// ICON_MP3_VOLUME
	{ 32,  32, TRANSPARENT_COLOR,  74,  168}, 	// ICON_MP3_MUTE
	{ 32,  32, TRANSPARENT_COLOR,  10,  168}, 	// ICON_MP3_STOP
	{ 32,  32, TRANSPARENT_COLOR,  /*10*/8,  1/*11*/},	// ICON_BATTERY_CHARGED	//wwj modify
	{ 32,  32, TRANSPARENT_COLOR, 242,   0},		// ICON_LOCKED
	{ 32,  32, TRANSPARENT_COLOR, 242,   0},		// ICON_VGA
	{ 32,  32, TRANSPARENT_COLOR,  /*10, 210*/88, 1},	// ICON_NIGHT_MODE_ENABLED	//wwj modify
	{ 32,  32, TRANSPARENT_COLOR,  /*10, 210*/88, 1},	// ICON_NIGHT_MODE_DISABLED	//wwj modify
	{ 32,  32, TRANSPARENT_COLOR, 280,   0}		// ICON_AUDIO_RECORD
};

static DISPLAY_ICONSHOW display_num_item[NUM_MAX] = {
	{ 16,  32, TRANSPARENT_COLOR, NULL,  NULL},	// NUM_TEXT_0
	{ 16,  32, TRANSPARENT_COLOR, NULL,  NULL},	// NUM_TEXT_1
	{ 16,  32, TRANSPARENT_COLOR, NULL,  NULL},	// NUM_TEXT_2
	{ 16,  32, TRANSPARENT_COLOR, NULL,  NULL},	// NUM_TEXT_3
	{ 16,  32, TRANSPARENT_COLOR, NULL,  NULL},	// NUM_TEXT_4
	{ 16,  32, TRANSPARENT_COLOR, NULL,  NULL},	// NUM_TEXT_5
	{ 16,  32, TRANSPARENT_COLOR, NULL,  NULL},	// NUM_TEXT_6
	{ 16,  32, TRANSPARENT_COLOR, NULL,  NULL},	// NUM_TEXT_7
	{ 16,  32, TRANSPARENT_COLOR, NULL,  NULL},	// NUM_TEXT_8
	{ 16,  32, TRANSPARENT_COLOR, NULL,  NULL},	// NUM_TEXT_9
	{  8,  32, TRANSPARENT_COLOR, NULL,  NULL},	// NUM_TEXT_COLON
	{  8,  32, TRANSPARENT_COLOR, NULL,  NULL}	// NUM_TEXT_SLASH
};

const INT16U *icon_table[] = {
	NULL,
	icon_battery_red,
	icon_battery1,
	icon_battery2,
	icon_battery3,
	icon_capture,
	icon_video,
	icon_review,
	icon_motion_detect,
	icon_red_light,
	icon_play,
	icon_pause,
	icon_mic_off,
	icon_ir_on,
	icon_focus,
	icon_video1,
	icon_no_sd_card,
	icon_motion_detect_start,
	icon_usb_connect,
	icon_scroll_bar,
	icon_scroll_bar_idx,
	icon_md_sts0,
	icon_md_sts1,
	icon_md_sts2,
	icon_md_sts3,
	icon_md_sts4,
	icon_md_sts5,
	icon_mp3_play,
	icon_mp3_pause,
	icon_mp3_play_one,
	icon_mp3_play_all,
	icon_mp3_index,
	icon_mp3_volume,
	icon_mp3_mute,
	icon_mp3_stop,
	icon_battery_charged,
	icon_locked,
	icon_VGA,
	icon_night_mode_enabled,
	icon_night_mode_disabled,
	icon_audio_record,
};

const INT16U *number_table[] = {
	ui_text_0,
	ui_text_1,
	ui_text_2,
	ui_text_3,
	ui_text_4,
	ui_text_5,
	ui_text_6,
	ui_text_7,
	ui_text_8,
	ui_text_9,
	ui_text_colon,
	ui_text_klino
};

const INT16U *mp3_index_table[] = {
	mp3_index_0,
	mp3_index_1,
	mp3_index_2,
	mp3_index_3,
	mp3_index_4,
	mp3_index_5,
	mp3_index_6,
	mp3_index_7,
	mp3_index_8,
	mp3_index_9,
	mp3_index_dot,
	mp3_index_MHz
};

const INT16U *mp3_input_number[] = {
	mp3_input_0,
	mp3_input_1,
	mp3_input_2,
	mp3_input_3,
	mp3_input_4,
	mp3_input_5,
	mp3_input_6,
	mp3_input_7,
	mp3_input_8,
	mp3_input_9
};

//	prototypes
void ap_display_draw(INT16U *frame_buff);
void ap_display_icon_draw(INT16U *frame_buff, INT16U *icon_stream, DISPLAY_ICONSHOW *icon_info);
void ap_display_rec_time_draw(INT16U *frame_buff, void *time_ptr, INT16U pos_x, INT16U pos_y, INT8U red_flag);
void ap_display_vblank_isr(void);
void ap_display_effect_draw(INT32U buff_addr);
INT32U ap_display_queue_get(INT32U *queue);
INT32S  ap_display_queue_put(INT32U *queue, INT32U data);
#if 0
static INT32S display_device_protect(void);
static void display_device_unprotect(INT32S mask);
#endif
extern INT32S dma_buffer_copy(INT32U s_addr, INT32U t_addr, INT32U byte_count, INT32U s_width, INT32U t_width);

/* global video argument */
extern VIDEO_ARGUMENT	gvarg;

void ap_display_init(void)
{
	INT32U	i, buff_size, *buff_ptr;
	
	video_preview_timerid = 0xFF;
	buff_size = ((INT32U) gvarg.DisplayBufferWidth) * ((INT32U) gvarg.DisplayBufferHeight) * 2UL;
//	buff_size = TFT_WIDTH * TFT_HEIGHT * 2;
	_dmsg((RED "[GG]: mm_dump() - buffer_size=%0d\r\n" NONE, buff_size, mm_dump()));

// ### for memory issue - by xyz 2016.01.27 ##################################
#if (zt_resolution() < ZT_HD_SCALED)
	display_frame[0] = gp_malloc_align(buff_size*TASK_DISPALY_BUFFER_NO, 64);
#else
	display_frame[0] = gp_malloc_align(buff_size*(TASK_DISPALY_BUFFER_NO+1), 64);
#endif
// ### for memory issue - by xyz 2016.01.27 ##################################

	if (display_frame[0]) {
		for (i=1; i<TASK_DISPALY_BUFFER_NO; i++) {
			display_frame[i] = display_frame[i-1] + buff_size;
		}
	} else {
		DBG_PRINT("display_frame[0].buff_ptr allocate fail\r\n");
	}	
	gplib_ppu_init(&p_register_set);
	user_defined_video_codec_entrance();
	tft_vblank_isr_register(ap_display_vblank_isr);
	tv_vblank_isr_register(ap_display_vblank_isr);
	R_PPU_IRQ_EN  |= 0x00002000;		// Enable V-blank interrupt.
//	R_PPU_ENABLE  |= (PPU_FRAME_BASE_MODE|PPU_RGB565_MODE|PPU_RGBG_MODE);
	R_PPU_ENABLE  |= (PPU_QVGA_MODE|TFT_SIZE_320X240|PPU_FRAME_BASE_MODE|PPU_RGB565_MODE);
	R_FREE_SIZE    =  (INT32U) gvarg.DisplayBufferHeight;		// Vertical
	R_FREE_SIZE   |= ((INT32U) gvarg.DisplayBufferWidth) << 16;	// Horizontal
//	R_FREE_SIZE    = TFT_HEIGHT;		// Vertical
//	R_FREE_SIZE   |= TFT_WIDTH << 16;	// Horizontal
	buff_size    >>= 2;
	buff_ptr       = (INT32U *) display_frame[TASK_DISPALY_BUFFER_NO - 1];	
	R_TFT_FBI_ADDR = (INT32U) display_frame[TASK_DISPALY_BUFFER_NO - 1];
	for (i=0; i<buff_size;  i++) {
		*buff_ptr++ = 0;
	}	
	for (i=0; i<TASK_DISPALY_BUFFER_NO - 1; i++) {
		if (ap_display_queue_put(display_isr_queue, (INT32U) display_frame[i]) == STATUS_FAIL) {
			DBG_PRINT("INIT put Q FAIL.\r\n");
		}
	}
	tft_tft_en_set(TRUE);

	_dmsg((RED "[GG]: mm_dump() - 2\r\n" NONE, mm_dump()));
}

void ap_display_queue_init(void)
{
	INT32U i;
	
	for (i=0 ; i<(ICOM_MAX_NUM >> 5) + 1 ; i++) {
		display_icon_sts[i] = NULL;
	}
	if (display_effect_buff) {
		gp_free((void *) display_effect_buff);
		display_effect_buff = NULL;
	}
	display_effect_sts = DISPALY_NONE_EFFECT;
	display_mp3_index_sts = DISPALY_MP3_INDEX_NONE_EFFECT;
	display_mp3_input_num_sts = DISPALY_MP3_INPUT_NUM_NONE_EFFECT;
}

void ap_display_scaler_buff_ack(void)
{
	INT32U i;
	
	for (i=0 ; i<TASK_DISPALY_BUFFER_NO ; i++) {
		OSQPost(scaler_task_q, (void *) (MSG_SCALER_TASK_PREVIEW_ON | (INT32U) display_frame[i]));
	}
}

void ap_display_setting_frame_buff_set(INT32U frame_buff)
{
	display_setting_buff = frame_buff;
}

void ap_display_icon_move(DISPLAY_ICONMOVE *icon)
{
	if (icon->idx < ICOM_MAX_NUM) {
		if (icon->pos_x == 0xFFFF && icon->pos_y == 0xFFFF) {
			if (icon->idx == ICON_SCROLL_BAR_IDX) {
				display_icon_item[icon->idx].pos_y = 80;
			}
		} else if (icon->pos_x == 0xFFFF) {
			display_icon_item[icon->idx].pos_y = icon->pos_y;
		} else if (icon->pos_y == 0xFFFF) {
			display_icon_item[icon->idx].pos_x = icon->pos_x;
		} else {
			display_icon_item[icon->idx].pos_x = icon->pos_x;
			display_icon_item[icon->idx].pos_y = icon->pos_y;
		}
	} else {
		return;
	}
}

void ap_display_draw(INT16U *frame_buff)
{
	INT32U i, x, y, offset_pixel, offset_pixel_tmp, offset_data, offset_data_tmp;

	for (i=0 ; i<ICOM_MAX_NUM ; i++) 
	{
		if( display_icon_sts[i >> 5] & (1<<(i&0x1F)) ) 
		{
			if( (i != ICON_REC) || ((i == ICON_REC) && !(display_rec_time.s & 0x1)) )	//make the ICON_REC Blink!
			{
				for (y=0 ; y<display_icon_item[i].icon_h ; y++) 
				{
					offset_data_tmp = y*display_icon_item[i].icon_w;
					offset_pixel_tmp = (display_icon_item[i].pos_y + y)*TFT_WIDTH + display_icon_item[i].pos_x;
					for (x=0 ; x<display_icon_item[i].icon_w ; x++) 
					{
						offset_data = offset_data_tmp + x;
						if (*(icon_table[i] + offset_data) == display_icon_item[i].transparent) {
							continue;
						}
						offset_pixel = offset_pixel_tmp + x;
						if (offset_pixel <= TFT_WIDTH*TFT_HEIGHT) {
							*(frame_buff + offset_pixel) = *(icon_table[i] + offset_data);
						}
					}
				}
			}
		}
	}
}

void ap_display_icon_draw(INT16U *frame_buff, INT16U *icon_stream, DISPLAY_ICONSHOW *icon_info)
{
	INT32U x, y, offset_pixel, offset_pixel_tmp, offset_data_tmp;
	INT16U *icon_stream_data, *frame_buff_data;
	
	icon_stream_data = 0;
	for (y=0 ; y<icon_info->icon_h ; y++) {
		offset_data_tmp = y*icon_info->icon_w;
		offset_pixel_tmp = (icon_info->pos_y + y)*TFT_WIDTH + icon_info->pos_x;
		for (x=0 ; x<icon_info->icon_w ; x++) {
			icon_stream_data = offset_data_tmp + x + icon_stream;
			if (*icon_stream_data == icon_info->transparent) {
				continue;
			} else if (*icon_stream_data == 0x7BCF) {
				frame_buff_data = frame_buff + offset_pixel_tmp + x;
				*frame_buff_data &= 0xE79C;
				*frame_buff_data >>= 2;
				continue;
			}
			offset_pixel = offset_pixel_tmp + x;
			if (offset_pixel <= TFT_WIDTH*TFT_HEIGHT) {
				*(frame_buff + offset_pixel) = *icon_stream_data;
			}
		}
	}
}

void ap_display_vblank_isr(void)
{
	INT32U buff_get, tft_buff;
	
	buff_get = ap_display_queue_get(display_dma_queue);
	if (buff_get) {
		tft_buff = R_TFT_FBI_ADDR;
		
		if (tft_buff != buff_get) {
			R_TFT_FBI_ADDR = buff_get;
			R_TV_FBI_ADDR = buff_get;
			OSQPost(scaler_task_q, (void *) (tft_buff|MSG_SCALER_TASK_PREVIEW_UNLOCK));
		} else {
			OSQPost(scaler_task_q, (void *) (0xFFFFFF|MSG_SCALER_TASK_PREVIEW_UNLOCK));
		}
		ap_display_queue_put(display_isr_queue, (INT32U) tft_buff);
	}
}

#if 0

extern INT16U ad_value_bak;
extern INT16U ad_18_value_bak;

INT8U motion_cnt=0;
INT8U motion_cnt2=0;
INT8U motion_cnt3=0;
void ap_display_motion_draw(INT16U *frame_buff, INT16U pos_x, INT16U pos_y)
{
	INT32U x, y, offset_pixel, offset_data;
	INT16U num_tmp0, num_tmp1, num_tmp2, num_tmp3;

	num_tmp0 = ad_value_bak;
	
	num_tmp1 = num_tmp0/10;
	num_tmp0 -= num_tmp1*10;
	
	num_tmp2 = num_tmp1/10;
	num_tmp1 -= num_tmp2*10;
	
	num_tmp3 = num_tmp2/10;
	num_tmp2 -= num_tmp3*10; 

	for (y=0 ; y<display_num_item[num_tmp3].icon_h ; y++) {
		for (x=0 ; x<display_num_item[num_tmp3].icon_w ; x++) {
			offset_data = (y*display_num_item[num_tmp3].icon_w) + x;
			if (*(number_table[num_tmp3] + offset_data) == display_num_item[num_tmp3].transparent) {
				continue;
			}
			offset_pixel =  (pos_y + y)*TFT_WIDTH + x + pos_x;
			if (offset_pixel <= TFT_WIDTH*TFT_HEIGHT) {
				*(frame_buff + offset_pixel) = *(number_table[num_tmp3] + offset_data);
			}
		}
	}
	pos_x += display_num_item[num_tmp3].icon_w;
	
	for (y=0 ; y<display_num_item[num_tmp2].icon_h ; y++) {
		for (x=0 ; x<display_num_item[num_tmp2].icon_w ; x++) {
			offset_data = (y*display_num_item[num_tmp2].icon_w) + x;
			if (*(number_table[num_tmp2] + offset_data) == display_num_item[num_tmp2].transparent) {
				continue;
			}
			offset_pixel =  (pos_y + y)*TFT_WIDTH + x + pos_x;
			if (offset_pixel <= TFT_WIDTH*TFT_HEIGHT) {
				*(frame_buff + offset_pixel) = *(number_table[num_tmp2] + offset_data);
			}
		}
	}
	pos_x += display_num_item[num_tmp2].icon_w;

	for (y=0 ; y<display_num_item[num_tmp1].icon_h ; y++) {
		for (x=0 ; x<display_num_item[num_tmp1].icon_w ; x++) {
			offset_data = (y*display_num_item[num_tmp1].icon_w) + x;
			if (*(number_table[num_tmp1] + offset_data) == display_num_item[num_tmp1].transparent) {
				continue;
			}
			offset_pixel =  (pos_y + y)*TFT_WIDTH + x + pos_x;
			if (offset_pixel <= TFT_WIDTH*TFT_HEIGHT) {
				*(frame_buff + offset_pixel) = *(number_table[num_tmp1] + offset_data);
			}
		}
	}
	pos_x += display_num_item[num_tmp1].icon_w;

	for (y=0 ; y<display_num_item[num_tmp0].icon_h ; y++) {
		for (x=0 ; x<display_num_item[num_tmp0].icon_w ; x++) {
			offset_data = (y*display_num_item[num_tmp0].icon_w) + x;
			if (*(number_table[num_tmp0] + offset_data) == display_num_item[num_tmp0].transparent) {
				continue;
			}
			offset_pixel =  (pos_y + y)*TFT_WIDTH + x + pos_x;
			if (offset_pixel <= TFT_WIDTH*TFT_HEIGHT) {
				*(frame_buff + offset_pixel) = *(number_table[num_tmp0] + offset_data);
			}
		}
	}
	
	pos_x += 32;
	num_tmp0 = ad_18_value_bak;
	
	num_tmp1 = num_tmp0/10;
	num_tmp0 -= num_tmp1*10;
	
	num_tmp2 = num_tmp1/10;
	num_tmp1 -= num_tmp2*10;
	
	num_tmp3 = num_tmp2/10;
	num_tmp2 -= num_tmp3*10;
	
	for (y=0 ; y<display_num_item[num_tmp3].icon_h ; y++) {
		for (x=0 ; x<display_num_item[num_tmp3].icon_w ; x++) {
			offset_data = (y*display_num_item[num_tmp3].icon_w) + x;
			if (*(number_table[num_tmp3] + offset_data) == display_num_item[num_tmp3].transparent) {
				continue;
			}
			offset_pixel =  (pos_y + y)*TFT_WIDTH + x + pos_x;
			if (offset_pixel <= TFT_WIDTH*TFT_HEIGHT) {
				*(frame_buff + offset_pixel) = *(number_table[num_tmp3] + offset_data);
			}
		}
	}
	
	pos_x += display_num_item[num_tmp3].icon_w;	
	
	for (y=0 ; y<display_num_item[num_tmp2].icon_h ; y++) {
		for (x=0 ; x<display_num_item[num_tmp2].icon_w ; x++) {
			offset_data = (y*display_num_item[num_tmp2].icon_w) + x;
			if (*(number_table[num_tmp2] + offset_data) == display_num_item[num_tmp2].transparent) {
				continue;
			}
			offset_pixel =  (pos_y + y)*TFT_WIDTH + x + pos_x;
			if (offset_pixel <= TFT_WIDTH*TFT_HEIGHT) {
				*(frame_buff + offset_pixel) = *(number_table[num_tmp2] + offset_data);
			}
		}
	}
	pos_x += display_num_item[num_tmp2].icon_w;

	for (y=0 ; y<display_num_item[num_tmp1].icon_h ; y++) {
		for (x=0 ; x<display_num_item[num_tmp1].icon_w ; x++) {
			offset_data = (y*display_num_item[num_tmp1].icon_w) + x;
			if (*(number_table[num_tmp1] + offset_data) == display_num_item[num_tmp1].transparent) {
				continue;
			}
			offset_pixel =  (pos_y + y)*TFT_WIDTH + x + pos_x;
			if (offset_pixel <= TFT_WIDTH*TFT_HEIGHT) {
				*(frame_buff + offset_pixel) = *(number_table[num_tmp1] + offset_data);
			}
		}
	}
	pos_x += display_num_item[num_tmp1].icon_w;

	for (y=0 ; y<display_num_item[num_tmp0].icon_h ; y++) {
		for (x=0 ; x<display_num_item[num_tmp0].icon_w ; x++) {
			offset_data = (y*display_num_item[num_tmp0].icon_w) + x;
			if (*(number_table[num_tmp0] + offset_data) == display_num_item[num_tmp0].transparent) {
				continue;
			}
			offset_pixel =  (pos_y + y)*TFT_WIDTH + x + pos_x;
			if (offset_pixel <= TFT_WIDTH*TFT_HEIGHT) {
				*(frame_buff + offset_pixel) = *(number_table[num_tmp0] + offset_data);
			}
		}
	}
	
/*	pos_x += 32;
	num_tmp0 = motion_cnt3;
	num_tmp1 = num_tmp0/10;
	num_tmp0 -= num_tmp1*10;
	num_tmp2 = num_tmp1/10;
	num_tmp1 -= num_tmp2*10;
	for (y=0 ; y<display_num_item[num_tmp2].icon_h ; y++) {
		for (x=0 ; x<display_num_item[num_tmp2].icon_w ; x++) {
			offset_data = (y*display_num_item[num_tmp2].icon_w) + x;
			if (*(number_table[num_tmp2] + offset_data) == display_num_item[num_tmp2].transparent) {
				continue;
			}
			offset_pixel =  (pos_y + y)*TFT_WIDTH + x + pos_x;
			if (offset_pixel <= TFT_WIDTH*TFT_HEIGHT) {
				*(frame_buff + offset_pixel) = *(number_table[num_tmp2] + offset_data);
			}
		}
	}
	pos_x += display_num_item[num_tmp2].icon_w;

	for (y=0 ; y<display_num_item[num_tmp1].icon_h ; y++) {
		for (x=0 ; x<display_num_item[num_tmp1].icon_w ; x++) {
			offset_data = (y*display_num_item[num_tmp1].icon_w) + x;
			if (*(number_table[num_tmp1] + offset_data) == display_num_item[num_tmp1].transparent) {
				continue;
			}
			offset_pixel =  (pos_y + y)*TFT_WIDTH + x + pos_x;
			if (offset_pixel <= TFT_WIDTH*TFT_HEIGHT) {
				*(frame_buff + offset_pixel) = *(number_table[num_tmp1] + offset_data);
			}
		}
	}
	pos_x += display_num_item[num_tmp1].icon_w;

	for (y=0 ; y<display_num_item[num_tmp0].icon_h ; y++) {
		for (x=0 ; x<display_num_item[num_tmp0].icon_w ; x++) {
			offset_data = (y*display_num_item[num_tmp0].icon_w) + x;
			if (*(number_table[num_tmp0] + offset_data) == display_num_item[num_tmp0].transparent) {
				continue;
			}
			offset_pixel =  (pos_y + y)*TFT_WIDTH + x + pos_x;
			if (offset_pixel <= TFT_WIDTH*TFT_HEIGHT) {
				*(frame_buff + offset_pixel) = *(number_table[num_tmp0] + offset_data);
			}
		}
	}	
*/		
}

#endif

void ap_display_buff_copy_and_draw(INT32U buff_addr, INT16U src_type)
{
	INT32U buff_size, buff_get;
	//INT32S mask;
	
	buff_size = (TFT_WIDTH * TFT_HEIGHT) << 1;
	//mask = display_device_protect();
	if (src_type == DISPALY_BUFF_SRC_SENSOR) {
		if (!ap_display_queue_get(display_isr_queue)) {
			//display_device_unprotect(mask);
			return;
		}
		
		buff_get = buff_addr;
	} else {
		buff_get = ap_display_queue_get(display_isr_queue);
	}
	//display_device_unprotect(mask);
	
	if (buff_get) {
		
		if (src_type != DISPALY_BUFF_SRC_SENSOR) {
			dma_buffer_copy(buff_addr, buff_get, buff_size, 0, 0);
		}
		if (!display_setting_buff) {
			if ( (display_effect_sts != DISPALY_NONE_EFFECT) || (display_mp3_index_sts != DISPALY_MP3_INDEX_NONE_EFFECT ) || (display_mp3_input_num_sts != DISPALY_MP3_INPUT_NUM_NONE_EFFECT ) ){
				ap_display_effect_draw(buff_get);
			}
			if (display_effect_sts != DISPALY_PIC_EFFECT) {
				if(display_left_rec_time) {
					ap_display_rec_time_draw((INT16U *) buff_get, (void *) &display_rec_time, 200, /* 200 */ 212, 0);	//wwj modify
				}
				ap_display_timer();
				ap_display_timer_browser(src_type);	//wwj add
				if (display_os_time) {
					ap_display_rec_time_draw((INT16U *) buff_get, (void *) &display_rec_time, 200, /* 200 */ 212, 1);	//wwj modify
				} else if(display_os_time_browser) { //wwj add
					ap_display_rec_time_draw((INT16U *) buff_get, (void *) &display_rec_time, 200, 212, 1);
					ap_display_rec_time_draw((INT16U *) buff_get, (void *) &display_total_time, /*160*/130, 0, 0);
				}
				if (video_preview_timerid == 0xFF) {
					ap_display_draw((INT16U *) buff_get);	//Icon draw
				}
			}
#if 0
{ //wwj test
		typedef struct{
		    INT16U font_color;
		    INT16U font_type;
		    INT16S pos_x;
		    INT16S pos_y;
		    INT16U buff_w;
		    INT16U buff_h;
		    char *str_ptr;
		}STRING_ASCII_INFO;

		extern CHAR g_curr_file_path[];
		extern INT32S ap_state_resource_string_ascii_draw(INT16U *frame_buff, STRING_ASCII_INFO *str_ascii_info);

		STRING_ASCII_INFO ascii_str;

		ascii_str.font_color = 0xFFFF;
		ascii_str.font_type = 0;
		ascii_str.buff_w = TFT_WIDTH;
		ascii_str.buff_h = TFT_HEIGHT;
		ascii_str.pos_x = 10;
		ascii_str.pos_y = 140;
		ascii_str.str_ptr = g_curr_file_path;
		ap_state_resource_string_ascii_draw((INT16U *)buff_get, &ascii_str);

		ap_display_motion_draw((INT16U *) buff_get, 10, 180);	//wwj test

}
#endif

		} else {
			DISPLAY_ICONSHOW cursor_icon;
		
			cursor_icon.icon_w = TFT_WIDTH;
			cursor_icon.icon_h = TFT_HEIGHT;
			cursor_icon.transparent = TRANSPARENT_COLOR;		
			cursor_icon.pos_x = 0;
			cursor_icon.pos_y = 0;
			ap_display_icon_draw((INT16U *) buff_get, (INT16U *) display_setting_buff, &cursor_icon);
			ap_display_draw((INT16U *) buff_get);	//Icon draw
		}
		//mask = display_device_protect();
		ap_display_queue_put(display_dma_queue, buff_get);
		//display_device_unprotect(mask);
	} else {
		if (src_type == DISPALY_BUFF_SRC_JPEG) {
			OSQPost(DisplayTaskQ, (void *) (buff_addr|MSG_DISPLAY_TASK_JPEG_DRAW));
		} else if(src_type == DISPALY_BUFF_SRC_WAV_TIME) {
			OSQPost(DisplayTaskQ, (void *) (buff_addr|MSG_DISPLAY_TASK_WAV_TIME_DRAW));
		} else if (src_type == DISPALY_BUFF_SRC_SENSOR) {
			OSQPost(scaler_task_q, (void *) (buff_addr|MSG_SCALER_TASK_PREVIEW_UNLOCK));
		}
	}	
}

void ap_display_effect_draw(INT32U buff_addr)
{
	INT32U i, buff_size, w_size, cnt, sft_size, tmp;
	INT8U index_3, index_2, index_1, index_0;
	DISPLAY_ICONSHOW index_icon;
	
	if (display_effect_sts == DISPALY_PIC_EFFECT || display_effect_sts == DISPALY_PIC_PREVIEW_EFFECT) {
		buff_size = (TFT_WIDTH * TFT_HEIGHT) << 1;
		if (!display_effect_buff) {
			display_effect_buff = (INT32U) gp_malloc_align(buff_size, 64);
			if (!display_effect_buff) {
				return;
			}
			dma_buffer_copy(buff_addr, display_effect_buff, buff_size, 0, 0);
			if (display_effect_sts == DISPALY_PIC_PREVIEW_EFFECT) {
				display_effect_offset = 25;
				if (video_preview_timerid == 0xFF) {
					video_preview_timerid = VIDEO_PREVIEW_TIMER_ID;
					sys_set_timer((void*)OSQPost, (void*) DisplayTaskQ, MSG_DISPLAY_TASK_PIC_EFFECT_END, video_preview_timerid, PHOTO_CAPTURE_TIME_INTERVAL);
				}
			} else {
				display_effect_offset = 24;
			}			
		} else {
			if (video_preview_timerid == 0xFF) {
				w_size = TFT_WIDTH << 1;
				buff_size = (TFT_WIDTH - display_effect_offset) << 1;
				sft_size = display_effect_offset << 1;
				cnt = (TFT_HEIGHT-display_effect_offset) << 1;
				for (i=0 ; i<cnt ; i++) {
					tmp = (display_effect_offset + i)*w_size + sft_size;
					if (tmp >= ((TFT_WIDTH * TFT_HEIGHT) << 1)) {
						break;
					}
					dma_buffer_copy((display_effect_buff + (w_size*i)), (buff_addr + tmp), buff_size, buff_size, w_size);
				}		
				display_effect_offset += display_effect_offset;
				if (display_effect_offset >= TFT_HEIGHT) {
					display_effect_sts = DISPALY_NONE_EFFECT;
					if (display_effect_buff) {
						gp_free((void *) display_effect_buff);
						display_effect_buff = 0;
					}
				}
				msgQSend(ApQ, MSG_APQ_PREVIEW_EFFECT_END, NULL, NULL, MSG_PRI_NORMAL);
			} else {
				dma_buffer_copy(display_effect_buff, buff_addr, buff_size, 0, 0);
			}	
		}
	} else if (display_effect_sts == DISPALY_STRING_EFFECT) {
		DISPLAY_ICONSHOW cursor_icon;
		
		cursor_icon.icon_w = display_str.w;
		cursor_icon.icon_h = display_str.h;
		cursor_icon.transparent = TRANSPARENT_COLOR;		
		cursor_icon.pos_x = (TFT_WIDTH >> 1) - (display_str.w >> 1);
		cursor_icon.pos_y = (TFT_HEIGHT >> 1) - (display_str.h >> 1);
		ap_display_icon_draw((INT16U *) buff_addr, (INT16U *) display_str.addr, &cursor_icon);
	}
	
	if(display_effect_sts != DISPALY_PIC_PREVIEW_EFFECT)
	{
		if(display_mp3_index_sts & DISPALY_MP3_INDEX_SHOW) {
			
			index_icon.icon_w = 16;
			index_icon.icon_h = 32;
			index_icon.transparent = TRANSPARENT_COLOR;
			index_icon.pos_y = PLAY_INDEX_Y;
			
			if(display_mp3_play_index < 10){
				index_3 = 0;
				index_2 = 0;
				index_1 = 0;
				index_0 = display_mp3_play_index;
			}else if(display_mp3_play_index < 100){
				index_3 = 0;
				index_2 = 0;
				index_1 = display_mp3_play_index/10;
				index_0 = display_mp3_play_index - 10*index_1;						
			}else if(display_mp3_play_index < 1000){
				index_3 = 0;
				index_2 = display_mp3_play_index/100;
				index_1 = (display_mp3_play_index - 100*index_2)/10;
				index_0 = (display_mp3_play_index - 100*index_2 - 10*index_1);						
			}else if(display_mp3_play_index < 10000){
				index_3 = display_mp3_play_index/1000;
				index_2 = (display_mp3_play_index - 1000*index_3)/100;
				index_1 = (display_mp3_play_index - 1000*index_3 - 100*index_2)/10;
				index_0 = (display_mp3_play_index - 1000*index_3 - 100*index_2 - 10*index_1);								
			}
			index_icon.pos_x = PLAY_INDEX_X;
			ap_display_icon_draw((INT16U *) buff_addr, (INT16U *) mp3_index_table[index_0], &index_icon);
			index_icon.pos_x = PLAY_INDEX_X - 14;
			ap_display_icon_draw((INT16U *) buff_addr, (INT16U *) mp3_index_table[index_1], &index_icon);
			index_icon.pos_x = PLAY_INDEX_X - 28;
			ap_display_icon_draw((INT16U *) buff_addr, (INT16U *) mp3_index_table[index_2], &index_icon);
			index_icon.pos_x = PLAY_INDEX_X - 42;
			ap_display_icon_draw((INT16U *) buff_addr, (INT16U *) mp3_index_table[index_3], &index_icon);			
		}
		if(display_mp3_index_sts & DISPALY_MP3_TOTAL_INDEX_SHOW) {
			
			index_icon.icon_w = 16;
			index_icon.icon_h = 32;
			index_icon.transparent = TRANSPARENT_COLOR;
			index_icon.pos_y = PLAY_INDEX_Y;
			
			if(display_mp3_total_index < 10){
				index_3 = 0;
				index_2 = 0;
				index_1 = 0;
				index_0 = display_mp3_total_index;
			}else if(display_mp3_total_index < 100){
				index_3 = 0;
				index_2 = 0;
				index_1 = display_mp3_total_index/10;
				index_0 = display_mp3_total_index - 10*index_1;						
			}else if(display_mp3_total_index < 1000){
				index_3 = 0;
				index_2 = display_mp3_total_index/100;
				index_1 = (display_mp3_total_index - 100*index_2)/10;
				index_0 = (display_mp3_total_index - 100*index_2 - 10*index_1);						
			}else if(display_mp3_total_index < 10000){
				index_3 = display_mp3_total_index/1000;
				index_2 = (display_mp3_total_index - 1000*index_3)/100;
				index_1 = (display_mp3_total_index - 1000*index_3 - 100*index_2)/10;
				index_0 = (display_mp3_total_index - 1000*index_3 - 100*index_2 - 10*index_1);								
			}
			index_icon.pos_x = PLAY_TOTAL_INDEX_X;
			ap_display_icon_draw((INT16U *) buff_addr, (INT16U *) mp3_index_table[index_0], &index_icon);
			index_icon.pos_x = PLAY_TOTAL_INDEX_X - 14;
			ap_display_icon_draw((INT16U *) buff_addr, (INT16U *) mp3_index_table[index_1], &index_icon);
			index_icon.pos_x = PLAY_TOTAL_INDEX_X - 28;
			ap_display_icon_draw((INT16U *) buff_addr, (INT16U *) mp3_index_table[index_2], &index_icon);
			index_icon.pos_x = PLAY_TOTAL_INDEX_X - 42;
			ap_display_icon_draw((INT16U *) buff_addr, (INT16U *) mp3_index_table[index_3], &index_icon);	
		}
		if(display_mp3_index_sts & DISPALY_MP3_VOLUME_SHOW) {
			
			index_icon.icon_w = 16;
			index_icon.icon_h = 32;
			index_icon.transparent = TRANSPARENT_COLOR;
			index_icon.pos_y = MP3_VOLUME_Y;
			
			if(display_mp3_volume < 10){
				index_0 = display_mp3_volume;
				index_icon.pos_x = MP3_VOLUME_X - 10;
				ap_display_icon_draw((INT16U *) buff_addr, (INT16U *) mp3_index_table[index_0], &index_icon);			
			}else{
				index_1 = display_mp3_volume/10;
				index_0 = display_mp3_volume - 10*index_1;
				index_icon.pos_x = MP3_VOLUME_X;
				ap_display_icon_draw((INT16U *) buff_addr, (INT16U *) mp3_index_table[index_0], &index_icon);
				index_icon.pos_x = MP3_VOLUME_X - 14;
				ap_display_icon_draw((INT16U *) buff_addr, (INT16U *) mp3_index_table[index_1], &index_icon);									
			}
		}
		if(display_mp3_index_sts & DISPALY_MP3_FM_CHANNEL_SHOW) {
			
			index_icon.icon_h = 32;
			index_icon.transparent = TRANSPARENT_COLOR;
			index_icon.pos_y = FM_CHANNEL_Y;
								
			if(display_mp3_FM_channel < 1000){
				index_2 = display_mp3_FM_channel/100;
				index_1 = (display_mp3_FM_channel - 100*index_2)/10;
				index_0 = (display_mp3_FM_channel - 100*index_2 - 10*index_1);
				//show MHz
				index_icon.icon_w = 64;
				index_icon.pos_x = FM_CHANNEL_X - 8;
				ap_display_icon_draw((INT16U *) buff_addr, (INT16U *) mp3_index_table[11], &index_icon);			
				//show index_0
				index_icon.icon_w = 16;
				index_icon.pos_x = FM_CHANNEL_X - 14;
				ap_display_icon_draw((INT16U *) buff_addr, (INT16U *) mp3_index_table[index_0], &index_icon);
				//show dot
				index_icon.icon_w = 8;
				index_icon.pos_x = FM_CHANNEL_X - 14 - 8;
				ap_display_icon_draw((INT16U *) buff_addr, (INT16U *) mp3_index_table[10], &index_icon);			
				//show index_1
				index_icon.icon_w = 16;
				index_icon.pos_x = FM_CHANNEL_X - 14 - 6 - 14;
				ap_display_icon_draw((INT16U *) buff_addr, (INT16U *) mp3_index_table[index_1], &index_icon);
				index_icon.pos_x = FM_CHANNEL_X - 14 - 6 - 28;
				ap_display_icon_draw((INT16U *) buff_addr, (INT16U *) mp3_index_table[index_2], &index_icon);									
			}else{
				index_3 = display_mp3_FM_channel/1000;
				index_2 = (display_mp3_FM_channel - 1000*index_3)/100;
				index_1 = (display_mp3_FM_channel - 1000*index_3 - 100*index_2)/10;
				index_0 = (display_mp3_FM_channel - 1000*index_3 - 100*index_2 - 10*index_1);
				//show MHz
				index_icon.icon_w = 64;
				index_icon.pos_x = FM_CHANNEL_X - 8;
				ap_display_icon_draw((INT16U *) buff_addr, (INT16U *) mp3_index_table[11], &index_icon);			
				//show index_0
				index_icon.icon_w = 16;
				index_icon.pos_x = FM_CHANNEL_X - 14;
				ap_display_icon_draw((INT16U *) buff_addr, (INT16U *) mp3_index_table[index_0], &index_icon);
				//show dot
				index_icon.icon_w = 8;
				index_icon.pos_x = FM_CHANNEL_X - 14 - 8;
				ap_display_icon_draw((INT16U *) buff_addr, (INT16U *) mp3_index_table[10], &index_icon);			
				//show index_1
				index_icon.icon_w = 16;
				index_icon.pos_x = FM_CHANNEL_X - 14 - 6 - 14;
				ap_display_icon_draw((INT16U *) buff_addr, (INT16U *) mp3_index_table[index_1], &index_icon);
				index_icon.pos_x = FM_CHANNEL_X - 14 - 6 - 28;
				ap_display_icon_draw((INT16U *) buff_addr, (INT16U *) mp3_index_table[index_2], &index_icon);
				index_icon.pos_x = FM_CHANNEL_X - 14 - 6 - 42;
				ap_display_icon_draw((INT16U *) buff_addr, (INT16U *) mp3_index_table[index_3], &index_icon);
			}	
		}
					
		if(display_mp3_input_num_sts & DISPALY_MP3_INPUT_NUM_SHOW) {
			
			index_icon.icon_w = 64;
			index_icon.icon_h = 128;
			index_icon.transparent = TRANSPARENT_COLOR;
			index_icon.pos_y = MP3_INPUT_NUM_Y;
			
			if(display_mp3_input_num < 10){
				index_0 = display_mp3_input_num;
				index_icon.pos_x = MP3_INPUT_NUM_X;
				ap_display_icon_draw((INT16U *) buff_addr, (INT16U *) mp3_input_number[index_0], &index_icon);			
			}else if(display_mp3_input_num < 100){
				index_1 = display_mp3_input_num/10;
				index_0 = display_mp3_input_num - 10*index_1;
				index_icon.pos_x = MP3_INPUT_NUM_X;
				ap_display_icon_draw((INT16U *) buff_addr, (INT16U *) mp3_input_number[index_0], &index_icon);
				index_icon.pos_x = MP3_INPUT_NUM_X - 44;
				ap_display_icon_draw((INT16U *) buff_addr, (INT16U *) mp3_input_number[index_1], &index_icon);									
			}else if(display_mp3_input_num < 1000){
				index_2 = display_mp3_input_num/100;
				index_1 = (display_mp3_input_num - 100*index_2)/10;
				index_0 = (display_mp3_input_num - 100*index_2 - 10*index_1);
				index_icon.pos_x = MP3_INPUT_NUM_X;
				ap_display_icon_draw((INT16U *) buff_addr, (INT16U *) mp3_input_number[index_0], &index_icon);
				index_icon.pos_x = MP3_INPUT_NUM_X - 44;
				ap_display_icon_draw((INT16U *) buff_addr, (INT16U *) mp3_input_number[index_1], &index_icon);
				index_icon.pos_x = MP3_INPUT_NUM_X - 88;
				ap_display_icon_draw((INT16U *) buff_addr, (INT16U *) mp3_input_number[index_2], &index_icon);									
			}else if(display_mp3_input_num < 10000){
				index_3 = display_mp3_input_num/1000;
				index_2 = (display_mp3_input_num - 1000*index_3)/100;
				index_1 = (display_mp3_input_num - 1000*index_3 - 100*index_2)/10;
				index_0 = (display_mp3_input_num - 1000*index_3 - 100*index_2 - 10*index_1);
				index_icon.pos_x = MP3_INPUT_NUM_X;
				ap_display_icon_draw((INT16U *) buff_addr, (INT16U *) mp3_input_number[index_0], &index_icon);
				index_icon.pos_x = MP3_INPUT_NUM_X - 44;
				ap_display_icon_draw((INT16U *) buff_addr, (INT16U *) mp3_input_number[index_1], &index_icon);
				index_icon.pos_x = MP3_INPUT_NUM_X - 88;
				ap_display_icon_draw((INT16U *) buff_addr, (INT16U *) mp3_input_number[index_2], &index_icon);
				index_icon.pos_x = MP3_INPUT_NUM_X - 132;
				ap_display_icon_draw((INT16U *) buff_addr, (INT16U *) mp3_input_number[index_3], &index_icon);
			}			
		}
	}	
}

void ap_display_video_preview_end(void)
{
	if (video_preview_timerid != 0xFF) {
		sys_kill_timer(video_preview_timerid);
		video_preview_timerid = 0xFF;
		display_effect_offset = 24;
	}
}

void ap_display_string_draw(STR_ICON *str_info)
{
	if ((INT32U) str_info == 0xFFFFFF) {
		display_effect_sts = DISPALY_NONE_EFFECT;
	} else {
		display_effect_sts = DISPALY_STRING_EFFECT;
		gp_memcpy((INT8S *) &display_str, (INT8S *) str_info, sizeof(STR_ICON));
	}
}

void ap_display_left_rec_time_draw(INT32U time, INT8U flag)
{
	if (flag) {
		display_left_rec_time = 1;
	    if(time < 60){
			display_rec_time.s = time;
			display_rec_time.m = 0;
			display_rec_time.h = 0;
		}else if(time < 3600){
			display_rec_time.h = 0;
			display_rec_time.m = time/60;
			display_rec_time.s = time - display_rec_time.m*60;
		}else if(time < 360000){
			display_rec_time.h = time/3600;
			display_rec_time.m = (time - display_rec_time.h*3600)/60;
			display_rec_time.s = time - display_rec_time.h*3600 - display_rec_time.m*60;
		}else{
			display_rec_time.s = 59;
			display_rec_time.m = 59;
			display_rec_time.h = 99;
		}
	} else {
		display_left_rec_time = 0;
		display_rec_time.s = 0;
		display_rec_time.m = 0;
		display_rec_time.h = 0;
	}
}

void ap_display_effect_sts_set(INT8U type)
{
	display_effect_sts = type;
}

void ap_display_mp3_index_sts_set(INT8U type)
{
	if(type == 0){
		display_mp3_index_sts = 0;
	}else{
		display_mp3_index_sts |= type;
	}
}

void ap_display_mp3_input_num_sts_set(INT8U type)
{
	display_mp3_input_num_sts = type;
}

INT32U ap_display_queue_get(INT32U *queue)
{
	INT32U i, data;
	
	if (queue[0]) {
		data = queue[0];
		for (i=0 ; i<TASK_DISPALY_BUFFER_NO ; i++) {
			if ((i+1) < TASK_DISPALY_BUFFER_NO) {
				queue[i] = queue[i+1];
			} else {
				queue[i] = 0;
			}
		}
		return data;
	} else {
		return NULL;
	}
}

INT32S  ap_display_queue_put(INT32U *queue, INT32U data)
{
	INT32U i;
	
	for (i=0 ; i<TASK_DISPALY_BUFFER_NO ; i++) {
		if (!queue[i]) {
			queue[i] = data;
			return STATUS_OK;
		}
	}
	return STATUS_FAIL;
}

void ap_display_icon_sts_set(INT32U msg)
{
	INT32U i, icon_idx;
	
	icon_idx = 0;
	for (i=0 ; i<3 ; i++) {
		icon_idx = ((msg >> (i<<3)) & 0xFF);
		if (icon_idx) {
			display_icon_sts[icon_idx >> 5] |= 1 << (icon_idx & 0x1F);
		}
	}
}

void ap_display_icon_sts_clear(INT32U msg)
{
	INT32U i, icon_idx;
	
	icon_idx = 0;
	for (i=0 ; i<3 ; i++) {
		icon_idx = ((msg >> (i<<3)) & 0xFF);
		if (icon_idx) {
			display_icon_sts[icon_idx >> 5] &= ~(1 << (icon_idx & 0x1F));
		}
	}
}


void ap_display_timer_browser(INT8U type)	//wwj add
{
    INT32S sec, total_sec;

	if (timer_force_en_browser == 1) {
		if(type == DISPALY_BUFF_SRC_WAV_TIME){
			sec = audio_dac_get_curr_play_time();
			total_sec = audio_dac_get_total_time();
		}else{
			sec = vid_dec_get_current_time();
			total_sec = vid_dec_get_total_time();
		}
		if ( (sec == -1) || (total_sec == -1) ) {
		    display_os_time_browser = 0;
		    return;
		}
		if (sec == 0 || (sec != browse_curr_time)) {
		    browse_curr_time = sec;
		    display_rec_time.s = sec % 60;
		    sec /= 60;
		    display_rec_time.m = sec % 60;
		    sec /= 60;
			display_rec_time.h = sec % 100;
		}
	    if(total_sec != display_browser_total_time) {
			display_browser_total_time = total_sec;
		    display_total_time.s = total_sec % 60;
		    total_sec /= 60;
		    display_total_time.m = total_sec % 60;
		    total_sec /= 60;
			display_total_time.h = total_sec % 100;
		}
		display_os_time_browser = 1;
	} else {
	    display_os_time_browser = 0;
	}
}


#if 1   // Dominant add, new function to count OSD time display 

void ap_display_timer(void)
{	
	INT8U temp[5] = {0, 1, 3, 5, 10};	//wwj add
    INT32U delta_time;
    INT8U  time_interval = temp[ap_state_config_record_time_get()];// = ap_state_config_record_time_get();	//wwj modify

//    if (display_icon_sts[ICON_REC >> 5] & 1<< ICON_REC) 
	if(timer_force_en)	//wwj modify
    {
		if (display_os_time) 
        {
            delta_time = (sw_timer_get_counter_L()-/*display_os_time*/last_os_time);  //wwj modify
    	    display_rec_time.s = delta_time>>10;
            if (display_rec_time.s > 59) {
                /*display_os_time*/last_os_time=sw_timer_get_counter_L() - (delta_time - (60<<10));  //wwj modify
                display_rec_time.s=0;
				display_rec_time.m++;
    			if (display_rec_time.m > 59) {
    				display_rec_time.m = 0;
    				display_rec_time.h++;
        			if (display_rec_time.h > 99) {
        				display_rec_time.h = 99;
        			}                    
    			}                
			}		
            if ((display_rec_time.m == time_interval) && (display_rec_time.s>0)) {
    		    display_rec_time.m = 0;
            }
		} else {
//			display_os_time = sw_timer_get_counter_L();
			last_os_time = sw_timer_get_counter_L();		//wwj modify
			display_os_time = 1;							//wwj add
			display_rec_time.s = 0;
			display_rec_time.h = 0;
            display_rec_time.m = 0;
		}		
	}/* else {  //wwj mark
		if (timer_force_en==0)
		{
			display_os_time = 0;
		}
	} */
}

#else    // original display time function, the count rule sloer than actual.

void ap_display_timer(void)
{
	INT32U time_tmp, time_tmp1;
	
	if (display_icon_sts[ICON_REC >> 5] & 1<< ICON_REC) {
		if (display_os_time) {
			time_tmp1 = OSTimeGet();
			time_tmp = time_tmp1 - display_os_time;
			while (time_tmp >= 100) {
				time_tmp -= 100;
				display_os_time =time_tmp1;
				display_rec_time.s++;
				if (display_rec_time.s > 59) {
					display_rec_time.s = 0;
					display_rec_time.m++;
				}
				if (display_rec_time.m > 59) {
					display_rec_time.m = 0;
					display_rec_time.h++;
				}
				if (display_rec_time.h > 99) {
					display_rec_time.h = 99;
				}
			}
		} else {
			display_os_time = OSTimeGet();
			display_rec_time.s = 1;
			display_rec_time.h = display_rec_time.m = 0;
		}		
	} else {
		display_os_time = 0;
	}
}
#endif



void timer_counter_force_display(INT8U force_en)
{
    timer_force_en = force_en;
	display_os_time = 0;
}

void timer_counter_force_display_browser(INT8U force_en)  //wwj add
{
    browse_curr_time = 0;
    timer_force_en_browser = force_en;
}

void ap_display_rec_time_draw(INT16U *frame_buff, void *time_ptr, INT16U pos_x, INT16U pos_y, INT8U red_flag)
{
	INT32U i, x, y, offset_pixel, offset_data;
	INT8U num_tmp0, num_tmp1;
						
	for (i=0 ; i<3 ; i++) {
		num_tmp0 = *((INT8U *)time_ptr + i);
		num_tmp1 = num_tmp0/10;
		num_tmp0 -= num_tmp1*10;
		for (y=0 ; y<display_num_item[num_tmp1].icon_h ; y++) {
			for (x=0 ; x<display_num_item[num_tmp1].icon_w ; x++) {
				offset_data = (y*display_num_item[num_tmp1].icon_w) + x;
				if (*(number_table[num_tmp1] + offset_data) == display_num_item[num_tmp1].transparent) {
					continue;
				}
				offset_pixel =  (pos_y + y)*TFT_WIDTH + x + pos_x;
				if (offset_pixel <= TFT_WIDTH*TFT_HEIGHT) {
					if(red_flag){
						*(frame_buff + offset_pixel) = 0xf800;	//red
					}else{
						*(frame_buff + offset_pixel) = *(number_table[num_tmp1] + offset_data);
					}
				}
			}
		}
		pos_x += display_num_item[num_tmp1].icon_w;
		for (y=0 ; y<display_num_item[num_tmp0].icon_h ; y++) {
			for (x=0 ; x<display_num_item[num_tmp0].icon_w ; x++) {
				offset_data = (y*display_num_item[num_tmp0].icon_w) + x;
				if (*(number_table[num_tmp0] + offset_data) == display_num_item[num_tmp0].transparent) {
					continue;
				}
				offset_pixel =  (pos_y + y)*TFT_WIDTH + x + pos_x;
				if (offset_pixel <= TFT_WIDTH*TFT_HEIGHT) {
					if(red_flag){
						*(frame_buff + offset_pixel) = 0xf800;	//red
					}else{
						*(frame_buff + offset_pixel) = *(number_table[/*num_tmp1*/num_tmp0] + offset_data);	//wwj modify
					}
				}
			}
		}
		pos_x += display_num_item[num_tmp0].icon_w;
		if (i < 2) {
			for (y=0 ; y<display_num_item[NUM_TEXT_COLON].icon_h ; y++) {
				for (x=0 ; x<display_num_item[NUM_TEXT_COLON].icon_w ; x++) {
					offset_data = (y*display_num_item[NUM_TEXT_COLON].icon_w) + x;
					if (*(number_table[NUM_TEXT_COLON] + offset_data) == display_num_item[NUM_TEXT_COLON].transparent) {
						continue;
					}
					offset_pixel =  (pos_y + y)*TFT_WIDTH + x + pos_x;
					if (offset_pixel <= TFT_WIDTH*TFT_HEIGHT) {
						if(red_flag){
							*(frame_buff + offset_pixel) = 0xf800;	//red
						}else{
							*(frame_buff + offset_pixel) = *(number_table[/*num_tmp1*/NUM_TEXT_COLON] + offset_data);	//wwj modify
						}
					}
				}
			}
		}
		pos_x += display_num_item[NUM_TEXT_COLON].icon_w;
	}
}
#if 0
static INT32S display_device_protect(void)
{
	return vic_irq_disable(VIC_PPU);
}

static void display_device_unprotect(INT32S mask)
{
	if (mask == 0) {						// Clear device interrupt mask
		vic_irq_enable(VIC_PPU);
	} else if (mask == 1) {
		vic_irq_disable(VIC_PPU);
	} else {								// Something is wrong, do nothing
		return;
	}
}
#endif
