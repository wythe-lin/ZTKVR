#include "ztkconfigs.h"
#include "main.h"

/* for debug */
#define DEBUG_MAIN			1
#if DEBUG_MAIN
    #include "gplib.h"
    #define _dmsg(x)			print_string x
#else
    #define _dmsg(x)
#endif

/* variables */
extern volatile void *NorWakeupVector;
extern volatile void *Wakeup;

#define PERIPHERAL_HANDLING_STACK_SIZE		1024
#define STATE_HANDLING_STACK_SIZE		1024
#define DISPLAY_TASK_STACK_SIZE			1024
#define STORAGE_SERVICE_TASK_STACK_SIZE		1024
#define USB_TASK_STACK_SIZE			512
#define IDLE_TASK_STACK_SIZE			32
#define C_AUDIO_TASK_STACK_SIZE			1024
#define C_AUDIO_DAC_TASK_STACK_SIZE		200
#define C_FILESRV_TASK_STACK_SIZE	        500
#define C_FILESCAN_TASK_STACK_SIZE	        500


#define PRINT_OUTPUT_NONE			0x00 
#define PRINT_OUTPUT_UART			0x01
#define PRINT_OUTPUT_USB			0x02



INT32U PeripheralHandlingStack[PERIPHERAL_HANDLING_STACK_SIZE];
INT32U StateHandlingStack[STATE_HANDLING_STACK_SIZE];
#if C_VIDEO_PREVIEW == CUSTOM_ON
	INT32U DisplayTaskStack[DISPLAY_TASK_STACK_SIZE];
#endif	
INT32U StorageServiceTaskStack[STORAGE_SERVICE_TASK_STACK_SIZE];
INT32U USBTaskStack[USB_TASK_STACK_SIZE];
INT32U IdleTaskStack[IDLE_TASK_STACK_SIZE];
INT32U AudioTaskStack[C_AUDIO_TASK_STACK_SIZE];
INT32U AudioDacTaskStack[C_AUDIO_DAC_TASK_STACK_SIZE];
INT32U Filesrv[C_FILESRV_TASK_STACK_SIZE];
INT32U FileScanTaskStack[C_FILESCAN_TASK_STACK_SIZE];
extern void set_print_output_type(INT32U type);

/*
 * idle task
 */
 void idle_task_entry(void *para)
{
	INT32U i;
	OS_CPU_SR cpu_sr;

	while (1) {
		OS_ENTER_CRITICAL();
		R_SYSTEM_WAIT = 0x5005;
		i = R_CACHE_CTRL;

		ASM(NOP);
		ASM(NOP);
		ASM(NOP);
		ASM(NOP);
		ASM(NOP);
		ASM(NOP);
		ASM(NOP);
		ASM(NOP);
		ASM(NOP);
		ASM(NOP);
		ASM(NOP);
		ASM(NOP);
		ASM(NOP);
		ASM(NOP);
		ASM(NOP);
		ASM(NOP);
	    OS_EXIT_CRITICAL();
	}
}

static void debug_uart_port_enable(void)
{
	uart0_buad_rate_set(UART0_BAUD_RATE);
	uart0_tx_enable();
	uart0_rx_enable();	
}


/*
 *
 */
VIDEO_ARGUMENT	gvarg;		// global video argument

void gvarg_init(VIDEO_ARGUMENT *arg)
{
	gvarg.bScaler = 1;

	switch (zt_resolution()) {
	case ZT_VGA_PANORAMA:
		arg->TargetWidth	= 640;
		arg->TargetHeight	= 480;
		arg->SensorWidth	= 640;
		arg->SensorHeight	= 480;
		arg->DisplayWidth	= 320;
		arg->DisplayHeight	= 240;
		break;

	case ZT_VGA:
		arg->TargetWidth	= AVI_WIDTH*2;		// AVI_WIDTH  = 640
		arg->TargetHeight	= AVI_HEIGHT;		// AVI_HEIGHT = 480
		arg->SensorWidth	= 1280;
		arg->SensorHeight	= 480;
		arg->DisplayWidth	= AVI_WIDTH;
		arg->DisplayHeight	= AVI_HEIGHT;
		break;

	case ZT_HD_SCALED:
		arg->TargetWidth	= 1920;
		arg->TargetHeight	= 560;
		arg->SensorWidth	= 1920;
		arg->SensorHeight	= 560;
		arg->DisplayWidth	= 320;
		arg->DisplayHeight	= 240;
		break;

	case ZT_HD:
		arg->TargetWidth	= 2560;
		arg->TargetHeight	= 720;
		arg->SensorWidth	= 2560;
		arg->SensorHeight	= 720;
		arg->DisplayWidth	= 320;
		arg->DisplayHeight	= 240;
		break;
	}

	arg->DisplayBufferWidth  = TFT_WIDTH;		// TFT_WIDTH  = 320
	arg->DisplayBufferHeight = TFT_HEIGHT;		// TFT_HEIGHT = 240
	arg->VidFrameRate	 = AVI_FRAME_RATE;
	arg->AudSampleRate	 = 8000;
	arg->OutputFormat	 = IMAGE_OUTPUT_FORMAT_RGB565; 
}


/*
 * main loop
 */
void Main(void *free_memory)
{
	INT32U free_memory_start, free_memory_end;

	// Touch those sections so that they won't be removed by linker
	if (!NorWakeupVector && !Wakeup) {
		*((INT32U *) free_memory+1) = 0x0;
	}

	free_memory_start = ((INT32U) free_memory + 3) & (~0x3);	// Align to 4-bytes boundry
	free_memory_end = (INT32U) SDRAM_END_ADDR & ~(0x3);

	// Initiate Operating System
	OSInit();

	// Initiate drvier layer 1 modules
	drv_l1_init();

	timer_freq_setup(0, OS_TICKS_PER_SEC, 0, OSTimeTick);

	// Initiate driver layer 2 modules
	drv_l2_init();

	// Initiate gplib layer modules
	gplib_init(free_memory_start, free_memory_end);

	// Enable UART port for debug
	debug_uart_port_enable();
	
	//Configure the output type of debug message, NONE, UART, USB or both
	set_print_output_type(PRINT_OUTPUT_UART | PRINT_OUTPUT_USB);
	
	// Initiate applications here
	OSTaskCreate(task_peripheral_handling_entry, (void *) 0, &PeripheralHandlingStack[PERIPHERAL_HANDLING_STACK_SIZE - 1], PERIPHERAL_HANDLING_PRIORITY);
	OSTaskCreate(state_handling_entry, (void *) 0, &StateHandlingStack[STATE_HANDLING_STACK_SIZE - 1], STATE_HANDLING_PRIORITY);
#if C_VIDEO_PREVIEW == CUSTOM_ON
	OSTaskCreate(task_display_entry, (void *) 0, &DisplayTaskStack[DISPLAY_TASK_STACK_SIZE - 1], DISPLAY_TASK_PRIORITY);
#endif
	OSTaskCreate(task_storage_service_entry, (void *) 0, &StorageServiceTaskStack[STORAGE_SERVICE_TASK_STACK_SIZE - 1], STORAGE_SERVICE_PRIORITY);
	OSTaskCreate(state_usb_entry, (void *) 0, &USBTaskStack[USB_TASK_STACK_SIZE - 1], USB_DEVICE_PRIORITY);

#if MINI_DVR_BOARD_VERSION == GPL32680_MINI_DVR_CAR_RECORD_V2
//	OSTaskCreate(idle_task_entry, (void *) 0, &IdleTaskStack[IDLE_TASK_STACK_SIZE - 1], (OS_LOWEST_PRIO - 2));
#endif
	OSTaskCreate(audio_dac_task_entry, (void *) 0, &AudioDacTaskStack[C_AUDIO_DAC_TASK_STACK_SIZE - 1], DAC_PRIORITY);
	OSTaskCreate(audio_task_entry, (void *) 0, &AudioTaskStack[C_AUDIO_TASK_STACK_SIZE - 1], AUD_DEC_PRIORITY);
	OSTaskCreate(filesrv_task_entry,(void *) 0, &Filesrv[C_FILESRV_TASK_STACK_SIZE - 1], TSK_PRI_FILE_SRV);
	OSTaskCreate(file_scan_task_entry, (void *) 0, &FileScanTaskStack[C_FILESCAN_TASK_STACK_SIZE - 1], TSK_PRI_FILE_SCAN);

	gvarg_init(&gvarg);

	tft_init();
	tft_start(C_DISPLAY_DEVICE);

	_dmsg((WHITE "GPL326XXB CBR V0.0\r\n" NONE));
	_dmsg((WHITE "free_memory = 0x%x\r\n" NONE, free_memory));
	OSStart();
}
