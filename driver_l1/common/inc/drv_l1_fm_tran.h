#include "driver_l1.h"
#include "drv_l1_sfr.h"

#define FM_SLAVE_ID	0x1D
#define FM_SCL	IO_G5
#define FM_SDA	IO_C2

extern void SetFrequency(INT32U freq);
extern void Chip_initialization(void);
extern INT32U fm_tx_status_get(void);