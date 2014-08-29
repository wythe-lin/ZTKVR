#ifndef __AVI_ENCODER_APP_H__
#define __AVI_ENCODER_APP_H__

#include "application.h"
#include "avi_encoder_scaler_jpeg.h"

#if MIC_INPUT_SRC == C_ADC_LINE_IN || MIC_INPUT_SRC == C_BUILDIN_MIC_IN
	#define AVI_AUDIO_RECORD_TIMER		ADC_AS_TIMER_F  //adc use timer, C ~ F
	#define AVI_AUDIO_RECORD_ADC_CH		ADC_LINE_0		//adc use channel, 0 ~ 3
#elif MIC_INPUT_SRC == C_GPY0050_IN
	#define AVI_AUDIO_RECORD_TIMER		TIMER_B		 	//timer, A,B,C
	#if MINI_DVR_BOARD_VERSION == GPL32680_MINI_DVR_EMU_V2_0 || MINI_DVR_BOARD_VERSION == GPL32680_MINI_DVR_DEMO_BOARD
		#define C_GPY0050_SPI_CS_PIN		IO_F6 		//gpy0500 spi interface cs pin
	#elif MINI_DVR_BOARD_VERSION == GPL32680_MINI_DVR_KEYCHAIN_QFN
		#define C_GPY0050_SPI_CS_PIN		IO_H2
	#else
		#define C_GPY0050_SPI_CS_PIN		IO_H4
	#endif
#endif

typedef struct AviEncAudPara_s
{
	//audio encode
	INT32U  audio_format;			// audio encode format
	INT16U  audio_sample_rate;		// sample rate
	INT16U  channel_no;				// channel no, 1:mono, 2:stereo
	INT8U   *work_mem;				// wave encode work memory 
	INT32U  pack_size;				// wave encode package size in byte
	INT32U  pack_buffer_addr;
	INT32U  pcm_input_size;			// wave encode pcm input size in short
	INT32U  pcm_input_addr[AVI_ENCODE_PCM_BUFFER_NO];
}AviEncAudPara_t;

typedef struct AviEncVidPara_s
{
	//sensor input 
    INT32U  sensor_output_format;   // sensor output format
    INT16U  sensor_capture_width;   // sensor width
    INT16U  sensor_capture_height;  // sensor height
    INT32U  csi_frame_addr[AVI_ENCODE_CSI_BUFFER_NO];	// sensor input buffer addr
    
    //scaler output
    INT16U 	scaler_flag;
    INT16U 	dispaly_scaler_flag;  	// 0: not scaler, 1: scaler again for display 
    FP32    scaler_zoom_ratio;      // for zoom scaler   
    INT32U  scaler_output_addr[AVI_ENCODE_SCALER_BUFFER_NO];	// scaler output buffer addr
      								
    //display scaler
    INT32U  display_output_format;  // for display use
    INT16U  display_width;          // display width
    INT16U  display_height;         // display height
    INT16U  display_buffer_width;   // display width
    INT16U  display_buffer_height;  // display height 
    INT32U  display_output_addr[AVI_ENCODE_DISPALY_BUFFER_NO];	// display scaler buffer addr
    
    //video encode
    INT32U  video_format;			// video encode format
    INT8U   dwScale;				// dwScale
    INT8U   dwRate;					// dwRate 
    INT16U  quality_value;			// video encode quality
    INT16U  encode_width;           // video encode width
    INT16U  encode_height;          // videoe ncode height
    INT32U  video_encode_addr[AVI_ENCODE_VIDEO_BUFFER_NO]; //video encode buffer
	
	//mpeg4 encode use
	INT32U  isram_addr;
	INT32U  write_refer_addr;
	INT32U  reference_addr;
}AviEncVidPara_t;

typedef struct AviEncPacker_s
{
	void 	*avi_workmem;
	INT16S  file_handle;
    INT16S  index_handle;
    INT8U   index_path[16];
    
    //avi video info
    GP_AVI_AVISTREAMHEADER *p_avi_vid_stream_header;
    INT32U  bitmap_info_cblen;		// bitmap header length
    GP_AVI_BITMAPINFO *p_avi_bitmap_info;
    
    //avi audio info
    GP_AVI_AVISTREAMHEADER *p_avi_aud_stream_header;
    INT32U  wave_info_cblen;		// wave header length
    GP_AVI_PCMWAVEFORMAT *p_avi_wave_info;
    
    //avi packer 
	INT32U  task_prio;
	void    *file_write_buffer;
	INT32U  file_buffer_size;		// AviPacker file buffer size in byte
	void    *index_write_buffer;
	INT32U	index_buffer_size;		// AviPacker index buffer size in byte
}AviEncPacker_t;

typedef struct AviEncPara_s
{   
	INT8U  source_type;				// SOURCE_TYPE_FS or SOURCE_TYPE_USER_DEFINE
    INT8U  key_flag;
    INT8U  fifo_enc_err_flag;
    INT8U  fifo_irq_cnt;
   	
    //avi file info
    INT32U  avi_encode_status;      // 0:IDLE
    AviEncPacker_t *AviPackerCur;
    
	//allow record size
	INT64U  disk_free_size;			// disk free size
	INT32U  record_total_size;		// AviPacker storage to disk total size
    
	//aud & vid sync
	INT64S  delta_ta, tick;
    INT64S  delta_Tv;
    INT32S  freq_div;
    INT64S  ta, tv, Tv;
   	INT32U  pend_cnt, post_cnt;
	
	INT32U  ready_frame;
	INT32S  ready_size;
	INT32U  vid_pend_cnt, vid_post_cnt;
}AviEncPara_t;

//extern os-event
extern OS_EVENT *AVIEncodeApQ;
extern OS_EVENT *scaler_task_q;
extern OS_EVENT *cmos_frame_q;
extern OS_EVENT *vid_enc_task_q;
extern OS_EVENT *avi_aud_q;
extern OS_EVENT *scaler_hw_app_sem;

extern AviEncPara_t *pAviEncPara;
extern AviEncAudPara_t *pAviEncAudPara;
extern AviEncVidPara_t *pAviEncVidPara;
extern AviEncPacker_t *pAviEncPacker0, *pAviEncPacker1;

#if MIC_INPUT_SRC == C_GPY0050_IN
	extern INT8U  g_mic_buffer;
	extern INT32U g_mic_cnt;
#endif
#if C_UVC == CUSTOM_ON
//USB Webcam 
extern OS_EVENT *usbwebcam_frame_q;
extern OS_EVENT  *USBCamApQ;
typedef struct   
{
    INT32U FrameSize;
    INT32U  AddrFrame;
    INT8U IsoSendState;
}ISOTaskMsg;
#endif

//jpeg header
extern INT8U jpeg_422_q90_header[];

//mpeg4 encoder
#if MPEG4_ENCODE_ENABLE == 1
	extern void mpeg4_encode_init(void);
	extern INT32S mpeg4_encode_config(INT8U input_format, INT16U width, INT16U height, INT8U vop_time_inc_reso);
	extern void mpeg4_encode_ip_set(INT8U byReg, INT32U IPXXX_No);
	extern INT32S mpeg4_encode_isram_set(INT32U addr);
	extern INT32S mpeg4_encode_start(INT8U ip_frame, INT8U quant_value, INT32U vlc_out_addr, 
									INT32U raw_data_in_addr, INT32U encode_out_addr, INT32U refer_addr);
	extern INT32S mpeg4_encode_get_vlc_size(void);
	extern INT32S mpeg4_wait_idle(INT8U wait_done);
	extern void mpeg4_encode_stop(void);
#endif

//avi encode state
extern INT32U avi_enc_packer_start(AviEncPacker_t *pAviEncPacker);
extern INT32U avi_enc_packer_stop(AviEncPacker_t *pAviEncPacker); 
extern INT32S vid_enc_preview_start(void);
extern INT32S vid_enc_preview_stop(void);
extern INT32S avi_enc_start(void);
extern INT32S avi_enc_stop(void);
extern INT32S avi_enc_pause(void);
extern INT32S avi_enc_resume(void);
extern INT32S avi_enc_save_jpeg(void);
extern INT32S avi_enc_storage_full(void);
extern INT32S avi_packer_err_handle(INT32S ErrCode);
extern INT32S avi_encode_state_task_create(INT8U pori);
extern INT32S avi_encode_state_task_del(void);
extern void   avi_encode_state_task_entry(void *para);
extern INT32S  (*pfn_avi_encode_put_data)(void* workmem, unsigned long fourcc, long cbLen, const void *ptr, int nSamples, int ChunkFlag);

//scaler task
extern INT32S scaler_task_create(INT8U pori);
extern INT32S scaler_task_del(void);
extern INT32S scaler_task_start(void);
extern INT32S scaler_task_stop(void);
extern void   scaler_task_entry(void *parm);

//video_encode task
extern INT32S video_encode_task_create(INT8U pori);
extern INT32S video_encode_task_del(void);
extern INT32S video_encode_task_start(void);
extern INT32S video_encode_task_stop(void);
extern void   video_encode_task_entry(void *parm);

//audio task
extern INT32S avi_adc_record_task_create(INT8U pori);
extern INT32S avi_adc_record_task_del(void);
extern INT32S avi_audio_record_start(void);
extern INT32S avi_audio_record_restart(void);
extern INT32S avi_audio_record_stop(void);
extern void avi_audio_record_entry(void *parm);

//avi encode api
extern void   avi_encode_init(void);

extern void avi_encode_video_timer_start(void);
extern void avi_encode_video_timer_stop(void);
extern void avi_encode_audio_timer_start(void);
extern void avi_encode_audio_timer_stop(void);

extern INT32S avi_encode_set_file_handle_and_caculate_free_size(AviEncPacker_t *pAviEncPacker, INT16S FileHandle);
extern INT16S avi_encode_close_file(AviEncPacker_t *pAviEncPacker);
extern INT32S avi_encode_set_avi_header(AviEncPacker_t *pAviEncPacker);
extern void avi_encode_set_curworkmem(void *pAviEncPacker);
//status
extern void   avi_encode_set_status(INT32U bit);
extern void   avi_encode_clear_status(INT32U bit);
extern INT32S avi_encode_get_status(void);
//memory
extern void vid_enc_buffer_lock(INT32U addr);
extern void vid_enc_buffer_unlock(void);
extern INT32U avi_encode_get_csi_frame(void);
extern INT32U avi_encode_get_scaler_frame(void);
extern INT32U avi_encode_get_display_frame(void);
extern INT32U avi_encode_get_video_frame(void);
extern INT32S avi_encode_memory_alloc(void);
extern void avi_encode_memory_free(void);
extern INT32S avi_encode_packer_memory_alloc(void);
extern void avi_encode_packer_memory_free(void);
extern INT32S avi_encode_mpeg4_malloc(INT16U width, INT16U height);
extern void avi_encode_mpeg4_free(void);

//format
extern void avi_encode_set_sensor_format(INT32U format);
extern void avi_encode_set_display_format(INT32U format);
//other
extern void avi_encode_set_display_scaler(void);
extern INT32S avi_encode_set_jpeg_quality(INT8U quality_value);
extern INT32S avi_encode_set_mp4_resolution(INT16U encode_width, INT16U encode_height);
extern BOOLEAN avi_encode_is_pause(void);
extern BOOLEAN avi_encode_disk_size_is_enough(INT32S cb_write_size);

extern void avi_encode_switch_csi_frame_buffer(void);
extern void vid_enc_csi_fifo_end(void);
extern void vid_enc_csi_frame_end(void);

extern INT32S scaler_zoom_once(INT32U scaler_mode, FP32 bScalerFactor, 
							INT32U InputFormat, INT32U OutputFormat,
							INT16U input_x, INT16U input_y, 
							INT16U output_x, INT16U output_y,
							INT16U output_buffer_x, INT16U output_buffer_y, 
							INT32U scaler_input_y, INT32U scaler_input_u, INT32U scaler_input_v, 
							INT32U scaler_output_y, INT32U scaler_output_u, INT32U scaler_output_v);
						
extern INT32U jpeg_encode_once(INT32U quality_value, INT32U input_format, INT32U width, INT32U height, 
							INT32U input_buffer_y, INT32U input_buffer_u, INT32U input_buffer_v, INT32U output_buffer);
// jpeg fifo encode
extern INT32S jpeg_encode_fifo_start(INT32U wait_done, INT32U quality_value, INT32U input_format, INT32U width, INT32U height, 
							INT32U input_buffer_y, INT32U input_buffer_u, INT32U input_buffer_v, 
							INT32U output_buffer, INT32U input_y_len, INT32U input_uv_len);
extern INT32S jpeg_encode_fifo_once(INT32U wait_done, INT32U jpeg_status, INT32U input_buffer_y, INT32U input_buffer_u, INT32U input_buffer_v,
							INT32U input_y_len, INT32U input_uv_len);		
extern INT32S jpeg_encode_fifo_stop(INT32U wait_done, INT32U jpeg_status, INT32U input_buffer_y, INT32U input_buffer_u, INT32U input_buffer_v,
							INT32U input_y_len, INT32U input_uv_len);
							
extern INT32S save_jpeg_file(INT16S fd, INT32U encode_frame, INT32U encode_size);
extern INT32S avi_audio_memory_allocate(INT32U	cblen);
extern void avi_audio_memory_free(void);
extern INT32U avi_audio_get_next_buffer(void);

#if MIC_INPUT_SRC == C_ADC_LINE_IN || MIC_INPUT_SRC == C_BUILDIN_MIC_IN
extern INT32S avi_adc_double_buffer_put(INT16U *data,INT32U cwlen, OS_EVENT *os_q);
extern INT32U avi_adc_double_buffer_set(INT16U *data, INT32U cwlen);
extern INT32S avi_adc_dma_status_get(void);
extern void avi_adc_double_buffer_free(void);
extern void avi_adc_hw_start(void);
extern void avi_adc_hw_stop(void);
#endif

#if MIC_INPUT_SRC == C_BUILDIN_MIC_IN
extern void mic_fifo_clear(void);
extern void mic_fifo_level_set(INT8U level);
extern INT32S mic_auto_sample_start(void);
extern void mic_auto_sample_stop(void);
extern INT32S mic_sample_rate_set(INT8U timer_id, INT32U hz);
extern INT32S mic_timer_stop(INT8U timer_id);
#endif

#if MIC_INPUT_SRC == C_GPY0050_IN
extern INT16U avi_adc_get_gpy0050(void);
extern void gpy0050_enable(void);
extern void gpy0050_disable(void);
extern void gpy0050_timer_isr(void);
#endif

extern INT8U video_encode_frame_rate_get(void);
extern void video_encode_frame_rate_set(INT8U fps);

extern INT32S scale_up_and_encode(INT32U sensor_input_addr, INT32U scaler_output_fifo, INT32U jpeg_encode_width, INT32U jpeg_encode_height, INT32U jpeg_output_addr, INT32U quality);
extern INT32S avi_mjpeg_decode_without_scaler(INT32U raw_data_addr, INT32U size, INT32U decode_output_addr);

#endif // __AVI_ENCODER_APP_H__
