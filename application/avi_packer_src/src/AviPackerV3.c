#include "AviPackerV3.h"


#define AVIPACKER_VERSION_STRING	"GP$322 Generalplus AviPackerV3 20110520"

#define MSGQ_SIZE				64
#define FOURCC(a,b,c,d)			((a&0xFF) | ((b&0xFF)<<8) | ((c&0xFF)<<16) | ((d&0xFF)<<24))

#define CLUSTER_SHIFT			15
#define CLUSTER_SIZE			(1<<CLUSTER_SHIFT)

#define INVALID_FILE_ID			-1
#define RETURN(x)				{ret = x; goto Return;}

#define AVIPACKER_FLAG_AUD_EXIST		1

#define AVIF_HASINDEX		0x00000010	// index at end of file
#define AVIF_ISINTERLEAVED	0x00000100

typedef enum
{
	AVIPACKER_MSG_WRITE = 1,
	AVIPACKER_MSG_STOP
} AVIPACKER_MSG;

typedef enum
{
	AVIPACKER_ACK_OK = 1,
	AVIPACKER_ACK_FILE_OPEN_ERROR
} AVIPACKER_ACK;

typedef struct
{
	unsigned int	dwMicroSecPerFrame;
	unsigned int	dwMaxBytesPerSec;
	unsigned int	dwPaddingGranularity;

	unsigned int	dwFlags;
	unsigned int	dwTotalFrames;
	unsigned int	dwInitialFrames;
	unsigned int	dwStreams;
	unsigned int	dwSuggestedBufferSize;
	
	unsigned int	dwWidth;
	unsigned int	dwHeight;

	unsigned int	dwReserved[4];
} MAINAVIHEADER;	// avih

#if __OPTIMISE_LEVEL<2
	#define DEBUG_ONLY(x)	{x;}
#else
	#define DEBUG_ONLY(x)	{}
#endif


static unsigned int fixed_frame_nums=0;
static unsigned int movie_size=0;
static char packer_suspend;
void VdoFramNumsLowBoundReg(unsigned int fix_frame_cnts);
unsigned long current_movie_Byte_size_get(void);

static long DIVI64BYI32(long long a, long b)
{
	long lo, hi;
	long i, q = 0;
	lo = (long)a;
	hi = a >> 32;

	for(i=0; i<32; i++)
	{
		hi <<= 1;
		if(lo<0) hi++;
		lo <<= 1;
		q <<= 1;
		if(hi >= b)
		{
			q++;
			hi -= b;
		}
	}
	return q;
}


static int ConfigHeader(
	unsigned long			*MoviSize,
	char					*buf,
	MAINAVIHEADER			**MainAviHeader,
	GP_AVI_AVISTREAMHEADER	**VidHdr,
	int						VidFmtLen,
	GP_AVI_BITMAPINFO		**VidFmt,
	GP_AVI_AVISTREAMHEADER	**AudHdr,
	int						AudFmtLen,			// If AudFmtLen = 0 => No audio stream 
	GP_AVI_PCMWAVEFORMAT	**AudFmt);



// AviEnc Working Memory ///////////////////////////////
typedef struct
{
	unsigned long fourcc;
	unsigned long flag;
	unsigned long offset;
	unsigned long size;
} IDXDATA;

#define INFO_LIST_BUF_SIZE	1024

#define JUNK1		FOURCC( 0 , 0 ,'J','U')
#define JUNK2		FOURCC('N','K', 2,  0 )
#define JUNK3		FOURCC( 0 , 0 , 0 , 0 )


typedef struct
{
	OS_STK stkAviPackerV3_Task[512];

	void			*MsgQ[MSGQ_SIZE];
	OS_EVENT		*Sem, *Msg, *Ack;

	short			MsgTx, MsgRx;

	// Stream Write Ring Buffer
	char			*RingBuf;
	int				RingBufSize;
	int				WI, RI;
	unsigned long	MoviCnt;

	// index write ring buffer
	char			*IdxRingBuf;
	int				IdxRingBufSize;
	int				IdxRI, IdxWI;
	unsigned long	IdxChkSize;

	int				IdxBlockSize;
	int				IdxBlockMask;

	// file stream JUNK
	int StreamJunkCnt;
	int IndexJunkCnt;
	
	

	unsigned int AudChkCnt, VidChkCnt;

	unsigned int MaxVChunkSize;
	unsigned int nAudSamples;			// size of audio stream data conut in sample

	int (*ErrHandler)(int ErrCode);

	INT16S fid;
	INT16S fid_idx;

	int Flag;

	int OrgHdrLen;	
	MAINAVIHEADER			*pMainAviHdr;
	GP_AVI_AVISTREAMHEADER	*pVidHdr;
	GP_AVI_AVISTREAMHEADER	*pAudHdr;
	
	unsigned long info_len;
	char info_data[INFO_LIST_BUF_SIZE];
	
	IDXDATA lastIdx;
	int lastIdx_flag;
} AVIPACKER_WORKMEM;


static int StrLen(const char *str)
{
	int len = 0;
	if(str==0) return 0;
	while(*str++) len++;
	return len;
}

// static AVIPACKER_WORKMEM AviPacker;
// EndOf AviEnc Working Memory /////////////////////////


#define READ_IN_TASK(fid, addr, len, msg)\
{\
	if(read(fid,(unsigned long)(addr),len)!=len && AviPacker->ErrHandler)\
	{\
		ErrAction = AviPacker->ErrHandler(msg); \
		if(ErrAction==1) goto ErrMode;\
	}\
}

#define WRITE_IN_TASK(fid, addr, len, msg) \
{\
	if(write(fid,(unsigned long)(addr),len)!=len && AviPacker->ErrHandler)\
	{\
		ErrAction = AviPacker->ErrHandler(msg); \
		if(ErrAction==1) goto ErrMode;\
	}\
}

#define SEEK_IN_TASK(fid, offset, org, msg) \
{\
	if(lseek(fid ,offset, org)!=0 && AviPacker->ErrHandler)\
	{\
		ErrAction = AviPacker->ErrHandler(msg); \
		if(ErrAction==1) goto ErrMode;\
	}\
}

#define CAT_IN_TASK(fid, fid_idx, msg) \
{\
    if(file_fast_cat(fid, fid_idx)!=0)\
    {\
       	ErrAction = AviPacker->ErrHandler(msg);\
		if(ErrAction==1) goto ErrMode;\
    }\
}

#define WRITE(fid, addr, len) write(fid,(unsigned long)(addr),len)


static void MemCpyAlign2(void *_dst, const void *_src, int len)
{
	register short *dst = (short*)_dst;
	const register short *src = (short*)_src;
	register short *end = (short*)((char*)_dst + len);
	len >>= 1;
	switch(len & 7)
	{
	case 7:		*dst++ = *src++;
	case 6:		*dst++ = *src++;
	case 5:		*dst++ = *src++;
	case 4:		*dst++ = *src++;
	case 3:		*dst++ = *src++;
	case 2:		*dst++ = *src++;
	case 1:		*dst++ = *src++;
	}

	while(dst < end)
	{
 		*dst++ = *src++;
 		*dst++ = *src++;
 		*dst++ = *src++;
 		*dst++ = *src++;
 		*dst++ = *src++;
 		*dst++ = *src++;
 		*dst++ = *src++;
 		*dst++ = *src++;
	}
}

static void MemCpyAlign4(void *_dst, const void *_src, int len)
{
	register long *dst = (long*)_dst;
	const register long *src = (long*)_src;
	register long *end = (long*)((char*)_dst + len);
	len >>= 2;
	switch(len & 15)
	{
	case 15:
		__asm{
			LDMIA src!,{r4-r10}
			STMIA dst!,{r4-r10}
			LDMIA src!,{r4-r11}
			STMIA dst!,{r4-r11}			
		}
		break;
	case 14:
		__asm{
			LDMIA src!,{r4-r9}
			STMIA dst!,{r4-r9}
			LDMIA src!,{r4-r11}
			STMIA dst!,{r4-r11}			
		}
		break;
	case 13:
		__asm{
			LDMIA src!,{r4-r8}
			STMIA dst!,{r4-r8}
			LDMIA src!,{r4-r11}
			STMIA dst!,{r4-r11}			
		}
		break;
	case 12:
		__asm{
			LDMIA src!,{r4-r7}
			STMIA dst!,{r4-r7}
			LDMIA src!,{r4-r11}
			STMIA dst!,{r4-r11}			
		}
		break;
	case 11:
		__asm{
			LDMIA src!,{r4-r6}
			STMIA dst!,{r4-r6}
			LDMIA src!,{r4-r11}
			STMIA dst!,{r4-r11}			
		}
		break;
	case 10:
		__asm{
			LDMIA src!,{r4-r5}
			STMIA dst!,{r4-r5}
			LDMIA src!,{r4-r11}
			STMIA dst!,{r4-r11}			
		}
		break;
	case 9:
		__asm{
			LDMIA src!,{r4-r12}
			STMIA dst!,{r4-r12}
		}
		break;
	case 8:
		__asm{
			LDMIA src!,{r4-r11}
			STMIA dst!,{r4-r11}
		}
		break;
	case 7:
		__asm{
			LDMIA src!,{r4-r10}
			STMIA dst!,{r4-r10}
		}
		break;
	case 6:
		__asm{
			LDMIA src!,{r4-r9}
			STMIA dst!,{r4-r9}
		}
		break;
	case 5:
		__asm{
			LDMIA src!,{r4-r8}
			STMIA dst!,{r4-r8}
		}
		break;
	case 4:
		__asm{
			LDMIA src!,{r4-r7}
			STMIA dst!,{r4-r7}
		}
		break;
	case 3:
		__asm{
			LDMIA src!,{r4-r6}
			STMIA dst!,{r4-r6}
		}
		break;
	case 2:
		__asm{
			LDMIA src!,{r4-r5}
			STMIA dst!,{r4-r5}
		}
		break;
	case 1:
		*dst++ = *src++;
	}

	while(dst < end)
	{
		__asm{
			LDMIA src!,{r4-r11}
			STMIA dst!,{r4-r11}
			LDMIA src!,{r4-r11}
			STMIA dst!,{r4-r11}			
		}
	}
}

static void MemClrAlign4(void *_dst, int len)
{
	register long *dst = (long*)_dst;
	register long *end = (long*)((char*)_dst + len);
	len >>= 2;
	__asm{
		MOV r4, 0
		MOV r5, 0
		MOV r6, 0
		MOV r7, 0
		MOV r8, 0
		MOV r9, 0
		MOV r10, 0
		MOV r11, 0
		MOV r12, 0
	}
	if(len & 16)
	{
		__asm{
			STMIA dst!,{r4-r11}
			STMIA dst!,{r4-r11}
		}
		len -= 16;
	}
	switch(len & 15)
	{
	case 15:
		__asm{
			STMIA dst!,{r4-r10}
			STMIA dst!,{r4-r11}
		}
		break;
	case 14:
		__asm{
			STMIA dst!,{r4-r9}
			STMIA dst!,{r4-r11}			
		}
		break;
	case 13:
		__asm{
			STMIA dst!,{r4-r8}
			STMIA dst!,{r4-r11}			
		}
		break;
	case 12:
		__asm{
			STMIA dst!,{r4-r7}
			STMIA dst!,{r4-r11}			
		}
		break;
	case 11:
		__asm{
			STMIA dst!,{r4-r6}
			STMIA dst!,{r4-r11}			
		}
		break;
	case 10:
		__asm{
			STMIA dst!,{r4-r5}
			STMIA dst!,{r4-r11}			
		}
		break;
	case 9:
		__asm{
			STMIA dst!,{r4-r12}
		}
		break;
	case 8:
		__asm{
			STMIA dst!,{r4-r11}
		}
		break;
	case 7:
		__asm{
			STMIA dst!,{r4-r10}
		}
		break;
	case 6:
		__asm{
			STMIA dst!,{r4-r9}
		}
		break;
	case 5:
		__asm{
			STMIA dst!,{r4-r8}
		}
		break;
	case 4:
		__asm{
			STMIA dst!,{r4-r7}
		}
		break;
	case 3:
		__asm{
			STMIA dst!,{r4-r6}
		}
		break;
	case 2:
		__asm{
			STMIA dst!,{r4-r5}
		}
		break;
	case 1:
		*dst++ = 0;
	}

	while(dst < end)
	{
		__asm{
			STMIA dst!,{r4-r11}
			STMIA dst!,{r4-r11}			
			STMIA dst!,{r4-r11}
			STMIA dst!,{r4-r11}			
		}
	}
}

static int FileStreamWrite_x32KB(AVIPACKER_WORKMEM *AviPacker)
{
	// DO NOT use "AviPacker->WI" directly!!
	// Becase "AviPacker->WI" might be changed by other task while file-write is processing.

	int RI, WI;
	int write_flag = 0;
	int ErrAction;

	WI = (AviPacker->WI >> CLUSTER_SHIFT) << CLUSTER_SHIFT; // 32KB multiple
	RI = AviPacker->RI;

	if(RI!=WI) write_flag = 1;
	while(RI!=WI)
	{
		WRITE_IN_TASK(AviPacker->fid, AviPacker->RingBuf + RI, CLUSTER_SIZE, AVIPACKER_RESULT_FILE_WRITE_ERR);
		RI += CLUSTER_SIZE;
		if(RI>=AviPacker->RingBufSize) RI -= AviPacker->RingBufSize;
		AviPacker->RI = RI;
	}
	return write_flag;
	
ErrMode:
	return -1;
}

static int IndexWriteBlocks(AVIPACKER_WORKMEM *AviPacker)
{
	// write index ring buffer to index file
	int IdxWI, IdxRI;
	int write_flag = 0;
	int ErrAction;
		
	IdxWI = AviPacker->IdxWI & AviPacker->IdxBlockMask;
	IdxRI = AviPacker->IdxRI;
	
	if(IdxRI!=IdxWI) write_flag = 1;
	while(IdxRI!=IdxWI)
	{
		WRITE_IN_TASK(AviPacker->fid_idx, AviPacker->IdxRingBuf + IdxRI, AviPacker->IdxBlockSize, AVIPACKER_RESULT_IDX_FILE_WRITE_ERR);
		IdxRI += AviPacker->IdxBlockSize;
		if(IdxRI>=AviPacker->IdxRingBufSize) IdxRI -= AviPacker->IdxRingBufSize;
		AviPacker->IdxRI = IdxRI;
	}
	return write_flag;

ErrMode:
	return -1;	
}

#define RING_BUF_ADD_4B_ALIGN(Ring, WI, BufSize, val) \
{\
	*(long*)((Ring) + (WI)) = (val);\
	(WI) += 4;\
	if((WI)>=(BufSize)) (WI) -= (BufSize);\
}

static void AviPackerV3_Task(void *pdata)
{
	INT8U err;
	AVIPACKER_MSG msg;
	int ret;
	int ErrAction;
	AVIPACKER_WORKMEM *AviPacker = (AVIPACKER_WORKMEM*)pdata;
	DEBUG_ONLY(DBG_PRINT("AviPackerV3_Task\r\n"));

	while(1)
	{
		msg = (AVIPACKER_MSG)(int)OSQPend(AviPacker->Msg, 0, &err);
		if(msg==AVIPACKER_MSG_WRITE)
		{
		    if (packer_suspend==0) 
            {
    			int write_flag = 1;
    			AviPacker->MsgRx ++;
    			
    			while(write_flag)
    			{
    				ret = FileStreamWrite_x32KB(AviPacker);
    				if(ret<0) goto ErrMode1;
    				write_flag  = ret;
    				ret = IndexWriteBlocks(AviPacker);
    				if(ret<0) goto ErrMode1;
    				write_flag |= ret;
    			}
    			movie_size = AviPacker->MoviCnt;
    			DEBUG_ONLY(
    				OSSchedLock();
    				DBG_PRINT("RING %d %d %d %d\r\n",AviPacker->WI, AviPacker->RI, AviPacker->IdxWI, AviPacker->IdxRI);
    				OSSchedUnlock();
    			);
                
                if (movie_size>0xFF000000)
                {
                    DBG_PRINT ("AVI MOVIE Max Size:%s Attend\r\n",movie_size);
                    break;
                }            
            } else {
                DBG_PRINT ("B");

            }
		}
		else if(msg==AVIPACKER_MSG_STOP)
        {      
			break;
	}
	}

	ret = FileStreamWrite_x32KB(AviPacker);
	if(ret<0) goto ErrMode;
	
	ret = IndexWriteBlocks(AviPacker);
	if(ret<0) goto ErrMode;

	// write INFO_LIST to index file 
	{
		int IdxWI = AviPacker->IdxWI;
		int t;
		const char *src;
	
		// Add LIST_INFO after index chunk
		RING_BUF_ADD_4B_ALIGN(AviPacker->IdxRingBuf, IdxWI, AviPacker->IdxRingBufSize, FOURCC('L','I','S','T'));
		RING_BUF_ADD_4B_ALIGN(AviPacker->IdxRingBuf, IdxWI, AviPacker->IdxRingBufSize, AviPacker->info_len);

		src = AviPacker->info_data;
		t = AviPacker->info_len;
		if(IdxWI + t>=AviPacker->IdxRingBufSize)
		{
			MemCpyAlign4(AviPacker->IdxRingBuf + IdxWI, src, AviPacker->IdxRingBufSize - IdxWI);
			src += AviPacker->IdxRingBufSize - IdxWI;
			t -= AviPacker->IdxRingBufSize - IdxWI;
			IdxWI = 0;
		}
		if(t)
		{
			MemCpyAlign4(AviPacker->IdxRingBuf + IdxWI, src, t);
			IdxWI += t;
		}
			
		AviPacker->IndexJunkCnt = (AviPacker->IdxBlockSize - IdxWI - 8) & (AviPacker->IdxBlockSize - 1);		
	
		// Add JUNK to pad index file to 32KB multiple

		RING_BUF_ADD_4B_ALIGN(AviPacker->IdxRingBuf, IdxWI, AviPacker->IdxRingBufSize, FOURCC('J','U','N','K'));
		RING_BUF_ADD_4B_ALIGN(AviPacker->IdxRingBuf, IdxWI, AviPacker->IdxRingBufSize, AviPacker->IndexJunkCnt);
		
		IdxWI += AviPacker->IndexJunkCnt;
		if(IdxWI>=AviPacker->IdxRingBufSize) IdxWI -= AviPacker->IdxRingBufSize;

		AviPacker->IdxWI = IdxWI;
	}
	ret = IndexWriteBlocks(AviPacker);
	if(ret<0) goto ErrMode;
	
	
	
	// write rest bs ring buffer to file : 32KB multiple //////////////////
	{
		// Add Stream-JUNK chunk and idx1 chunk header to file
		// At this time stream file WILL be paded to 32KB
		int WI = AviPacker->WI;
		AviPacker->StreamJunkCnt = (CLUSTER_SIZE - WI - 8 - 8) & (CLUSTER_SIZE - 1);

		RING_BUF_ADD_4B_ALIGN(AviPacker->RingBuf, WI, AviPacker->RingBufSize, FOURCC('J','U','N','K'));
		RING_BUF_ADD_4B_ALIGN(AviPacker->RingBuf, WI, AviPacker->RingBufSize, AviPacker->StreamJunkCnt);
		
		WI += AviPacker->StreamJunkCnt;
		if(WI>=AviPacker->RingBufSize) WI -= AviPacker->RingBufSize;

		RING_BUF_ADD_4B_ALIGN(AviPacker->RingBuf, WI, AviPacker->RingBufSize, FOURCC('i','d','x','1'));
		RING_BUF_ADD_4B_ALIGN(AviPacker->RingBuf, WI, AviPacker->RingBufSize, AviPacker->IdxChkSize);	
		
		AviPacker->WI = WI;
	}
	ret = FileStreamWrite_x32KB(AviPacker);
	if(ret<0) goto ErrMode;

    // Cascate data and index before header seek
    CAT_IN_TASK(AviPacker->fid,AviPacker->fid_idx,AVIPACKER_RESULT_FILE_CAT_ERR);  
  
	// Read header back to Buffer
	SEEK_IN_TASK(AviPacker->fid, 0, SEEK_SET, AVIPACKER_RESULT_FILE_SEEK_ERR);
	READ_IN_TASK(AviPacker->fid, AviPacker->RingBuf, AviPacker->OrgHdrLen, AVIPACKER_RESULT_FILE_READ_ERR);

	// RIFF len
	*(long*)(AviPacker->RingBuf+4)				=	(AviPacker->OrgHdrLen - 8 - 12) + 
													8 + AviPacker->MoviCnt + 
													8 + AviPacker->StreamJunkCnt +
													8 + AviPacker->IdxChkSize + 
													8 + AviPacker->info_len +
													8 + AviPacker->IndexJunkCnt;
		
	// LIST movi len
	*(long*)(AviPacker->RingBuf+AviPacker->OrgHdrLen-8)
												= AviPacker->MoviCnt;
#if 1
        //DBG_PRINT ("Org Fram nums %d \r\n",AviPacker->VidChkCnt);
    if (AviPacker->VidChkCnt<fixed_frame_nums)
    {   
        //DBG_PRINT ("MODIFY frame nums from %d to %d\r\n",AviPacker->VidChkCnt,fixed_frame_nums);
        AviPacker->VidChkCnt = fixed_frame_nums;
    }
#endif
                                                
	AviPacker->pMainAviHdr->dwTotalFrames		= AviPacker->VidChkCnt;  // domi1
	AviPacker->pVidHdr->dwLength				= AviPacker->VidChkCnt;  // domi2
	AviPacker->pVidHdr->dwSuggestedBufferSize	= AviPacker->MaxVChunkSize;
	AviPacker->pAudHdr->dwLength				= AviPacker->nAudSamples;

    fixed_frame_nums=0; // reset fix fram nums

//domittt = sw_timer_get_counter_L();

//    CAT_IN_TASK(AviPacker->fid,AviPacker->fid_idx,AVIPACKER_RESULT_FILE_CAT_ERR);  
//    domittt = sw_timer_get_counter_L()-domittt;
//    DBG_PRINT ("Cat time3: %d ms\r\n",domittt);

    
	SEEK_IN_TASK(AviPacker->fid, 0, SEEK_SET, AVIPACKER_RESULT_FILE_SEEK_ERR);
	WRITE_IN_TASK(AviPacker->fid, AviPacker->RingBuf, AviPacker->OrgHdrLen, AVIPACKER_RESULT_FILE_WRITE_ERR);

ErrMode:
	OSMboxPost(AviPacker->Ack, (void*)AVIPACKER_ACK_OK);
	OSTaskDel(OS_PRIO_SELF);

ErrMode1:
	while(1)
	{
		msg = (AVIPACKER_MSG)(int)OSQPend(AviPacker->Msg, 0, &err);
		if(msg==AVIPACKER_MSG_STOP)
			break;
	}

	OSMboxPost(AviPacker->Ack, (void*)AVIPACKER_ACK_OK);
	OSTaskDel(OS_PRIO_SELF);	
}

void AviPackerV3_SetErrHandler(void *_AviPacker, int (*ErrHandler)(int ErrCode))
{
	AVIPACKER_WORKMEM *AviPacker = (AVIPACKER_WORKMEM *)_AviPacker;
	AviPacker->ErrHandler = ErrHandler;
}


int AviPackerV3_Open(
	void *_AviPacker,
	int								fid,
	int								fid_idx,
	const GP_AVI_AVISTREAMHEADER	*VidHdr,
	int								VidFmtLen,
	const GP_AVI_BITMAPINFO			*VidFmt,
	const GP_AVI_AVISTREAMHEADER	*AudHdr,
	int								AudFmtLen,
	const GP_AVI_PCMWAVEFORMAT		*AudFmt,
	unsigned char					prio,
	void							*pRingBuf,
	int								RingBufSize,
	void							*pIdxRingBuf,
	int								IdxRingBufSize)
{
	int ret;
	INT8U err;
	GP_AVI_BITMAPINFO		*pVidFmt;
	GP_AVI_PCMWAVEFORMAT	*pAudFmt;
	AVIPACKER_WORKMEM *AviPacker = (AVIPACKER_WORKMEM *)_AviPacker;

    DEBUG_ONLY(DBG_PRINT("AviPackerV3_Open\r\n"));
    packer_suspend = 0;
	AviPacker->Msg = AviPacker->Ack = AviPacker->Sem = NULL;
	AviPacker->ErrHandler = 0;
    
    
	// check input argument
	if((unsigned long)pRingBuf & 0x3)		RETURN(AVIPACKER_RESULT_MEM_ALIGN_ERR);
	if((unsigned long)pIdxRingBuf & 0x3)	RETURN(AVIPACKER_RESULT_MEM_ALIGN_ERR);
	if(fid==INVALID_FILE_ID)				RETURN(AVIPACKER_RESULT_FILE_OPEN_ERROR);
	if(fid_idx==INVALID_FILE_ID)			RETURN(AVIPACKER_RESULT_IDX_FILE_OPEN_ERROR);
	if(VidHdr == 0 || VidFmt == 0)			RETURN(AVIPACKER_RESULT_PARAMETER_ERROR);
	if(AudHdr == 0 && AudFmt != 0)			RETURN(AVIPACKER_RESULT_PARAMETER_ERROR);
	if(AudHdr != 0 && AudFmt == 0)			RETURN(AVIPACKER_RESULT_PARAMETER_ERROR);
	
	if(AudHdr != 0 && AudFmt != 0 && AudFmtLen!=0)
		AviPacker->Flag = AVIPACKER_FLAG_AUD_EXIST;
	else
		AudFmtLen = 0;
	
	if(RingBufSize<65536)					RETURN(AVIPACKER_RESULT_BS_BUF_TOO_SMALL);
	if(IdxRingBufSize<8192)					RETURN(AVIPACKER_RESULT_IDX_BUF_TOO_SMALL);
		
	
	
	AviPacker->RingBuf			= pRingBuf;
	AviPacker->RingBufSize		= (RingBufSize >> 15) << 15;			// 32KB multiple. File stream buffer MUST be 32KB multiple, because of file merge.
	
	AviPacker->IdxRingBuf		= pIdxRingBuf;
	if(IdxRingBufSize >= 65536)
	{
		AviPacker->IdxRingBufSize	= IdxRingBufSize >> 15 << 15;
	}
	else if(IdxRingBufSize >= 32768)
	{
		AviPacker->IdxRingBufSize	= IdxRingBufSize >> 14 << 14;
	}
	else if(IdxRingBufSize >= 16384)
	{
		AviPacker->IdxRingBufSize	= IdxRingBufSize >> 13 << 13;
	}
	else if(IdxRingBufSize >= 8192)
	{
		AviPacker->IdxRingBufSize	= IdxRingBufSize >> 12 << 12;
	}
	AviPacker->IdxBlockSize = AviPacker->IdxRingBufSize >> 1;
	AviPacker->IdxBlockMask = ~(AviPacker->IdxBlockSize - 1);
		

	AviPacker->Msg = OSQCreate(AviPacker->MsgQ, MSGQ_SIZE);	if(AviPacker->Msg==NULL) RETURN(AVIPACKER_RESULT_OS_ERR);
	AviPacker->MsgTx = AviPacker->MsgRx = 0;
	AviPacker->Ack = OSMboxCreate(0);						if(AviPacker->Ack==NULL) RETURN(AVIPACKER_RESULT_OS_ERR);
	AviPacker->Sem = OSSemCreate(1);						if(AviPacker->Sem==NULL) RETURN(AVIPACKER_RESULT_OS_ERR);

	// AviPacker->Gap0 = AviPacker->Gap1 = 0xFF00FF00;

	AviPacker->fid = fid;
	AviPacker->fid_idx = fid_idx;

	AviPacker->WI = AviPacker->RI = 0;
	AviPacker->IdxWI = AviPacker->IdxRI = 0;
	
	AviPacker->IdxChkSize = AviPacker->AudChkCnt = AviPacker->VidChkCnt = 0;

	AviPacker->MaxVChunkSize = 0;
	AviPacker->nAudSamples = 0;

	AviPacker->lastIdx_flag = 0;

	AviPacker->OrgHdrLen = ConfigHeader(
		&AviPacker->MoviCnt,
		AviPacker->RingBuf,
		&AviPacker->pMainAviHdr,
		&AviPacker->pVidHdr,
		VidFmtLen,
		&pVidFmt,
		&AviPacker->pAudHdr,
		AudFmtLen,
		&pAudFmt);
		
	AviPacker->WI = AviPacker->OrgHdrLen;
	

	MemCpyAlign2(AviPacker->pVidHdr,		VidHdr, sizeof(GP_AVI_AVISTREAMHEADER));
	MemCpyAlign2(pVidFmt,					VidFmt, VidFmtLen);
	if(AviPacker->pAudHdr)
		MemCpyAlign2(AviPacker->pAudHdr,	AudHdr, sizeof(GP_AVI_AVISTREAMHEADER));
	if(pAudFmt)
		MemCpyAlign2(pAudFmt,				AudFmt, AudFmtLen);
	
	MemClrAlign4(AviPacker->pMainAviHdr, sizeof(MAINAVIHEADER));
	AviPacker->pMainAviHdr->dwMicroSecPerFrame	= DIVI64BYI32((long long)AviPacker->pVidHdr->dwScale * 1000000, AviPacker->pVidHdr->dwRate);
	AviPacker->pMainAviHdr->dwFlags				= AVIF_ISINTERLEAVED | AVIF_HASINDEX;
	AviPacker->pMainAviHdr->dwStreams			= AviPacker->Flag & AVIPACKER_FLAG_AUD_EXIST ? 2 : 1;
	AviPacker->pMainAviHdr->dwWidth				= pVidFmt->biWidth;
	AviPacker->pMainAviHdr->dwHeight			= pVidFmt->biHeight;
	
	
	MemClrAlign4(AviPacker->stkAviPackerV3_Task, sizeof(AviPacker->stkAviPackerV3_Task));
	if(OSTaskCreate(
		AviPackerV3_Task,
		AviPacker,
		AviPacker->stkAviPackerV3_Task + sizeof(AviPacker->stkAviPackerV3_Task)/sizeof(OS_STK) - 1,
		prio) != OS_NO_ERR)
	{
		RETURN(AVIPACKER_RESULT_OS_ERR);
	}

	AviPacker->info_len = 4;
	AviPacker->info_data[0] = 'I';	AviPacker->info_data[1] = 'N';
	AviPacker->info_data[2] = 'F';	AviPacker->info_data[3] = 'O';

   	ret = AVIPACKER_RESULT_OK;

Return:
	if(ret != AVIPACKER_RESULT_OK)
	{
		if(AviPacker->Sem) OSSemDel(AviPacker->Sem, 0, &err);
		if(AviPacker->Ack) OSMboxDel(AviPacker->Ack, 0, &err);
	 	if(AviPacker->Msg) OSQDel(AviPacker->Msg, 0, &err);
		AviPacker->Msg = AviPacker->Ack = AviPacker->Sem = 0;
	}
	return ret;
}

int AviPackerV3_PutData(void *_AviPacker, unsigned long fourcc, long cbLen, const void *ptr, int nSamples, int ChunkFlag)
{
	AVIPACKER_WORKMEM *AviPacker = (AVIPACKER_WORKMEM *)_AviPacker;
	INT8U err;
	int ret;
	IDXDATA *idxptr;
	int IdxWI;
	int cnt, cbRealChunkSize, padding;

	OSSemPend(AviPacker->Sem, 0, &err);
	if(err!=OS_NO_ERR) return AVIPACKER_RESULT_OS_ERR;

	if(cbLen<0)
		RETURN(AVIPACKER_RESULT_PARAMETER_ERROR);

DEBUG_ONLY
(
	if(AviPacker->fid==INVALID_FILE_ID)		RETURN(AVIPACKER_RESULT_FILE_OPEN_ERROR);
	if(AviPacker->fid_idx==INVALID_FILE_ID)	RETURN(AVIPACKER_RESULT_IDX_FILE_OPEN_ERROR);
);
	
	cbRealChunkSize = cbLen;
	cbRealChunkSize += cbRealChunkSize & 1; // fource to even

	// check idx ring buffer size
	cnt = AviPacker->IdxRI - AviPacker->IdxWI;
	if(cnt<=0) cnt += AviPacker->IdxRingBufSize;
	if(cnt<=sizeof(IDXDATA))
	{
		RETURN(AVIPACKER_RESULT_IDX_BUF_OVERFLOW);
	}

	// check bs ring buffer size
	cnt = AviPacker->RI - AviPacker->WI;
	if(cnt<=0) cnt += AviPacker->RingBufSize;
	if(cnt<=cbRealChunkSize + 8 + 10)
	{
		RETURN(AVIPACKER_RESULT_BS_BUF_OVERFLOW);
	}
	

	// if null video frame appears, duplicates last index and then skip write current frame
	if((fourcc==FOURCC('0','0','d','b') || fourcc==FOURCC('0','0','d','c')) && cbLen==0 && AviPacker->lastIdx_flag)
	{
		IdxWI = AviPacker->IdxWI;
		idxptr = (IDXDATA*)(AviPacker->IdxRingBuf + IdxWI);
		*idxptr = AviPacker->lastIdx;		
		AviPacker->VidChkCnt ++;
		IdxWI += sizeof(IDXDATA);
		if(IdxWI>=AviPacker->IdxRingBufSize) IdxWI -= AviPacker->IdxRingBufSize;
		AviPacker->IdxWI = IdxWI;
		AviPacker->IdxChkSize += sizeof(IDXDATA);
		RETURN(AVIPACKER_RESULT_OK);	
	}

	// write index table //////////////////////////////////////////////////////
	IdxWI = AviPacker->IdxWI;
	idxptr = (IDXDATA*)(AviPacker->IdxRingBuf + IdxWI);
	idxptr->fourcc = fourcc;
	if(fourcc==FOURCC('0','0','d','b') || fourcc==FOURCC('0','0','d','c') || fourcc==FOURCC('0','0','p','c'))
	{
		AviPacker->VidChkCnt ++;
		idxptr->flag = ChunkFlag; // 0x00000010;
		if(AviPacker->MaxVChunkSize<cbRealChunkSize) AviPacker->MaxVChunkSize = cbRealChunkSize;	
	}
	else if((AviPacker->Flag & AVIPACKER_FLAG_AUD_EXIST) && fourcc==FOURCC('0','1','w','b'))
	{
		AviPacker->AudChkCnt ++;
		idxptr->flag = ChunkFlag;
		AviPacker->nAudSamples += nSamples;
	}
	else
	{
		RETURN(AVIPACKER_RESULT_IGNORE_CHUNK);
	}
	idxptr->offset = AviPacker->MoviCnt;
	idxptr->size = cbLen;
	IdxWI += sizeof(IDXDATA);
	if(IdxWI>=AviPacker->IdxRingBufSize) IdxWI -= AviPacker->IdxRingBufSize;
	AviPacker->IdxWI = IdxWI;
	AviPacker->IdxChkSize += sizeof(IDXDATA);
	
	if((fourcc==FOURCC('0','0','d','b') || fourcc==FOURCC('0','0','d','c')) && cbLen)
	{
		AviPacker->lastIdx = *idxptr;
		AviPacker->lastIdx_flag = 1;
	}
	
	// write to bs ring buffer //////////////////////////////////////////////////////
	{
		int t;
		int WI;
		const char *src;
		char *dst;
		padding = 0;
		WI = AviPacker->WI;
		// chunk name and size
		RING_BUF_ADD_4B_ALIGN(AviPacker->RingBuf, WI, AviPacker->RingBufSize, fourcc);
		RING_BUF_ADD_4B_ALIGN(AviPacker->RingBuf, WI, AviPacker->RingBufSize, cbLen);
		// write chunk data
		dst = AviPacker->RingBuf + WI;
		src = ptr;
		cnt = cbRealChunkSize >> 2 << 2;
		if(WI + cnt >= AviPacker->RingBufSize)
		{
			t = AviPacker->RingBufSize - WI;
			MemCpyAlign4(dst, src, t);
			cnt -= t;
			src += t;
			dst = AviPacker->RingBuf;
			WI = 0;
		}
		MemCpyAlign4(dst, src, cnt);
		src += cnt;
		WI += cnt;
		if(cbRealChunkSize & 2)
		{
			RING_BUF_ADD_4B_ALIGN(AviPacker->RingBuf, WI, AviPacker->RingBufSize, *(unsigned short*)src | JUNK1);
			RING_BUF_ADD_4B_ALIGN(AviPacker->RingBuf, WI, AviPacker->RingBufSize, JUNK2);
			RING_BUF_ADD_4B_ALIGN(AviPacker->RingBuf, WI, AviPacker->RingBufSize, JUNK3);
			padding = 10;
		}
		AviPacker->WI = WI;
	}


	AviPacker->MoviCnt += 8 + cbRealChunkSize + padding;

	DEBUG_ONLY(
		OSSchedLock();
		DBG_PRINT("F %d %d %d\r\n", AviPacker->VidChkCnt + AviPacker->AudChkCnt, AviPacker->VidChkCnt, AviPacker->AudChkCnt);
		OSSchedUnlock();
	);
	
	if(AviPacker->MsgTx == AviPacker->MsgRx)
	{
		if(OSQPost(AviPacker->Msg, (void*)AVIPACKER_MSG_WRITE)!=OS_NO_ERR) // notify file write
		{
			RETURN(AVIPACKER_RESULT_OS_ERR);
		}
		AviPacker->MsgTx++;
	}

	ret = AVIPACKER_RESULT_OK;
Return:
	OSSemPost(AviPacker->Sem);

	return ret;
}

int AviPackerV3_Close(void *_AviPacker)
{
	AVIPACKER_WORKMEM *AviPacker = (AVIPACKER_WORKMEM *)_AviPacker;
	INT8U err;
	DEBUG_ONLY(DBG_PRINT("AviPackerV3_Close\r\n"));

    OSSemPend(AviPacker->Sem, 0, &err);

    if(err!=OS_NO_ERR) 
    {
        AviPacker_Break_Set(0);  // packer work continue
	    return AVIPACKER_RESULT_OS_ERR;
    }

	if(OSQPost(AviPacker->Msg, (void*)AVIPACKER_MSG_STOP)!=OS_NO_ERR)
    {
        AviPacker_Break_Set(0);  // packer work continue
		return AVIPACKER_RESULT_OS_ERR;
    }

	OSMboxPend(AviPacker->Ack, 0, &err);

    OSSemPost(AviPacker->Sem);

	if(AviPacker->Sem) OSSemDel(AviPacker->Sem, 0, &err);
	if(AviPacker->Ack) OSMboxDel(AviPacker->Ack, 0, &err);
	if(AviPacker->Msg) OSQDel(AviPacker->Msg, 0, &err);
	
	AviPacker->Msg = AviPacker->Ack = AviPacker->Sem = 0;

    AviPacker_Break_Set(0);        
	return AVIPACKER_RESULT_OK;
}

////////////////////////////////////////////////////////////////////////////////////
// Config Header
////////////////////////////////////////////////////////////////////////////////////
#define PutData4(v)	{*(long*)ptr = (v); ptr += 4;}
#define SIZEOF_VESION_JUNK	((sizeof(AVIPACKER_VERSION_STRING) + 3)>>2<<2)
int ConfigHeader(
	unsigned long			*MoviSize,
	char					*ptr,
	MAINAVIHEADER			**MainAviHeader,
	GP_AVI_AVISTREAMHEADER	**VidHdr,
	int						VidFmtLen,
	GP_AVI_BITMAPINFO		**VidFmt,
	GP_AVI_AVISTREAMHEADER	**AudHdr,
	int						AudFmtLen,	
	GP_AVI_PCMWAVEFORMAT	**AudFmt)
{
	int pad1, pad2;
	int SizeOf_RIFF, SizeOf_hdrl, SizeOf_strl_v, SizeOf_strl_a;
	VidFmtLen = (VidFmtLen + 1) >> 1 << 1;
	AudFmtLen = (AudFmtLen + 1) >> 1 << 1;
	pad1 = (4 - (VidFmtLen & 3)) & 3;


	SizeOf_strl_v =
		// strl
		4 + 
			// strh
			8 + sizeof(GP_AVI_AVISTREAMHEADER) + 
			// strf
			8 + VidFmtLen +
			// JUNK (if pad1)
			(pad1 ? 8+pad1 : 0);
	
	if(AudFmtLen)
	{
		pad2 = (4 - (AudFmtLen & 3)) & 3;
		SizeOf_strl_a = 
			// strl
			4 +
				// strh
				8 + sizeof(GP_AVI_AVISTREAMHEADER) + 
				// strf
				8 + AudFmtLen +
				// JUNK (if pad1)
				(pad2 ? 8+pad2 : 0);
	}
	else
	{
		SizeOf_strl_a = 0;
		pad2 = 0;
		*AudFmt = NULL;
	}

	SizeOf_hdrl = 
		// hdrl
		4 +
			// avih
			8 + sizeof(MAINAVIHEADER) +
			// LIST strl
			8 + SizeOf_strl_v +
			// LIST strl
			(SizeOf_strl_a ? 8 + SizeOf_strl_a : 0 )+
			// JUNK version
			8 + SIZEOF_VESION_JUNK;
	
	*MoviSize = 4;

	SizeOf_RIFF = 
		// AVI
		4 +
			// LIST hdrl
			8 + SizeOf_hdrl + 
			// LIST movi
			8 + *MoviSize;

	// RIFF AVI
	PutData4(FOURCC('R','I','F','F'));
	PutData4(SizeOf_RIFF);
	PutData4(FOURCC('A','V','I',' '));
		// LIST hdrl
		PutData4(FOURCC('L','I','S','T'));
		PutData4(SizeOf_hdrl);
		PutData4(FOURCC('h','d','r','l'));
			// avih
			PutData4(FOURCC('a','v','i','h'));
			PutData4(sizeof(MAINAVIHEADER));
				*MainAviHeader = (MAINAVIHEADER*)ptr;
				ptr += sizeof(MAINAVIHEADER);
			
			// LIST strl
			PutData4(FOURCC('L','I','S','T'));
			PutData4(SizeOf_strl_v);
			PutData4(FOURCC('s','t','r','l'));
				// strh
				PutData4(FOURCC('s','t','r','h'));
				PutData4(sizeof(GP_AVI_AVISTREAMHEADER));
				*VidHdr = (GP_AVI_AVISTREAMHEADER*)ptr;
				ptr += sizeof(GP_AVI_AVISTREAMHEADER);
				// strf
				PutData4(FOURCC('s','t','r','f'));
				PutData4(VidFmtLen);
				*VidFmt = (GP_AVI_BITMAPINFO*)ptr;
				ptr += VidFmtLen;
				// JUNK for 4-byte padding if necessary
				if(pad1)
				{
					PutData4(FOURCC('J','U','N','K'));
					PutData4(pad1);
					ptr += pad1;					
				}

			if(SizeOf_strl_a)
			{
			// LIST strl
			PutData4(FOURCC('L','I','S','T'));
			PutData4(SizeOf_strl_a);
			PutData4(FOURCC('s','t','r','l'));
				// strh
				PutData4(FOURCC('s','t','r','h'));
				PutData4(sizeof(GP_AVI_AVISTREAMHEADER));
				*AudHdr = (GP_AVI_AVISTREAMHEADER*)ptr;
				ptr += sizeof(GP_AVI_AVISTREAMHEADER);
				// strf
				PutData4(FOURCC('s','t','r','f'));
				PutData4(AudFmtLen);
				*AudFmt = (GP_AVI_PCMWAVEFORMAT*)ptr;
				ptr += AudFmtLen;
				// JUNK for 4-byte padding if necessary
				if(pad2)
				{
					PutData4(FOURCC('J','U','N','K'));
					PutData4(pad2);
					ptr += pad2;					
				}
			}
			
			
			// JUNK for version information
			PutData4(FOURCC('J','U','N','K'));
			PutData4(SIZEOF_VESION_JUNK);
			MemCpyAlign4(ptr, AVIPACKER_VERSION_STRING, SIZEOF_VESION_JUNK); ptr += SIZEOF_VESION_JUNK;

		// LIST movi
		PutData4(FOURCC('L','I','S','T'));
		PutData4(*MoviSize);
		PutData4(FOURCC('m','o','v','i'));
	
	return SizeOf_RIFF + 8;
}



const char *AviPackerV3_GetVersion(void)
{
	return AVIPACKER_VERSION_STRING;
}


int AviPackerV3_AddInfoStr(void *_AviPacker, const char *fourcc, const char *str)
{
	AVIPACKER_WORKMEM *AviPacker = (AVIPACKER_WORKMEM *)_AviPacker;
	int len;
	long *ptr;
	const long *str4 = (const long*)str;
	if(str==0) return 0;
	len = StrLen(str);
	len = (len + 3) >> 2 << 2;
	if(len<=0) return 0;
	if(AviPacker->info_len + 8 + len > INFO_LIST_BUF_SIZE) return 0;
	
	ptr = (long*)(AviPacker->info_data + AviPacker->info_len);
	*ptr++ = *(long*)fourcc;
	*ptr++ = len;
	AviPacker->info_len += 8 + len;
	
	len >>= 2;
	while(len--)
	{
		*ptr++ = *str4++;
	}
	return 1;
}


int AviPackerV3_GetWorkMemSize(void)
{
	return sizeof(AVIPACKER_WORKMEM);
}

void VdoFramNumsLowBoundReg(unsigned int fix_frame_cnts)
{
    fixed_frame_nums = fix_frame_cnts;
}

unsigned long current_movie_Byte_size_get(void)
{
    return movie_size;
}

void AviPacker_Break_Set(INT8U Break1_Work0)
{   
    packer_suspend = Break1_Work0;    
}

