#define Max_LoopCnt 10  //最大階數設定

#define C_LPF_ENABLE	1

#define Cut_Off_Freq		18022	//Cut_Off_Freq = CutOff_Frequency*65536, Q16 (ex: 0.25 * 65536 = 16384)
#define Filter_Order		2		//Filter_Order = 1~10

extern void LPF_init(long coef_frq,short coef_loop);
//extern INT16U LPF_process(INT16U FilterBuf);
extern INT16S LPF_process(INT16S FilterBuf);