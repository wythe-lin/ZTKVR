#include "task_state_handling.h"

typedef struct{
    const INT8U *item;
    INT8U item_max;
    INT8U sub_item_start;
    INT8U sub_item_max;
    INT16U stage;
}SETUP_ITEM_INFO;

extern void state_setting_entry(void *para);
extern void ap_setting_init(INT32U prev_state, INT8U *tag);
extern void ap_setting_exit(void);
extern void ap_setting_reply_action(INT32U state, INT8U *tag, STOR_SERV_PLAYINFO *info_ptr);
extern void ap_setting_page_draw(INT32U state, INT8U *tag);
extern void ap_setting_sub_menu_draw(INT8U curr_tag);
extern void ap_setting_func_key_active(INT8U *tag, INT8U *sub_tag, INT32U state);
extern void ap_setting_direction_key_active(INT8U *tag, INT8U *sub_tag, INT32U key_type, INT32U state);
extern EXIT_FLAG_ENUM ap_setting_menu_key_active(INT8U *tag, INT8U *sub_tag, INT32U *state, INT32U *state1);
extern EXIT_FLAG_ENUM ap_setting_mode_key_active(INT32U next_state, INT8U *sub_tag);
extern void ap_setting_format_reply(INT8U *tag, INT32U state);
extern void ap_setting_sensor_command_switch(INT16U cmd_addr, INT16U reg_bit, INT8U enable);
extern void ap_setting_value_set_from_user_config(void);
extern void ap_setting_frame_buff_display(void);