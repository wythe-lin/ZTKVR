#include "ap_state_config.h"

INT32U CRC32_tbl[256];
SYSTEM_USER_OPTION Global_User_Optins;
USER_ITEMS user_item;

//	prototypes
void ap_state_config_default_set(void);

void ap_state_config_initial(INT32S status)
{
	//default setting
	Global_User_Optins.item.ifdirty = 0;
	if (status == STATUS_FAIL) { /* check nvdb initial status */
		ap_state_config_default_set();
	}
}

void ap_state_config_default_set(void)
{
	// some default config is 0
	gp_memset((INT8S *)&Global_User_Optins, 0x00, sizeof(SYSTEM_USER_OPTION));
	ap_state_resource_user_option_load(&Global_User_Optins);

	ap_state_config_light_freq_set(1);	// add by xyz - 2014.12.11

	//MP3 function reset value
	if(fm_tx_status_get() == 1){
		ap_state_music_fm_ch_set(989);	//default value for fm tx
		ap_music_reset();
	}
	Global_User_Optins.item.ifdirty = 1;
}

void ap_state_config_store(void)
{
	if (Global_User_Optins.item.ifdirty == 1) {
		/* calculate CRC */
		CRC_cal((INT8U*)&Global_User_Optins.item, sizeof(USER_ITEMS),Global_User_Optins.crc);
		DBG_PRINT("store CRC = %02x %02x %02x %02x\r\n",Global_User_Optins.crc[0],Global_User_Optins.crc[1],Global_User_Optins.crc[2],Global_User_Optins.crc[3]);
		nvmemory_user_sector_store(0, (INT32U *)&Global_User_Optins, 1);
#if 0 //Do not retry
		gp_memcpy((INT8S*)&user_item, (INT8S*)&Global_User_Optins.item, sizeof(USER_ITEMS));
		nvmemory_user_sector_load(0, (INT32U *)&Global_User_Optins, 1);
		if (gp_memcmp((INT8S*)&user_item, (INT8S*)&Global_User_Optins.item, sizeof(USER_ITEMS)) != 0) {
			DBG_PRINT("verify failed, store again\r\n");
			gp_memcpy((INT8S*)&Global_User_Optins.item,(INT8S*)&user_item, sizeof(USER_ITEMS));
			CRC_cal((INT8U*)&Global_User_Optins.item, sizeof(USER_ITEMS),Global_User_Optins.crc);
			nvmemory_user_sector_store(0, (INT32U *)&Global_User_Optins, 1);
		}
#endif
		Global_User_Optins.item.ifdirty = 0;
	}
}

INT32S ap_state_config_load(void)
{
	INT8U expect_crc[4];

	nvmemory_user_sector_load(0, (INT32U *)&Global_User_Optins, 1);		//load user setting
	DBG_PRINT("load CRC = %02x %02x %02x %02x\r\n",Global_User_Optins.crc[0],Global_User_Optins.crc[1],Global_User_Optins.crc[2],Global_User_Optins.crc[3]);
	/* calculate expect CRC */
	CRC_cal((INT8U*)&Global_User_Optins.item, sizeof(USER_ITEMS),expect_crc);
	DBG_PRINT("expect CRC = %02x %02x %02x %02x\r\n",expect_crc[0],expect_crc[1],expect_crc[2],expect_crc[3]);
	/* compare CRC and expect CRC */
	if (gp_memcmp((INT8S*)expect_crc, (INT8S*)Global_User_Optins.crc, 4) != 0) {
		DBG_PRINT("crc error\r\n");
		return STATUS_FAIL; /* CRC error */
	}
	return STATUS_OK;
}

void ap_state_config_ev_set(INT8U ev)
{
	if (ev != ap_state_config_ev_get()) {
		Global_User_Optins.item.midi_exist_mode = ev;
		Global_User_Optins.item.ifdirty = 1;
	}
}

INT8U ap_state_config_ev_get(void)
{
	return Global_User_Optins.item.midi_exist_mode;
}

void ap_state_config_white_balance_set(INT8U white_balance)
{
	if (white_balance != ap_state_config_white_balance_get()) {
		Global_User_Optins.item.alarm_mute2 = white_balance;
		Global_User_Optins.item.ifdirty = 1;
	}
}

INT8U ap_state_config_white_balance_get(void)
{
	return Global_User_Optins.item.alarm_mute2;
}

void ap_state_config_md_set(INT8U md)
{
	if (md != ap_state_config_md_get()) {
		Global_User_Optins.item.alarm_volume2 = md;
		Global_User_Optins.item.ifdirty = 1;
	}
}

INT8U ap_state_config_md_get(void)
{
	return Global_User_Optins.item.alarm_volume2;
}

//void ap_state_config_night_mode_set(INT8U night_mode)
void ap_state_config_voice_record_switch_set(INT8U mic_switch)	//wwj modify
{
//	if (night_mode != ap_state_config_night_mode_get()) {
	if (mic_switch != ap_state_config_voice_record_switch_get()) {
//		Global_User_Optins.item.alarm_volume1 = night_mode;
		Global_User_Optins.item.alarm_volume1 = mic_switch;
		Global_User_Optins.item.ifdirty = 1;
	}

	if(mic_switch == 0) {	//wwj add
		audio_encode_audio_format_set(WAVE_FORMAT_PCM);
	} else {
		audio_encode_audio_format_set(0);
	}
}

//INT8U ap_state_config_night_mode_get(void)
INT8U ap_state_config_voice_record_switch_get(void)
{
	return Global_User_Optins.item.alarm_volume1;
}

void ap_state_config_pic_size_set(INT8U pic_size)
{
	DBG_PRINT("\033[40;32m%s()\033[0m - pic_size=%02x\r\n", __func__, pic_size);
	if (pic_size != ap_state_config_pic_size_get()) {
		Global_User_Optins.item.slideshow_photo_date_on = pic_size;
		Global_User_Optins.item.ifdirty = 1;
	}
}

INT8U ap_state_config_pic_size_get(void)
{
	return Global_User_Optins.item.slideshow_photo_date_on;
}

void ap_state_config_quality_set(INT8U quality)
{
	if (quality != ap_state_config_quality_get()) {
		Global_User_Optins.item.slideshow_bg_music[0] = quality;
		Global_User_Optins.item.ifdirty = 1;
	}
}

INT8U ap_state_config_quality_get(void)
{
	return Global_User_Optins.item.slideshow_bg_music[0];
}

void ap_state_config_scene_mode_set(INT8U scene_mode)
{
	if (scene_mode != ap_state_config_scene_mode_get()) {
		Global_User_Optins.item.slideshow_duration = scene_mode;
		Global_User_Optins.item.ifdirty = 1;
	}
}

INT8U ap_state_config_scene_mode_get(void)
{
	return Global_User_Optins.item.slideshow_duration;
}

void ap_state_config_iso_set(INT8U iso)
{
	if (iso != ap_state_config_iso_get()) {
		Global_User_Optins.item.lcd_backlight = iso;
		Global_User_Optins.item.ifdirty = 1;
	}
}

INT8U ap_state_config_iso_get(void)
{
	return Global_User_Optins.item.lcd_backlight;
}

void ap_state_config_color_set(INT8U color)
{
	if (color != ap_state_config_color_get()) {
		Global_User_Optins.item.ui_style = color;
		Global_User_Optins.item.ifdirty = 1;
	}
}

INT8U ap_state_config_color_get(void)
{
	return Global_User_Optins.item.ui_style;
}

void ap_state_config_saturation_set(INT8U saturation)
{
	if (saturation != ap_state_config_saturation_get()) {
		Global_User_Optins.item.calendar_duration = saturation;
		Global_User_Optins.item.ifdirty = 1;
	}
}

INT8U ap_state_config_saturation_get(void)
{
	return Global_User_Optins.item.calendar_duration;
}

void ap_state_config_sharpness_set(INT8U sharpness)
{
	if (sharpness != ap_state_config_sharpness_get()) {
		Global_User_Optins.item.calendar_displaymode = sharpness;
		Global_User_Optins.item.ifdirty = 1;
	}
}

INT8U ap_state_config_sharpness_get(void)
{
	return Global_User_Optins.item.calendar_displaymode;
}

void ap_state_config_preview_set(INT8U preview)
{
	if (preview != ap_state_config_preview_get()) {
		Global_User_Optins.item.alarm_mute1 = preview;
		Global_User_Optins.item.ifdirty = 1;
	}
}

INT8U ap_state_config_preview_get(void)
{
	return Global_User_Optins.item.alarm_mute1;
}

void ap_state_config_burst_set(INT8U burst)
{
	if (burst != ap_state_config_burst_get()) {
		Global_User_Optins.item.music_on_off = burst;
		Global_User_Optins.item.ifdirty = 1;
	}
}

INT8U ap_state_config_burst_get(void)
{
	return Global_User_Optins.item.music_on_off;
}

void ap_state_config_auto_off_set(INT8U auto_off)
{
	if (auto_off != ap_state_config_auto_off_get()) {
//		Global_User_Optins.item.save_as_logo_flag = auto_off;
		Global_User_Optins.item.date_format_display = auto_off;	//wwj modify
		Global_User_Optins.item.ifdirty = 1;
	}
}

INT8U ap_state_config_auto_off_get(void)
{
//	return Global_User_Optins.item.save_as_logo_flag;
	return	Global_User_Optins.item.date_format_display;	//wwj modify
}

void ap_state_config_light_freq_set(INT8U light_freq)
{
	if (light_freq != ap_state_config_light_freq_get()) {
		Global_User_Optins.item.alarm_modual = light_freq;
		Global_User_Optins.item.ifdirty = 1;
	}
}

INT8U ap_state_config_light_freq_get(void)
{
	return Global_User_Optins.item.alarm_modual;
}

void ap_state_config_tv_out_set(INT8U tv_out)
{
	if (tv_out != ap_state_config_tv_out_get()) {
		Global_User_Optins.item.powertime_happened = tv_out;
		Global_User_Optins.item.ifdirty = 1;
	}
}

INT8U ap_state_config_tv_out_get(void)
{
	return Global_User_Optins.item.powertime_happened;
}

void ap_state_config_usb_mode_set(INT8U usb_mode)
{
	if (usb_mode != ap_state_config_usb_mode_get()) {
		Global_User_Optins.item.week_format_display = usb_mode;
		Global_User_Optins.item.ifdirty = 1;
	}
}

INT8U ap_state_config_usb_mode_get(void)
{
	return Global_User_Optins.item.week_format_display;
}

void ap_state_config_language_set(INT8U language)
{
	//if (language != ap_state_config_language_get()) {
	if (language != (ap_state_config_language_get() - 1)) {	//wwj modify
		//Global_User_Optins.item.language = language;
		Global_User_Optins.item.language = language + 1;	//wwj modify
		Global_User_Optins.item.ifdirty = 1;
	}
}

INT8U ap_state_config_language_get(void)
{
	return Global_User_Optins.item.language;
}

void ap_state_config_volume_set(INT8U sound_volume)
{
	if (sound_volume != ap_state_config_volume_get()) {
		Global_User_Optins.item.sound_volume = sound_volume;
		Global_User_Optins.item.ifdirty = 1;	
	}
}

INT8U ap_state_config_volume_get(void)
{
	return Global_User_Optins.item.sound_volume;
}

void ap_state_config_motion_detect_set(INT8U motion_detect)
{
	if (motion_detect != ap_state_config_motion_detect_get()) {
		Global_User_Optins.item.music_play_mode = motion_detect;
		Global_User_Optins.item.ifdirty = 1;
	}
}

INT8U ap_state_config_motion_detect_get(void)
{
	return Global_User_Optins.item.music_play_mode;
}

void ap_state_config_record_time_set(INT8U record_time)
{
	if (record_time != ap_state_config_record_time_get()) {
		Global_User_Optins.item.full_screen = record_time;
		Global_User_Optins.item.ifdirty = 1;
	}
}

INT8U ap_state_config_record_time_get(void)
{
	return Global_User_Optins.item.full_screen;
}

void ap_state_config_date_stamp_set(INT8U date_stamp)
{
	if (date_stamp != ap_state_config_date_stamp_get()) {
		Global_User_Optins.item.thumbnail_mode = date_stamp;
		Global_User_Optins.item.ifdirty = 1;
	}
}

INT8U ap_state_config_date_stamp_get(void)
{
	return Global_User_Optins.item.thumbnail_mode;
}

void ap_state_musiic_play_onoff_set(INT8U on_off)
{
	if (on_off != ap_state_music_play_onoff_get()) {
		Global_User_Optins.item.slideshow_transition = on_off;
		Global_User_Optins.item.ifdirty = 1;	
	}
}

INT8U ap_state_music_play_onoff_get(void)
{
	return Global_User_Optins.item.slideshow_transition;
}

void ap_state_music_set(INT32U music_index)
{
	if (music_index != ap_state_music_get()) {
		Global_User_Optins.item.current_photo_index[0] = (INT8U)(music_index);
		Global_User_Optins.item.current_photo_index[1] = (INT8U)(music_index >> 8);
		Global_User_Optins.item.current_photo_index[2] = (INT8U)(music_index >> 16);
		Global_User_Optins.item.ifdirty = 1;
	}
}

INT32U ap_state_music_get(void)
{
	INT32U index;

	index = Global_User_Optins.item.current_photo_index[2];
	index <<= 8;
	index += Global_User_Optins.item.current_photo_index[1];
	index <<= 8;
	index += Global_User_Optins.item.current_photo_index[0];
	return index;
}

void ap_state_music_play_mode_set(INT8U music_play_mode)
{
	if (music_play_mode != ap_state_music_play_mode_get()) {
		Global_User_Optins.item.sleep_time = music_play_mode;
		Global_User_Optins.item.ifdirty = 1;
	}

}

INT8U ap_state_music_play_mode_get(void)
{
	return Global_User_Optins.item.sleep_time;
}

void ap_state_music_fm_ch_set(INT32U freq)
{
	if (freq != ap_state_music_fm_ch_get()) {
		Global_User_Optins.item.FM_struct.FM_Station[0].Frequency = freq;
		Global_User_Optins.item.ifdirty = 1;
	}

}

INT32U ap_state_music_fm_ch_get(void)
{
	return Global_User_Optins.item.FM_struct.FM_Station[0].Frequency;
}

void ap_state_config_factory_date_get(INT8U *date)
{
	*date++ = Global_User_Optins.item.factory_date[0];
	*date++ = Global_User_Optins.item.factory_date[1];
	*date = Global_User_Optins.item.factory_date[2];
}

void ap_state_config_factory_time_get(INT8U *time)
{
	*time++ = Global_User_Optins.item.factory_time[0];
	*time++ = Global_User_Optins.item.factory_time[1];
	*time = Global_User_Optins.item.factory_time[2];
}

void ap_state_config_base_day_set(INT32U day)
{
	if (day != ap_state_config_base_day_get()) {
		Global_User_Optins.item.base_day = day;
		Global_User_Optins.item.ifdirty = 1;
	}

}

INT32U ap_state_config_base_day_get(void)
{
	return Global_User_Optins.item.base_day;
}


void CRC32tbl_init(void)
{
    INT32U i,j;
    INT32U c;

    for (i=0 ; i<256 ; ++i) {
        c = i ;
        for (j=0 ; j<8 ; j++) { 
            c = (c & 1) ? (c >> 1) ^ CRC32_POLY : (c >> 1);
        }
        CRC32_tbl[i] = c;
    }
}

void CRC_cal(INT8U *pucBuf, INT32U len, INT8U *CRC)
{
    INT32U i_CRC, value;
    INT32U i;

    if (!CRC32_tbl[1]) {
        CRC32tbl_init();
    }    
    i_CRC = 0xFFFFFFFF;
    for (i=0 ; i<len ; i++) {
        i_CRC = (i_CRC >> 8) ^ CRC32_tbl[(i_CRC ^ pucBuf[i]) & 0xFF];
    }
    value = ~i_CRC;
    CRC[0] = (value & 0x000000FF) >> 0;
    CRC[1] = (value & 0x0000FF00) >> 8;
    CRC[2] = (value & 0x00FF0000) >> 16;
    CRC[3] = (value & 0xFF000000) >> 24;
}