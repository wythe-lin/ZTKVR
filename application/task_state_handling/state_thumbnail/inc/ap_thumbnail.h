#include "state_thumbnail.h"

#define VIDEO_STREAM				0x63643030
#define AUDIO_STREAM				0x62773130

#define THUMBNAIL_ROW_X_AMOUNT		3
#define THUMBNAIL_COLUMN_Y_AMOUNT	3
#define THUMBNAIL_ONE_PAGE_ICON_AMOUNT	(THUMBNAIL_ROW_X_AMOUNT*THUMBNAIL_COLUMN_Y_AMOUNT)

#define THUMBNAIL_CURSOR_HORIZONTAL_WIDTH		3
#define THUMBNAIL_CURSOR_VIRTICAL_WIDTH			3

#define THUMBNAIL_ICON_START_X_POSITION			10//4
#define THUMBNAIL_ICON_START_Y_POSITION			0

#if THUMBNAIL_ROW_X_AMOUNT == 3
	#define THUMBNAIL_ICON_WIDTH		96//100
#endif

#if THUMBNAIL_COLUMN_Y_AMOUNT == 3
	#define THUMBNAIL_ICON_HEIGHT		64//72
#endif	

#define THUMBNAIL_FILE_NAME_AREA_Y 		(THUMBNAIL_ICON_HEIGHT*3 + THUMBNAIL_CURSOR_HORIZONTAL_WIDTH*4)

#define THUMBNAIL_FILE_NAME_COLOR		255
#define THUMBNAIL_FILE_NAME_START_X		170
#define THUMBNAIL_FILE_NAME_START_Y		210

#define THUMBNAIL_ICON_INDEX_COLOR			255
#define THUMBNAIL_ICON_INDEX_X_OFFSET		60
#define THUMBNAIL_ICON_INDEX_Y_OFFSET		40//50

#define THUMBNAIL_VIDEO_ICON_X_OFFSET		10
#define THUMBNAIL_VIDEO_ICON_Y_OFFSET		46//50

#define THUMBNAIL_LOCKED_ICON_X_OFFSET		70
#define THUMBNAIL_LOCKED_ICON_Y_OFFSET		10//50
extern void ap_thumbnail_init(void);
extern void ap_thumbnail_exit(void);
extern INT16U ap_thumbnail_func_key_active(void);
extern void ap_thumbnail_next_key_active(INT8U err_flag);
extern void ap_thumbnail_prev_key_active(INT8U err_flag);
extern void ap_thumbnail_sts_set(INT8S sts);
extern void ap_thumbnail_reply_action(STOR_SERV_PLAYINFO *info_ptr);
extern INT8S ap_thumbnail_stop_handle(void);
extern void ap_thumbnail_no_media_show(INT16U str_type);
extern void ap_thumbnail_connect_to_pc(void);
extern void ap_thumbnail_disconnect_to_pc(void);
extern void ap_thumbnail_clean_frame_buffer(void);
extern INT8S ap_thumbnail_sts_get(void);