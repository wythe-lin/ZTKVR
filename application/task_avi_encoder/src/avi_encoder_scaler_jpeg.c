#include "ztkconfigs.h"
#include "avi_encoder_app.h"

/* for debug */
#define DEBUG_AVI_ENCODER_SCALER_JPEG	1
#if DEBUG_AVI_ENCODER_SCALER_JPEG
    #include "gplib.h"
    #define _dmsg(x)			print_string x
#else
    #define _dmsg(x)
#endif

/* */


#if MPEG4_ENCODE_ENABLE == 1
#include "drv_l1_mpeg4.h"
#endif

/* os task stack size */
#define C_SCALER_STACK_SIZE			512
#define	C_JPEG_STACK_SIZE			512
#define C_SCALER_QUEUE_MAX			5
#define C_JPEG_QUEUE_MAX			5
#if VIDEO_ENCODE_MODE == C_VIDEO_ENCODE_FRAME_MODE
	#define C_CMOS_FRAME_QUEUE_MAX	AVI_ENCODE_CSI_BUFFER_NO
#elif VIDEO_ENCODE_MODE == C_VIDEO_ENCODE_FIFO_MODE
	#define C_CMOS_FRAME_QUEUE_MAX	1024/SENSOR_FIFO_LINE	//for sensor height = 1024
#endif

/* os tsak stack */
INT32U	ScalerTaskStack[C_SCALER_STACK_SIZE];
INT32U	JpegTaskStack[C_JPEG_STACK_SIZE];
INT32U  skip_flag;

/* os task queue */
OS_EVENT *scaler_task_q;
OS_EVENT *scaler_task_ack_m;
OS_EVENT *cmos_frame_q;
OS_EVENT *vid_enc_task_q;
OS_EVENT *vid_enc_task_ack_m;
OS_EVENT *scaler_hw_app_sem;
void *scaler_task_q_stack[C_SCALER_QUEUE_MAX];
void *cmos_frame_q_stack[C_CMOS_FRAME_QUEUE_MAX];
void *video_encode_task_q_stack[C_JPEG_QUEUE_MAX];

#if 1
	INT32U video_preview_flag;
#endif

extern INT8U take_pic;
extern INT8U ap_state_config_pic_size_get(void);
extern INT32S dma_sprite_buffer_copy(INT32U s_addr, INT32U t_addr, INT32U byte_count, INT32U s_width, INT32U t_width);
///////////////////////////////////////////////////////////////////////////////////////////////////////
// scaler task
///////////////////////////////////////////////////////////////////////////////////////////////////////
INT32S scaler_task_create(INT8U pori)
{
	INT8U  err;
	INT32S nRet;

	scaler_task_q = OSQCreate(scaler_task_q_stack, C_SCALER_QUEUE_MAX);
	if(!scaler_task_q) RETURN(STATUS_FAIL);

	scaler_task_ack_m = OSMboxCreate(NULL);
	if(!scaler_task_ack_m) RETURN(STATUS_FAIL);

	cmos_frame_q = OSQCreate(cmos_frame_q_stack, C_CMOS_FRAME_QUEUE_MAX);
	if(!cmos_frame_q) RETURN(STATUS_FAIL);

	scaler_hw_app_sem = OSSemCreate(1);
	if(!scaler_hw_app_sem) RETURN(STATUS_FAIL);

	err = OSTaskCreate(scaler_task_entry, (void *)NULL, &ScalerTaskStack[C_SCALER_STACK_SIZE - 1], pori);
	if(err != OS_NO_ERR) RETURN(STATUS_FAIL);

	nRet = STATUS_OK;
Return:
	return nRet;
}

INT32S scaler_task_del(void)
{
	INT8U  err;
	INT32S nRet, msg;

	nRet = STATUS_OK;
	POST_MESSAGE(scaler_task_q, MSG_SCALER_TASK_EXIT, scaler_task_ack_m, 5000, msg, err);
Return:
	OSQFlush(scaler_task_q);
   	OSQDel(scaler_task_q, OS_DEL_ALWAYS, &err);

   	OSQFlush(cmos_frame_q);
   	OSQDel(cmos_frame_q, OS_DEL_ALWAYS, &err);

	OSMboxDel(scaler_task_ack_m, OS_DEL_ALWAYS, &err);
	OSSemDel(scaler_hw_app_sem, OS_DEL_ALWAYS, &err);
	return nRet;
}

INT32S scaler_task_start(void)
{
	INT8U  err;
	INT32S nRet, msg;

	if(avi_encode_memory_alloc() < 0)
	{
		avi_encode_memory_free();
		DEBUG_MSG(DBG_PRINT("avi memory alloc fail!!!\r\n"));
		RETURN(STATUS_FAIL);
	}

	nRet = STATUS_OK;
	POST_MESSAGE(scaler_task_q, MSG_SCALER_TASK_INIT, scaler_task_ack_m, 5000, msg, err);
Return:
	return nRet;
}

INT32S scaler_task_stop(void)
{
	INT8U  err;
	INT32S nRet, msg;

	nRet = STATUS_OK;
	POST_MESSAGE(scaler_task_q, MSG_SCALER_TASK_STOP, scaler_task_ack_m, 5000, msg, err);
Return:
	avi_encode_memory_free();
	return nRet;
}

#if 0
INT32U char_array_ptr, buff_base;
void ppu_done_callback(INT32U addr)
{
	OSQPost(DisplayTaskQ, (void *) addr);
}
#endif


INT16U clip_start_x = 0;
INT16U clip_start_y = 0;
INT16U clip_width = 640;
INT16U clip_height = 480;
void preview_zoom_set(INT32U factor)
{
	switch (factor) {
	case 2:				// 1.25X
		clip_start_x = 64;
		clip_start_y = 48;
		clip_width = 512;
		clip_height = 384;
		break;

	case 3:				// 1.5X
		clip_start_x = 112;
		clip_start_y = 80;
		clip_width = 416;
		clip_height = 320;
		break;

	case 4:				// 2X
		clip_start_x = 160;
		clip_start_y = 120;
		clip_width = 320;
		clip_height = 240;
		break;

	case 5:				// 2.5X
		clip_start_x = 192;
		clip_start_y = 144;
		clip_width = 256;
		clip_height = 192;
		break;

	case 6:				// 3X
		clip_start_x = 216;
		clip_start_y = 160;
		clip_width = 208;
		clip_height = 160;
		break;

	case 7:				// 4X
		clip_start_x = 240;
		clip_start_y = 176;
		clip_width = 160;
		clip_height = 128;
		break;

	default:			// 1X
		clip_start_x = 0;
		clip_start_y = 0;
		clip_width = 640;
		clip_height = 480;
		break;
	}
}

INT8U capture_mode_flag = 0;
void capture_mode_enter(void)
{
	capture_mode_flag = 1;
}

void capture_mode_exit(void)
{
	capture_mode_flag = 0;
}

static void black_disp_buf(unsigned int disp_address,int width,int height)
{
	unsigned int black_color = 0x00800080;
	int i=0,j=0;
	unsigned short *disp = (unsigned short *)disp_address;

	for (i=0; i<height; i++)
		for (j=0; j<width; j++) {
			*disp = black_color;
			disp++;
		}
}

PPU_REGISTER_SETS p_register_set;
void scaler_task_entry(void *parm)
{
	INT8U err, scaler_mode, skip_cnt=0;//, lock_cnt;
	INT16U csi_width, csi_height;
	INT16U encode_width, encode_height;
	INT16U dip_width, dip_height;
	INT16U dip_buff_width, dip_buff_height;
	INT32U msg_id, sensor_frame, scaler_frame;
	INT32U input_format, output_format, display_input_format, display_output_format;
	INT32U y_frame, u_frame, v_frame, uv_length;
//	FP32 fp_tft_width, fp_avi_width;

	INT32U video_frame, output_frame;
	INT32S header_size, encode_size;
	INT8U *double_disp_buf=NULL;
	INT8U black_disp_count=0;
#if C_UVC == CUSTOM_ON
	ISOTaskMsg isosend;
#endif
	INT8U disp_mode;

	disp_mode = ap_state_config_pic_size_get();
	while(1)
	{
		msg_id = (INT32U) OSQPend(scaler_task_q, 0, &err);
		if((err != OS_NO_ERR)||	!msg_id)
			continue;

		switch(msg_id & 0xFF000000)
		{
		case MSG_SCALER_TASK_INIT:
			_dmsg((GREEN "[zt]: scaler task init\r\n" NONE));
//			lock_cnt        = 0;
			skip_cnt        = 0;
			csi_width       = pAviEncVidPara->sensor_capture_width;
			csi_height      = pAviEncVidPara->sensor_capture_height;
			encode_width    = pAviEncVidPara->encode_width;
			encode_height   = pAviEncVidPara->encode_height;
			dip_width       = pAviEncVidPara->display_width;
			dip_height      = pAviEncVidPara->display_height;
			dip_buff_width  = pAviEncVidPara->display_buffer_width;
			dip_buff_height = pAviEncVidPara->display_buffer_height;
			input_format    = pAviEncVidPara->sensor_output_format;
			output_format   = C_SCALER_CTRL_OUT_RGB565;//C_SCALER_CTRL_OUT_YUYV; //wwj modify for remove PPU
			pAviEncPara->fifo_enc_err_flag = 0;
			pAviEncPara->fifo_irq_cnt = 0;
			pAviEncPara->vid_pend_cnt = 0;
			pAviEncPara->vid_post_cnt = 0;
			display_input_format = C_SCALER_CTRL_IN_YUYV;
			display_output_format = pAviEncVidPara->display_output_format;
			if (pAviEncVidPara->scaler_flag)
				scaler_mode = C_SCALER_FIT_BUFFER;
				//scaler_mode = C_NO_SCALER_FIT_BUFFER;
			else
				scaler_mode = C_SCALER_FULL_SCREEN;


#if AVI_ENCODE_VIDEO_ENCODE_EN == 1
			header_size = avi_encode_set_jpeg_quality(pAviEncVidPara->quality_value);
#endif
			OSQPost(DisplayTaskQ, (void *) MSG_DISPLAY_TASK_PPU_READY);

			OSMboxPost(scaler_task_ack_m, (void*)C_ACK_SUCCESS);
			break;

		case MSG_SCALER_TASK_STOP:
//			gp_free((void *)buff_base);
//			char_array_ptr = 0;
			OSQFlush(scaler_task_q);
			OSMboxPost(scaler_task_ack_m, (void*)C_ACK_SUCCESS);
			break;

		case MSG_SCALER_TASK_EXIT:
			OSMboxPost(scaler_task_ack_m, (void*)C_ACK_SUCCESS);
			OSTaskDel(OS_PRIO_SELF);
			break;
#if 1
		case MSG_SCALER_TASK_PREVIEW_ON:
			{
				INT32U	i;

				for (i=0; i<AVI_ENCODE_DISPALY_BUFFER_NO; i++) {
					if (!pAviEncVidPara->display_output_addr[i]) {
						pAviEncVidPara->display_output_addr[i] = msg_id & 0xFFFFFF;
//						black_disp_buf(pAviEncVidPara->display_output_addr[i], dip_buff_width, dip_buff_height);
						if (i == 0) {
							pAviEncVidPara->display_output_addr[i] |= MSG_SCALER_TASK_PREVIEW_LOCK;	/* lock logo buffer */
						}
						break;
					}
				}
				if (pAviEncVidPara->display_output_addr[AVI_ENCODE_DISPALY_BUFFER_NO-1]) {
					video_preview_flag = 1;
				}

				if (double_disp_buf == NULL) {
					double_disp_buf = gp_malloc_align(320*240*2*2, 32);
					if (double_disp_buf == NULL) {
						DBG_PRINT("malloc double_disp_buf fail\n");
					}
				}

			}
			break;

		case MSG_SCALER_TASK_PREVIEW_OFF:
			video_preview_flag = 0;
			if (double_disp_buf != NULL) {
				gp_free(double_disp_buf);
				double_disp_buf = NULL;
			}
			break;
		case MSG_SCALER_TASK_PREVIEW_LOCK:
			{
				INT32U	ack_ptr, tmp_ptr, i;
				ack_ptr = (msg_id & 0xFFFFFF);
				for (i=0; i<AVI_ENCODE_DISPALY_BUFFER_NO; i++) {
					tmp_ptr = (pAviEncVidPara->display_output_addr[i] & 0xFFFFFF);
					if (ack_ptr == tmp_ptr) {
						pAviEncVidPara->display_output_addr[i] |= MSG_SCALER_TASK_PREVIEW_LOCK;
						break;
					}
				}
			}
			break;
		case MSG_SCALER_TASK_PREVIEW_UNLOCK:
			{
				INT32U	ack_ptr, tmp_ptr, i;
				ack_ptr = (msg_id & 0xFFFFFF);
				if (ack_ptr != 0xFFFFFF) {
					for (i=0; i<AVI_ENCODE_DISPALY_BUFFER_NO; i++) {
						tmp_ptr = (pAviEncVidPara->display_output_addr[i] & 0xFFFFFF);
						if (ack_ptr == tmp_ptr) {
							pAviEncVidPara->display_output_addr[i] = ack_ptr;
							break;
						}
					}
				} else {
					for (i=0; i<AVI_ENCODE_DISPALY_BUFFER_NO; i++) {
						pAviEncVidPara->display_output_addr[i] &= 0xFFFFFF;
					}
				}
			}
			break;
#endif
		case MSG_SCALER_TASK_ALLOCATE:
			if(avi_encode_memory_alloc() < 0)
			{
				DBG_PRINT("avi memory alloc fail\r\n");
				avi_encode_memory_free();
			}
			break;

		case MSG_SCALER_TASK_FREE:
			avi_encode_memory_free();
			break;
		case MSG_SCALER_TASK_PACKER_ALLOCATE:
			if (avi_encode_packer_memory_alloc()<0){
				avi_encode_packer_memory_free();
				DBG_PRINT("packer memory alloc fail\r\n");
			}
			break;
		case MSG_SCALER_TASK_PACKER_FREE:
			avi_encode_packer_memory_free();
			msgQSend(ApQ, MSG_APQ_FREE_PACKER_MEM_DONE, NULL, NULL, MSG_PRI_NORMAL);
			break;
		case MSG_SCALER_TASK_UPDATE_DISP_MODE:
			disp_mode = ap_state_config_pic_size_get();
			_dmsg((GREEN "[zt]: update disp_mode=%0d\r\n" NONE, disp_mode));
			break;

		default:
			sensor_frame = msg_id;
			if (!skip_flag) {
				skip_cnt++;
				if (skip_cnt > 8) {
					skip_flag = 1;
				}
				else {
					OSQPost(cmos_frame_q, (void *) sensor_frame);
					break;
				}
			}
#if AVI_ENCODE_DIGITAL_ZOOM_EN == 1
			scaler_frame = avi_encode_get_scaler_frame();
			scaler_zoom_once(C_SCALER_ZOOM_FIT_BUFFER, pAviEncVidPara->scaler_zoom_ratio, input_format, output_format,
					 csi_width, csi_height, encode_width, encode_height, encode_width, encode_height, sensor_frame, 0, 0, scaler_frame, 0, 0);
#else
			if (pAviEncVidPara->scaler_flag) {
				scaler_frame = avi_encode_get_scaler_frame();
				scaler_zoom_once(C_SCALER_FULL_SCREEN, pAviEncVidPara->scaler_zoom_ratio, input_format, output_format,
						 csi_width, csi_height, encode_width, encode_height, encode_width, encode_height, sensor_frame, 0, 0, scaler_frame, 0, 0);
			} else {
				scaler_frame = sensor_frame;
			}
#endif

			// zoom start
			if (capture_mode_flag && ((clip_width != 640) || (clip_height != 480))) {
				INT32U	i, j;
				INT32U	*src_addr, *tar_addr;

				tar_addr = (INT32U *)pAviEncPacker1->file_write_buffer;
				if(tar_addr != NULL) {
					for(i=0; i<clip_height; i++) {
						src_addr = (INT32U *)(scaler_frame + csi_width*(clip_start_y+i)*2 + clip_start_x*2);
						for(j=0; j<clip_width/2; j++) {
							*tar_addr++ = *src_addr++;
						}
					}

					tar_addr -= clip_width*clip_height/2;
					scaler_zoom_once(C_SCALER_FULL_SCREEN,
									0,
									C_SCALER_CTRL_IN_YUYV, C_SCALER_CTRL_OUT_YUYV,
									clip_width, clip_height,
									csi_width, csi_height,
									csi_width, csi_height,
									(INT32U)tar_addr, 0, 0,
									scaler_frame, 0, 0);
				}
			}
			// zoom end
/*
#if AVI_ENCODE_SHOW_TIME
			{
				if (!ap_state_config_date_stamp_get()) {
					TIME_T	g_osd_time;
					cal_time_get(&g_osd_time);
					cpu_draw_time_osd(g_osd_time, scaler_frame, RGB565_DRAW, STATE_VIDEO_RECORD & 0xF);
				}
			}
#endif
*/
#if 0 //AVI_ENCODE_SHOW_TIME //wwj add
			{
				INT8U draw_type;

				if (pAviEncVidPara->scaler_zoom_ratio == 1) {
					draw_type = YUYV_DRAW;
				} else {
					draw_type = YUV420_DRAW;
				}
				if (ap_state_config_date_stamp_get()) {
					if ((avi_encode_get_status() & C_AVI_ENCODE_USB_WEBCAM) == 0) {
						TIME_T	g_osd_time;
						cal_time_get(&g_osd_time);
						cpu_draw_time_osd(g_osd_time, scaler_frame, draw_type, STATE_VIDEO_RECORD & 0xF);
					}
				}
			}
#endif
/*
		 	if (AVI_ENCODE_DIGITAL_ZOOM_EN || pAviEncVidPara->scaler_flag) {
				OSQPost(cmos_frame_q, (void *) sensor_frame);
			}
*/

			{ //display mode start
				INT32U	*display_ptr;

				if (video_preview_flag) {
					display_ptr = (INT32U *) avi_encode_get_display_frame();
					if (display_ptr) {
						*display_ptr |= MSG_SCALER_TASK_PREVIEW_LOCK;
						if ((disp_mode == 0) && (black_disp_count < MSG_SCALER_TASK_PREVIEW_ON)) {
							switch (zt_resolution()) {
							case ZT_VGA:		black_disp_buf(*display_ptr & 0xFFFFFF, dip_buff_width, 60);	break;
							case ZT_HD_SCALED:	black_disp_buf(*display_ptr & 0xFFFFFF, dip_buff_width, 80);	break;
							}
							black_disp_count++;
						} else if (disp_mode != 0) {
							black_disp_count=0;
						}

						//disp_mode = 0;
						switch (disp_mode) {
						case 0:
							switch (zt_resolution()) {
							case ZT_VGA:
								scaler_zoom_once(C_SCALER_FIT_BUFFER, 0,			// INT32U scaler_mode, FP32 bScalerFactor
									input_format, output_format,				// INT32U InputFormat, INT32U OutputFormat,
									csi_width, csi_height,					// INT16U input_x, INT16U input_y,
									dip_buff_width, 120,					// INT16U output_x, INT16U output_y,
									dip_buff_width, dip_buff_height-60,			// INT16U output_buffer_x, INT16U output_buffer_y,
									sensor_frame, 0, 0,					// INT32U scaler_input_y, INT32U scaler_input_u, INT32U scaler_input_v,
									((*display_ptr+30*csi_width) & 0xFFFFFF), 0, 0);	// INT32U scaler_output_y, INT32U scaler_output_u, INT32U scaler_output_v
								break;
							case ZT_HD_SCALED:
								scaler_zoom_once(C_SCALER_FIT_BUFFER, 0,
									input_format, output_format,
									csi_width, csi_height,
									dip_buff_width, 120/*(csi_height >> 2)*/,
									dip_buff_width, dip_buff_height-60,
									sensor_frame, 0, 0,
									((*display_ptr+25*csi_width) & 0xFFFFFF), 0, 0);
								break;
							}
							break;
						case 1:
							scaler_zoom_once(C_SCALER_FULL_SCREEN, 0,
									input_format, output_format,
									csi_width, csi_height,
									320*2, 240,
									320*2, 240,
									sensor_frame, 0, 0,
									(INT32U)double_disp_buf, 0, 0);

							dma_sprite_buffer_copy((INT32U)double_disp_buf, (*display_ptr&0xFFFFFF), dip_buff_width*dip_buff_height*2, dip_buff_width*2, dip_buff_width*2*2);
							/*
							for (i=0; i<dip_buff_height; i++)
								dma_buffer_copy(double_disp_buf+i*320*2*2, (*display_ptr&0xFFFFFF)+i*320*2, 320*2, 320*2, 320*2);
							*/
							break;
						case 2:
							scaler_zoom_once(C_SCALER_FULL_SCREEN, 0,
									input_format, output_format,
									csi_width, csi_height,
									320*2, 240,
									320*2, 240,
									sensor_frame, 0, 0,
									(INT32U)double_disp_buf, 0, 0);

							dma_sprite_buffer_copy((INT32U)(double_disp_buf+320*2), (*display_ptr&0xFFFFFF), dip_buff_width*dip_buff_height*2, dip_buff_width*2, dip_buff_width*2*2);
							break;

						}
						OSQPost(DisplayTaskQ, (void *)(*display_ptr & 0xFFFFFF));
//						OSQPost(cmos_frame_q, (void *)sensor_frame);
//						lock_cnt = 0;
					}
				}
			}
			//display mode end

#if AVI_ENCODE_SHOW_TIME // wwj add
			{
				if (ap_state_config_date_stamp_get()) {
					if ((avi_encode_get_status() & C_AVI_ENCODE_USB_WEBCAM) == 0) {
						INT8U	draw_type;
						TIME_T	g_osd_time;

						if (pAviEncVidPara->scaler_zoom_ratio == 1) {
							draw_type = YUYV_DRAW;
						} else {
							draw_type = YUV420_DRAW;
						}
						cal_time_get(&g_osd_time);
						cpu_draw_time_osd(g_osd_time, scaler_frame, draw_type, STATE_VIDEO_RECORD & 0xF);
					}
				}
			}
#endif

		if ((avi_encode_get_status() & C_AVI_VID_ENC_START)||(avi_encode_get_status() & C_AVI_ENCODE_USB_WEBCAM))
		{
//for zoom
			if (pAviEncVidPara->scaler_zoom_ratio != 1) {
				uv_length = encode_width * encode_height;
				y_frame = avi_encode_get_scaler_frame();
				u_frame = y_frame + uv_length;
				v_frame = u_frame + (uv_length >> 1);
				scaler_zoom_once(C_SCALER_ZOOM_FIT_BUFFER,
									pAviEncVidPara->scaler_zoom_ratio,
									C_SCALER_CTRL_IN_YUYV,
									C_SCALER_CTRL_OUT_YUV420,
									encode_width, encode_height,
									encode_width, encode_height, //+2 is to fix block line
									encode_width, encode_height, //+2 is to fix block line
									scaler_frame, 0, 0,
									y_frame, u_frame, v_frame);
				OSQPost(cmos_frame_q, (void *)scaler_frame);
			} else {
				y_frame = scaler_frame;
			}

			if(pAviEncPara->vid_post_cnt == pAviEncPara->vid_pend_cnt)
			{
				pAviEncPara->vid_post_cnt++;
//				OSQPost(vid_enc_task_q, (void *)y_frame);

				pAviEncPara->vid_pend_cnt++;
#if C_UVC == CUSTOM_ON
				if((avi_encode_get_status()&C_AVI_ENCODE_USB_WEBCAM))
				{
					video_frame = (INT32U) OSQAccept(usbwebcam_frame_q, &err);
					if(!((err == OS_NO_ERR) && video_frame))
	    			{
	    				goto VIDEO_ENCODE_FRAME_MODE_END1;
	    			}

				}
				else
				{
					video_frame = avi_encode_get_video_frame();
				}
#else
				video_frame = avi_encode_get_video_frame();
#endif
				output_frame = video_frame + header_size;
				if(pAviEncVidPara->video_format == C_MJPG_FORMAT)
				{
					INT32U input_format1;

					y_frame = scaler_frame;
					if (pAviEncVidPara->scaler_zoom_ratio == 1) {
						input_format1 = C_JPEG_FORMAT_YUYV;
						u_frame = 0;
						v_frame = 0;
					} else {
						input_format1 = C_JPEG_FORMAT_YUV_SEPARATE;
						u_frame = y_frame + uv_length;
						v_frame = u_frame + (uv_length >> 1);
					}
					encode_size = jpeg_encode_once(pAviEncVidPara->quality_value, input_format1,
													encode_width, encode_height, y_frame, u_frame, v_frame, output_frame);
					pAviEncPara->ready_frame = video_frame;
					pAviEncPara->ready_size = encode_size + header_size;
					pAviEncPara->key_flag = AVIIF_KEYFRAME;

					if (pAviEncVidPara->scaler_zoom_ratio == 1) {
						OSQPost(cmos_frame_q, (void *)scaler_frame);
					}

					if (take_pic) {
						avi_encode_clear_status(C_AVI_VID_ENC_START);
						cache_invalid_range(pAviEncPara->ready_frame, pAviEncPara->ready_size);
						vid_enc_buffer_lock(pAviEncPara->ready_frame);
						take_pic = 0;
					}
				}
#if C_UVC == CUSTOM_ON
				if((avi_encode_get_status()&C_AVI_ENCODE_USB_WEBCAM))
				{
					//DBG_PRINT("start AVI USB CAM \r\n");
					isosend.FrameSize = pAviEncPara->ready_size;
					isosend.AddrFrame = video_frame;
					err = OSQPost(USBCamApQ, (void*)(&isosend));//usb start send data
				}
#endif
				break;

VIDEO_ENCODE_FRAME_MODE_END1:
				OSQPost(cmos_frame_q, (void *)sensor_frame);
				break;
			} else {
				OSQPost(cmos_frame_q, (void *)sensor_frame);
			}
		} else {
			OSQPost(cmos_frame_q, (void *)sensor_frame);
		}
		break;
/*
#if AVI_ENCODE_PREVIEW_DISPLAY_EN == 1
			if(pAviEncVidPara->dispaly_scaler_flag)
			{
				INT32U *display_ptr;

				display_ptr = (INT32U *) avi_encode_get_display_frame();
				if (display_ptr && video_preview_flag) {
					if (skip_cnt == 0) {
						display_frame = *display_ptr;
						scaler_zoom_once(scaler_mode,
										pAviEncVidPara->scaler_zoom_ratio,
										display_input_format, display_output_format,
										encode_width, encode_height,
										dip_width, dip_height,
										dip_buff_width, dip_buff_height,
										scaler_frame, 0, 0,
										display_frame, 0, 0);
						*display_ptr |= MSG_SCALER_TASK_PREVIEW_LOCK;
						OSQPost(DisplayTaskQ, (void *) display_frame);
					}
				}
				skip_cnt++;
				if (skip_cnt > 1) {
					skip_cnt = 0;
				}
        	}
        	else
        	{
				display_frame =	scaler_frame;
			}
#endif



			if((AVI_ENCODE_DIGITAL_ZOOM_EN == 0) && (pAviEncVidPara->scaler_flag ==0))
			{
				OSQPost(cmos_frame_q, (void *)sensor_frame);
			}
*/
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
//	video encode task
///////////////////////////////////////////////////////////////////////////////////////////////////////
INT32S video_encode_task_create(INT8U pori)
{
	INT8U  err;
	INT32S nRet;

	vid_enc_task_q = OSQCreate(video_encode_task_q_stack, C_JPEG_QUEUE_MAX);
	if(!scaler_task_q) RETURN(STATUS_FAIL);

	vid_enc_task_ack_m = OSMboxCreate(NULL);
	if(!scaler_task_ack_m) RETURN(STATUS_FAIL);

	err = OSTaskCreate(video_encode_task_entry, (void *)NULL, &JpegTaskStack[C_JPEG_STACK_SIZE-1], pori);
	if(err != OS_NO_ERR) RETURN(STATUS_FAIL);

	nRet = STATUS_OK;
Return:
	return nRet;
}

INT32S video_encode_task_del(void)
{
	INT8U  err;
	INT32S nRet, msg;

	nRet = STATUS_OK;
	POST_MESSAGE(vid_enc_task_q, MSG_VIDEO_ENCODE_TASK_EXIT, vid_enc_task_ack_m, 5000, msg, err);
Return:
	OSQFlush(vid_enc_task_q);
   	OSQDel(vid_enc_task_q, OS_DEL_ALWAYS, &err);
	OSMboxDel(vid_enc_task_ack_m, OS_DEL_ALWAYS, &err);

#if C_UVC == CUSTOM_ON
	OSQFlush(usbwebcam_frame_q);
   	OSQDel(usbwebcam_frame_q, OS_DEL_ALWAYS, &err);
#endif
	return nRet;
}

INT32S video_encode_task_start(void)
{
	INT8U  err;
	INT32S nRet, msg;

	nRet = STATUS_OK;
	POST_MESSAGE(vid_enc_task_q, MSG_VIDEO_ENCODE_TASK_MJPEG_INIT, vid_enc_task_ack_m, 5000, msg, err);
Return:
	return nRet;
}

INT32S video_encode_task_stop(void)
{
	INT8U  err;
	INT32S nRet, msg;

	nRet = STATUS_OK;
	POST_MESSAGE(vid_enc_task_q, MSG_VIDEO_ENCODE_TASK_STOP, vid_enc_task_ack_m, 5000, msg, err);
Return:
    return nRet;
}

void video_encode_task_entry(void *parm)
{
	INT8U   err;
	INT16U  encode_width, encode_height;
	INT16U	csi_width, csi_height;
	INT32S  nRet, header_size, encode_size;
	INT32U	output_frame, video_frame, scaler_frame;
	INT32U  msg_id, yuv_sampling_mode;
#if MPEG4_ENCODE_ENABLE == 1
	#define MAX_P_FRAME		10
	INT8U	time_inc_bit, p_cnt;
	INT32U  temp, write_refer_addr, reference_addr;
#endif
INT32U y_frame, u_frame, v_frame, uv_length;
#if VIDEO_ENCODE_MODE == C_VIDEO_ENCODE_FIFO_MODE
	INT8U  jpeg_start_flag;
	INT16U scaler_height;
	INT32U input_y_len, input_uv_len, uv_length;
	INT32U y_frame, u_frame, v_frame;
	INT32S status;
#endif
#if C_UVC == CUSTOM_ON
	ISOTaskMsg isosend;
#endif
	//R_IOC_DIR |= 0x0C; R_IOC_ATT |= 0x0C; R_IOC_O_DATA = 0x0;
	while(1)
	{
		msg_id = (INT32U) OSQPend(vid_enc_task_q, 0, &err);
		if(err != OS_NO_ERR)
		    continue;

		switch(msg_id)
		{
		case MSG_VIDEO_ENCODE_TASK_MJPEG_INIT:
		 	encode_width = pAviEncVidPara->encode_width;
		 	encode_height = pAviEncVidPara->encode_height;
		 	csi_width = pAviEncVidPara->sensor_capture_width;
		 	csi_height = pAviEncVidPara->sensor_capture_height;
		 	yuv_sampling_mode = C_JPEG_FORMAT_YUYV;
			output_frame = 0;
			uv_length = encode_width * encode_height;
#if VIDEO_ENCODE_MODE == C_VIDEO_ENCODE_FIFO_MODE
			pAviEncPara->vid_post_cnt = pAviEncVidPara->sensor_capture_height / SENSOR_FIFO_LINE;
			if(pAviEncVidPara->sensor_capture_height % SENSOR_FIFO_LINE)
				while(1);//this must be no remainder

			jpeg_start_flag = 0;
			scaler_height = encode_height / pAviEncPara->vid_post_cnt;
			uv_length = encode_width * (scaler_height + 2);
			input_y_len = encode_width * scaler_height;
			input_uv_len = input_y_len >> 1; //YUV422
			//input_uv_len = input_y_len >> 2; //YUV420
			if(pAviEncVidPara->scaler_flag)
				yuv_sampling_mode = C_JPEG_FORMAT_YUV_SEPARATE;
			else
				yuv_sampling_mode = C_JPEG_FORMAT_YUYV;
#endif
#if AVI_ENCODE_VIDEO_ENCODE_EN == 1
			header_size = avi_encode_set_jpeg_quality(pAviEncVidPara->quality_value);
#endif
			OSMboxPost(vid_enc_task_ack_m, (void*)C_ACK_SUCCESS);
			break;

		case MSG_VIDEO_ENCODE_TASK_MPEG4_INIT:
			nRet = STATUS_FAIL;
		#if MPEG4_ENCODE_ENABLE == 1
			encode_width = pAviVidPara->encode_width;
		 	encode_height = pAviVidPara->encode_height;
		 	yuv_sampling_mode = MP4_ENC_INPUT_FORMAT;
		 	q_value = pAviVidPara->quality_value & 0x1F;
		 	if(q_value == 0) q_value = pAviVidPara->quality_value = 0x05;
		 	nRet = avi_encode_mpeg4_malloc(encode_width, encode_height);
		 	if(nRet < 0)
		 	{
		 		DEBUG_MSG(DBG_PRINT("mpeg4 memory alloc fail!!!\r\n"));
		 		avi_encode_mpeg4_free();
		 		OSMboxPost(vid_enc_task_ack_m, (void*)C_ACK_FAIL);
		 		continue;
		 	}

			header_size = avi_encode_set_mp4_resolution(encode_width, encode_height);
			if(header_size < 0)
			{
				avi_encode_mpeg4_free();
		 		OSMboxPost(vid_enc_task_ack_m, (void*)C_ACK_FAIL);
		 		continue;
			}

		 	write_refer_addr = pAviVidPara->write_refer_addr;
		 	reference_addr = pAviVidPara->reference_addr;
		 	time_inc_bit = 0x0F;
		 	p_cnt = 0;
		 	mpeg4_encode_init();
		 	mpeg4_encode_isram_set(pAviEncPara->isram_addr);
		 	mpeg4_encode_config(yuv_sampling_mode, encode_width, encode_height, time_inc_bit - 1);
		 	mpeg4_encode_ip_set(FALSE, MAX_P_FRAME);
		#endif
			if(nRet >= 0)
		 		OSMboxPost(vid_enc_task_ack_m, (void*)C_ACK_SUCCESS);
			else
				OSMboxPost(vid_enc_task_ack_m, (void*)C_ACK_FAIL);
			break;

		case MSG_VIDEO_ENCODE_TASK_STOP:
		#if MPEG4_ENCODE_ENABLE == 1
			if(pAviVidPara->video_format == C_XVID_FORMAT)
				avi_encode_mpeg4_free();
		#endif
			OSQFlush(vid_enc_task_q);
			OSMboxPost(vid_enc_task_ack_m, (void*)C_ACK_SUCCESS);
			break;

		case MSG_VIDEO_ENCODE_TASK_EXIT:
			OSMboxPost(vid_enc_task_ack_m, (void*)C_ACK_SUCCESS);
			OSTaskDel(OS_PRIO_SELF);
			break;

		default:
#if VIDEO_ENCODE_MODE == C_VIDEO_ENCODE_FRAME_MODE
			pAviEncPara->vid_pend_cnt++;
			scaler_frame = msg_id;
#if C_UVC == CUSTOM_ON
			if((avi_encode_get_status()&C_AVI_ENCODE_USB_WEBCAM))
			{
				video_frame = (INT32U) OSQAccept(usbwebcam_frame_q, &err);
				if(!((err == OS_NO_ERR) && video_frame))
    			{
    				goto VIDEO_ENCODE_FRAME_MODE_END;
    			}

			}
			else
			{
				video_frame = avi_encode_get_video_frame();
			}
#else
				video_frame = avi_encode_get_video_frame();
#endif
			output_frame = video_frame + header_size;
			if(pAviEncVidPara->video_format == C_MJPG_FORMAT)
			{
				INT32U input_format;
//for zoom
				y_frame = scaler_frame;
				if (pAviEncVidPara->scaler_zoom_ratio == 1) {
					input_format = C_JPEG_FORMAT_YUYV;
					u_frame = 0;
					v_frame = 0;
				} else {
					input_format = C_JPEG_FORMAT_YUV_SEPARATE;
					u_frame = y_frame + uv_length;
					v_frame = u_frame + (uv_length >> 1);
				}
				encode_size = jpeg_encode_once(pAviEncVidPara->quality_value, input_format,
												encode_width, encode_height, y_frame, u_frame, v_frame, output_frame);
				pAviEncPara->ready_frame = video_frame;
				pAviEncPara->ready_size = encode_size + header_size;
				pAviEncPara->key_flag = AVIIF_KEYFRAME;
				if (pAviEncVidPara->scaler_zoom_ratio == 1) {
					OSQPost(cmos_frame_q, (void *)scaler_frame);
				}
				if (take_pic) {
					avi_encode_clear_status(C_AVI_VID_ENC_START);
					cache_invalid_range(pAviEncPara->ready_frame, pAviEncPara->ready_size);
					vid_enc_buffer_lock(pAviEncPara->ready_frame);
					take_pic = 0;
				}
			}
#if C_UVC == CUSTOM_ON
			if((avi_encode_get_status()&C_AVI_ENCODE_USB_WEBCAM))
			{
				isosend.FrameSize = pAviEncPara->ready_size;
				isosend.AddrFrame = video_frame;
				err = OSQPost(USBCamApQ, (void*)(&isosend));//usb start send data
			}
#endif
		#if MPEG4_ENCODE_ENABLE == 1
			else
			{
				temp = reference_addr;
				reference_addr = write_refer_addr;
				write_refer_addr = temp;
				mpeg4_encode_start(C_MPEG_I_FRAME, pAviEncPara->quality_value, output_frame, (INT32U)scaler_frame,
								write_refer_addr, reference_addr);
				nRet = mpeg4_wait_idle(TRUE);
				if(nRet != 0x04)
					while(1);

				mpeg4_encode_stop();
				encode_size = mpeg4_encode_get_vlc_size();
				if(p_cnt == 0)
				{	//i frame
					pAviEncPara->ready_frame = video_frame;
					pAviEncPara->ready_size = encode_size + header_size;
					pAviEncPara->key_flag = AVIIF_KEYFRAME;
				}
				else
				{	//p frame
					pAviEncPara->ready_frame = output_frame;
					pAviEncPara->ready_size = encode_size;
					pAviEncPara->key_flag = 0x00;
				}

				p_cnt++;
				if(p_cnt > MAX_P_FRAME)
					p_cnt = 0;
			}
		#endif
VIDEO_ENCODE_FRAME_MODE_END:
			break;

#elif VIDEO_ENCODE_MODE == C_VIDEO_ENCODE_FIFO_MODE
			if(msg_id & C_AVI_ENCODE_FIFO_ERR)
			{
				//DEBUG_MSG(DBG_PRINT("F1"));
				goto VIDEO_ENCODE_FIFO_MODE_FAIL;
			}

			if(pAviEncVidPara->scaler_flag)
			{
				//R_IOC_O_DATA ^= 0x08;
				scaler_frame = msg_id & (~C_AVI_ENCODE_FRAME_END);
				y_frame = avi_encode_get_scaler_frame();
				u_frame = y_frame + uv_length;
				v_frame = u_frame + (uv_length >> 1);
				scaler_zoom_once(C_SCALER_FULL_SCREEN,
								0,
								pAviEncVidPara->sensor_output_format,
								C_SCALER_CTRL_OUT_YUV422,
								csi_width, SENSOR_FIFO_LINE,
								encode_width, scaler_height + 2, //+2 is to fix block line
								encode_width, scaler_height + 2, //+2 is to fix block line
								scaler_frame, 0, 0,
								y_frame, u_frame, v_frame);
				//R_IOC_O_DATA ^= 0x08;
			}
			else
			{
				y_frame = msg_id & (~C_AVI_ENCODE_FRAME_END);
				u_frame = v_frame = 0;
			}

			pAviEncPara->vid_pend_cnt++;
			if(msg_id & C_AVI_ENCODE_FRAME_END)
			{
				if(pAviEncPara->vid_pend_cnt != pAviEncPara->vid_post_cnt)
				{
					DEBUG_MSG(DBG_PRINT("F2"));
					goto VIDEO_ENCODE_FIFO_MODE_FAIL;
				}

				if(jpeg_start_flag == 0)
					goto VIDEO_ENCODE_FIFO_MODE_FAIL;

				nRet = 3; //jpeg encode end
				pAviEncPara->vid_pend_cnt = 0;
			}
			else if(pAviEncPara->vid_pend_cnt == 1)
			{
				if(jpeg_start_flag == 1)
				{
					jpeg_start_flag = 0;
					jpeg_encode_stop();
				}
				nRet = 1; //jpeg encode start
			}
			else if(pAviEncPara->vid_pend_cnt < pAviEncPara->vid_post_cnt)
			{
				if(jpeg_start_flag == 0)
					goto VIDEO_ENCODE_FIFO_MODE_FAIL;

				nRet = 2; //jpeg encode once
			}
			else
			{
				// error happen
				goto VIDEO_ENCODE_FIFO_MODE_FAIL;
			}

			switch(nRet)
			{
			case 1:
				//DEBUG_MSG(DBG_PRINT("J"));
				video_frame = avi_encode_get_video_frame();
				status = jpeg_encode_fifo_start(0,
												pAviEncVidPara->quality_value,
												yuv_sampling_mode,
												encode_width, encode_height,
												y_frame, u_frame, v_frame,
												video_frame + header_size, input_y_len, input_uv_len);
				if(status < 0) goto VIDEO_ENCODE_FIFO_MODE_FAIL;
				jpeg_start_flag = 1;
				break;

			case 2:
				//DEBUG_MSG(DBG_PRINT("*"));
				status = jpeg_encode_fifo_once(	0,
												status,
												y_frame, u_frame, v_frame,
												input_y_len, input_uv_len);
				if(status < 0) goto VIDEO_ENCODE_FIFO_MODE_FAIL;
				break;

			case 3:
				//DEBUG_MSG(DBG_PRINT("G\r\n"));
				encode_size = jpeg_encode_fifo_stop(0,
													status,
													y_frame, u_frame, v_frame,
													input_y_len, input_uv_len);
				jpeg_start_flag = 0;
				if(encode_size > 0)
				{
					if(pAviEncPara->avi_encode_status & C_AVI_VID_ENC_START &&
						pAviEncPara->ready_frame == 0 &&
						pAviEncPara->ready_size == 0)
					{
						pAviEncPara->ready_frame = video_frame;
						pAviEncPara->ready_size = encode_size + header_size;
						pAviEncPara->key_flag = AVIIF_KEYFRAME;
						cache_invalid_range(pAviEncPara->ready_frame, pAviEncPara->ready_size);
						vid_enc_buffer_lock(pAviEncPara->ready_frame);
					}
				}
				else
				{
					goto VIDEO_ENCODE_FIFO_MODE_FAIL;
				}
				break;
			}
			break;

VIDEO_ENCODE_FIFO_MODE_FAIL:
			//DEBUG_MSG(DBG_PRINT("E"));
			pAviEncPara->vid_pend_cnt = 0;
			pAviEncPara->ready_frame = 0;
			pAviEncPara->ready_size = 0;
			pAviEncPara->key_flag = 0;
			if(jpeg_start_flag == 1)
			{
				jpeg_start_flag = 0;
				jpeg_encode_stop();
			}
#endif
		break;
		}
	}
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
