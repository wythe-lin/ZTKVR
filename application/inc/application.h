#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include "gplib.h"
#include "application_cfg.h"
#include "drv_l1_sfr.h"

/* Pseudo Header Include there*/
#include "turnkey_sys_msg.h"
#include "turnkey_drv_msg.h"

/* AUDIO Task */

typedef struct
{
	INT8U	*ring_buf;
	INT8S   *work_mem;
	INT8U   reading;
	INT16S  file_handle;
	INT32U  frame_size;
	INT32U  ring_size;
	INT32U  ri;
	INT32U  wi;
	INT32U  f_last;
	INT32U  file_len;
	INT32U  file_cnt;
	INT32U  try_cnt;
	INT32U  read_secs;
	INT32U  file_end;
}AUDIO_CTRL;

typedef struct
{
    INT32S curr_play_time;
    INT32S total_play_time;
} ST_AUDIO_PLAY_TIME;

extern void audio_task_entry(void *p_arg);
extern void audio_send_stop(void);
extern OS_EVENT	*hAudioTaskQ;
extern OS_EVENT    *SemAudio;
extern AUDIO_CTRL		audio_ctrl;		// added by Bruce, 2008/09/18

extern INT32U g_MIDI_index;		// added by Bruce, 2008/10/09
extern void (*decode_end)(INT32U audio_decoder_num);	// modified by Bruce, 2008/11/20
extern INT32S (*audio_move_data)(INT32U buf_addr, INT32U buf_size);	// added by Bruce, 2008/10/27


// File Service Task
#define C_FILE_SERVICE_CMD_READ			0x8000
#define C_FILE_SERVICE_CMD_FLUSH		0x4000

#define C_FILE_SERVICE_FLUSH_DONE		0xFF010263

typedef struct {
	INT16U cmd;
	INT16S fd;
	INT32U buf_addr;
	INT32U buf_size;
  #if _OPERATING_SYSTEM == 1	
	OS_EVENT *result_queue;
  #endif
} FILE_SERVICE_STRUCT, *P_FILE_SERVICE_STRUCT;

extern INT32S file_service_task_create(INT8U prio);
extern void file_service_set_cmd(P_FILE_SERVICE_STRUCT cmd);

// JPEG Deocde Task
extern INT32S jpeg_decode_task_create(INT8U prio);



/* audio */
#define MAX_DAC_BUFFERS   16
#define C_DMA_DBF_START   2
#define C_DMA_WIDX_CLEAR  3
#define AUDIO_PRIOR_DYNAMIC_SW 1
extern INT32S audio_dbf_task_create(INT32U task_priority);
extern INT32S audio_task_create(INT32U task_priority);
extern INT32S  audio_songlist_create(INT8U device);

typedef enum
{
	AUDIO_CMD_START,
	AUDIO_CMD_STOP,
	AUDIO_CMD_NEXT,
	AUDIO_CMD_PREV,
	AUDIO_CMD_PAUSE,
	AUDIO_CMD_RESUME,
	AUDIO_CMD_AVI_START,
	AUDIO_CMD_DECODE_NEXT_FRAME
}AUDIO_CMD;


/*==================== Turnkey Solution Application header declare */
/* turn key message manager */
#define MSG_PRI_NORMAL		0
#define MSG_PRI_URGENT		1

typedef struct 
{
	INT32U		maxMsgs;			/* max messages that can be queued */
	INT32U		maxMsgLength;		/* max bytes in a message */
	OS_EVENT*	pEvent;
	void*		pMsgQ;
	INT8U*		pMsgPool;
} *MSG_Q_ID;

//========================================================
//Function Name:msgQCreate
//Syntax:		MSG_Q_ID msgQCreate(INT32U maxQSize, INT32U maxMsgs, INT32U maxMsgLength)
//Purpose:		create and initialize a message queue
//Note:			
//Parameters:   INT32U maxQSize			/* max queue can be creat */
//				INT32U	maxMsgs			/* max messages that can be queued */
//				INT32U	maxMsgLength	/* max bytes in a message */
//Return:		NULL if faile
//=======================================================
extern MSG_Q_ID msgQCreate(INT32U maxQSize, INT32U maxMsgs, INT32U maxMsgLength);

//========================================================
//Function Name:msgQDelete
//Syntax:		void msgQDelete (MSG_Q_ID msgQId)
//Purpose:		delete a message queue
//Note:			
//Parameters:   MSG_Q_ID msgQId		/* message queue to delete */
//Return:		
//=======================================================
extern void msgQDelete (MSG_Q_ID msgQId);

//========================================================
//Function Name:msgQSend
//Syntax:		INT32S msgQSend(MSG_Q_ID msgQId, INT32U msg_id, void *para, INT32U nParaByte, INT32U priority)
//Purpose:		send a message to a message queue
//Note:			
//Parameters:   MSG_Q_ID msgQId			/* message queue on which to send */
//				INT32U msg_id			/* message id */
//				void *para				/* message to send */
//				INT32U nParaByte		/* byte number of para buffer */
//				INT32U priority			/* MSG_PRI_NORMAL or MSG_PRI_URGENT */
//Return:		-1 if faile 
//				0 success
//=======================================================
extern INT32S msgQSend(MSG_Q_ID msgQId, INT32U msg_id, void *para, INT32U nParaByte, INT32U priority);

//========================================================
//Function Name:msgQReceive
//Syntax:		INT32S msgQReceive(MSG_Q_ID msgQId, INT32U *msg_id, void *para, INT32U maxParaNByte)
//Purpose:		receive a message from a message queue
//Note:			
//Parameters:   MSG_Q_ID msgQId			/* message queue on which to send */
//				INT32U *msg_id			/* message id */
//				void *para				/* message and type received */
//				INT32U maxNByte			/* message size */
//Return:		-1: if faile
//				0: success
//=======================================================
extern INT32S msgQReceive(MSG_Q_ID msgQId, INT32U *msg_id, void *para, INT32U maxParaNByte);

//========================================================
//Function Name:msgQAccept
//Syntax:		INT32S msgQAccept(MSG_Q_ID msgQId, INT32U *msg_id, void *para, INT32U maxParaNByte)
//Purpose:		Check whether a message is available from a message queue
//Note:
//Parameters:   MSG_Q_ID msgQId			/* message queue on which to send */
//				INT32U *msg_id			/* message id */
//				void *para				/* message and type received */
//				INT32U maxNByte			/* message size */
//Return:		-1: queue is empty or fail
//				0: success
//=======================================================
extern INT32S msgQAccept(MSG_Q_ID msgQId, INT32U *msg_id, void *para, INT32U maxParaNByte);

//========================================================
//Function Name:msgQFlush
//Syntax:		void msgQFlush(MSG_Q_ID msgQId)
//Purpose:		flush message queue
//Note:			
//Parameters:   MSG_Q_ID msgQId
//Return:		
//=======================================================
extern void msgQFlush(MSG_Q_ID msgQId);

//========================================================
//Function Name:msgQQuery
//Syntax:		INT8U msgQQuery(MSG_Q_ID msgQId, OS_Q_DATA *pdata)
//Purpose:		get current Q message information
//Note:			
//Parameters:   MSG_Q_ID msgQId, OS_Q_DATA *pdata
//Return:		
//=======================================================
extern INT8U msgQQuery(MSG_Q_ID msgQId, OS_Q_DATA *pdata);

//========================================================
//Function Name:msgQSizeGet
//Syntax:		INT32U msgQSizeGet(MSG_Q_ID msgQId)
//Purpose:		get current Q message number
//Note:			
//Parameters:   MSG_Q_ID msgQId
//Return:		
//=======================================================
extern INT32U msgQSizeGet(MSG_Q_ID msgQId);


/* ap - error call function define */
extern INT32S err_call(INT32S status, INT32S err_define, void(* ErrHandleFunction)(void));
extern INT32S not_OK_call(INT32S status, INT32S ok_define, void(* NotOkHandleFunction)(void));

/* Turn Key Task Message Structure Declare */

/* TurnKey Task entry declare */
/* Turn Key Audio DAC task */
extern void audio_dac_task_entry(void * p_arg);
extern INT32U      last_send_idx;	// added by Bruce, 2008/12/01

/*== Turn Key Audio Decoder task ==*/
typedef struct 
{
    MSG_AUD_ENUM  msg_id; 
    void* pPara1;
} STTkAudioTaskMsg;

typedef struct
{
    BOOLEAN   mute;
    INT8U     volume;
    INT16S    fd;
    INT32U    src_id;
    INT32U    src_type;
    INT32U    audio_format;
    INT32U    file_len;
    INT32U    play_speed;
    INT32U    reverse;
} STAudioTaskPara;

typedef enum
{
	AUDIO_SRC_TYPE_FS=0,
	AUDIO_SRC_TYPE_SDRAM,
	AUDIO_SRC_TYPE_SPI,
	AUDIO_SRC_TYPE_RS,			// added by Bruce, 2009/02/19
	AUDIO_SRC_TYPE_USER_DEFINE,	// added by Bruce, 2008/10/27
	AUDIO_SRC_TYPE_FS_RESOURCE_IN_FILE,	// added by Bruce, 2010/01/22
	AUDIO_SRC_TYPE_GPRS,
	AUDIO_SRC_TYPE_APP_RS,
	AUDIO_SRC_TYPE_APP_PACKED_RS,
	AUDIO_SRC_TYPE_MAX
}AUDIO_SOURCE_TYPE;

typedef enum
{
	AUDIO_TYPE_NONE = -1,
	AUDIO_TYPE_WAV,
	AUDIO_TYPE_MP3,
	AUDIO_TYPE_WMA,
	AUDIO_TYPE_AVI,
	AUDIO_TYPE_AVI_MP3,
	AUDIO_TYPE_A1800,//080722
	AUDIO_TYPE_MIDI,//080903
	AUDIO_TYPE_A1600,
	AUDIO_TYPE_A6400,
	AUDIO_TYPE_S880
}AUDIO_TYPE;

typedef struct
{
    MSG_AUD_ENUM result_type;
    AUDIO_SOURCE_TYPE source_type;
    INT32S result;
} STAudioConfirm;

typedef enum
{
	AUDIO_ERR_NONE,
	AUDIO_ERR_FAILED,
	AUDIO_ERR_INVALID_FORMAT,
	AUDIO_ERR_OPEN_FILE_FAIL,
	AUDIO_ERR_GET_FILE_FAIL,
	AUDIO_ERR_DEC_FINISH,
	AUDIO_ERR_DEC_FAIL,
	AUDIO_ERR_READ_FAIL,
	AUDIO_ERR_MEM_ALLOC_FAIL
}AUDIO_STATUS;

extern void audio_task_entry(void * p_arg); 
extern void audio_bg_task_entry(void * p_arg); 
extern MSG_Q_ID	AudioTaskQ;
extern MSG_Q_ID	AudioBGTaskQ;
extern OS_EVENT	*audio_wq;
extern OS_EVENT	*audio_bg_wq;
extern INT16S   *pcm_out[MAX_DAC_BUFFERS];
extern INT32U    pcm_len[MAX_DAC_BUFFERS];
extern INT16S   *pcm_bg_out[MAX_DAC_BUFFERS];
extern INT32U    pcm_bg_len[MAX_DAC_BUFFERS];

// added by Bruce, 2008/10/09
extern void midi_task_entry(void *p_arg);
extern OS_EVENT	*midi_wq;
extern MSG_Q_ID MIDITaskQ;
extern INT16S   *pcm_midi_out[MAX_DAC_BUFFERS];
extern INT32U    pcm_midi_len[MAX_DAC_BUFFERS];
// added by Bruce, 2008/10/09
extern INT32U g_audio_data_length;
/*== Turn Key File Server Task ==*/
typedef struct 
{
    MSG_FILESRV_ENUM  msg_id; 
    void* pPara1;
} STTkFileSrvTaskMsg;

typedef struct {
	INT8U   src_id;
	INT8U   src_name[10];
	INT16U  sec_offset;
	INT16U  sec_cnt;
} TK_FILE_SERVICE_SPI_STRUCT, *P_TK_FILE_SERVICE_SPI_STRUCT;


typedef struct {
	INT16S fd;
	INT32U buf_addr;
	INT32U buf_size;
	INT32U rev_seek;
	TK_FILE_SERVICE_SPI_STRUCT spi_para;
	OS_EVENT *result_queue;
} TK_FILE_SERVICE_STRUCT, *P_TK_FILE_SERVICE_STRUCT;

typedef struct 
{
	INT16S disk;
	OS_EVENT *result_queue;
} STMountPara;

typedef struct 
{
	struct STFileNodeInfo *pstFNodeInfo;
	OS_EVENT *result_queue;
} STScanFilePara;

extern void filesrv_task_entry(void * p_arg);
extern void file_scan_task_entry(void *parm);
//extern void file_service_set_cmd(STFileSrvTaskMsg *FileSrvTaskMsg);

//========================================================
//Function Name:WaitScanFile
//Syntax:		void ScanFileWait(struct STFileNodeInfo *pstFNodeInfo, INT32S index)
//Purpose:		wait for search file
//Note:			
//Parameters:   pstFNodeInfo	/* the point to file node information struct */
//				index			/* the file index you want to find */
//Return:		
//=======================================================
extern INT32S ScanFileWait(struct STFileNodeInfo *pstFNodeInfo, INT32S index);

/*== Turn Key Image processing task ==*/
#define IMAGE_CMD_STATE_MASK					0xFC000000
#define IMAGE_CMD_STATE_SHIFT_BITS				26

typedef enum {
	TK_IMAGE_SOURCE_TYPE_FILE = 0x0,  
	TK_IMAGE_SOURCE_TYPE_BUFFER,
	TK_IMAGE_SOURCE_TYPE_NVRAM,
	TK_IMAGE_SOURCE_TYPE_MAX
} TK_IMAGE_SOURCE_TYPE_ENUM;

typedef enum {
	TK_IMAGE_TYPE_JPEG = 0x1,  
	TK_IMAGE_TYPE_PROGRESSIVE_JPEG,
	TK_IMAGE_TYPE_MOTION_JPEG,
	TK_IMAGE_TYPE_BMP,
	TK_IMAGE_TYPE_GIF,
	TK_IMAGE_TYPE_PNG,
	TK_IMAGE_TYPE_GPZP,
	TK_IMAGE_TYPE_WAV,
	//#ifdef _DPF_PROJECT
	TK_IMAGE_TYPE_MOV_JPEG,		//mov	// added by Bruce, 2009/02/19
	//#endif
	TK_IMAGE_TYPE_MAX
} TK_IMAGE_TYPE_ENUM;

typedef struct {
	INT32U cmd_id;
	INT32S image_source;          	// File handle/resource handle/pointer
	INT32U source_size;             // File size or buffer size
	INT8U source_type;              // TK_IMAGE_SOURCE_TYPE_FILE/TK_IMAGE_SOURCE_TYPE_BUFFER/TK_IMAGE_SOURCE_TYPE_NVRAM

	INT8S parse_status;				// 0=ok, others=fail
	INT8U image_type;				// TK_IMAGE_TYPE_ENUM
	INT8U orientation;
	INT8U date_time_ptr[20];		// 20 bytes including NULL terminator. Format="YYYY:MM:DD HH:MM:SS"
	INT16U width;
	INT16U height;
} IMAGE_HEADER_PARSE_STRUCT;

typedef struct {
	INT32U cmd_id;
	INT32S image_source;          	// File handle/resource handle/pointer
	INT32U source_size;             // File size or buffer size
	INT8U source_type;              // 0=File System, 1=SDRAM, 2=NVRAM
	INT8S decode_status;            // 0=ok, others=fail
	INT8U output_format;
	INT8U output_ratio;             // 0=Fit to output_buffer_width and output_buffer_height, 1=Maintain ratio and fit to output_buffer_width or output_buffer_height, 2=Same as 1 but without scale up, 3=Special case for thumbnail show
	INT16U output_buffer_width;
	INT16U output_buffer_height;
	INT16U output_image_width;
	INT16U output_image_height;
	INT32U out_of_boundary_color;
	INT32U output_buffer_pointer;
} IMAGE_DECODE_STRUCT, *P_IMAGE_DECODE_STRUCT;

typedef struct {
	IMAGE_DECODE_STRUCT basic;
	INT16U clipping_start_x;
	INT16U clipping_start_y;
	INT16U clipping_width;
	INT16U clipping_height;
} IMAGE_DECODE_EXT_STRUCT, *P_IMAGE_DECODE_EXT_STRUCT;

typedef struct {
	INT32U cmd_id;
	INT8S status;
	INT8U special_effect;
	INT8U scaler_input_format;	
	INT8U scaler_output_format;
			
	INT16U input_width;
	INT16U input_height;
	INT16U input_visible_width;
	INT16U input_visible_height;	
	INT32U input_offset_x;
	INT32U input_offset_y;
	void *input_buf1;
//	void *input_buf2;					// This parameter is valid only when separate YUV input format is used
//	void *input_buf3;					// This parameter is valid only when separate YUV input format is used
	
	INT16U output_buffer_width;
	INT16U output_buffer_height;
	INT32U output_width_factor;			// factor = (input_size<<16)/output_size
	INT32U output_height_factor;
	INT32U out_of_boundary_color;
	void *output_buf1;
//	void *output_buf2;					// This parameter is valid only when separate YUV output format is used
//	void *output_buf3;					// This parameter is valid only when separate YUV output format is used
} IMAGE_SCALE_STRUCT, *P_IMAGE_SCALE_STRUCT;

typedef enum
{
  TK_JPEG_DECODE_OK,  
  TK_JPEG_DECODE_FAIL,
  TK_JPEG_DECODE_START,
  TK_JPEG_MAX
} TK_JPEG_STATUS_ENUM;

typedef enum {
    JPEG_FIFO_DISABLE =0x00000000, 
    JPEG_FIFO_ENABLE  =0x00000001, 
    JPEG_FIFO_LINE_16 =0x00000009, 
    JPEG_FIFO_LINE_32 =0x00000001, 
    JPEG_FIFO_LINE_64 =0x00000003, 
    JPEG_FIFO_LINE_128=0x00000005, 
    JPEG_FIFO_LINE_256=0x00000007  
} JPEG_FIFO_LINE_ENUM;

typedef enum {
    SCALER_FIFO_DISABLE	=0x00000000,
    SCALER_FIFO_LINE_16	=0x00001000,
    SCALER_FIFO_LINE_32	=0x00002000,
    SCALER_FIFO_LINE_64	=0x00003000,
    SCALER_FIFO_LINE_128=0x00004000,
    SCALER_FIFO_LINE_256=0x00005000 
} SCALER_FIFO_LINE_ENUM;

typedef enum {    
    SCALER_CTRL_IN_RGB1555		=0x00000000,
	SCALER_CTRL_IN_RGB565		=0x00000001,
	SCALER_CTRL_IN_RGBG			=0x00000002,
	SCALER_CTRL_IN_GRGB			=0x00000003,
	SCALER_CTRL_IN_YUYV			=0x00000004,
	SCALER_CTRL_IN_UYVY			=0x00000005,
	SCALER_CTRL_IN_YUV422		=0x00000008,
	SCALER_CTRL_IN_YUV420		=0x00000009,
	SCALER_CTRL_IN_YUV411		=0x0000000A,
	SCALER_CTRL_IN_YUV444		=0x0000000B,
	SCALER_CTRL_IN_Y_ONLY		=0x0000000C,
	SCALER_CTRL_IN_YUV422V		=0x0000000D,
	SCALER_CTRL_IN_YUV411V		=0x0000000E
} SCALER_IN_FORMAT_ENUM;

typedef enum {    
    SCALER_CTRL_OUT_RGB1555=0x00000000,
    SCALER_CTRL_OUT_RGB565 =0x00000010,
    SCALER_CTRL_OUT_RGBG   =0x00000020,
    SCALER_CTRL_OUT_GRGB   =0x00000030,
    SCALER_CTRL_OUT_YUYV   =0x00000040,
    SCALER_CTRL_OUT_UYVY   =0x00000050,
    SCALER_CTRL_OUT_YUYV32 =0x00000060,
    SCALER_CTRL_OUT_YUYV64 =0x00000070,
    SCALER_CTRL_OUT_YUV422 =0x00000080,
    SCALER_CTRL_OUT_YUV420 =0x00000090,
    SCALER_CTRL_OUT_YUV411 =0x000000A0,
    SCALER_CTRL_OUT_YUV444 =0x000000B0,
    SCALER_CTRL_OUT_Y_ONLY =0x000000C0
} SCALER_OUT_FORMAT_ENUM;

typedef struct st_avi_buf {
	INT32U buf_status;	
	INT32U display_mode;
	INT32U avi_image_width;
	INT32U avi_image_height;
	INT32U avi_framenum_silp;
	INT8U *avibufptr; 	
	struct st_avi_buf *next;
}STAVIBufConfig;

typedef struct {
	INT8U  	display_mode;		//bit0: 0 =>use the file num open this file. 1=> user the file name open this file.
								//bit1:	0 =>default size. 1=> user define size				
								//bit2: 0 =>central show. 1=> user define position
	INT8U   *string_ptr;
	INT32U	filenum;
  	INT8U 	*Avi_Frame_buf1;    //Ìí¼Óbuffer ptr …¢”µ
  	INT8U 	*Avi_Frame_buf2;
	INT16S  MovFileHandle;		//mov file handle, Roy ADD
	INT16S  AudFileHandle;		//audio file handle, Roy ADD
	INT32U	display_width;		//user define size.(use this setting , display_mode must be 1)
	INT32U  display_height;		//user define size.(use this setting , display_mode must be 1)
	INT32S	display_position_x; //tft or tv display area central position x.
	INT32S	display_position_y; //tft or tv display area central position y.
	INT32U  first_action;
}AVI_MESSAGE;

typedef enum {
	AUDIO_BUF_FIRST_FILL = 0,
	AUDIO_BUF_FILL_ALL_AGAIN,
	AUDIO_BUF_EMPTY,
	AUDIO_BUF_FILLING,
	AUDIO_BUF_GETING,
	AUDIO_BUF_OVER
}AUDIO_BUF_STATUS;

typedef struct tk_audiobuf{
	INT32U status;
	INT32U ri;
	INT32U wi;
	INT32U length;
	INT8U  *audiobufptr; 	
	struct tk_audiobuf *next;
}STAVIAUDIOBufConfig;

typedef struct
{
    void *decode_buf;
    void *display_buf;
    INT32U jpeg_file_id;
    INT32U output_width;
    INT32U output_high;
    TK_JPEG_STATUS_ENUM jpeg_status_flag;
} JPEG_PARA_STRUCT;

extern MSG_Q_ID    image_msg_queue_id;
extern void image_task_entry(void * p_arg);
extern INT32S image_task_scale_image(IMAGE_SCALE_STRUCT *para);
extern INT32S image_task_parse_image(IMAGE_HEADER_PARSE_STRUCT *para);
extern INT32S image_task_remove_request(INT32U state_id);

extern void avi_idx1_tabel_free(void);
extern INT32S avi_settime_frame_pos_get(INT32S time);
extern INT32S mjpeg_file_decode_nth_frame(IMAGE_DECODE_STRUCT *img_decode_struct, INT32U nth);
extern INT32S mov_mjpeg_file_decode_nth_frame(IMAGE_DECODE_STRUCT *img_decode_struct, INT32U nth);
/*== Turn Key SYSTEM Task ==*/

typedef enum
{
	STORAGE_DEV_CF=0,
	STORAGE_DEV_NAND,
	STORAGE_DEV_SD,
	STORAGE_DEV_MS,
	STORAGE_DEV_USBH,
	STORAGE_DEV_XD,
	STORAGE_DEV_MAX
}STORAGE_DEV_ID_ENUM;

typedef enum
{
	MEDIA_AUDIO,
	MEDIA_PHOTO_AVI
}MEDIA_TYPE_ENUM;

typedef enum
{
	SCAN_FILE_COMPLETE,
	SCAN_FILE_NOT_COMPLETE
}
SCAN_FILE_STATUS_ENUM;

typedef enum
{
	STORAGE_PLGU_OUT,
	STORAGE_PLGU_IN
}STORAGE_PLUG_STATUS_ENUM;

typedef struct 
{
	INT8U storage_id;
	INT8U plug_status;
} PLUG_STATUS_ST;

#define SOURCE_AD_KEY 0
#define SOURCE_IR_KEY 1

typedef struct
{
	INT8U key_source;
	INT8U key_index;
	INT8U key_type;
} KEY_EVENT_ST;

typedef struct 
{
	INT16S fd;
	INT8U  f_name[256];
	INT8U  f_extname[4];
	INT32U f_size;
	INT16U f_time;
	INT16U f_date;
}STORAGE_FINFO;

typedef enum {
	IT_KEY_SINGLE_TAP = 0,
	IT_KEY_SLIDE,
	IT_KEY_RELEASE
}ITOUCH_KEY_ENUM;

typedef struct
{
  INT8U time_enable;
  INT8U time_hour;
  INT8U time_minute;
}ALARM_FORMAT;

#define ALARM_NUM 2

typedef struct
{
  ALARM_FORMAT power_on_time;
  ALARM_FORMAT power_off_time;
  ALARM_FORMAT alarm_time[ALARM_NUM];
}ALARM_INFO;


typedef struct
{
  INT8U alarm_onoff;
  ALARM_FORMAT alarm_time[7];
}ALARM_PARAMETER;

#define NO_STORAGE 0xFF

extern INT8U newest_storage;
extern OS_EVENT    *SemScanFile;
typedef void (*func_ptr)(void);


/* Turn Key UMI Task */
typedef struct 
{
    STATE_ID_ENUM  msg_id; 
    void* pPara1;
} STTkUMITaskMsg;

typedef struct
{
	INT32U type;
}
STException;
/*
#define AP_QUEUE_MSG_MAX_LEN 48//sizeof(STAudioConfirm)+10

extern MSG_Q_ID    UmiTaskQ;
extern void umi_task_entry(void * p_arg);
extern INT8U  ApQ_para[AP_QUEUE_MSG_MAX_LEN];
extern struct STFileNodeInfo PhotoFNodeInfo;
extern STORAGE_FINFO photo_finfo;
*/
extern void ppu_resource_release_table_init(void);

/* audio dac task */
typedef struct 
{
    STATE_ID_ENUM  msg_id; 
    void* pPara1;
}STAudioDacTaskMsg;

#define SPU_LEFT_CH             14//30
#define SPU_RIGHT_CH            15//31

extern OS_EVENT	*hAudioDacTaskQ;
extern OS_EVENT	*aud_send_q;
extern OS_EVENT *aud_bg_send_q;
extern OS_EVENT *aud_right_q;
/*============== State Declare ==============*/
#define AP_QUEUE_MSG_MAX_LEN	60
extern MSG_Q_ID    ApQ;
extern MSG_Q_ID	Audio_FG_status_Q, Audio_BG_status_Q, MIDI_status_Q;
extern MSG_Q_ID StorageServiceQ;
extern MSG_Q_ID PeripheralTaskQ;
extern OS_EVENT *scaler_task_q;
extern OS_EVENT *DisplayTaskQ;
extern OS_EVENT *USBAPPTaskQ;
extern INT8U  ApQ_para[AP_QUEUE_MSG_MAX_LEN];



/*==Turnkey AVI state ==*/
typedef struct
{
	INT16S	vedhandle;
	INT16S	audhandle;
	INT16S	status;
}MJPEG_HEADER_PARSE_STRUCT;

// added by Bruce, 2009/02/19
//#ifdef _DPF_PROJECT
#define C_AVI_MOV_DEFAULT		0x0000
#define C_AVI_MODE_PLAY			0x0001
#define C_MOV_MODE_PLAY			0x0002
//#endif
typedef struct 
{
    INT32U result_type;
    INT32S result;
} ST_APQ_HANDLE;

extern void audio_confirm_handler(STAudioConfirm *aud_con);
//extern EXIT_FLAG_ENUM apq_sys_msg_handler(INT32U msg_id);

// message q id define
extern MSG_Q_ID	fs_msg_q_id;
extern MSG_Q_ID	fs_scan_msg_q_id;
extern MSG_Q_ID	ap_msg_q_id;
extern OS_EVENT *image_task_fs_queue_a, *image_task_fs_queue_b;
//-------------------------------------------------------------------------------------------------
//ap_graphic
//extern  char *dpf_str_array[LCD_MAX][STR_MAX];
typedef struct
{
	 INT16U start_x;
	 INT16U start_y;
	 INT16U frame_width;//Only "1","2"width can use,"0" mean no outline
	 INT16U front_color;
	 INT16U frame_color;
}GraphicFrameChar;

extern INT32U cl1_graphic_init(GraphicBitmap *bitmap);
extern void cl1_graphic_finish(GraphicBitmap *bitmap);
extern void cl1_graphic_strings_draw(GraphicBitmap *bitmap,char *strings,GraphicCoordinates *coordinates,INT8U language,INT8U font_type);
extern void cl1_graphic_char_draw(GraphicBitmap *bitmap,unsigned short character,GraphicCoordinates *coordinates,INT8U language,INT8U font_type);
extern void cl1_graphic_line_draw(GraphicBitmap *bitmap,GraphicLineAttirbute *line);
extern void cl1_graphic_rectangle_draw(GraphicBitmap *bitmap,GraphicRectangleAttirbute *rectangle);
extern INT32U cl1_graphic_font_get_width(INT16U character_code,INT8U language,INT8U font_type);
extern INT32U cl1_graphic_font_get_height(INT16U character_code,INT8U language,INT8U font_type);
extern INT32U cl1_graphic_font_line_bytes_get(INT16U character_code,INT8U language,INT8U font_type);
extern void cl1_graphic_frame_string_draw(GraphicBitmap *bitmap,char *strings,INT8U language,INT8U font_type,GraphicFrameChar *frame);
extern void cl1_graphic_frame_char_draw(GraphicBitmap *bitmap,	INT16U character,INT8U language,INT8U font_type,GraphicFrameChar *frame);

extern void cl1_graphic_bitmap_front_color_set(GraphicBitmap *bitmap,INT16U color);
extern void cl1_graphic_bitmap_background_color_set(GraphicBitmap *bitmap,INT16U color);
typedef struct GUI_FONT_PROP {
  INT16U First;                                /* first character               */
  INT16U Last;                                 /* last character                */
  INT16U Index;            /* address of first character    */
  const struct GUI_FONT_PROP  * pNext;        /* pointer to next */
} GUI_FONT_PROP;
// Pls added other country language here if we need.
#if _LOAD_FONT_FROM_NVRAM == 0

#if _DPF_SUPPORT_ENGLISH == 1

#if _FONT_Times_New_Roman_32 == 1
extern GUI_FLASH const FontTableAttribute GUI_FontHZTimes_New_Roman_32_CharInfo[];
#define FONT_TIMES_NEW_ROMAN_32_HEIGHT (36)
#endif
#if _FONT_Arail_8 == 1 
extern GUI_FLASH const FontTableAttribute GUI_FontHZArial_11_CharInfo[] ;
#define FONT_ARAIL_8_HEIGHT  (14)
#endif

#if _FONT_Arail_12 == 1
extern GUI_FLASH const FontTableAttribute GUI_FontHZArial_16_CharInfo[] ;
#define FONT_ARAIL_12_HEIGHT  (18)
#endif

#if _FONT_Arail_16 == 1
extern GUI_FLASH const FontTableAttribute GUI_FontHZArial_21_CharInfo[] ;
#define FONT_ARAIL_16_HEIGHT  (24)
#endif	

#if _FONT_Arail_20 == 1
extern GUI_FLASH const FontTableAttribute GUI_FontHZArial_27_CharInfo[] ;
#define FONT_ARAIL_20_HEIGHT  (32)
#endif

#if _FONT_TimesNewRoman_72 == 1
extern GUI_FLASH const FontTableAttribute GUI_FontHZTimes_New_Roman_96_CharInfo[];
#endif
#endif

#endif

#if _DPF_SUPPORT_SCHINESE == 1    //LCD_SCH,    //Simple Chinese

#if _LOAD_SCHINESE_FONT_FROM_NVRAM == 1
extern INT16U cl1_GBK2312_offset_get(INT16U code);
#endif


#if _LOAD_SCHINESE_FONT_FROM_NVRAM == 0
#if _FONT_SIMSUN_8   ==  1
extern GUI_FLASH const FontTableAttribute GUI_FontHZSimSun_13_CharInfo[] ;
#endif

#if _FONT_SIMSUN_12   ==  1
extern GUI_FLASH const FontTableAttribute GUI_FontHZSimSun_16_CharInfo[] ;
#endif

#if _FONT_SIMSUN_16   ==  1
extern GUI_FLASH const FontTableAttribute GUI_FontHZSimSun_24_CharInfo[] ;
#endif

#if _FONT_SIMSUN_20 == 1
extern GUI_FLASH const FontTableAttribute GUI_FontHZKaiTi_29_CharInfo[] ;
#endif

#endif
#endif

// current storage
/*	//wwj mark
extern void umi_current_storage_set(INT8U current_storage);
extern INT8U umi_current_storage_get(void);
*/

// avi packer api
extern int Avi_Rec(INT16S file_handle,	unsigned long _fs, unsigned long _scale, unsigned long _rate,
			unsigned short _width,	unsigned short _height,	unsigned char _prio);
extern void Avi_RecStop(void);

// MutilMedia Codec //////////////////////////////////////////////////////////////////////////////////////////

/* define task pority use  */
#define DAC_PRIORITY            0
#define MIDI_DEC_PRIORITY       2
#define AUD_DEC_HIGH_PRIORITY   4
#define AUD_DEC_PRIORITY        30//28
#define AUD_DEC_BG_PRIORITY	    6
#define FS_PRIORITY	            8
#define IMAGE_PRIORITY          10
#define AVI_DEC_PRIORITY  	    12

//video codec
#define JPEG_ENC_PRIORITY       14
#define USB_CAM_PRIORITY            15  //add by erichan for usb cam
#define AUD_ENC_PRIORITY        16
#define STORAGE_SERVICE_PRIORITY    17
#define SCALER_PRIORITY	        18
#define STATE_HANDLING_PRIORITY	    19
#define AVI_ENC_PRIORITY        20
#define AUD_REC_ENC_PRIORITY        21   // dominant add for audio record
#define AVI_PACKER0_PRIORITY    22
#define AVI_PACKER1_PRIORITY    24
#define AVI_PACKER_PRIORITY	    26

#define VIDEO_PASER_PRIORITY    16
#define AUDIO_DECODE_PRIORITY   18
#define VIDEO_DECODE_PRIORITY   20
#define VID_DEC_STATE_PRIORITY  22


#define DISPLAY_TASK_PRIORITY	31//29//21//19
#define PERIPHERAL_HANDLING_PRIORITY	27

#define STORAGE_SERVICE_PRIORITY2		32//31
#define USB_DEVICE_PRIORITY		37//36

#define TSK_PRI_VID_PARSER		33//32
#define TSK_PRI_VID_AUD_DEC		34//33
#define TSK_PRI_VID_VID_DEC		35//34
#define TSK_PRI_VID_VID_STATE	36//35

#define TSK_PRI_FILE_SRV        28//37
#define TSK_PRI_FILE_SCAN       29//38
//===============     Timer_C Used ID     ===============
#define PERIPHERAL_KEY_TIMER_ID					0
#define STORAGE_SERVICE_MOUNT_TIMER_ID			1
#define STORAGE_SERVICE_FREESIZE_TIMER_ID		2
#define VIDEO_RECORD_CYCLE_TIMER_ID				3
#define AD_DETECT_TIMER_ID						4
#define SCROLL_BAR_TIMER_ID						5
#define VIDEO_PREVIEW_TIMER_ID					6

//===============     Key Mode     ===============
#define DISABLE_KEY		0x0
#define GENERAL_KEY		0x1
#define USBD_DETECT		0x2

//===============     Type for draw string     ===============
#define YUYV_DRAW		0x0
#define YUV420_DRAW		0x1
#define RGB565_DRAW		0x2



//========================= Media Format Defintion ============================
// including Audio, Image, Video
//=============================================================================
typedef enum
{
		AUD_AUTOFORMAT=0,
		MIDI,
		WMA,
		MP3,
		WAV,
		A1800,
		S880,
		A6400,
		A1600, 
		IMA_ADPCM,
		MICROSOFT_ADPCM
} AUDIO_FORMAT;

typedef enum
{
		IMG_AUTOFORMAT=0,
		JPEG,
		JPEG_P,		// JPEG Progressive
		MJPEG_S,	// Single JPEG from M-JPEG video
		GIF,
		BMP
} IMAGE_FORMAT;

typedef enum
{
		VID_AUTOFORMAT=0,
		MJPEG,
		MPEG4
} VIDEO_FORMAT;

//====================== Media Information Defintion ==========================
// including Audio, Image, Video
//=============================================================================
typedef struct {
		AUDIO_FORMAT	Format;
		char*		SubFormat;
		
		INT32U		DataRate;				// unit: bit-per-second
		INT32U		SampleRate;				// unit: Hz
		INT8U		Channel;				// if 1, Mono, if 2 Stereo
		INT16U		Duration;				// unit: second
		INT16U		FrameSize;				// unit: sample per single frame

		INT32U		FileSize;				// unit: byte
		char		*FileDate;				// string pointer
		char		*FileName;				// file name		
} AUDIO_INFO;

typedef struct {
		IMAGE_FORMAT	Format;
		char*		SubFormat;
		
		INT32U		Width;					// unit: pixel
		INT32U		Height;					// unit: pixel
		INT8U		Color;					// unit: color number

		INT32U		FileSize;				// unit: byte
		char		*FileDate;				// string pointer
		char		*FileName;				// file name pointer

		INT8U		Orientation;			// Portrait or Landscape
		char		*PhotoDate;				// string pointer
		INT8U		ExposureTime;			// Exporsure Time
		INT8U		FNumber;				// F Number
		INT16U		ISOSpeed;				// ISO Speed Ratings		
} IMAGE_INFO;

typedef struct {
		AUDIO_FORMAT	AudFormat;
		char		AudSubFormat[6];
		
		INT32U		AudBitRate;				// unit: bit-per-second
		INT32U		AudSampleRate;			// unit: Hz
		INT8U		AudChannel;				// if 1, Mono, if 2 Stereo
		INT16U		AudFrameSize;			// unit: sample per single frame

		VIDEO_FORMAT	VidFormat;
		char		VidSubFormat[4];
		INT8U		VidFrameRate;			// unit: FPS
		INT32U		Width;					// unit: pixel
		INT32U		Height;					// unit: pixel

		INT32U		TotalDuration;			// unit: second
		//INT32U	FileSize;				// unit: byte
		//char		*FileDate;				// string pointer
		//char		*FileName;				// file name
} VIDEO_INFO;

#define DISPLAY_TV              		0
#define DISPLAY_TFT						1
#define	DISPLAY_DEVICE				    DISPLAY_TV

#define	QVGA_MODE			      		0
#define VGA_MODE						1
#define D1_MODE							2
#define TFT_320x240_MODE				3
#define TFT_800x480_MODE				4
#define TV_TFT_MODE						VGA_MODE

//IMAGE_OUTPUT_FORMAT_RGB565
//IMAGE_OUTPUT_FORMAT_RGBG, IMAGE_OUTPUT_FORMAT_GRGB
//IMAGE_OUTPUT_FORMAT_UYVY, IMAGE_OUTPUT_FORMAT_YUYV
#define DISPLAY_OUTPUT_FORMAT			IMAGE_OUTPUT_FORMAT_YUYV
#define VIDEO_DISPALY_WITH_PPU			0 	//0; disable, 1;enable
#define DISPLAY_PPU_WIDTH				640
#define DISPLAY_PPU_HEIGHT				480
#define PPU_FRAME_NO					2

// tft io pin setting
#define C_TFT_DATA						IO_C2
#define C_TFT_CLK						IO_C1
#define C_TFT_CS						IO_C3

#define	PPU_YUYV_TYPE3					(3<<20)
#define	PPU_YUYV_TYPE2					(2<<20)
#define	PPU_YUYV_TYPE1					(1<<20)
#define	PPU_YUYV_TYPE0					(0<<20)

#define	PPU_RGBG_TYPE3					(3<<20)
#define	PPU_RGBG_TYPE2					(2<<20)
#define	PPU_RGBG_TYPE1					(1<<20)
#define	PPU_RGBG_TYPE0					(0<<20)

#define PPU_LB							(1<<19)
#define	PPU_YUYV_MODE					(1<<10)
#define	PPU_RGBG_MODE			        (0<<10)

#define TFT_SIZE_800X480                (5<<16)
#define TFT_SIZE_720x480				(4<<16)
#define TFT_SIZE_640X480                (1<<16)
#define TFT_SIZE_320X240                (0<<16)

#define	PPU_YUYV_RGBG_FORMAT_MODE		(1<<8)
#define	PPU_RGB565_MODE			        (0<<8)

#define	PPU_FRAME_BASE_MODE			    (1<<7)
#define	PPU_VGA_NONINTL_MODE			(0<<5)

#define	PPU_VGA_MODE					(1<<4)
#define	PPU_QVGA_MODE					(0<<4)


//=============================================================================
//======================== Media Argument Defintion ===========================
// including Audio, Image, Video
//=============================================================================
typedef enum
{
		PPUAUTOFORMAT=0,
		BITMAP_YUYV,
		BITMAP_RGRB,
		CHARATER64_YUYV,
		CHARATER64_RGRB,
		CHARATER32_YUYV,
		CHARATER32_RGRB,
		CHARATER16_YUYV,
		CHARATER16_RGRB,
		CHARATER8_YUYV,
		CHARATER8_RGRB
} PPU_FORMAT;

typedef enum
{
		FIT_OUTPUT_SIZE=0,//Fit to output_buffer_width and output_buffer_height
		MAINTAIN_IMAGE_RATIO_1,//Maintain ratio and fit to output_buffer_width or output_buffer_height
		MAINTAIN_IMAGE_RATIO_2//Same as MAINTAIN_IMAGE_RATIO_1 but without scale up
}SCALER_OUTPUT_RATIO;

typedef enum
{
		SCALER_INPUT_FORMAT_RGB1555=0,
		SCALER_INPUT_FORMAT_RGB565,
		SCALER_INPUT_FORMAT_RGBG,
		SCALER_INPUT_FORMAT_GRGB,
		SCALER_INPUT_FORMAT_YUYV,
     	SCALER_INPUT_FORMAT_UYVY			
} IMAGE_INPUT_FORMAT;

typedef enum
{
		IMAGE_OUTPUT_FORMAT_RGB1555=0,
		IMAGE_OUTPUT_FORMAT_RGB565,
		IMAGE_OUTPUT_FORMAT_RGBG,
		IMAGE_OUTPUT_FORMAT_GRGB,
		IMAGE_OUTPUT_FORMAT_YUYV,
     	IMAGE_OUTPUT_FORMAT_UYVY,	
	    IMAGE_OUTPUT_FORMAT_YUYV8X32,		
        IMAGE_OUTPUT_FORMAT_YUYV8X64,		
        IMAGE_OUTPUT_FORMAT_YUYV16X32,		
        IMAGE_OUTPUT_FORMAT_YUYV16X64,
        IMAGE_OUTPUT_FORMAT_YUYV32X32,	
        IMAGE_OUTPUT_FORMAT_YUYV64X64,
        IMAGE_OUTPUT_FORMAT_YUV422,
        IMAGE_OUTPUT_FORMAT_YUV420,
        IMAGE_OUTPUT_FORMAT_YUV411,
        IMAGE_OUTPUT_FORMAT_YUV444,        
        IMAGE_OUTPUT_FORMAT_Y_ONLY        
} IMAGE_OUTPUT_FORMAT;

typedef struct {
		INT8U		Main_Channel;			//0: SPU Channels
											//1: DAC Channel A+B (can't assign MIDI)
											//   only 0 and 1 are available now
		INT8U		L_R_Channel;			//0: Invalid Setting				
											//1: Left Channel only
											//2: Right Channel only
											//3: Left + Right Channel
											//   only Left + Right Channel are avialable now
	    BOOLEAN   	mute;
	    INT8U     	volume;
	    INT8U		midi_index;				// MIDI index in *.idi
} AUDIO_ARGUMENT;

typedef struct {
		INT8U       *OutputBufPtr;     
		INT16U		OutputBufWidth;
		INT16U		OutputBufHeight;
		INT16U		OutputWidth;
		INT16U		OutputHeight;
		INT32U      OutBoundaryColor;
		SCALER_OUTPUT_RATIO ScalerOutputRatio;
		IMAGE_OUTPUT_FORMAT	OutputFormat;

} IMAGE_ARGUMENT;

typedef struct
{
		INT8U *yaddr;
		INT8U *uaddr;
		INT8U *vaddr;
} IMAGE_ENCODE_PTR;

typedef enum
{
		IMAGE_ENCODE_INPUT_FORMAT_YUYV=0,
        IMAGE_ENCODE_INPUT_FORMAT_YUV_SEPARATE
} IMAGE_ENCODE_INPUT_FORMAT;

typedef enum
{
		IMAGE_ENCODE_OUTPUT_FORMAT_YUV422=0,
		IMAGE_ENCODE_OUTPUT_FORMAT_YUV420
} IMAGE_ENCODE_OUTPUT_FORMAT;

typedef enum
{
		IMAGE_ENCODE_ONCE_READ=0,
		IMAGE_ENCODE_BLOCK_READ,
		IMAGE_ENCODE_BLOCK_READ_WRITE
} IMAGE_ENCODE_MODE;

typedef struct {
        INT8U        *InputBufPtr;
        INT8U        *OutputBufPtr;
		INT16U		 InputWidth;
		INT16U		 InputHeight;
	    INT16U       inputvisiblewidth;
	    INT16U       inputvisibleheight;
	    INT32U       outputwidthfactor;			
	    INT32U       outputheightfactor;	
		INT16U		 OutputWidth;
		INT16U		 OutputHeight;
	    INT32U       inputoffsetx;
	    INT32U       inputoffsety;
		INT16U		 OutputBufWidth;
		INT16U		 OutputBufHeight;
		INT32U       OutBoundaryColor;
		IMAGE_INPUT_FORMAT	InputFormat;
		IMAGE_OUTPUT_FORMAT	OutputFormat;
} IMAGE_SCALE_ARGUMENT;

typedef struct {
        INT8U        *OutputBufPtr;
        INT8U        BlockLength;             //BlockLength=16,32..
        INT8U        UseDisk;
		INT16U		 InputWidth;
		INT16U		 InputHeight;
		INT16S       FileHandle;
		INT32U       quantizationquality;    //quality=50,70,80,85,90,95,100
		IMAGE_ENCODE_MODE EncodeMode;        // 0=read once, 1=block read
		IMAGE_ENCODE_PTR InputBufPtr;
		IMAGE_ENCODE_INPUT_FORMAT	InputFormat;
		IMAGE_ENCODE_OUTPUT_FORMAT	OutputFormat;
		IMAGE_SCALE_ARGUMENT        *scalerinfoptr;
} IMAGE_ENCODE_ARGUMENT;

typedef struct {
		INT8U		bScaler;
		INT8U		bUseDefBuf;			//video decode use user define buffer or not 
		INT8U		*AviDecodeBuf1;		//video decode user define buffer address 
		INT8U		*AviDecodeBuf2;		//video decode user define buffer address
		INT16U		TargetWidth;		//video encode use
		INT16U		TargetHeight;		//video encode use
		INT16U      SensorWidth;        //video encode use
		INT16U      SensorHeight;       //video encode use
		INT16U      DisplayWidth;       
		INT16U      DisplayHeight;      
		INT16U		DisplayBufferWidth;
		INT16U 		DisplayBufferHeight;
		INT32U      VidFrameRate;       //for avi encode only
		INT32U      AudSampleRate;      //for avi encode only
		IMAGE_OUTPUT_FORMAT	OutputFormat;
} VIDEO_ARGUMENT;


typedef enum
{
  STATE_NORMAL_MODE=0,
  STATE_RESUME_MODE,
  STATE_NULL_ENTRY_MODE,
  STATE_MODE_MAX
} STATE_ENTRY_MODE_ENUM;

typedef enum {
	EXIT_RESUME=0,  /*no exit*/
    EXIT_SUSPEND,  /* prepare to entry state suspend mode */   /* after exit, the next state is decision by UMI Task*/
    EXIT_PAUSE_SUSPEND,  /* no release resource suspend, like as suspend for storage INT */  /* after exit, the next state is decision by UMI Task*/
    EXIT_STG_SUSPEND,  /* no release resource suspend, only for storage INT */  /* after exit, the next state is decision by UMI Task*/
    EXIT_USB_SUSPEND,  /* no release resource suspend, only for storage INT */  /* after exit, the next state is decision by UMI Task*/
    EXIT_TO_SHORTCUT,
    EXIT_ALARM_SUSPEND,
    EXIT_BREAK  /* after exit, the next state is define by current state*/
}EXIT_FLAG_ENUM;

typedef enum
{
    STACK_PUSH_OK = 0,
    STACK_POP_OK = STACK_PUSH_OK,
    STACK_PUSH_FAIL,
    STACK_POP_FAIL = STACK_PUSH_FAIL,
    STACK_MONITOR_FAIL
} STACK_STATUS_ENUM;

typedef enum
{
    KeyUnlock,
    KeyLock,
    KeyLock_Max,
    KeyLock_Error=KeyLock_Max
} KEY_LOCK_ENUM;

typedef struct
{
    STATE_ENTRY_MODE_ENUM state_entry_mode;
    void* state_entry_para;
} STATE_ENTRY_PARA;


//=============================================================================
//======================== Media Status Defintion ============================
// including Audio, Image, Video
//=============================================================================
typedef enum
{
		START_OK=0,
		RESOURCE_NO_FOUND_ERROR,
		RESOURCE_READ_ERROR,
		RESOURCE_WRITE_ERROR,
		CHANNEL_ASSIGN_ERROR,
		REENTRY_ERROR,
		AUDIO_ALGORITHM_NO_FOUND_ERROR,
		CODEC_START_STATUS_ERROR_MAX
} CODEC_START_STATUS;

typedef enum
{
		AUDIO_CODEC_PROCESSING=0,					// Decoding or Encoding
		AUDIO_CODEC_PROCESS_END,					// Decoded or Encoded End
		AUDIO_CODEC_BREAK_OFF,						// Due to unexpended card-plug-in-out
		AUDIO_CODEC_PROCESS_PAUSED,
		AUDIO_CODEC_STATUS_MAX
} AUDIO_CODEC_STATUS;

typedef enum
{
		IMAGE_CODEC_DECODE_END=0,                // Decoded or Encoded End
		IMAGE_CODEC_DECODE_FAIL,
		IMAGE_CODEC_DECODING,					  // Decoding or Encoding				
		IMAGE_CODEC_BREAK_OFF,					// Due to unexpended card-plug-in-out
		IMAGE_CODEC_STATUS_MAX
} IMAGE_CODEC_STATUS;

typedef enum
{
		VIDEO_CODEC_PROCESSING=0,					// Decoding or Encoding
		VIDEO_CODEC_PROCESS_PAUSE,
		VIDEO_CODEC_PROCESS_END,					// Decoded or Encoded End
		VIDEO_CODEC_BREAK_OFF,					// Due to unexpended card-plug-in-out
		VIDEO_CODEC_RESOURCE_NO_FOUND,		
		VIDEO_CODEC_CHANNEL_ASSIGN_ERROR,
		VIDEO_CODEC_RESOURCE_READ_ERROR,
		VIDEO_CODEC_RESOURCE_WRITE_ERROR,
		VIDEO_CODEC_PASER_HEADER_FAIL,
		VIDEO_CODEC_STATUS_ERR,
		VIDEO_CODEC_STATUS_OK, 
		VIDEO_CODEC_STATUS_MAX
} VIDEO_CODEC_STATUS;

//=============================================================================
//========================= Media Source Defintion ============================
// including Audio, Image, Video
//=============================================================================
typedef enum
{
		SOURCE_TYPE_FS=0,
		SOURCE_TYPE_SDRAM,
		SOURCE_TYPE_NVRAM,
		SOURCE_TYPE_USER_DEFINE,
		SOURCE_TYPE_FS_RESOURCE_IN_FILE,	// added by Bruce, 2010/01/22
		SOURCE_TYPE_MAX
} SOURCE_TYPE;


typedef struct {
		SOURCE_TYPE type;					//0: GP FAT16/32 File System by File SYSTEM 
											//1: Directly from Memory Mapping (SDRAM)
											//2: Directly from Memory Mapping (NVRAM)
											//3: User Defined defined by call out function:audio_encoded_data_read								
		
		struct User							//Source File handle and memory address
		{
				INT16S		FileHandle;		//File Number by File System or user Define	
				INT32S      temp;			//Reserve for special use 
				INT8U       *memptr;		//Memory start address						
		}type_ID;
		
		union SourceFormat					//Source File Format
		{
				AUDIO_FORMAT	AudioFormat;		//if NULL,auto detect
				IMAGE_FORMAT	ImageFormat;		//if NULL,auto detect
				VIDEO_FORMAT	VideoFormat;		//if NULL,auto detect
		}Format;
} MEDIA_SOURCE;

//=============================================================================
typedef enum
{
		SND_OFFSET_TYPE_NONE = 0,
		SND_OFFSET_TYPE_HEAD,
		SND_OFFSET_TYPE_SIZE,
		SND_OFFSET_TYPE_TIME,
		SND_OFFSET_TYPE_MAX
} SndOffset_TYPE;

typedef struct {
		INT32U		DataRate;				// unit: bit-per-second
		INT32U		SampleRate;				// unit: Hz
		INT8U		Channel;				// if 1, Mono, if 2 Stereo

		INT16U		Speed;
		INT16U		Pitch;
		INT32U		Offset;					// play offset
		SndOffset_TYPE Offsettype;
		
		AUDIO_FORMAT AudioFormat;

		SOURCE_TYPE Srctype;		//0: GP FAT16/32 File System by File SYSTEM 
											//1: Directly from Memory Mapping (SDRAM)
											//2: Directly from Memory Mapping (NVRAM)
											//3: User Defined defined by call out function:audio_decode_data_fetch								
} SND_INFO;

typedef struct {
		INT32U		Offset;					// play offset
		SndOffset_TYPE Offsettype;
		
		AUDIO_FORMAT AudioFormat;

} SACM_CTRL;
//=============================================================================
//========================= Platform Initialization ===========================
//
// Task Create : Audio, DAC, Image, File, AP(Demo Code) Task
// Global Variable Initialization
// Component Initialization : File system, Memory Manager,
// Driver Initialization: DMA Manager, SDC, USB Host
//=============================================================================
extern void platform_entrance(void* free_memory);
extern void platform_exit(void);

// Audio Decode ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern INT8U* g_audio_data_start;	// added by Bruce, 2008/10/28
extern INT32S g_auido_data_offset;	// added by Bruce, 2008/10/28
extern INT32U Audio_Decoder_Status_0;	// added by Bruce, 2008/11/20
extern INT32U Audio_Decoder_Status_1;	// added by Bruce, 2008/11/20
extern INT32U Audio_Decoder_Status_2;	// added by Bruce, 2008/11/20
extern INT32U g_delta_time_start;	// added by Bruce, 2008/11/14
extern INT32U g_delta_time_end;		// added by Bruce, 2008/11/14
extern void	audio_decode_end(INT32U audio_decoder_num);	// modified by Bruce, 2008/11/20
extern INT32S audio_encoded_data_read(INT32U buf_addr, INT32U buf_size);	// added by Bruce, 2008/10/27
extern void audio_decode_entrance(void);						// peripheral setting, global variable initial, memory allocation
extern void audio_decode_exit(void);							// peripheral unset, global variable reset, memory free
extern void audio_decode_rampup(void);							// Ramp to Middle Level 0x80(Standby)
extern void audio_decode_rampdown(void);						// Ramp to Zero Level 0x0(Power Saving)
extern CODEC_START_STATUS audio_decode_start(AUDIO_ARGUMENT arg, MEDIA_SOURCE src);
extern void audio_decode_stop(AUDIO_ARGUMENT arg);				// resource should be completely released if card accendentially plug out
extern void audio_decode_pause(AUDIO_ARGUMENT arg);
extern void audio_decode_resume(AUDIO_ARGUMENT arg);	
extern void audio_decode_volume_set(AUDIO_ARGUMENT *arg, int volume);
extern void audio_decode_mute(AUDIO_ARGUMENT *arg);
extern void audio_decode_unmute(AUDIO_ARGUMENT *arg);
extern AUDIO_CODEC_STATUS audio_decode_status(AUDIO_ARGUMENT arg);
extern void audio_decode_get_info(AUDIO_ARGUMENT arg, MEDIA_SOURCE src, AUDIO_INFO *audio_info);
// Audio Codec SACM Interface//////////////////////////////////////////////////////////////////////////////////////////////////////
extern INT16U GetSoundLibVersion(void);
extern void Snd_SetVolume(INT8U CodecType,INT8U vol );
extern INT8U Snd_GetVolume( INT8U CodecType);
extern INT16S Snd_Stop( INT8U CodecType );
extern INT16S Snd_Pause( INT8U CodecType );
extern INT16S Snd_Resume( INT8U CodecType );
extern INT16U Snd_GetStatus(INT8U CodecType);
//extern void Snd_Init( int *SACMstack, int SACMstacksize );
extern INT16S Snd_Init(void);
extern INT16S Snd_Play(SND_INFO *sndinfo);	//added by Dexter.Ming 081009
extern INT32S Snd_GetData(INT32U buf_adr,INT32U buf_size);
extern INT32U Snd_GetLen(void);	//read file length
extern SACM_CTRL G_SACM_Ctrl;
extern INT16S G_Snd_UserFd;
extern SND_INFO G_snd_info;
extern INT32U GetSndTotalTime(INT16S	filehandle,AUDIO_FORMAT audioformat);
extern INT32U GetSndCurTime(INT16S	filehandle,AUDIO_FORMAT audioformat);

extern INT32S audio_effect_play(INT32U effect_type); //wwj add

// Audio Encode ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void audio_encode_entrance(void);
void audio_encode_exit(void);
CODEC_START_STATUS audio_encode_start(MEDIA_SOURCE src, INT16U SampleRate, INT32U BitRate);
void audio_encode_stop(void);
AUDIO_CODEC_STATUS audio_encode_status(void);
CODEC_START_STATUS audio_encode_set_downsample(INT8U bEnable, INT8U DownSampleFactor);
INT32U audio_encode_data_write(INT8U bHeader, INT32U buffer_addr, INT32U cbLen);
extern void audio_encode_audio_format_set(INT32U aud_format);
extern void audio_encode_mute_set(INT8U aud_mute);
extern INT8U audio_encode_mute_get(void);
extern INT32U audio_record_get_status(void);
extern INT32U wave_encode_once_get_file_length(void);

// Audio Codec SACM Interface//////////////////////////////////////////////////////////////////////////////////////////////////////
extern INT16U GetSoundLibVersion(void);
extern void Snd_SetVolume(INT8U CodecType,INT8U vol );
extern INT8U Snd_GetVolume( INT8U CodecType);
extern INT16S Snd_Stop( INT8U CodecType );
extern INT16S Snd_Pause( INT8U CodecType );
extern INT16S Snd_Resume( INT8U CodecType );
extern INT16U Snd_GetStatus(INT8U CodecType);
extern INT16S Snd_Init(void);
extern INT16S Snd_Play(SND_INFO *sndinfo);	
extern INT32S Snd_GetData(INT32U buf_adr,INT32U buf_size);
extern INT32U Snd_GetLen(void);		//read file length
extern INT32S Snd_Lseek(INT32S offset,INT16S fromwhere);
extern SACM_CTRL G_SACM_Ctrl;
extern INT16S G_Snd_UserFd;

//Image Decode /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern void image_decode_entrance(void);						// peripheral setting, global variable initial, memory allocation
extern void image_decode_exit(void);							// peripheral unset, global variable reset, memory free
extern CODEC_START_STATUS image_decode_Info(IMAGE_INFO *info,MEDIA_SOURCE src);							
extern CODEC_START_STATUS image_decode_start(IMAGE_ARGUMENT arg,MEDIA_SOURCE src);						
extern void image_decode_stop(void); 
extern IMAGE_CODEC_STATUS image_decode_status(void);
extern CODEC_START_STATUS image_scale_start(IMAGE_SCALE_ARGUMENT arg);
extern CODEC_START_STATUS image_block_encode_scale(IMAGE_SCALE_ARGUMENT *scaler_arg,IMAGE_ENCODE_PTR *output_ptr);
extern IMAGE_CODEC_STATUS image_scale_status(void);
extern void image_decode_end(void);		//call-back

//Image Encode /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern void image_encode_entrance(void);
extern void image_encode_exit(void);
extern INT32U image_encode_start(IMAGE_ENCODE_ARGUMENT arg);
extern void image_encode_stop(void);
extern IMAGE_CODEC_STATUS image_encode_status(void);
extern void image_encode_end(INT32U encode_size);									//call-back
extern CODEC_START_STATUS image_encoded_block_read(IMAGE_SCALE_ARGUMENT *scaler_info,IMAGE_ENCODE_PTR *encode_ptr);	//call-back
extern void user_defined_image_decode_entrance(INT32U TV_TFT);
extern void image_sensor_start(void);

// Video decode /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern void video_decode_entrance(void);
extern void video_decode_exit(void);
extern void avi_audio_decode_rampup(void);
extern void avi_audio_decode_rampdown(void);
extern VIDEO_CODEC_STATUS video_decode_Info(VIDEO_INFO *info);
extern VIDEO_CODEC_STATUS video_decode_paser_header(VIDEO_INFO *video_info, VIDEO_ARGUMENT arg, MEDIA_SOURCE src);
extern CODEC_START_STATUS video_decode_start(VIDEO_ARGUMENT arg, MEDIA_SOURCE src);
extern void video_decode_stop(void);
extern void video_decode_pause(void);
extern void video_decode_resume(void);
extern void audio_decode_volume(INT8U volume);
extern VIDEO_CODEC_STATUS video_decode_status(void);
extern void video_decode_end(void);		//call-back
extern void video_decode_FrameReady(INT8U *FrameBufPtr);
extern VIDEO_CODEC_STATUS video_decode_set_play_time(INT32S SecTime);
extern VIDEO_CODEC_STATUS video_decode_set_play_speed(FP32 speed);
extern VIDEO_CODEC_STATUS video_decode_set_reserve_play(INT32U enable);
extern INT32U video_decode_get_current_time(void);
extern VIDEO_CODEC_STATUS video_decode_get_nth_video_frame(VIDEO_ARGUMENT arg, MEDIA_SOURCE src, INT32U nth_frame);

// Video encode /////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define DYNAMIC_QVALUE              1   // dominant add

extern void video_encode_entrance(void);
extern void video_encode_exit(void);
extern CODEC_START_STATUS video_encode_preview_start(VIDEO_ARGUMENT arg);
extern CODEC_START_STATUS video_encode_preview_stop(void);
extern CODEC_START_STATUS video_encode_start(MEDIA_SOURCE src);
extern CODEC_START_STATUS video_encode_stop(void);
extern CODEC_START_STATUS video_encode_Info(VIDEO_INFO *info);
extern VIDEO_CODEC_STATUS video_encode_status(void);
extern INT32U avi_encode_get_csi_frame(void);
extern CODEC_START_STATUS video_encode_auto_switch_csi_frame(void);
extern CODEC_START_STATUS video_encode_auto_switch_csi_fifo_end(void);
extern CODEC_START_STATUS video_encode_auto_switch_csi_fifo_frame_end(void);
extern CODEC_START_STATUS video_encode_set_zoom_scaler(FP32 zoom_ratio);
extern INT32U video_encode_sensor_start(INT32U csi_frame1, INT32U csi_frame2);
extern INT32U video_encode_sensor_stop(void);
extern INT32S video_encode_display_frame_ready(INT32U frame_buffer);
extern INT32S  video_encode_frame_ready(void *workmem, unsigned long fourcc, long cbLen, const void *ptr, int nSamples, int ChunkFlag);
extern void video_encode_end(void *workmem);
extern CODEC_START_STATUS video_encode_pause(void);
extern CODEC_START_STATUS video_encode_resume(void);
extern CODEC_START_STATUS video_encode_set_jpeg_quality(INT8U quality_value);
INT32U video_encode_get_jpeg_quality(void);
extern CODEC_START_STATUS video_encode_capture_picture(MEDIA_SOURCE src);
extern CODEC_START_STATUS video_encode_fast_switch_stop_and_start(MEDIA_SOURCE src);
#if AUDIO_SFX_HANDLE
extern INT32U video_encode_audio_sfx(INT16U *PCM_Buf, INT32U cbLen);
#endif
#if VIDEO_SFX_HANDLE
extern INT32U video_encode_video_sfx(INT32U buf_addr, INT32U cbLen); 
#endif
extern INT32S  video_encode_frame_ready(void *workmem, unsigned long fourcc, long cbLen, const void *ptr, int nSamples, int ChunkFlag);
extern void video_encode_end(void *workmem);
extern INT32S g_nand_flash;

// display /////////////////////////////////////////////////////////////////////////////////////////////
extern void video_codec_show_image(INT8U TV_TFT, INT32U BUF,INT32U DISPLAY_MODE ,INT32U SHOW_TYPE);
extern void user_defined_video_codec_entrance(void);
extern void TFT_TD025THED1_Init(void);
extern void *hVC;		//for voice change
extern void *hUpSample;	//for upsample

extern INT16S unlink2(CHAR *filename);
extern void state_usb_entry(void* para1);
extern void usb_cam_entrance(INT16U usetask);
extern INT32S usb_webcam_start(void);
extern void usb_webcam_state(void);
extern INT32S usb_webcam_stop(void);
extern void usb_cam_exit(void);
extern void timer_counter_force_display(INT8U force_en);
extern void timer_counter_force_display_browser(INT8U force_en); //wwj add
extern void ap_peripheral_auto_off_force_disable_set(INT8U auto_off_disable);	//wwj add


typedef struct 
{
    INT16S file_handle;
    INT16S storage_free_size;
    INT32U file_path_addr;
} STOR_SERV_FILEINFO;

typedef enum
{
	STOR_SERV_SEARCH_INIT = 0,
	STOR_SERV_SEARCH_ORIGIN,
	STOR_SERV_SEARCH_PREV,
	STOR_SERV_SEARCH_NEXT,
	STOR_SERV_SEARCH_GIVEN
} STOR_SERV_PLAYSEARCH;

typedef enum
{
	STOR_SERV_IDLE = 0,
	STOR_SERV_OPEN_FAIL,
	STOR_SERV_OPEN_OK,
	STOR_SERV_DECODE_ALL_FAIL,
	STOR_SERV_NO_MEDIA
} STOR_SERV_PLAYSTS;

typedef struct 
{
    INT16S file_handle;
    INT16S play_index;
    INT16U total_file_number;
    INT16U deleted_file_number;
    INT8U file_type;
    INT8U search_type;
    INT8S err_flag;
    INT32U file_size;
    INT32U file_path_addr;
} STOR_SERV_PLAYINFO;

typedef struct{
    INT16U font_color;
    INT16U font_type;
    INT16S pos_x;
    INT16S pos_y;
    INT16U buff_w;
    INT16U buff_h;
    INT16U language;
    INT16U str_idx;
}STRING_INFO;

typedef struct 
{
    INT16U w;
    INT16U h;
    INT32U addr;
} STR_ICON;

extern void cpu_draw_time_osd(TIME_T current_time, INT32U target_buffer, INT8U draw_type, INT8U state);
extern INT32S ap_state_resource_char_draw(INT16U target_char, INT16U *frame_buff, STRING_INFO *str_info, INT8U type, INT8U num_type);
extern INT8U ap_state_config_date_stamp_get(void);
extern INT8U ap_state_config_motion_detect_get(void);
extern INT8U ap_state_config_usb_mode_get(void);
extern INT8U ap_state_config_auto_off_get(void);

typedef struct {
	INT16U icon_w;
	INT16U icon_h;
	INT16U transparent;
	INT16U pos_x;
	INT16U pos_y;
} DISPLAY_ICONSHOW;

typedef struct {
	INT16U idx;
	INT16U pos_x;
	INT16U pos_y;
} DISPLAY_ICONMOVE;

typedef struct {
	INT32U size;
	INT32U tag;
} FREE_SIZE_CHECK;

// For state setting
extern INT16U sub_menu_2[];
extern INT16U sub_menu_3[];
extern INT16U sub_menu_4[];
extern INT16U select_bar_gs[];
extern INT16U background_c[];
extern INT16U select_bar_bb[];
extern INT16U background_b[];
extern INT16U select_bar_gb[];
extern INT16U background_a[];

extern INT16U thumbnail_video_icon[];
extern INT16U thumbnail_cursor_3x3_96x64[];
extern INT16U thumbnail_cursor_3x3_black_96x64[];
extern INT16U thumbnail_lock_icon[];

extern INT16U ui_up[];
extern INT16U ui_left[];
extern INT16U ui_right[];
extern INT16U ui_down[];

extern void task_peripheral_handling_entry(void *para);
extern void state_handling_entry(void *para);
extern void task_display_entry(void *para);
extern void task_storage_service_entry(void *para);
extern void tft_vblank_isr_register(void (*user_isr)(void));
extern void tv_vblank_isr_register(void (*user_isr)(void));
#if C_MOTION_DETECTION == CUSTOM_ON
	extern void motion_detect_isr_register(void (*user_isr)(void));
#endif

extern INT32S vid_dec_entry(void);
extern INT32S vid_dec_get_file_format(INT8S *pdata);
extern INT32S vid_dec_parser_start(INT16S fd, INT32S FileType, INT64U FileSize);
extern void vid_dec_get_size(INT16U *width, INT16U *height);
extern INT32S vid_dec_start(void);
extern INT32S vid_dec_stop(void);
extern INT32S vid_dec_parser_stop(void);
extern INT32S vid_dec_exit(void);
extern INT32S vid_dec_pause(void);
extern INT32S vid_dec_resume(void);
extern INT32S vid_dec_get_status(void);
extern INT32S vid_dec_set_scaler(INT32U scaler_flag, INT32U video_output_format, INT16U image_output_width, INT16U image_output_height, INT16U buffer_output_width, INT16U buffer_output_height);
extern void vid_dec_set_user_define_buffer(INT8U user_flag, INT32U addr0, INT32U addr1);
extern INT32S vid_dec_nth_frame(INT32U nth);

// card detect /////////////////////////////////////////////////////////////////////////////////////////
extern void card_detect_set_cf_callback(void (*cf_callback)(void));
extern void card_detect_set_sdms_callback(void (*sdms_callback)(void));
extern void card_detect_set_usb_h_callback(void (*usb_h_callback)(void));
extern void card_detect_set_usb_d_callback(void (*usb_d_callback)(void));
extern INT8U card_detect_get_plug_status(void);
extern void card_detect_init(INT16U devicetpye);
extern void sdms_detection(void);
extern void cfc_detection(void);
extern INT16U card_detection(INT16U type);

//face detection///////////////////////////////////////////////////////////////////////////////////////
extern INT32S face_detect_start(INT16U input_width, INT16U input_height,INT32U frame_buffer);
extern INT32S face_detect_get_face_coor(INT8U index, INT16U *x_postion, INT16U *y_postion, INT16U *width, INT16U *height);
extern INT32S face_detect_get_eye_coor(INT8U index, INT16U *x_postion, INT16U *y_postion, INT16U *width, INT16U *height);

typedef struct
{
	struct STFileNodeInfo audio;
	INT8S  ext_name[24];//080903
	INT8U  scan_status;
}st_storage_file_node_info;

#define SD_SLOT_ID              0
#define MAX_SLOT_NUMS          	1
extern  st_storage_file_node_info FNodeInfo[MAX_SLOT_NUMS];
extern INT32U audio_present;
extern INT8U storage_sd_upgrade_file_flag_get(void);

extern INT32S ap_peripheral_irkey_message_init(void);
extern INT32S ap_peripheral_irkey_message_set(INT8U key_code, INT32U msg_id);
extern INT32U ap_peripheral_irkey_message_get(INT8U key_code);
extern INT8U ap_state_config_tv_out_get(void);

#if MINI_DVR_BOARD_VERSION == GPL32680_MINI_DVR_CAR_RECORD_V2
#define USB_PHY_SUSPEND 1
#else
#define USB_PHY_SUSPEND 0
#endif

extern INT8U ap_state_config_record_time_get(void);
extern void VdoFramNumsLowBoundReg(unsigned int fix_frame_cnts);
extern unsigned long current_movie_Byte_size_get(void);
extern void AviPacker_Break_Set(INT8U Break1_Work0);
extern INT8U video_encode_frame_rate_get(void);
extern void video_encode_frame_rate_set(INT8U fps);


extern INT32U bkground_del_thread_size_get(void);
extern void bkground_del_disable(INT32U disable1_enable0);
extern INT8S bkground_del_disable_status_get(void);

extern INT16S ap_state_resource_time_stamp_position_x_get(void);
extern INT16S ap_state_resource_time_stamp_position_y_get(void);

#define UVC_STILL_IMAGE_HW_TRIGGER_SUPPORT 0
#define AUDIO_PROGRESS_SUPPORT        1

extern void ap_storage_service_del_thread_mb_set(void); //wwj add

#endif 		// __APPLICATION_H__

