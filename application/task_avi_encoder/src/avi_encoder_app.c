#include "ztkconfigs.h"
#include "avi_encoder_app.h"
#include "jpeg_header.h"
#include "video_codec_callback.h"

/* for debug */
#define DEBUG_AVI_ENCODER_APP	1
#if DEBUG_AVI_ENCODER_APP
    #include "gplib.h"
    #define _dmsg(x)		print_string x
#else
    #define _dmsg(x)
#endif

/* global varaible */
AviEncPara_t AviEncPara, *pAviEncPara;
AviEncAudPara_t AviEncAudPara, *pAviEncAudPara;
AviEncVidPara_t AviEncVidPara, *pAviEncVidPara;
AviEncPacker_t AviEncPacker0, *pAviEncPacker0;
AviEncPacker_t AviEncPacker1, *pAviEncPacker1;

GP_AVI_AVISTREAMHEADER	avi_aud_stream_header;
GP_AVI_AVISTREAMHEADER	avi_vid_stream_header;
GP_AVI_BITMAPINFO		avi_bitmap_info;
GP_AVI_PCMWAVEFORMAT	avi_wave_info;

#if MIC_INPUT_SRC == C_ADC_LINE_IN || MIC_INPUT_SRC == C_BUILDIN_MIC_IN
	static DMA_STRUCT g_avi_adc_dma_dbf;
#elif MIC_INPUT_SRC == C_GPY0050_IN
	INT8U  g_mic_buffer;
	INT16U g_gpy0050_pre_value;
	INT32U g_mic_cnt;
#endif

static INT8U g_csi_index;
static INT8U g_scaler_index;
static INT8U g_display_index;
static INT8U g_video_index;
static INT8U g_pcm_index;
static INT32U g_video_lock_addr;

static void sensor_mem_free(void);
static void scaler_mem_free(void);
static void video_mem_free(void);
static void AviPacker_mem_free(AviEncPacker_t *pAviEncPacker);
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// avi encode api
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/* */
static void scaler_app_lock(void)
{
	INT8U err;
	OSSemPend(scaler_hw_app_sem, 0, &err);
}

static void scaler_app_unlock(void)
{
	OSSemPost(scaler_hw_app_sem);
}

void avi_encode_init(void)
{
	pAviEncPara = &AviEncPara;
	gp_memset((INT8S *)pAviEncPara, 0, sizeof(AviEncPara_t));

	pAviEncAudPara = &AviEncAudPara;
	gp_memset((INT8S *)pAviEncAudPara, 0, sizeof(AviEncAudPara_t));

	pAviEncVidPara = &AviEncVidPara;
	gp_memset((INT8S *)pAviEncVidPara, 0, sizeof(AviEncVidPara_t));

	pAviEncPacker0 = &AviEncPacker0;
	gp_memset((INT8S *)pAviEncPacker0, 0, sizeof(AviEncPacker_t));
	pAviEncPacker1 = &AviEncPacker1;
	gp_memset((INT8S *)pAviEncPacker1, 0, sizeof(AviEncPacker_t));

	pAviEncPacker0->file_handle = -1;
	pAviEncPacker0->index_handle = -1;
	pAviEncPacker1->file_handle = -1;
	pAviEncPacker1->index_handle = -1;
	pAviEncVidPara->scaler_zoom_ratio = 1;
}

static void avi_encode_sync_timer_isr(void)
{
#if 0
	if(pAviEncPara->AviPackerCur->p_avi_wave_info)
	{
		if((pAviEncPara->tv - pAviEncPara->ta) < pAviEncPara->delta_ta)
		{
			pAviEncPara->tv += pAviEncPara->tick;
		}
	}
	else
	{
		pAviEncPara->tv += pAviEncPara->tick;
	}

	if((pAviEncPara->tv - pAviEncPara->Tv) > 0)
	{
		if(pAviEncPara->post_cnt == pAviEncPara->pend_cnt)
		{
			if(OSQPost(AVIEncodeApQ, (void*)MSG_AVI_ONE_FRAME_ENCODE)==OS_NO_ERR)
            {
	    		pAviEncPara->post_cnt++;
            }
		}
	}

#else   // dominant modify
    pAviEncPara->tv += pAviEncPara->tick;

	if((pAviEncPara->tv - pAviEncPara->Tv) > 0)
	{
		if(pAviEncPara->post_cnt == pAviEncPara->pend_cnt)
		{
			if(OSQPost(AVIEncodeApQ, (void*)MSG_AVI_ONE_FRAME_ENCODE)==OS_NO_ERR)
            {
	    		pAviEncPara->post_cnt++;
            }
        }
	}
#endif

}

void avi_encode_video_timer_start(void)
{
	INT32U temp, freq_hz;
	INT32U preload_value;

	pAviEncPara->ta = 0;
	pAviEncPara->tv = 0;
	pAviEncPara->Tv = 0;
	pAviEncPara->pend_cnt = 0;
	pAviEncPara->post_cnt = 0;
#if MIC_INPUT_SRC == C_ADC_LINE_IN
	if(AVI_AUDIO_RECORD_TIMER == ADC_AS_TIMER_C)
		preload_value = R_TIMERC_PRELOAD;
	else if(AVI_AUDIO_RECORD_TIMER == ADC_AS_TIMER_D)
		preload_value = R_TIMERD_PRELOAD;
	else if(AVI_AUDIO_RECORD_TIMER == ADC_AS_TIMER_E)
		preload_value = R_TIMERE_PRELOAD;
	else	//timerf
		preload_value = R_TIMERF_PRELOAD;
#elif MIC_INPUT_SRC == C_BUILDIN_MIC_IN
/*
	if(AVI_AUDIO_RECORD_TIMER == ADC_AS_TIMER_A)
		preload_value = R_TIMERA_PRELOAD;
	else if(AVI_AUDIO_RECORD_TIMER == ADC_AS_TIMER_B)
		preload_value = R_TIMERB_PRELOAD;
	else
*/
	if(AVI_AUDIO_RECORD_TIMER == ADC_AS_TIMER_C)
		preload_value = R_TIMERC_PRELOAD;
	else if(AVI_AUDIO_RECORD_TIMER == ADC_AS_TIMER_D)
		preload_value = R_TIMERD_PRELOAD;
	else if(AVI_AUDIO_RECORD_TIMER == ADC_AS_TIMER_E)
		preload_value = R_TIMERE_PRELOAD;
	else	//timerf
		preload_value = R_TIMERF_PRELOAD;
#elif MIC_INPUT_SRC == C_GPY0050_IN
	if(AVI_AUDIO_RECORD_TIMER == TIMER_A)
		preload_value = R_TIMERA_PRELOAD;
    else if(AVI_AUDIO_RECORD_TIMER == TIMER_B)
		preload_value = R_TIMERB_PRELOAD;
	else if(AVI_AUDIO_RECORD_TIMER == TIMER_C)
		preload_value = R_TIMERC_PRELOAD;
	else if(AVI_AUDIO_RECORD_TIMER == TIMER_D)
		preload_value = R_TIMERD_PRELOAD;
	else if(AVI_AUDIO_RECORD_TIMER == TIMER_E)
		preload_value = R_TIMERE_PRELOAD;
	else	//timer F
		preload_value = R_TIMERF_PRELOAD;
#endif

	if(pAviEncPara->AviPackerCur->p_avi_wave_info)
	{
		//temp = 0x10000 -((0x10000 - (R_TIMERE_PRELOAD & 0xFFFF)) * p_vid_dec_para->n);
		temp = (0x10000 - (preload_value & 0xFFFF)) * pAviEncPara->freq_div;
		freq_hz = MCLK/2/temp;
		if(MCLK%(2*temp))	freq_hz++;
	}
	else
		freq_hz = AVI_ENCODE_TIME_BASE;

	timer_freq_setup(AVI_ENCODE_VIDEO_TIMER, freq_hz, 0, avi_encode_sync_timer_isr);
}

void avi_encode_video_timer_stop(void)
{
	timer_stop(AVI_ENCODE_VIDEO_TIMER);
}

void avi_encode_audio_timer_start(void)
{
#if MIC_INPUT_SRC == C_ADC_LINE_IN
	adc_sample_rate_set(AVI_AUDIO_RECORD_TIMER, pAviEncAudPara->audio_sample_rate);
#elif MIC_INPUT_SRC == C_BUILDIN_MIC_IN
	mic_sample_rate_set(AVI_AUDIO_RECORD_TIMER, pAviEncAudPara->audio_sample_rate);
#elif MIC_INPUT_SRC == C_GPY0050_IN
	timer_freq_setup(AVI_AUDIO_RECORD_TIMER, pAviEncAudPara->audio_sample_rate, 0, gpy0050_timer_isr);
#endif
}

void avi_encode_audio_timer_stop(void)
{
#if MIC_INPUT_SRC == C_ADC_LINE_IN
	adc_timer_stop(AVI_AUDIO_RECORD_TIMER);
#elif MIC_INPUT_SRC == C_BUILDIN_MIC_IN
	mic_timer_stop(AVI_AUDIO_RECORD_TIMER);
#elif MIC_INPUT_SRC == C_GPY0050_IN
	timer_stop(AVI_AUDIO_RECORD_TIMER);
#endif
}

// file handle
INT32S avi_encode_set_file_handle_and_caculate_free_size(AviEncPacker_t *pAviEncPacker, INT16S FileHandle)
{
    #if 0
	INT16S disk_no;
    #endif
	struct stat_t statbuf;

	if(FileHandle < 0)
    {
		return STATUS_FAIL;
    }


	//create index file handle
    #if 0  // dominant mark, no need chdir now.
    disk_no = GetDiskOfFile(FileHandle);
    if(disk_no == 0)
    	gp_strcpy((INT8S*)pAviEncPacker->index_path, (INT8S*)"A:\\DCIM\\");
   	else if(disk_no == 1)
    	gp_strcpy((INT8S*)pAviEncPacker->index_path, (INT8S*)"B:\\DCIM\\");
    else if(disk_no == 2)
    	gp_strcpy((INT8S*)pAviEncPacker->index_path, (INT8S*)"C:\\DCIM\\");
    else if(disk_no == 3)
    	gp_strcpy((INT8S*)pAviEncPacker->index_path, (INT8S*)"D:\\DCIM\\");
    else if(disk_no == 4)
    	gp_strcpy((INT8S*)pAviEncPacker->index_path, (INT8S*)"E:\\DCIM\\");
    else if(disk_no == 5)
    	gp_strcpy((INT8S*)pAviEncPacker->index_path, (INT8S*)"F:\\DCIM\\");
    else if(disk_no == 6)
    	gp_strcpy((INT8S*)pAviEncPacker->index_path, (INT8S*)"G:\\DCIM\\");
    else
    	return STATUS_FAIL;

    chdir((CHAR*)pAviEncPacker->index_path);
    #else
        gp_strcpy((INT8S*)pAviEncPacker->index_path, (INT8S*)"C:\\");
    #endif

    if(stat("index0.tmp", &statbuf) < 0) {
    	gp_strcat((INT8S*)pAviEncPacker->index_path, (INT8S*)"index0.tmp");
    } else {
    	gp_strcat((INT8S*)pAviEncPacker->index_path, (INT8S*)"index1.tmp");
    }
    pAviEncPacker->file_handle = FileHandle;
    pAviEncPacker->index_handle = open((char*)pAviEncPacker->index_path, O_RDWR|O_CREAT|O_TRUNC);
    if(pAviEncPacker->index_handle < 0)
    	return STATUS_FAIL;


   	pAviEncPara->disk_free_size = 600<<20;
   	pAviEncPara->record_total_size = 2*32*1024 + 16; //avi header + data is 32k align + index header

   	return STATUS_OK;

   	/*
   	//caculate storage free size
    disk_free = vfsFreeSpace(disk_no);
    DEBUG_MSG(DBG_PRINT("DISK FREE SIZE = %dMByte\r\n", disk_free>>20));
   if(disk_free >= 0x80000)	//512Kbyte
   //if ((disk_free >> 20) > 500)
    {
    	pAviEncPara->disk_free_size = disk_free;
   		pAviEncPara->record_total_size = 2*32*1024 + 16; //avi header + data is 32k align + index header
    	return STATUS_OK;
  	}
  	else
  	{
  		OSQPost(AVIDelFileQ, (void *) AVI_DEL_FILE);
  		pAviEncPara->disk_free_size = disk_free;
   		pAviEncPara->record_total_size = 2*32*1024 + 16; //avi header + data is 32k align + index header
    	return STATUS_OK;
	}
	*/
}

INT16S avi_encode_close_file(AviEncPacker_t *pAviEncPacker)
{
	INT32S nRet;
/*	INT32U len, buf;


	buf = (INT32U) gp_malloc_align(32768, 32);
	if (!buf) {
		DEBUG_MSG(DBG_PRINT("buf allocate fail.\r\n"));
	}
	lseek(pAviEncPacker->file_handle, 0, SEEK_END);
	lseek(pAviEncPacker->index_handle, 0, SEEK_SET);


	while(1) {
		len = read(pAviEncPacker->index_handle, buf, 32768);
		if (len) {
			write(pAviEncPacker->file_handle, buf, len);
		}else {
			break;
		}
	}
	*/

// has done file cat function in new AVIPackerV3, dominant mark
//	file_cat(pAviEncPacker->file_handle,pAviEncPacker->index_handle);  // dominant mark, move into AVIPacker library

	nRet = close(pAviEncPacker->file_handle);
	nRet = close(pAviEncPacker->index_handle);
    nRet = unlink2((CHAR*)pAviEncPacker->index_path);
	pAviEncPacker->file_handle = -1;
	pAviEncPacker->index_handle = -1;
	//gp_free((INT32U*)buf);

	return nRet;
}

INT32S avi_encode_set_avi_header(AviEncPacker_t *pAviEncPacker)
{
	INT16U sample_per_block, package_size;

	pAviEncPacker->p_avi_aud_stream_header = &avi_aud_stream_header;
	pAviEncPacker->p_avi_vid_stream_header = &avi_vid_stream_header;
	pAviEncPacker->p_avi_bitmap_info = &avi_bitmap_info;
	pAviEncPacker->p_avi_wave_info = &avi_wave_info;
	gp_memset((INT8S*)pAviEncPacker->p_avi_aud_stream_header, 0, sizeof(GP_AVI_AVISTREAMHEADER));
	gp_memset((INT8S*)pAviEncPacker->p_avi_vid_stream_header, 0, sizeof(GP_AVI_AVISTREAMHEADER));
	gp_memset((INT8S*)pAviEncPacker->p_avi_bitmap_info, 0, sizeof(GP_AVI_BITMAPINFO));
	gp_memset((INT8S*)pAviEncPacker->p_avi_wave_info, 0, sizeof(GP_AVI_PCMWAVEFORMAT));

	//audio
	avi_aud_stream_header.fccType[0] = 'a';
	avi_aud_stream_header.fccType[1] = 'u';
	avi_aud_stream_header.fccType[2] = 'd';
	avi_aud_stream_header.fccType[3] = 's';

	switch(pAviEncAudPara->audio_format)
	{
	case WAVE_FORMAT_PCM:
		pAviEncPacker->wave_info_cblen = 16;
		avi_aud_stream_header.fccHandler[0] = 1;
		avi_aud_stream_header.fccHandler[1] = 0;
		avi_aud_stream_header.fccHandler[2] = 0;
		avi_aud_stream_header.fccHandler[3] = 0;

		avi_wave_info.wFormatTag = WAVE_FORMAT_PCM;
		avi_wave_info.nChannels = pAviEncAudPara->channel_no;
		avi_wave_info.nSamplesPerSec =  pAviEncAudPara->audio_sample_rate;
		avi_wave_info.nAvgBytesPerSec =  pAviEncAudPara->channel_no * pAviEncAudPara->audio_sample_rate * 2;
		avi_wave_info.nBlockAlign = pAviEncAudPara->channel_no * 2;
		avi_wave_info.wBitsPerSample = 16; //16bit

		avi_aud_stream_header.dwScale = avi_wave_info.nBlockAlign;
		avi_aud_stream_header.dwRate = avi_wave_info.nAvgBytesPerSec;
		avi_aud_stream_header.dwSampleSize = avi_wave_info.nBlockAlign;;
		break;

	case WAVE_FORMAT_ADPCM:
		pAviEncPacker->wave_info_cblen = 50;
		avi_aud_stream_header.fccHandler[0] = 0;
		avi_aud_stream_header.fccHandler[1] = 0;
		avi_aud_stream_header.fccHandler[2] = 0;
		avi_aud_stream_header.fccHandler[3] = 0;

		package_size = 0x100;
		if(pAviEncAudPara->channel_no == 1)
			sample_per_block = 2 * package_size - 12;
		else if(pAviEncAudPara->channel_no == 2)
			sample_per_block = package_size - 12;
		else
			sample_per_block = 1;

		avi_wave_info.wFormatTag = WAVE_FORMAT_ADPCM;
		avi_wave_info.nChannels = pAviEncAudPara->channel_no;
		avi_wave_info.nSamplesPerSec =  pAviEncAudPara->audio_sample_rate;
		avi_wave_info.nAvgBytesPerSec =  package_size * pAviEncAudPara->audio_sample_rate / sample_per_block; // = PackageSize * FrameSize / fs
		avi_wave_info.nBlockAlign = package_size; //PackageSize
		avi_wave_info.wBitsPerSample = 4; //4bit
		avi_wave_info.cbSize = 32;
		// extension ...
		avi_wave_info.ExtInfo[0] = 0x01F4;	avi_wave_info.ExtInfo[1] = 0x0007;
		avi_wave_info.ExtInfo[2] = 0x0100;	avi_wave_info.ExtInfo[3] = 0x0000;
		avi_wave_info.ExtInfo[4] = 0x0200;	avi_wave_info.ExtInfo[5] = 0xFF00;
		avi_wave_info.ExtInfo[6] = 0x0000;	avi_wave_info.ExtInfo[7] = 0x0000;

		avi_wave_info.ExtInfo[8] =  0x00C0;	avi_wave_info.ExtInfo[9] =  0x0040;
		avi_wave_info.ExtInfo[10] = 0x00F0; avi_wave_info.ExtInfo[11] = 0x0000;
		avi_wave_info.ExtInfo[12] = 0x01CC; avi_wave_info.ExtInfo[13] = 0xFF30;
		avi_wave_info.ExtInfo[14] = 0x0188; avi_wave_info.ExtInfo[15] = 0xFF18;
		break;

	case WAVE_FORMAT_IMA_ADPCM:
		pAviEncPacker->wave_info_cblen = 20;
		avi_aud_stream_header.fccHandler[0] = 0;
		avi_aud_stream_header.fccHandler[1] = 0;
		avi_aud_stream_header.fccHandler[2] = 0;
		avi_aud_stream_header.fccHandler[3] = 0;

		package_size = 0x100;
		if(pAviEncAudPara->channel_no == 1)
			sample_per_block = 2 * package_size - 7;
		else if(pAviEncAudPara->channel_no == 2)
			sample_per_block = package_size - 7;
		else
			sample_per_block = 1;

		avi_wave_info.wFormatTag = WAVE_FORMAT_IMA_ADPCM;
		avi_wave_info.nChannels = pAviEncAudPara->channel_no;
		avi_wave_info.nSamplesPerSec =  pAviEncAudPara->audio_sample_rate;
		avi_wave_info.nAvgBytesPerSec =  package_size * pAviEncAudPara->audio_sample_rate / sample_per_block;
		avi_wave_info.nBlockAlign = package_size;	//PackageSize
		avi_wave_info.wBitsPerSample = 4; //4bit
		avi_wave_info.cbSize = 2;
		// extension ...
		avi_wave_info.ExtInfo[0] = sample_per_block;
		break;

	default:
		pAviEncPacker->wave_info_cblen = 0;
		pAviEncPacker->p_avi_aud_stream_header = NULL;
		pAviEncPacker->p_avi_wave_info = NULL;
	}

	avi_aud_stream_header.dwScale = avi_wave_info.nBlockAlign;
	avi_aud_stream_header.dwRate = avi_wave_info.nAvgBytesPerSec;
	avi_aud_stream_header.dwSampleSize = avi_wave_info.nBlockAlign;

	//video
	avi_vid_stream_header.fccType[0] = 'v';
	avi_vid_stream_header.fccType[1] = 'i';
	avi_vid_stream_header.fccType[2] = 'd';
	avi_vid_stream_header.fccType[3] = 's';
	avi_vid_stream_header.dwScale = pAviEncVidPara->dwScale;
	avi_vid_stream_header.dwRate = pAviEncVidPara->dwRate;
	avi_vid_stream_header.rcFrame.right = pAviEncVidPara->encode_width;
	avi_vid_stream_header.rcFrame.bottom = pAviEncVidPara->encode_height;
	switch(pAviEncVidPara->video_format)
	{
	case C_MJPG_FORMAT:
		avi_vid_stream_header.fccHandler[0] = 'm';
		avi_vid_stream_header.fccHandler[1] = 'j';
		avi_vid_stream_header.fccHandler[2] = 'p';
		avi_vid_stream_header.fccHandler[3] = 'g';

		avi_bitmap_info.biSize = pAviEncPacker->bitmap_info_cblen = 40;
		avi_bitmap_info.biWidth = pAviEncVidPara->encode_width;
		avi_bitmap_info.biHeight = pAviEncVidPara->encode_height;
		avi_bitmap_info.biBitCount = 24;
		avi_bitmap_info.biCompression[0] = 'M';
		avi_bitmap_info.biCompression[1] = 'J';
		avi_bitmap_info.biCompression[2] = 'P';
		avi_bitmap_info.biCompression[3] = 'G';
		avi_bitmap_info.biSizeImage = pAviEncVidPara->encode_width * pAviEncVidPara->encode_height * 3;
		break;

	case C_XVID_FORMAT:
		avi_vid_stream_header.fccHandler[0] = 'x';
		avi_vid_stream_header.fccHandler[1] = 'v';
		avi_vid_stream_header.fccHandler[2] = 'i';
		avi_vid_stream_header.fccHandler[3] = 'd';

		avi_bitmap_info.biSize = pAviEncPacker->bitmap_info_cblen = 68;
		avi_bitmap_info.biWidth = pAviEncVidPara->encode_width;
		avi_bitmap_info.biHeight = pAviEncVidPara->encode_height;
		avi_bitmap_info.biBitCount = 24;
		avi_bitmap_info.biCompression[0] = 'X';
		avi_bitmap_info.biCompression[1] = 'V';
		avi_bitmap_info.biCompression[2] = 'I';
		avi_bitmap_info.biCompression[3] = 'D';
		avi_bitmap_info.biSizeImage = pAviEncVidPara->encode_width * pAviEncVidPara->encode_height * 3;
		break;
	}

	return STATUS_OK;
}

void avi_encode_set_curworkmem(void *pAviEncPacker)
{
	 pAviEncPara->AviPackerCur = pAviEncPacker;
}

// status
void avi_encode_set_status(INT32U bit)
{
	pAviEncPara->avi_encode_status |= bit;
}

void avi_encode_clear_status(INT32U bit)
{
	pAviEncPara->avi_encode_status &= ~bit;
}

INT32S avi_encode_get_status(void)
{
    return pAviEncPara->avi_encode_status;
}

//memory
void vid_enc_buffer_lock(INT32U addr)
{
	g_video_lock_addr = addr;
}

void vid_enc_buffer_unlock(void)
{
	g_video_lock_addr = 0xFFFFFFFF;
}

INT32U avi_encode_get_csi_frame(void)
{
	INT32U addr;

	addr =  pAviEncVidPara->csi_frame_addr[g_csi_index++];
	if(g_csi_index >= AVI_ENCODE_CSI_BUFFER_NO)
		g_csi_index = 0;

	return addr;
}

INT32U avi_encode_get_scaler_frame(void)
{
	INT32U addr;

	addr = pAviEncVidPara->scaler_output_addr[g_scaler_index++];
	if(g_scaler_index >= AVI_ENCODE_SCALER_BUFFER_NO)
		g_scaler_index = 0;

	return addr;
}

INT32U avi_encode_get_display_frame(void)
{
	INT32U	addr, lock_cnt;

	lock_cnt = AVI_ENCODE_DISPALY_BUFFER_NO;

	do {
		g_display_index++;
		if (g_display_index >= AVI_ENCODE_DISPALY_BUFFER_NO) {
			g_display_index = 0;
		}
		if (pAviEncVidPara->display_output_addr[g_display_index] & MSG_SCALER_TASK_PREVIEW_LOCK) {
			addr = NULL;
		} else {
			addr = (INT32U) &pAviEncVidPara->display_output_addr[g_display_index];
//			addr = (INT32U) pAviEncVidPara->display_output_addr[g_display_index];
			return addr;
		}
		lock_cnt--;
	} while (lock_cnt);
	return NULL;
}

INT32U avi_encode_get_video_frame(void)
{
	INT32U addr;

	do{
		addr = pAviEncVidPara->video_encode_addr[g_video_index++];
		if(g_video_index >= AVI_ENCODE_VIDEO_BUFFER_NO)
			g_video_index = 0;
	}while(addr == g_video_lock_addr);
	return addr;
}

static INT32S sensor_mem_alloc(void)
{
	INT32S i, buffer_size, nRet;

#if VIDEO_ENCODE_MODE == C_VIDEO_ENCODE_FRAME_MODE
	buffer_size = pAviEncVidPara->sensor_capture_width * pAviEncVidPara->sensor_capture_height << 1;
#elif VIDEO_ENCODE_MODE == C_VIDEO_ENCODE_FIFO_MODE
	buffer_size = pAviEncVidPara->sensor_capture_width * SENSOR_FIFO_LINE << 1;
#endif
	for(i=0; i<AVI_ENCODE_CSI_BUFFER_NO; i++)
	{
		if(!pAviEncVidPara->csi_frame_addr[i]) {
			pAviEncVidPara->csi_frame_addr[i] = (INT32U) gp_malloc_align(buffer_size, 32);
        }

		if(!pAviEncVidPara->csi_frame_addr[i]) {
            RETURN(STATUS_FAIL);
        }
		DEBUG_MSG(DBG_PRINT("sensor_frame_addr[%d] = 0x%x\r\n", i, pAviEncVidPara->csi_frame_addr[i]));
	}
	nRet = STATUS_OK;
Return:
    if (nRet!=STATUS_OK)
    {
        sensor_mem_free();
    }
	return nRet;
}

static INT32S scaler_mem_alloc(void)
{
	INT32S i, buffer_size, nRet;

#if 0
	buffer_size = pAviEncVidPara->encode_width * pAviEncVidPara->encode_height << 1;
	for(i=0; i<AVI_ENCODE_SCALER_BUFFER_NO; i++)
	{
		if(!pAviEncVidPara->scaler_output_addr[i]) {
			pAviEncVidPara->scaler_output_addr[i] = (INT32U) gp_malloc_align(buffer_size, 32);
        }
		if(!pAviEncVidPara->scaler_output_addr[i]) {
        	RETURN(STATUS_FAIL);
        }
		DEBUG_MSG(DBG_PRINT("scaler_frame_addr[%d] = 0x%x\r\n", i, pAviEncVidPara->scaler_output_addr[i]));
	}
#endif

#if VIDEO_ENCODE_MODE == C_VIDEO_ENCODE_FRAME_MODE
	if((AVI_ENCODE_DIGITAL_ZOOM_EN == 1) || pAviEncVidPara->scaler_flag)
	{
		buffer_size = pAviEncVidPara->encode_width * pAviEncVidPara->encode_height << 1;
		for(i=0; i<AVI_ENCODE_SCALER_BUFFER_NO; i++)
		{
			if(!pAviEncVidPara->scaler_output_addr[i]) {
				pAviEncVidPara->scaler_output_addr[i] = (INT32U) gp_malloc_align(buffer_size, 32);
            }
			if(!pAviEncVidPara->scaler_output_addr[i]) {
                RETURN(STATUS_FAIL);
            }
			DEBUG_MSG(DBG_PRINT("scaler_frame_addr[%d] = 0x%x\r\n", i, pAviEncVidPara->scaler_output_addr[i]));
		}
	}
#elif VIDEO_ENCODE_MODE == C_VIDEO_ENCODE_FIFO_MODE
	if(pAviEncVidPara->scaler_flag)
	{
		nRet = pAviEncVidPara->encode_height /(pAviEncVidPara->sensor_capture_height / SENSOR_FIFO_LINE);
		nRet += 2; //+2 is to fix block line
		buffer_size = pAviEncVidPara->encode_width * nRet << 1;
		for(i=0; i<AVI_ENCODE_SCALER_BUFFER_NO; i++)
		{
			if(!pAviEncVidPara->scaler_output_addr[i]) {
				pAviEncVidPara->scaler_output_addr[i] = (INT32U) gp_malloc_align(buffer_size, 32);
            }
			if(!pAviEncVidPara->scaler_output_addr[i]) {
                RETURN(STATUS_FAIL);
            }
			DEBUG_MSG(DBG_PRINT("scaler_frame_addr[%d] = 0x%x\r\n", i, pAviEncVidPara->scaler_output_addr[i]));
		}
	}
#endif
	nRet = STATUS_OK;
Return:
    if(nRet!=STATUS_OK) {  // dominant add
        scaler_mem_free();
    }

	return nRet;
}

static INT32S display_mem_alloc(void)
{
#if AVI_ENCODE_PREVIEW_DISPLAY_EN == 1
	INT32S nRet;
	INT32U buffer_size, i;

	if(pAviEncVidPara->dispaly_scaler_flag)
	{
		buffer_size = pAviEncVidPara->display_buffer_width * pAviEncVidPara->display_buffer_height << 1;
		for(i=0; i<AVI_ENCODE_DISPALY_BUFFER_NO; i++)
		{
			if(!pAviEncVidPara->display_output_addr[i]) {
				pAviEncVidPara->display_output_addr[i] = (INT32U) gp_malloc_align(buffer_size, 32);
            }

			if(!pAviEncVidPara->display_output_addr[i]) {
                RETURN(STATUS_FAIL);
            }
			DEBUG_MSG(DBG_PRINT("display_frame_addr[%d]= 0x%x\r\n", i, pAviEncVidPara->display_output_addr[i]));
		}
	}
	nRet = STATUS_OK;
Return:
    if (nRet!=STATUS_OK)
    {
        //display_mem_free();
    }

	return nRet;
#else
	return STATUS_OK;
#endif
}

static INT32S video_mem_alloc(void)
{
	INT32S i, buffer_size, nRet;

#if AVI_ENCODE_VIDEO_ENCODE_EN == 1
	//buffer_size = pAviEncVidPara->encode_width * pAviEncVidPara->encode_height >> 1;
	buffer_size = 200*1024;
	for(i=0; i<AVI_ENCODE_VIDEO_BUFFER_NO; i++)
	{
		if(!pAviEncVidPara->video_encode_addr[i]) {
			pAviEncVidPara->video_encode_addr[i] = (INT32U) gp_malloc_align(buffer_size, 32);
        }

		if(!pAviEncVidPara->video_encode_addr[i]) {
            RETURN(STATUS_FAIL);
        }
		DEBUG_MSG(DBG_PRINT("jpeg_frame_addr[%d]   = 0x%x\r\n", i, pAviEncVidPara->video_encode_addr[i]));
	}
#endif
	nRet = STATUS_OK;
Return:
    if (nRet!=STATUS_OK)  // Dominant add
    {
        video_mem_free();
    }
	return nRet;
}

static INT32S AviPacker_mem_alloc(AviEncPacker_t *pAviEncPacker)
{
	INT32S nRet;

#if AVI_ENCODE_VIDEO_ENCODE_EN == 1
	pAviEncPacker->file_buffer_size = FileWriteBuffer_Size;
	if (!pAviEncPacker->file_write_buffer) {
		pAviEncPacker->file_write_buffer = (INT32U *) gp_malloc(FileWriteBuffer_Size);
	}

	if (!pAviEncPacker->file_write_buffer) {
		RETURN(STATUS_FAIL);
	}

	pAviEncPacker->index_buffer_size = IndexBuffer_Size;
	if (!pAviEncPacker->index_write_buffer) {
		pAviEncPacker->index_write_buffer = (INT32U *) gp_malloc(IndexBuffer_Size);
	}

	if (!pAviEncPacker->index_write_buffer) {
		RETURN(STATUS_FAIL);
	}

	if (!pAviEncPacker->avi_workmem) {
		pAviEncPacker->avi_workmem = (void *) gp_malloc(AviPackerV3_GetWorkMemSize());
	}
	if (!pAviEncPacker->avi_workmem) {
		RETURN(STATUS_FAIL);
	}
	gp_memset((INT8S *) pAviEncPacker->avi_workmem, 0x00, AviPackerV3_GetWorkMemSize());
	DEBUG_MSG(DBG_PRINT("file_write_buffer    = 0x%x\r\n", pAviEncPacker->file_write_buffer));
	DEBUG_MSG(DBG_PRINT("index_write_buffer   = 0x%x\r\n", pAviEncPacker->index_write_buffer));
#endif
	nRet = STATUS_OK;
Return:
	if (nRet != STATUS_OK) {	// Dominant add
		AviPacker_mem_free(pAviEncPacker);
	}
	return nRet;
}

static void sensor_mem_free(void)
{
	INT32S i;
	for(i=0; i<AVI_ENCODE_CSI_BUFFER_NO; i++)
	{
		if(pAviEncVidPara->csi_frame_addr[i]) {
			gp_free((void*)pAviEncVidPara->csi_frame_addr[i]);
		pAviEncVidPara->csi_frame_addr[i] = 0;
	}
}
}

static void scaler_mem_free(void)
{
	INT32S i;
	for(i=0; i<AVI_ENCODE_SCALER_BUFFER_NO; i++)
	{
		if(pAviEncVidPara->scaler_output_addr[i])
			gp_free((void*)pAviEncVidPara->scaler_output_addr[i]);

		pAviEncVidPara->scaler_output_addr[i] = 0;
	}
}
/*
static void display_mem_free(void)
{
	INT32S i;
	for(i=0; i<AVI_ENCODE_DISPALY_BUFFER_NO; i++)
	{
		if(pAviEncVidPara->display_output_addr[i])
			gp_free((void*)(pAviEncVidPara->display_output_addr[i] & 0xFFFFFF));

		pAviEncVidPara->display_output_addr[i] = 0;
	}
}
*/
static void video_mem_free(void)
{
    INT32S i;
    for(i=0; i<AVI_ENCODE_VIDEO_BUFFER_NO; i++)
    {
        if(pAviEncVidPara->video_encode_addr[i]) {
            gp_free((void*)pAviEncVidPara->video_encode_addr[i]);
            pAviEncVidPara->video_encode_addr[i] = 0;
        }
    }
}

static void AviPacker_mem_free(AviEncPacker_t *pAviEncPacker)
{
	if(pAviEncPacker->file_write_buffer)
		gp_free((void*)pAviEncPacker->file_write_buffer);

	if(pAviEncPacker->index_write_buffer)
		gp_free((void*)pAviEncPacker->index_write_buffer);
	if(pAviEncPacker->avi_workmem)
		gp_free((void*)pAviEncPacker->avi_workmem);

	pAviEncPacker->file_write_buffer = 0;
	pAviEncPacker->index_write_buffer = 0;
	pAviEncPacker->avi_workmem = 0;
}

INT32S avi_encode_memory_alloc(void)
{
	INT32S nRet;

DBG_PRINT ("\r\n==========AVI MEM ALLOCATE\r\n");

	//inti index
	g_csi_index = 0;
	g_scaler_index = 0;
	g_display_index = 0;
	g_pcm_index = 0;
	g_video_index = 0;
	g_video_lock_addr = 0xFFFFFFFF;
	if(sensor_mem_alloc() < 0) RETURN(STATUS_FAIL);
	if(scaler_mem_alloc() < 0) RETURN(STATUS_FAIL);
	if(display_mem_alloc() < 0) RETURN(STATUS_FAIL);
	if(video_mem_alloc() < 0) RETURN(STATUS_FAIL);
	if(AviPacker_mem_alloc(pAviEncPacker0) < 0) RETURN(STATUS_FAIL);
#if AVI_ENCODE_FAST_SWITCH_EN == 1
	if(AviPacker_mem_alloc(pAviEncPacker1) < 0) RETURN(STATUS_FAIL);
#endif
	nRet = STATUS_OK;
Return:
	return nRet;
}

void avi_encode_memory_free(void)
{
DBG_PRINT ("\r\n==========AVI MEM FREE\r\n");


	sensor_mem_free();
	scaler_mem_free();
	//display_mem_free();
	video_mem_free();
	AviPacker_mem_free(pAviEncPacker0);
#if AVI_ENCODE_FAST_SWITCH_EN == 1
	AviPacker_mem_free(pAviEncPacker1);
#endif
}

INT32S avi_encode_packer_memory_alloc(void)
{
	INT32S nRet;

	if(AviPacker_mem_alloc(pAviEncPacker0) < 0) RETURN(STATUS_FAIL);
#if AVI_ENCODE_FAST_SWITCH_EN == 1
	if(AviPacker_mem_alloc(pAviEncPacker1) < 0) RETURN(STATUS_FAIL);
#endif
	nRet = STATUS_OK;
Return:
	return nRet;
}

void avi_encode_packer_memory_free(void)
{
	AviPacker_mem_free(pAviEncPacker0);
#if AVI_ENCODE_FAST_SWITCH_EN == 1
	AviPacker_mem_free(pAviEncPacker1);
#endif
}

#if MPEG4_ENCODE_ENABLE == 1
INT32S avi_encode_mpeg4_malloc(INT16U width, INT16U height)
{
	INT32S size, nRet;

	size = 16*width*2*3/4;
	pAviEncPara->isram_addr = (INT32U)gp_iram_malloc_align(size, 32);
	if(!pAviEncPara->isram_addr) RETURN(STATUS_FAIL);

	size = width*height*2;
	pAviEncPara->write_refer_addr = (INT32U)gp_malloc_align(size, 32);
	if(!pAviEncPara->write_refer_addr) RETURN(STATUS_FAIL);

	size = width*height*2;
	pAviEncPara->reference_addr = (INT32U)gp_malloc_align(size, 32);
	if(!pAviEncPara->reference_addr) RETURN(STATUS_FAIL);

	nRet = STATUS_OK;
Return:
	return nRet;
}

void avi_encode_mpeg4_free(void)
{
	gp_free((void*)pAviEncPara->isram_addr);
	pAviEncPara->isram_addr = 0;

	gp_free((void*)pAviEncPara->write_refer_addr);
	pAviEncPara->write_refer_addr = 0;

	gp_free((void*)pAviEncPara->reference_addr);
	pAviEncPara->reference_addr = 0;
}
#endif //MPEG4_ENCODE_ENABLE

// format
void avi_encode_set_sensor_format(INT32U format)
{
	if(format == BITMAP_YUYV)
    	pAviEncVidPara->sensor_output_format = C_SCALER_CTRL_IN_YUYV;
    else if(format ==  BITMAP_RGRB)
    	pAviEncVidPara->display_output_format = C_SCALER_CTRL_IN_RGBG;
    else
    	pAviEncVidPara->sensor_output_format = C_SCALER_CTRL_IN_YUYV;
}

void avi_encode_set_display_format(INT32U format)
{
	if(format == IMAGE_OUTPUT_FORMAT_RGB565)
		pAviEncVidPara->display_output_format = C_SCALER_CTRL_OUT_RGB565;
    else if(format == IMAGE_OUTPUT_FORMAT_RGBG)
    	pAviEncVidPara->display_output_format = C_SCALER_CTRL_OUT_RGBG;
	else if(format == IMAGE_OUTPUT_FORMAT_YUYV)
    	pAviEncVidPara->display_output_format = C_SCALER_CTRL_OUT_YUYV;
	else if(format == IMAGE_OUTPUT_FORMAT_GRGB)
    	pAviEncVidPara->display_output_format = C_SCALER_CTRL_OUT_GRGB;
    else if(format == IMAGE_OUTPUT_FORMAT_UYVY)
    	pAviEncVidPara->display_output_format = C_SCALER_CTRL_OUT_UYVY;
    else
    	pAviEncVidPara->display_output_format = C_SCALER_CTRL_OUT_YUYV;
}

void avi_encode_set_display_scaler(void)
{
	if((pAviEncVidPara->encode_width != pAviEncVidPara->display_buffer_width)||
		(pAviEncVidPara->encode_height != pAviEncVidPara->display_buffer_height) ||
		(pAviEncVidPara->display_output_format != C_SCALER_CTRL_OUT_YUYV))
		pAviEncVidPara->dispaly_scaler_flag = 1;
	else
		pAviEncVidPara->dispaly_scaler_flag = 0;

	if((pAviEncVidPara->sensor_capture_width != pAviEncVidPara->encode_width) ||
		(pAviEncVidPara->sensor_capture_height != pAviEncVidPara->encode_height))
		pAviEncVidPara->scaler_flag = 1;
	else
		pAviEncVidPara->scaler_flag = 0;
}

INT32S avi_encode_set_jpeg_quality(INT8U quality_value)
{
	INT8U *src;
	INT32U i, header_size, video_frame;

	switch(quality_value)
	{
	case 35: src = jpeg_422_q35_header; break;
	case 50: src = jpeg_422_q50_header; break;
	case 70: src = jpeg_422_q70_header; break;
	case 80: src = jpeg_422_q80_header; break;
	case 90: src = jpeg_422_q90_header; break;
	default:
		quality_value = 50;
		src = jpeg_422_q50_header;
		break;
	}

    pAviEncVidPara->quality_value = quality_value;
    header_size = sizeof(jpeg_422_q50_header);
	//set jpeg width and height to buffer
    for(i = 0; i<AVI_ENCODE_VIDEO_BUFFER_NO; i++)
    {
    	video_frame = avi_encode_get_video_frame();
    	gp_memcpy((INT8S*)video_frame, (INT8S*)src, header_size);
    	src = (INT8U *)video_frame;
		*(src+0x9E) = (pAviEncVidPara->encode_height >> 8);
	    *(src+0x9F) = (pAviEncVidPara->encode_height & 0xFF);
	    *(src+0xA0) = (pAviEncVidPara->encode_width >> 8);
	    *(src+0xA1) = (pAviEncVidPara->encode_width & 0xFF);
	    if (pAviEncVidPara->scaler_zoom_ratio != 1) {
	    	*(src+0xA4) = 0x22;
	    }
    }
	return header_size;
}

INT32S avi_encode_set_mp4_resolution(INT16U encode_width, INT16U encode_height)
{
	INT8U *src;
	INT32U i, header_size, video_frame;

	if(encode_width == 640 && encode_height == 480)
		src = mpeg4_header_vga;
 	else if(encode_width == 320 && encode_height == 240)
 		src = mpeg4_header_qvga;
 	else if(encode_width == 160 && encode_height == 120)
 		src = mpeg4_header_qqvga;
 	else if(encode_width == 352 && encode_height == 288)
 		src = mpeg4_header_cif;
 	else if(encode_width == 176 && encode_height == 144)
 		src = mpeg4_header_qcif;
 	else if(encode_width == 128 && encode_height == 96)
 		src = mpeg4_header_sqcif;
 	else
 	{
 		DEBUG_MSG(DBG_PRINT("mpeg4 header fail!!!\r\n"));
 		return STATUS_FAIL;
 	}

	header_size = sizeof(mpeg4_header_vga);
	for(i = 0; i<AVI_ENCODE_VIDEO_BUFFER_NO; i++)
	{
	   	video_frame = avi_encode_get_video_frame();
	  	gp_memcpy((INT8S*)video_frame, (INT8S*)src, header_size);
	}
	return header_size;
}

BOOLEAN avi_encode_is_pause(void)
{
	INT32U status;
	status = pAviEncPara->avi_encode_status & C_AVI_ENCODE_PAUSE;
	if(!status)
		return FALSE;

	return TRUE;
}

// check disk free size
BOOLEAN avi_encode_disk_size_is_enough(INT32S cb_write_size)
{
#if AVI_ENCODE_CAL_DISK_SIZE_EN
	INT32U temp;
	INT32S nRet;
	INT64U disk_free_size;

	temp = pAviEncPara->record_total_size;
	disk_free_size = pAviEncPara->disk_free_size;
	temp += cb_write_size;
	if(temp >= AVI_FILE_MAX_RECORD_SIZE) RETURN(FALSE);
	if(temp >= disk_free_size) RETURN(FALSE);
	pAviEncPara->record_total_size = temp;
#endif

	nRet = TRUE;
Return:
	return nRet;
}
// csi frame mode switch buffer
void avi_encode_switch_csi_frame_buffer(void)
{
    INT8U err;
	INT32U frame, ready_frame;

    frame = (INT32U) OSQAccept(cmos_frame_q, &err);
    if ((err != OS_NO_ERR) || !frame) {
		frame = 0x40000000;
		//DEBUG_MSG(DBG_PRINT("L"));
	}
	ready_frame = *P_CSI_TG_FBSADDR;
	*P_CSI_TG_FBSADDR = frame;
	if (ready_frame!=0x40000000 && ready_frame!=frame) {
    	if (OSQPost(scaler_task_q, (void *)ready_frame)) {
    		OSQPost(cmos_frame_q, (void *)ready_frame);
    	}
    }
}
// csi fifo mode switch buffer
void vid_enc_csi_fifo_end(void)
{
	INT8U err;
	INT32U ready_frame;

	//R_IOC_O_DATA ^= 0x04;
	pAviEncPara->fifo_irq_cnt++;
	if((pAviEncPara->fifo_irq_cnt - pAviEncPara->vid_pend_cnt) >= 2)
	{
		pAviEncPara->fifo_enc_err_flag = 1;
		if(pAviEncPara->avi_encode_status & C_AVI_VID_ENC_START)
			DEBUG_MSG(DBG_PRINT("X"));
		return;
	}

	if(((pAviEncPara->avi_encode_status & C_AVI_VID_ENC_START) == 0) ||
		pAviEncPara->fifo_enc_err_flag)
	{
		return;
	}

	ready_frame = (INT32U) OSQAccept(cmos_frame_q, &err);
	if(err != OS_NO_ERR || !ready_frame)
	{
		DEBUG_MSG(DBG_PRINT("l"));
		pAviEncPara->fifo_enc_err_flag = 1;
		return;
	}

	if(pAviEncPara->fifo_irq_cnt >= pAviEncPara->vid_post_cnt)
		ready_frame |= C_AVI_ENCODE_FRAME_END;

	err = OSQPost(vid_enc_task_q, (void *)ready_frame);
	if(err != OS_NO_ERR)
	{
		DEBUG_MSG(DBG_PRINT("L"));
		pAviEncPara->fifo_enc_err_flag = 1;
		return;
	}
}
// csi fifo mode frame end, take time: post 30 times 80us
void vid_enc_csi_frame_end(void)
{
	INT8U i;

	if(pAviEncPara->fifo_irq_cnt != pAviEncPara->vid_post_cnt)
	{
		DEBUG_MSG(DBG_PRINT("[%x]\r\n", pAviEncPara->fifo_irq_cnt));
		pAviEncPara->fifo_enc_err_flag = 1;
		//while(1);
	}

	if(pAviEncPara->vid_post_cnt && pAviEncPara->fifo_enc_err_flag)
	{
		OSQFlush(vid_enc_task_q);
		OSQPost(vid_enc_task_q, (void *)C_AVI_ENCODE_FIFO_ERR);
	}

	pAviEncPara->fifo_enc_err_flag = 0;
	pAviEncPara->fifo_irq_cnt = 0;
	OSQFlush(cmos_frame_q);
	for(i = 0; i<pAviEncPara->vid_post_cnt; i++)
		OSQPost(cmos_frame_q, (void *) avi_encode_get_csi_frame());
}

INT32S scaler_zoom_once(INT32U scaler_mode, FP32 bScalerFactor,
						INT32U InputFormat, INT32U OutputFormat,
						INT16U input_x, INT16U input_y,
						INT16U output_x, INT16U output_y,
						INT16U output_buffer_x, INT16U output_buffer_y,
						INT32U scaler_input_y, INT32U scaler_input_u, INT32U scaler_input_v,
						INT32U scaler_output_y, INT32U scaler_output_u, INT32U scaler_output_v)
{
	FP32    zoom_temp, ratio;
	INT32S  scaler_status;
	INT32U  temp_x, temp_y;

	scaler_app_lock();

  	// Initiate Scaler
  	scaler_init();
	switch(scaler_mode)
	{
	case C_SCALER_FULL_SCREEN:
		scaler_image_pixels_set(input_x, input_y, output_buffer_x, output_buffer_y);
		break;

	case C_SCALER_FIT_RATIO:
		temp_x = (input_x<<16) / output_x;
		temp_y = (input_y<<16) / output_y;
		scaler_input_pixels_set(input_x, input_y);
		scaler_input_visible_pixels_set(input_x, input_y);
		scaler_output_pixels_set(temp_x, temp_y, output_buffer_x, output_buffer_y);
		scaler_input_offset_set(0, 0);
		break;

	case C_NO_SCALER_FIT_BUFFER:
		if((input_x <= output_x) && (input_y <= output_y))
		{
			temp_x = temp_y = (1<<16);
		}
		else
		{
			if (output_y*input_x > output_x*input_y)
			{
	      			temp_x = temp_y = (input_x<<16) / output_x;
	      			output_y = (output_y<<16) / temp_x;
	     	 	}
	   	   	else
	 	     	{
	  	    		temp_x = temp_y = (input_y<<16) / output_y;
	  	    		output_x = (input_x<<16) / temp_x;
		      	}
		}
		scaler_input_pixels_set(input_x, input_y);
		scaler_input_visible_pixels_set(input_x, input_y);
		scaler_output_pixels_set(temp_x, temp_y, output_buffer_x, output_buffer_y);
		scaler_input_offset_set(0, 0);
		break;

	case C_SCALER_FIT_BUFFER:
		if (output_y*input_x > output_x*input_y)
		{
			temp_x = (input_x<<16) / output_x;
			output_y = (output_y<<16) / temp_x;
		}
		else
		{
			temp_x = (input_y<<16) / output_y;
			output_x = (input_x<<16) / temp_x;
		}
		scaler_input_pixels_set(input_x, input_y);
		scaler_input_visible_pixels_set(input_x, input_y);
		scaler_output_pixels_set(temp_x, temp_x, output_buffer_x, output_buffer_y);
		scaler_input_offset_set(0, 0);
		break;

	case C_SCALER_ZOOM_FIT_BUFFER:
		scaler_input_pixels_set(input_x, input_y);
		scaler_input_visible_pixels_set(input_x, input_y);
		ratio = output_x/input_x;
		zoom_temp = 65536 / (ratio * bScalerFactor);
		scaler_output_pixels_set((int)zoom_temp, (int)zoom_temp, output_x, output_y);
		zoom_temp = 1 - (1 / bScalerFactor);
		scaler_input_offset_set((int)((float)(output_x/2)*zoom_temp)<<16, (int)((float)(output_y/2)*zoom_temp)<<16);
		break;

	default:
		return -1;
	}

	scaler_input_addr_set(scaler_input_y, scaler_input_u, scaler_input_v);
   	scaler_output_addr_set(scaler_output_y, scaler_output_u, scaler_output_v);
   	scaler_fifo_line_set(C_SCALER_CTRL_FIFO_DISABLE);
	scaler_input_format_set(InputFormat);
	scaler_output_format_set(OutputFormat);
	scaler_out_of_boundary_color_set(0x008080);

	while(1)
	{
		scaler_status = scaler_wait_idle();
		if (scaler_status == C_SCALER_STATUS_STOP)
			scaler_start();
		else if (scaler_status & C_SCALER_STATUS_DONE)
		{
			scaler_stop();
			break;
		}
		else if (scaler_status & C_SCALER_STATUS_TIMEOUT)
		{
			DEBUG_MSG(DBG_PRINT("Scaler Timeout failed to finish its job\r\n"));
		}
		else if(scaler_status & C_SCALER_STATUS_INIT_ERR)
		{
			DEBUG_MSG(DBG_PRINT("Scaler INIT ERR failed to finish its job\r\n"));
		}
		else if (scaler_status & C_SCALER_STATUS_INPUT_EMPTY)
		{
  			scaler_restart();
  		}
  		else
  		{
	  		DEBUG_MSG(DBG_PRINT("Un-handled Scaler status!\r\n"));
	  	}
  	}

	scaler_app_unlock();

	return scaler_status;
}
// jpeg once encode

static INT32U max_jpg_size=0;
INT32U jpeg_encode_once(INT32U quality_value, INT32U input_format, INT32U width, INT32U height, INT32U input_buffer_y, INT32U input_buffer_u, INT32U input_buffer_v, INT32U output_buffer)
{
	INT32S	jpeg_status;
	INT32U 	encode_size;

	scaler_app_lock();

	jpeg_encode_init();
	gplib_jpeg_default_quantization_table_load(quality_value);	// Load default qunatization table(quality)
	gplib_jpeg_default_huffman_table_load();			// Load default huffman table

	jpeg_encode_input_size_set(width, height);
	jpeg_encode_input_format_set(input_format);			// C_JPEG_FORMAT_YUYV / C_JPEG_FORMAT_YUV_SEPARATE
	if (input_format == C_JPEG_FORMAT_YUV_SEPARATE) {
		jpeg_encode_yuv_sampling_mode_set(C_JPG_CTRL_YUV420);
	} else {
		jpeg_encode_yuv_sampling_mode_set(C_JPG_CTRL_YUV422);
	}
	jpeg_encode_output_addr_set((INT32U) output_buffer);
	jpeg_encode_once_start(input_buffer_y, input_buffer_u, input_buffer_v);

	while(1)
	{
		jpeg_status = jpeg_encode_status_query(TRUE);
		if(jpeg_status & C_JPG_STATUS_ENCODE_DONE)
		{
			encode_size = jpeg_encode_vlc_cnt_get();		// Get encode length
            if (encode_size>max_jpg_size)
            {
                max_jpg_size = encode_size;
                DBG_PRINT ("\r\nJPG SIZE = %d KB\r\n",encode_size/1024);
            }

            cache_invalid_range(output_buffer, encode_size);
			break;
		}
		else if(jpeg_status & C_JPG_STATUS_ENCODING)
			continue;
		else
			DEBUG_MSG(DBG_PRINT("JPEG encode error!\r\n"));
	}
	jpeg_encode_stop();

	scaler_app_unlock();

	return encode_size;
}
// jpeg fifo encode
INT32S jpeg_encode_fifo_start(INT32U wait_done, INT32U quality_value, INT32U input_format, INT32U width, INT32U height,
							INT32U input_buffer_y, INT32U input_buffer_u, INT32U input_buffer_v,
							INT32U output_buffer, INT32U input_y_len, INT32U input_uv_len)
{
	INT32S nRet;

	jpeg_encode_init();
	gplib_jpeg_default_quantization_table_load(quality_value);		// Load default qunatization table(quality=50)
	gplib_jpeg_default_huffman_table_load();			        // Load default huffman table
	nRet = jpeg_encode_input_size_set(width, height);
	if(nRet < 0) RETURN(STATUS_FAIL);

	nRet = jpeg_encode_input_format_set(input_format);
	if(nRet < 0) RETURN(STATUS_FAIL);

	nRet = jpeg_encode_yuv_sampling_mode_set(C_JPG_CTRL_YUV422);
	if(nRet < 0) RETURN(STATUS_FAIL);

	nRet = jpeg_encode_output_addr_set((INT32U)output_buffer);
	if(nRet < 0) RETURN(STATUS_FAIL);

	nRet = jpeg_encode_on_the_fly_start(input_buffer_y, input_buffer_u, input_buffer_v, input_y_len+input_uv_len+input_uv_len);
	if(nRet < 0) RETURN(STATUS_FAIL);

	if(wait_done == 0) RETURN(STATUS_OK);
	nRet = jpeg_encode_status_query(TRUE);	//wait jpeg block encode done
Return:
	return nRet;
}

INT32S jpeg_encode_fifo_once(INT32U wait_done, INT32U jpeg_status, INT32U input_buffer_y, INT32U input_buffer_u, INT32U input_buffer_v,
							INT32U input_y_len, INT32U input_uv_len)
{
	INT32S nRet;

	while(1)
	{
		if(wait_done == 0)
			jpeg_status = jpeg_encode_status_query(TRUE);

	   	if(jpeg_status & C_JPG_STATUS_ENCODE_DONE)
	    {
	    	RETURN(C_JPG_STATUS_ENCODE_DONE);
	    }
	    else if(jpeg_status & C_JPG_STATUS_INPUT_EMPTY)
	    {
	    	nRet = jpeg_encode_on_the_fly_start(input_buffer_y, input_buffer_u, input_buffer_v, input_y_len+input_uv_len+input_uv_len);
	    	if(nRet < 0) RETURN(nRet);

	    	if(wait_done == 0) RETURN(STATUS_OK);
	    	jpeg_status = jpeg_encode_status_query(TRUE);	//wait jpeg block encode done
	    	RETURN(jpeg_status);
	    }
	    else if(jpeg_status & C_JPG_STATUS_STOP)
	    {
	        DEBUG_MSG(DBG_PRINT("\r\njpeg encode is not started!\r\n"));
	    	RETURN(C_JPG_STATUS_STOP);
	    }
	    else if(jpeg_status & C_JPG_STATUS_TIMEOUT)
	    {
	        DEBUG_MSG(DBG_PRINT("\r\njpeg encode execution timeout!\r\n"));
	        RETURN(C_JPG_STATUS_TIMEOUT);
	    }
	    else if(jpeg_status & C_JPG_STATUS_INIT_ERR)
	    {
	         DEBUG_MSG(DBG_PRINT("\r\njpeg encode init error!\r\n"));
	         RETURN(C_JPG_STATUS_INIT_ERR);
	    }
	    else
	    {
	    	DEBUG_MSG(DBG_PRINT("\r\nJPEG status error = 0x%x!\r\n", jpeg_status));
	        RETURN(STATUS_FAIL);
	    }
    }

Return:
	return nRet;
}

INT32S jpeg_encode_fifo_stop(INT32U wait_done, INT32U jpeg_status, INT32U input_buffer_y, INT32U input_buffer_u, INT32U input_buffer_v,
							INT32U input_y_len, INT32U input_uv_len)
{
	INT32S nRet;

	if(wait_done == 0)
		jpeg_status = jpeg_encode_status_query(TRUE);

	while(1)
	{
		if(jpeg_status & C_JPG_STATUS_ENCODE_DONE)
		{
			nRet = jpeg_encode_vlc_cnt_get();	// get jpeg encode size

            if (nRet>max_jpg_size)
            {
                max_jpg_size = nRet;
                DBG_PRINT ("\r\bJPG SIZE = %d KB\r\n",nRet/1024);
            }
            RETURN(nRet);
		}
		else if(jpeg_status & C_JPG_STATUS_INPUT_EMPTY)
		{
			nRet = jpeg_encode_on_the_fly_start(input_buffer_y, input_buffer_u, input_buffer_v, input_y_len+input_uv_len+input_uv_len);
			if(nRet < 0) RETURN(STATUS_FAIL);
			jpeg_status = jpeg_encode_status_query(TRUE);	//wait jpeg block encode done
		}
		else if(jpeg_status & C_JPG_STATUS_STOP)
	    {
	        DEBUG_MSG(DBG_PRINT("\r\njpeg encode is not started!\r\n"));
	    	RETURN(STATUS_FAIL);
	    }
	    else if(jpeg_status & C_JPG_STATUS_TIMEOUT)
	    {
	        DEBUG_MSG(DBG_PRINT("\r\njpeg encode execution timeout!\r\n"));
	        RETURN(STATUS_FAIL);
	    }
	    else if(jpeg_status & C_JPG_STATUS_INIT_ERR)
	    {
	         DEBUG_MSG(DBG_PRINT("\r\njpeg encode init error!\r\n"));
	         RETURN(STATUS_FAIL);
	    }
	    else
	    {
	    	DEBUG_MSG(DBG_PRINT("\r\nJPEG status error = 0x%x!\r\n", jpeg_status));
	        RETURN(STATUS_FAIL);
	    }
	}

Return:
	jpeg_encode_stop();
	return nRet;
}

INT32S avi_mjpeg_decode_without_scaler(INT32U raw_data_addr, INT32U size, INT32U decode_output_addr)
{
    INT32U retry;
    INT32S status;

	scaler_app_lock();

	mjpeg_decode_init();
	jpeg_decode_parse_header((INT8U *) raw_data_addr, size);
	jpeg_image_size_set(pAviEncVidPara->sensor_capture_width, pAviEncVidPara->sensor_capture_height);
	jpeg_yuv_sampling_mode_set((INT32U) C_JPG_CTRL_YUV422);
	jpeg_fifo_line_set(C_JPG_FIFO_DISABLE);
	jpeg_decode_vlc_maximum_length_set(size);
	jpeg_using_scaler_mode_enable();
	mjpeg_decode_output_addr_set(decode_output_addr, NULL, NULL);
	mjpeg_decode_output_format_set(C_SCALER_CTRL_OUT_YUYV);

	scaler_input_format_set(C_SCALER_CTRL_IN_YUV422);
	scaler_output_pixels_set(1<<16, 1<<16, pAviEncVidPara->sensor_capture_width, pAviEncVidPara->sensor_capture_height);
	scaler_YUV_type_set(C_SCALER_CTRL_TYPE_YCBCR);
	scaler_bypass_zoom_mode_enable();

	if (jpeg_vlc_addr_set((INT32U) raw_data_addr+/*0x253*/0x270) || //Novatek header: 0x253,  GP header: 0x270
		jpeg_vlc_maximum_length_set(size) ||
		jpeg_decompression_start()) {
		DBG_PRINT("Calling to jpeg_decompression_start() failed 1\r\n");

		scaler_app_unlock();
		return STATUS_FAIL;
	}

    retry = 0;
	do {
		status = mjpeg_decode_status_query(1);
		if (status & (C_JPG_STATUS_STOP | C_JPG_STATUS_TIMEOUT | C_JPG_STATUS_INIT_ERR | C_JPG_STATUS_RST_MARKER_ERR)) {
			retry = 10;
			break;
		}
	} while (!(status & C_JPG_STATUS_SCALER_DONE)) ;

	mjpeg_decode_stop();

	scaler_app_unlock();
	if (retry >= 3) {
		return STATUS_FAIL;
	}
	return STATUS_OK;
}

#define FIFO_LINE 32
#define ENCODE_WRITE_SIZE 256*1024
INT32S scale_up_and_encode(INT32U sensor_input_addr, INT32U scaler_output_fifo, INT32U jpeg_encode_width, INT32U jpeg_encode_height, INT32U jpeg_output_addr, INT32U quality)
{
	INT32S scaler_status, jpeg_status;
	INT32U padding_width;
	INT32U padding_height;
	INT32U total_fifo_size,y_single_fifo_size,cb_single_fifo_size;
	INT32U fifo_encode_size;
	INT32U total_encode_size;
	INT32U restart_index;
	INT32U jpeg_input_fifo_cnt;
	INT32U fifo_y_addr, fifo_cb_addr, fifo_cr_addr;

	padding_width = (jpeg_encode_width + 15) & 0xFFF0;
	padding_height = (jpeg_encode_height + 15) & 0xFFF0;

	y_single_fifo_size = padding_width*FIFO_LINE;
	cb_single_fifo_size = padding_width*FIFO_LINE/2;
	total_fifo_size = y_single_fifo_size + cb_single_fifo_size + cb_single_fifo_size;

	fifo_y_addr = (scaler_output_fifo+0xF) & (~0xF);
	fifo_cb_addr = fifo_y_addr + (y_single_fifo_size<<1);
	fifo_cr_addr = fifo_cb_addr + (cb_single_fifo_size<<1);

	scaler_app_lock();

	// Scaler output fifo mode
	scaler_init();
	scaler_input_pixels_set(pAviEncVidPara->sensor_capture_width, pAviEncVidPara->sensor_capture_height);							// 1~8000 pixels, including the padding pixels
	scaler_input_visible_pixels_set(pAviEncVidPara->sensor_capture_width, pAviEncVidPara->sensor_capture_height);					// 1~8000 pixels, not including the padding pixels
	scaler_input_addr_set(sensor_input_addr, NULL, NULL);
	scaler_input_format_set(C_SCALER_CTRL_IN_YUYV);
	scaler_output_pixels_set(((pAviEncVidPara->sensor_capture_width)<<16)/jpeg_encode_width, (pAviEncVidPara->sensor_capture_height<<16)/jpeg_encode_height, padding_width, jpeg_encode_height);
	scaler_output_addr_set(fifo_y_addr, fifo_cb_addr, fifo_cr_addr);
	scaler_output_format_set(C_SCALER_CTRL_OUT_YUV422);
	scaler_input_fifo_line_set(C_SCALER_CTRL_IN_FIFO_DISABLE);	// C_SCALER_CTRL_IN_FIFO_DISABLE/C_SCALER_CTRL_IN_FIFO_16LINE/C_SCALER_CTRL_IN_FIFO_32LINE/C_SCALER_CTRL_IN_FIFO_64LINE/C_SCALER_CTRL_IN_FIFO_128LINE/C_SCALER_CTRL_IN_FIFO_256LINE
  #if FIFO_LINE == 16
	scaler_output_fifo_line_set(C_SCALER_CTRL_OUT_FIFO_16LINE);	// C_SCALER_CTRL_OUT_FIFO_DISABLE/C_SCALER_CTRL_OUT_FIFO_16LINE/C_SCALER_CTRL_OUT_FIFO_32LINE/C_SCALER_CTRL_OUT_FIFO_64LINE
  #else
    scaler_output_fifo_line_set(C_SCALER_CTRL_OUT_FIFO_32LINE);
  #endif

	scaler_out_of_boundary_mode_set(0);							// mode: 0=Use the boundary data of the input picture, 1=Use Use color defined in scaler_out_of_boundary_color_set()

	// Scaler start, restart and stop function APIs
	scaler_start();
	// JPEG input fifo mode
	jpeg_encode_init();
	gplib_jpeg_default_quantization_table_load(quality);
	gplib_jpeg_default_huffman_table_load();

	if (padding_height > FIFO_LINE*2) {
		jpeg_encode_input_size_set(padding_width, FIFO_LINE*2);		// Width and height(A/B fifo) of the image that will be compressed
	} else {
		jpeg_encode_input_size_set(padding_width, padding_height);
	}

	jpeg_encode_input_format_set(C_JPEG_FORMAT_YUV_SEPARATE);			// format: C_JPEG_FORMAT_YUV_SEPARATE or C_JPEG_FORMAT_YUYV
	jpeg_encode_yuv_sampling_mode_set(C_JPG_CTRL_YUV422);		// encode_mode: C_JPG_CTRL_YUV422 or C_JPG_CTRL_YUV420(only valid for C_JPEG_FORMAT_YUV_SEPARATE format)
	jpeg_encode_output_addr_set(jpeg_output_addr);				// The address that VLC(variable length coded) data will be output

	total_encode_size = 0;
	restart_index = 0;
	jpeg_input_fifo_cnt = 0;

	while (1) {
		scaler_status = scaler_wait_idle();

		if (scaler_status & 0x10) {
		    DBG_PRINT("scaler timeout\r\n");
		}
		if (scaler_status & C_SCALER_STATUS_DONE) {
		  	// Wait until scaler finish its job
		  	while (1) {
		  		jpeg_status = jpeg_encode_status_query(1);
		  		if (jpeg_status & C_JPG_STATUS_ENCODE_DONE) {
		  			fifo_encode_size = jpeg_encode_vlc_cnt_get();
		  			cache_invalid_range(jpeg_output_addr+total_encode_size, fifo_encode_size);
					if (padding_height > FIFO_LINE*2) {
						// Replace 0xFFD9 by 0xFFD0~0xFFD7
						if (!(fifo_encode_size & 0xF)) {
							*((INT8U *) (jpeg_output_addr+total_encode_size+fifo_encode_size-1)) = 0xD0 + restart_index;
						} else {
							INT32U temp;
							INT8U *ptr;

							fifo_encode_size -= 2;
							ptr = (INT8U *) (jpeg_output_addr+total_encode_size+fifo_encode_size);
							temp = fifo_encode_size & 0xF;
							if (temp == 0xF) {
								*ptr++ = 0xFF;
								temp = 0;
								fifo_encode_size++;
							}
							while (temp < 14) {
								*ptr++ = 0xFF;
								temp++;
								fifo_encode_size++;
							}
							*ptr++ = 0xFF;
							*ptr++ = 0xD0 + restart_index;
							fifo_encode_size += 2;
						}
						restart_index++;
						if (restart_index >= 8) {
							restart_index = 0;
						}
					}
					// TBD: Save VLC of this FIFO to SD card
					total_encode_size += fifo_encode_size;
					write(pAviEncPara->AviPackerCur->file_handle, jpeg_output_addr, total_encode_size);
					total_encode_size = 0;

					if (padding_height > FIFO_LINE*2) {
						padding_height -= FIFO_LINE*2;

						jpeg_encode_init();
						if (padding_height > FIFO_LINE*2) {
							jpeg_encode_input_size_set(padding_width, FIFO_LINE*2);		// Width and height(A/B fifo) of the image that will be compressed
						} else {
							jpeg_encode_input_size_set(padding_width, padding_height);
						}
						jpeg_encode_input_format_set(C_JPEG_FORMAT_YUV_SEPARATE);			// format: C_JPEG_FORMAT_YUV_SEPARATE or C_JPEG_FORMAT_YUYV
						jpeg_encode_yuv_sampling_mode_set(C_JPG_CTRL_YUV422);		// encode_mode: C_JPG_CTRL_YUV422 or C_JPG_CTRL_YUV420(only valid for C_JPEG_FORMAT_YUV_SEPARATE format)
						jpeg_encode_output_addr_set(jpeg_output_addr+total_encode_size);	// The address that VLC(variable length coded) data will be output

						if (jpeg_encode_on_the_fly_start(fifo_y_addr, fifo_cb_addr, fifo_cr_addr, total_fifo_size)) {
							jpeg_encode_stop();
							scaler_stop();

							scaler_app_unlock();
							return 0;
						}
					} else {
						break;
					}

				} else if (jpeg_status & (C_JPG_STATUS_INPUT_EMPTY|C_JPG_STATUS_STOP)) {
					if (!(jpeg_input_fifo_cnt & 0x1)) {
						if (jpeg_encode_on_the_fly_start(fifo_y_addr, fifo_cb_addr, fifo_cr_addr, total_fifo_size)) {
							jpeg_encode_stop();
							scaler_stop();

							scaler_app_unlock();
							return 0;
						}
					} else {
						if (jpeg_encode_on_the_fly_start(fifo_y_addr+y_single_fifo_size, fifo_cb_addr+cb_single_fifo_size, fifo_cr_addr+cb_single_fifo_size, total_fifo_size)) {
							jpeg_encode_stop();
							scaler_stop();

							scaler_app_unlock();
							return 0;
						}
					}
			  	}
		  	}
			break;
		}

		if (scaler_status & C_SCALER_STATUS_OUTPUT_FULL) {
			while (1) {
			  	// Start JPEG to handle the full output FIFO now
		  		jpeg_status = jpeg_encode_status_query(1);
		  		if (jpeg_status & C_JPG_STATUS_ENCODE_DONE) {
		  			fifo_encode_size = jpeg_encode_vlc_cnt_get();
		  			cache_invalid_range(jpeg_output_addr+total_encode_size, fifo_encode_size);
					// Replace 0xFFD9 by 0xFFD0~0xFFD7
					if (!(fifo_encode_size & 0xF)) {
						*((INT8U *) (jpeg_output_addr+total_encode_size+fifo_encode_size-1)) = 0xD0 + restart_index;
					} else {
						INT32U temp;
						INT8U *ptr;

						fifo_encode_size -= 2;
						ptr = (INT8U *) (jpeg_output_addr+total_encode_size+fifo_encode_size);
						temp = fifo_encode_size & 0xF;
						if (temp == 0xF) {
							*ptr++ = 0xFF;
							temp = 0;
							fifo_encode_size++;
						}
						while (temp < 14) {
							*ptr++ = 0xFF;
							temp++;
							fifo_encode_size++;
						}
						*ptr++ = 0xFF;
						*ptr++ = 0xD0 + restart_index;
						fifo_encode_size += 2;
					}
					restart_index++;
					if (restart_index >= 8) {
						restart_index = 0;
					}
					// TBD: Save VLC of this FIFO to SD card
					total_encode_size += fifo_encode_size;

					if (total_encode_size >= ENCODE_WRITE_SIZE) {
					    write(pAviEncPara->AviPackerCur->file_handle, jpeg_output_addr, total_encode_size);
					    total_encode_size = 0;
					}

					if (padding_height > FIFO_LINE*2) {
						padding_height -= FIFO_LINE*2;

						jpeg_encode_init();
						if (padding_height > FIFO_LINE*2) {
							jpeg_encode_input_size_set(padding_width, FIFO_LINE*2);		// Width and height(A/B fifo) of the image that will be compressed
						} else {
							jpeg_encode_input_size_set(padding_width, padding_height);
						}
						jpeg_encode_input_format_set(C_JPEG_FORMAT_YUV_SEPARATE);			// format: C_JPEG_FORMAT_YUV_SEPARATE or C_JPEG_FORMAT_YUYV
						jpeg_encode_yuv_sampling_mode_set(C_JPG_CTRL_YUV422);		// encode_mode: C_JPG_CTRL_YUV422 or C_JPG_CTRL_YUV420(only valid for C_JPEG_FORMAT_YUV_SEPARATE format)
						jpeg_encode_output_addr_set(jpeg_output_addr+total_encode_size);	// The address that VLC(variable length coded) data will be output

					} else {
						jpeg_encode_stop();
						scaler_stop();

						scaler_app_unlock();
						return total_encode_size;
					}

					jpeg_input_fifo_cnt = 0;

				} else if ((jpeg_status & (C_JPG_STATUS_INPUT_EMPTY|C_JPG_STATUS_STOP)) && (jpeg_input_fifo_cnt<2)) {
			  		// Now restart Scaler to output to next FIFO
			  		if (scaler_restart()) {
			  			jpeg_encode_stop();
						scaler_stop();

						scaler_app_unlock();
						return 0;
				  	}

					if (!(jpeg_input_fifo_cnt & 0x1)) {
						if (jpeg_encode_on_the_fly_start(fifo_y_addr, fifo_cb_addr, fifo_cr_addr, total_fifo_size)) {
							jpeg_encode_stop();
							scaler_stop();

							scaler_app_unlock();
							return 0;
						}
					} else {
						if (jpeg_encode_on_the_fly_start(fifo_y_addr+y_single_fifo_size, fifo_cb_addr+cb_single_fifo_size, fifo_cr_addr+cb_single_fifo_size, total_fifo_size)) {
							jpeg_encode_stop();
							scaler_stop();

							scaler_app_unlock();
							return 0;
						}
					}
					jpeg_input_fifo_cnt++;
					break;
			  	}
			}
		}
	}

	jpeg_encode_stop();
	scaler_stop();

	scaler_app_unlock();
	return total_encode_size;
}

/*
INT32S save_jpeg_file(INT16S fd, INT32U encode_frame, INT32U encode_size)
{
	INT32S nRet;

	nRet = write(fd, encode_frame, encode_size);
	if(nRet != encode_size) RETURN(STATUS_FAIL);
	nRet = close(fd);
	if(nRet < 0) RETURN(STATUS_FAIL);
	fd = -1;
	nRet = STATUS_OK;
Return:
	return nRet;
}
*/

//audio
INT32S avi_audio_memory_allocate(INT32U	cblen)
{
	INT16U *ptr;
	INT32S i, j, size, nRet;

	_dmsg((GREEN "[S]: avi_audio_memory_allocate() - cblen=%0d\r\n" NONE, cblen));

	for (i=0; i<AVI_ENCODE_PCM_BUFFER_NO; i++) {		// AVI_ENCODE_PCM_BUFFER_NO = 3
		if (pAviEncAudPara->pcm_input_addr[i]==0) {	// dominant add
			pAviEncAudPara->pcm_input_addr[i] = (INT32U) gp_malloc(cblen);
			_dmsg((GREEN "[D]: avi_audio_memory_allocate() - pcm_input_addr[%0d] = 0x%08x\r\n" NONE, i, pAviEncAudPara->pcm_input_addr[i]));
	        }
		if (!pAviEncAudPara->pcm_input_addr[i]) {
			_dmsg((GREEN "[E]: avi_audio_memory_allocate() - gp_malloc() array fail\r\n" NONE));
			RETURN(STATUS_FAIL);
		}
		ptr = (INT16U *) pAviEncAudPara->pcm_input_addr[i];
		for (j=0; j<(cblen/2); j++)
			*ptr++ = 0x8000;
	}

	size = pAviEncAudPara->pack_size * C_WAVE_ENCODE_TIMES;
	pAviEncAudPara->pack_buffer_addr = (INT32U) gp_malloc(size);
	if (!pAviEncAudPara->pack_buffer_addr) {
		_dmsg((GREEN "[E]: avi_audio_memory_allocate() - gp_malloc() fail!!!\r\n" NONE));
		RETURN(STATUS_FAIL);
	}

	_dmsg((GREEN "[E]: avi_audio_memory_allocate() - pass\r\n" NONE));
	nRet = STATUS_OK;
Return:
	if (nRet!=STATUS_OK) {	// Dominant add
		avi_audio_memory_free();  // Dominant, any allocate fail, we should free all allocatted memory
	}
	return nRet;
}

void avi_audio_memory_free(void)
{
	INT32U i;

	for (i=0; i<AVI_ENCODE_PCM_BUFFER_NO; i++) {
		gp_free((void *) pAviEncAudPara->pcm_input_addr[i]);
		pAviEncAudPara->pcm_input_addr[i] = 0;
	}
	gp_free((void *) pAviEncAudPara->pack_buffer_addr);
	pAviEncAudPara->pack_buffer_addr = 0;
}

INT32U avi_audio_get_next_buffer(void)
{
	INT32U addr;

	addr = pAviEncAudPara->pcm_input_addr[g_pcm_index++];
	if(g_pcm_index >= AVI_ENCODE_PCM_BUFFER_NO)
		g_pcm_index = 0;

	return addr;
}

#if MIC_INPUT_SRC == C_ADC_LINE_IN || MIC_INPUT_SRC == C_BUILDIN_MIC_IN
INT32S avi_adc_double_buffer_put(INT16U *data,INT32U cwlen, OS_EVENT *os_q)
{
	INT32S status;

#if MIC_INPUT_SRC == C_ADC_LINE_IN
	g_avi_adc_dma_dbf.s_addr = (INT32U) P_ADC_ASADC_DATA;
#elif MIC_INPUT_SRC == C_BUILDIN_MIC_IN
	g_avi_adc_dma_dbf.s_addr = (INT32U) P_MIC_ASADC_DATA;
#endif
	g_avi_adc_dma_dbf.t_addr = (INT32U) data;
	g_avi_adc_dma_dbf.width = DMA_DATA_WIDTH_2BYTE;
	g_avi_adc_dma_dbf.count = (INT32U) cwlen;
	g_avi_adc_dma_dbf.notify = NULL;
	g_avi_adc_dma_dbf.timeout = 0;
	status = dma_transfer_with_double_buf(&g_avi_adc_dma_dbf, os_q);
	if(status < 0) return status;
	return STATUS_OK;
}

INT32U avi_adc_double_buffer_set(INT16U *data, INT32U cwlen)
{
	INT32S status;

#if MIC_INPUT_SRC == C_ADC_LINE_IN
	g_avi_adc_dma_dbf.s_addr = (INT32U) P_ADC_ASADC_DATA;
#elif MIC_INPUT_SRC == C_BUILDIN_MIC_IN
	g_avi_adc_dma_dbf.s_addr = (INT32U) P_MIC_ASADC_DATA;
#endif
	g_avi_adc_dma_dbf.t_addr = (INT32U) data;
	g_avi_adc_dma_dbf.width = DMA_DATA_WIDTH_2BYTE;
	g_avi_adc_dma_dbf.count = (INT32U) cwlen;
	g_avi_adc_dma_dbf.notify = NULL;
	g_avi_adc_dma_dbf.timeout = 0;
	status = dma_transfer_double_buf_set(&g_avi_adc_dma_dbf);
	if(status < 0) return status;
	return STATUS_OK;
}

INT32S avi_adc_dma_status_get(void)
{
	if (g_avi_adc_dma_dbf.channel == 0xff)
		return -1;

	return dma_status_get(g_avi_adc_dma_dbf.channel);
}

void avi_adc_double_buffer_free(void)
{
	dma_transfer_double_buf_free(&g_avi_adc_dma_dbf);
	g_avi_adc_dma_dbf.channel = 0xff;
}

void avi_adc_hw_start(void)
{
#if MIC_INPUT_SRC == C_ADC_LINE_IN
	R_ADC_SETUP = 0;
	R_ADC_ASADC_CTRL = 0;
	//R_ADC_MADC_CTRL = 0;
	R_ADC_TP_CTRL = 0;
	R_ADC_SH_WAIT = 0x0807;

	//set adc pin is input floating
#if AVI_AUDIO_RECORD_ADC_CH == ADC_LINE_0
	gpio_init_io(IO_F6, GPIO_INPUT);
	gpio_set_port_attribute(IO_F6, ATTRIBUTE_HIGH);
#elif AVI_AUDIO_RECORD_ADC_CH == ADC_LINE_1
	gpio_init_io(IO_F7, GPIO_INPUT);
	gpio_set_port_attribute(IO_F7, ATTRIBUTE_HIGH);
#elif AVI_AUDIO_RECORD_ADC_CH == ADC_LINE_2
	gpio_init_io(IO_F8, GPIO_INPUT);
	gpio_set_port_attribute(IO_F8, ATTRIBUTE_HIGH);
#elif AVI_AUDIO_RECORD_ADC_CH == ADC_LINE_3
	gpio_init_io(IO_F9, GPIO_INPUT);
	gpio_set_port_attribute(IO_F9, ATTRIBUTE_HIGH);
#elif AVI_AUDIO_RECORD_ADC_CH == ADC_LINE_4
	gpio_init_io(IO_H6, GPIO_INPUT);
	gpio_set_port_attribute(IO_H6, ATTRIBUTE_HIGH);
#elif AVI_AUDIO_RECORD_ADC_CH == ADC_LINE_5
	gpio_init_io(IO_H7, GPIO_INPUT);
	gpio_set_port_attribute(IO_H7, ATTRIBUTE_HIGH);
#endif

	adc_fifo_clear();
	adc_fifo_level_set(4);
	adc_auto_ch_set(AVI_AUDIO_RECORD_ADC_CH);
	adc_auto_sample_start();

#elif MIC_INPUT_SRC == C_BUILDIN_MIC_IN
	R_MIC_SETUP = 0;
	R_MIC_ASADC_CTRL = 0;
	R_MIC_SH_WAIT = 0x0807;

	mic_fifo_clear();
	mic_fifo_level_set(4);
	R_MIC_SETUP = 0xA0C0; //enable AGC
	mic_auto_sample_start();
#endif
}

void avi_adc_hw_stop(void)
{
#if MIC_INPUT_SRC == C_ADC_LINE_IN
	adc_auto_sample_stop();
#elif MIC_INPUT_SRC == C_BUILDIN_MIC_IN
	mic_auto_sample_stop();
#endif
}
#endif

#if MIC_INPUT_SRC == C_GPY0050_IN
INT16U avi_adc_get_gpy0050(void)
{
	INT16U temp;
	INT32S i;

	for(i=0; i<5000; i++)					//wait busy
	{
		if((R_SPI0_MISC & 0x0010) == 0)
			break;
	}

	gpio_write_io(C_GPY0050_SPI_CS_PIN, 1);	//pull cs high

	temp = R_SPI0_RX_DATA;// don't care
	temp = (INT16U)(R_SPI0_RX_DATA << 8);// high byte
	temp |= R_SPI0_RX_DATA & 0xFF;				 // low byte

	/*
	temp = R_SPI0_RX_DATA;								// don't care
	temp = (((INT16U) R_SPI0_RX_DATA) & 0xFF) << 8;		// high byte
	temp |= (R_SPI0_RX_DATA & 0xF0);				 	// low byte
	if (temp >= 0xAAAA) {
		temp = 0xFFF0;
	} else {
		temp += (temp>>1);
	}
	*/

	gpio_write_io(C_GPY0050_SPI_CS_PIN, 0);	//pull cs low
	R_SPI0_TX_DATA = C_CMD_DUMMY_COM;		//dummy command
	R_SPI0_TX_DATA = C_CMD_ADC_IN4;			//select IN4
	R_SPI0_TX_DATA = C_CMD_ZERO_COM;		// get low byte

/*
	if(temp <= 0x00FF)
		temp = g_gpy0050_pre_value;
	else if(temp >= 0xFFC0)
		temp = g_gpy0050_pre_value;
	else
		g_gpy0050_pre_value = temp;
*/
	return temp^0x8000;
}

void gpy0050_enable(void)
{
	INT32S i;

	R_NF_SHARE_DELAY = 0x10;
	R_NF_SHARE_BYTES = 0x1;

	gpio_init_io(C_GPY0050_SPI_CS_PIN, GPIO_OUTPUT);
	gpio_set_port_attribute(C_GPY0050_SPI_CS_PIN, ATTRIBUTE_HIGH);
	gpio_write_io(C_GPY0050_SPI_CS_PIN, DATA_HIGH);	//pull cs high

	//GPL32 SPI IF 1 initialization
	R_SPI0_CTRL = 0x0800;	//reset SPI0
	R_SPI0_MISC |= 0x0100;  //enable smart mode
	R_SPI0_CTRL = 0x8035;	//Master mode 3, SPI_CLK = SYS_CLK/64

	//sw reset gpy0050
	gpio_write_io(C_GPY0050_SPI_CS_PIN, 0);	//pull cs low
	R_SPI0_TX_DATA = C_CMD_RESET_IN4;		//reset IN4

	for(i=0; i<5000; i++)					//wait busy
	{
		if((R_SPI0_MISC & 0x0010) == 0)
			break;
	}
	gpio_write_io(C_GPY0050_SPI_CS_PIN, 1);	//pull cs high

	//enable MIC AGC and ADC
	gpio_write_io(C_GPY0050_SPI_CS_PIN, 0);		//pull cs low
	R_SPI0_TX_DATA = C_CMD_ENABLE_MIC_AGC_ADC;	//enable MIC & AGC;
	R_SPI0_TX_DATA = C_CMD_ADC_IN4;				//select IN4

	for(i=0; i<5000; i++)					//wait busy
	{
		if((R_SPI0_MISC & 0x0010) == 0)
			break;
	}
	gpio_write_io(C_GPY0050_SPI_CS_PIN, 1);	//pull cs high

	OSTimeDly(30);	//wait 300ms after AGC & MIC enable

	R_SPI0_TX_STATUS |= 0x8020;
	R_SPI0_RX_STATUS |= 0x8020;

	//dummy data
	avi_adc_get_gpy0050();
	avi_adc_get_gpy0050();
}

void gpy0050_disable()
{
	INT32S i;

	//power down
	gpio_write_io(C_GPY0050_SPI_CS_PIN, 0);	//pull cs low
	R_SPI0_TX_DATA = C_CMD_POWER_DOWN;

	for(i=0; i<5000; i++)					//wait busy
	{
		if(R_SPI0_MISC & 0x0010 == 0)
			break;
	}
	gpio_write_io(C_GPY0050_SPI_CS_PIN, 1);	//pull cs high

	//GPL32 SPI disable
	R_SPI0_CTRL = 0;
	R_SPI0_MISC = 0;
}

void gpy0050_timer_isr(void)
{
	INT16U *ptr;
	INT32U pcm_cwlen;

	ptr = (INT16U*)pAviEncAudPara->pcm_input_addr[g_mic_buffer];
	*(ptr + g_mic_cnt++) = avi_adc_get_gpy0050();

	pcm_cwlen = pAviEncAudPara->pcm_input_size * C_WAVE_ENCODE_TIMES;
	if(g_mic_cnt >= pcm_cwlen)
	{
		if(g_mic_buffer)
			g_mic_buffer = 0;
		else
			g_mic_buffer = 1;

		g_mic_cnt = 0;
		OSQPost(avi_aud_q, (void*)AVI_AUDIO_DMA_DONE);
	}
}
#endif	// MIC_INPUT_SRC