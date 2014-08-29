/*****************************************************************************
 *               Copyright Generalplus Corp. All Rights Reserved.
 *
 * FileName:       application_cfg.h
 * Author:         Lichuanyue
 * Description:    Created
 * Version:        1.0
 * Function List:
 *                 Null
 * History:
 *                 1>. 2008/7/15 Created
 *****************************************************************************/

#ifndef __APPLICATION_CFG_H__
#define __APPLICATION_CFG_H__

#define _DPF_HOT_KEY_SUPPORT                0
#define DISPLAY_AVI_FROM_IMAGE_TASK 	    1
#define _GPL32_FONT_TYPE_MONO               1  /* MONO: Monospaced fonts */
#define _GET_MAINMENU_RESOURC_FROM_NVRAM    1
#define _GET_SETUPMENU_RESOURC_FROM_NVRAM   1
#define _UMI_DEBUG_FLAG                     1
//Contry language

#define _SUPPORT_MIDI_IN_ITEM_SELECTION  0
#if _SUPPORT_MIDI_IN_ITEM_SELECTION == 1
#define MAX_MIDI_NUMBER  5
typedef enum
{
	MIDI_EXIST_FIRST,
	MIDI_EXIST_LAST
}MIDI_EXIST_MODE;
#endif


#define _EIGHT_COUNTRY_LANGUAGE_MODE    0
#define _LOAD_FONT_FROM_NVRAM 			0

#define _SCHINESE_ENGLISH_LANGUAGE_MODE      1
#define _LOAD_SCHINESE_FONT_FROM_NVRAM 		 0

#define _TCHINESE_ENGLISH_LANGUAGE_MODE      0
#define _KOREA_ENGLISH_LANGUAGE_MODE         0
#define _JANPANESE_ENGLISH_LANGUAGE_MODE     0

//avi code define.
//#define _DPF_PROJECT

#if _SCHINESE_ENGLISH_LANGUAGE_MODE == 1

#define _DPF_SUPPORT_ENGLISH    1
#define _DPF_SUPPORT_SCHINESE   1
#define _DPF_SUPPORT_TCHINESE   1
#define _DPF_SUPPORT_FRENCH     0
#define _DPF_SUPPORT_ITALIAN    0
#define _DPF_SUPPORT_SPANISH    0
#define _DPF_SUPPORT_PORTUGUESE 0
#define _DPF_SUPPORT_GERMAN     0
#define _DPF_SUPPORT_RUSSIAN    0
#define _DPF_SUPPORT_SPANISH    0
#define _DPF_SUPPORT_TURKISH    0
#define _DPF_SUPPORT_WWEDISH    0
#elif _EIGHT_COUNTRY_LANGUAGE_MODE == 1
#define _DPF_SUPPORT_ENGLISH    1
#define _DPF_SUPPORT_SCHINESE   0
#define _DPF_SUPPORT_TCHINESE   0
#define _DPF_SUPPORT_FRENCH     0
#define _DPF_SUPPORT_ITALIAN    0
#define _DPF_SUPPORT_SPANISH    0
#define _DPF_SUPPORT_PORTUGUESE 0
#define _DPF_SUPPORT_GERMAN     0
#define _DPF_SUPPORT_RUSSIAN    0
#define _DPF_SUPPORT_SPANISH    0
#define _DPF_SUPPORT_TURKISH    0
#define _DPF_SUPPORT_WWEDISH    0
#endif



/*
*  Languange code
*/
typedef enum {
	#if _DPF_SUPPORT_ENGLISH == 1
	LCD_EN = 0,  //English
	#endif
	#if _DPF_SUPPORT_TCHINESE == 1
	LCD_TCH,	//Traditional Chinese
 	#endif
	#if _DPF_SUPPORT_SCHINESE == 1
	LCD_SCH,    //Simple Chinese
	#endif
 	#if _DPF_SUPPORT_FRENCH == 1
    LCD_FR,     //French
 	#endif
	#if _DPF_SUPPORT_ITALIAN == 1
    LCD_IT, 	//Italian
 	#endif
	#if _DPF_SUPPORT_PORTUGUESE == 1
    LCD_ES,     // Spanish
 	#endif
 	#if _DPF_SUPPORT_GERMAN == 1
	LCD_PT,     // Portuguese
 	#endif
	#if _DPF_SUPPORT_RUSSIAN == 1
	LCD_DE,     // German
 	#endif
	#if _DPF_SUPPORT_SPANISH == 1
	LCD_RU,     // Russian
 	#endif
	#if _DPF_SUPPORT_TURKISH == 1
	LCD_TR,     // Turkish
 	#endif
	#if _DPF_SUPPORT_WWEDISH == 1
	LCD_SV,     //  Swedish
 	#endif
 	LCD_MAX
}_DPF_LCD;



#if _GPL32_FONT_TYPE_MONO == 1
	#define GUI_FLASH

	#define _FONT_Times_New_Roman_32  0
	#if _DPF_SUPPORT_ENGLISH == 1

	#define _FONT_Arail_8     1    		 //height 14,width not sure,size equal to windows system size "8"
	#define _FONT_Arail_12    1          //height 18,width not sure,size equal to windows system size "12"
	#define _FONT_Arail_16    1          //height 24,width not sure.size equal to windows system size "16"
	#define _FONT_Arail_20    1          //height 32,width not sure.size equal to windows system size "20"
	#endif

	#define _FONT_TimesNewRoman_72 1      //height 108,width not sure.size equal to windows system size "72",
	                                      // for set intervoke time use,only 0-9 ten fonts.
	#if _DPF_SUPPORT_SCHINESE == 1


	#define _FONT_SIMSUN_8      1          //height 14,width not sure,size equal to windows system size "8"
	#define _FONT_SIMSUN_12     1          //height 18,width not sure,size equal to windows system size "12"
	#define _FONT_SIMSUN_16     1          //height 24,width not sure.size equal to windows system size "16"
	#define _FONT_SIMSUN_20     1          //height 32,width not sure.size equal to windows system size "20"
	#endif

#endif

#define APP_G_MIDI_DECODE_EN	1

//define the special effect when use video encode
#define AUDIO_SFX_HANDLE	0
#define VIDEO_SFX_HANDLE	0

//define audio algorithm
#define APP_WAV_CODEC_EN		1	//must enable when use Audio Encode and AVI Decode 	
#define APP_MP3_DECODE_EN		1
#define APP_MP3_ENCODE_EN		1
#define APP_A1800_DECODE_EN		0
#define APP_A1800_ENCODE_EN		0
#define APP_WMA_DECODE_EN		0
#define APP_A1600_DECODE_EN		0
#define APP_A6400_DECODE_EN		0
#define APP_S880_DECODE_EN		0

#define	APP_VOICE_CHANGER_EN	0
#define APP_UP_SAMPLE_EN		0
#define	APP_DOWN_SAMPLE_EN		0
#define A1800_DOWN_SAMPLE_EN	0

#if APP_WAV_CODEC_EN
	#define APP_WAV_CODEC_FG_EN		1	
	#define APP_WAV_CODEC_BG_EN     0
#else
	#define APP_WAV_CODEC_FG_EN		0
	#define APP_WAV_CODEC_BG_EN     0
#endif

#if APP_MP3_DECODE_EN
	#define APP_MP3_DECODE_FG_EN	1
	#define APP_MP3_DECODE_BG_EN	0
#else
	#define APP_MP3_DECODE_FG_EN	0
	#define APP_MP3_DECODE_BG_EN	0
#endif

#if APP_A1800_DECODE_EN
	#define APP_A1800_DECODE_FG_EN	1
	#define APP_A1800_DECODE_BG_EN  1
#else
	#define APP_A1800_DECODE_FG_EN	0
	#define APP_A1800_DECODE_BG_EN  0	
#endif

#if APP_WMA_DECODE_EN
	#define APP_WMA_DECODE_FG_EN	1
	#define APP_WMA_DECODE_BG_EN	1
#else
	#define APP_WMA_DECODE_FG_EN	0
	#define APP_WMA_DECODE_BG_EN	0
#endif

#if APP_A1600_DECODE_EN
	#define APP_A1600_DECODE_FG_EN	1
	#define APP_A1600_DECODE_BG_EN	1
#else
	#define APP_A1600_DECODE_FG_EN	0
	#define APP_A1600_DECODE_BG_EN	0
#endif

#if APP_A6400_DECODE_EN
	#define APP_A6400_DECODE_FG_EN	1
	#define APP_A6400_DECODE_BG_EN	1
#else
	#define APP_A6400_DECODE_FG_EN	0
	#define APP_A6400_DECODE_BG_EN	0
#endif

#if APP_S880_DECODE_EN
	#define APP_S880_DECODE_FG_EN	1
	#define APP_S880_DECODE_BG_EN	1
#else
	#define APP_S880_DECODE_FG_EN	0
	#define APP_S880_DECODE_BG_EN	0
#endif

#define AUDIO_BG_DECODE_EN APP_WAV_CODEC_BG_EN | APP_MP3_DECODE_BG_EN | APP_A1800_DECODE_BG_EN | \
						   APP_WMA_DECODE_BG_EN | APP_A1600_DECODE_BG_EN | APP_A6400_DECODE_BG_EN |\
						   APP_S880_DECODE_BG_EN

#define AUDIO_FG_DECODE_EN APP_WAV_CODEC_FG_EN | APP_MP3_DECODE_FG_EN | APP_A1800_DECODE_FG_EN | \
						   APP_WMA_DECODE_FG_EN | APP_A1600_DECODE_FG_EN | APP_A6400_DECODE_FG_EN |\
						   APP_S880_DECODE_FG_EN						 
						  


// audio record config 
#define AUD_FILE_MAX_RECORD_SIZE		300*AUD_REC_SAMPLING_RATE*2	//2GB



#endif		// __APPLICATION_CFG_H__
