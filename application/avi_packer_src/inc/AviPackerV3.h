#ifndef __AVIPACKER_H__
#define __AVIPACKER_H__

#include "application.h"

#define AVIPACKER_RESULT_OK						0
#define AVIPACKER_RESULT_PARAMETER_ERROR		0x80000000

#define AVIPACKER_RESULT_FILE_OPEN_ERROR		0x80000001
#define AVIPACKER_RESULT_BS_BUF_TOO_SMALL		0x80000002
#define AVIPACKER_RESULT_BS_BUF_OVERFLOW		0x80000003
#define AVIPACKER_RESULT_FILE_WRITE_ERR			0x80000004
#define AVIPACKER_RESULT_FILE_SEEK_ERR			0x80000005

#define AVIPACKER_RESULT_MEM_ALIGN_ERR			0x80000006
#define AVIPACKER_RESULT_OS_ERR					0x80000007

#define AVIPACKER_RESULT_IDX_FILE_OPEN_ERROR	0x80000008
#define AVIPACKER_RESULT_IDX_BUF_TOO_SMALL		0x80000009
#define AVIPACKER_RESULT_IDX_BUF_OVERFLOW		0x8000000A
#define AVIPACKER_RESULT_IDX_FILE_WRITE_ERR		0x8000000B
#define AVIPACKER_RESULT_IDX_FILE_SEEK_ERR		0x8000000C
#define AVIPACKER_RESULT_IDX_FILE_READ_ERR		0x8000000D

#define AVIPACKER_RESULT_FILE_READ_ERR			0x8000000E
#define AVIPACKER_RESULT_IGNORE_CHUNK			0x8000000F

#define AVIPACKER_RESULT_FRAME_OVERFLOW			0x80000010

#define AVIPACKER_RESULT_FILE_CAT_ERR			0x80000011  // dominant add

#ifndef AVIIF_KEYFRAME
#define AVIIF_KEYFRAME							0x00000010
#endif // AVIIF_KEYFRAME


typedef struct
{
	unsigned short	left;
	unsigned short	top;
	unsigned short	right;
	unsigned short	bottom;
} GP_AVI_RECT;

typedef struct
{
	unsigned char	fccType[4];
	unsigned char	fccHandler[4];
	unsigned int	dwFlags;
	unsigned short	wPriority;
	unsigned short	wLanguage;
	unsigned int	dwInitialFrames;
	unsigned int	dwScale;
	unsigned int	dwRate;
	unsigned int	dwStart;
	unsigned int	dwLength;
	unsigned int	dwSuggestedBufferSize;
	unsigned int	dwQuality;
	unsigned int	dwSampleSize;
	GP_AVI_RECT	rcFrame;
} GP_AVI_AVISTREAMHEADER;	// strh

typedef struct
{
	unsigned int	biSize;
	unsigned int	biWidth;
	unsigned int	biHeight;
	unsigned short	biPlanes;
	unsigned short	biBitCount;
	unsigned char	biCompression[4];
	unsigned int	biSizeImage;
	unsigned int	biXPelsPerMeter;
	unsigned int	biYPelsPerMeter;
	unsigned int	biClrUsed;
	unsigned int	biClrImportant;
	// unsigned int	Unknown[7];
} GP_AVI_BITMAPINFO;	// strf

typedef struct
{
	unsigned short	wFormatTag;
	unsigned short	nChannels;
	unsigned int	nSamplesPerSec;
	unsigned int	nAvgBytesPerSec;
	unsigned short	nBlockAlign;
	unsigned short	wBitsPerSample;
	//unsigned short	cbSize;				// useless when wFormatTag is WAVE_FORMAT_PCM
} GP_AVI_PCMWAVEFORMAT;	// strf



/////////////////////////////////////////////////////////////////////////////
int AviPackerV3_Open(
	void *WorkMem,
	int								fid,			// Record file ID
	int								fid_idx,		// Temporary file ID for IDX	
	const GP_AVI_AVISTREAMHEADER	*VidHdr,		// Video stearm header
	int								VidFmtLen,		// Size of video stream format, count in byte
	const GP_AVI_BITMAPINFO			*VidFmt,		// Video stream format
	const GP_AVI_AVISTREAMHEADER	*AudHdr,		// Audio stearm header = NULL if no audio stream
	int								AudFmtLen,		// Size of audio stream format, count in byte. If zero => no audio stream
	const GP_AVI_PCMWAVEFORMAT		*AudFmt,		// Audio stream format. = NULL if no audio stream
	unsigned char					prio,			// Task priority
	void							*pRingBuf,		// File write buffer
	int								RingBufSize,	// File write buffer size, count in byte
	void							*pIdxRingBuf,	// Index buffer
	int								IdxRingBufSize);// Index buffer size, count in byte
	
/*---------------------------------------------------------------------------
Description:
	open an AVI file for AVI recoder

Return Value:
	AVIPACKER_RESULT_OK
	AVIPACKER_RESULT_FILE_OPEN_ERROR
	AVIPACKER_RESULT_IDX_FILE_OPEN_ERROR
	AVIPACKER_RESULT_MEM_ALIGN_ERR
	AVIPACKER_RESULT_OS_ERR
	AVIPACKER_RESULT_FILE_WRITE_ERR
////////////////////////////////////////////////////////////////////////// */





/////////////////////////////////////////////////////////////////////////////
int AviPackerV3_PutData(
	void *WorkMem,
	unsigned long fourcc,	// Chunk name of AVI stream '00dc' or '01wb'
	long cbLen,				// number of byte to write
	const void *ptr,		// pointer to write data, MUST be 4-byte alignment
	int nSamples,			// number of samples in this chunk of this stream
	int ChunkFlag);			// CHUNK Flag
/*---------------------------------------------------------------------------
Description
	AviPackerV3_PutData will put audio/video stream data to an AVI file

Return Value:
	If no error occured, return value will be a positive value and it is the
	actual bytes that has been writen into file;
	if error occured, return value will be a negtive value. Following are
	those error:
	1) AVIPACKER_RESULT_PARAMETER_ERROR
	2) AVIPACKER_RESULT_FILE_OPEN_ERROR
	3) AVIPACKER_RESULT_FILE_OVERFLOW
	4) AVIPACKER_RESULT_FRAME_OVERFLOW
////////////////////////////////////////////////////////////////////////// */	





/////////////////////////////////////////////////////////////////////////////
int AviPackerV3_Close(void *WorkMem);
/*---------------------------------------------------------------------------
Description:
	Close an AVI file that has been opened by AviPackerV3_Open
	
Return Value:
	AVIPACKER_RESULT_OK
	AVIPACKER_RESULT_OS_ERR
////////////////////////////////////////////////////////////////////////// */



/////////////////////////////////////////////////////////////////////////////
const char *AviPackerV3_GetVersion(void);
/*---------------------------------------------------------------------------
Description
	AviPackerV3_GetVersion will return version string

Return Value:
	The version string of AviPacker 
////////////////////////////////////////////////////////////////////////// */	


/////////////////////////////////////////////////////////////////////////////
void AviPackerV3_SetErrHandler(void *WorkMem, int (*ErrHandler)(int ErrCode));
/*---------------------------------------------------------------------------
Description
1. If error occured while running AviPacker, AviPacker will call ErrHandler
   back with current error code (ErrCode)
2. ErrHandler should return non-zero value if user wants to continue porcess
   AviPacker; or zero to stop.
////////////////////////////////////////////////////////////////////////// */	


/////////////////////////////////////////////////////////////////////////////
int AviPackerV3_AddInfoStr(void *WorkMem, const char *fourcc, const char *info_string);
/*---------------------------------------------------------------------------
Description
	Add info string to INFO LIST

	fourcc : the type of information string
	info_string : information string

Return Value:
	Return 1 if info_string has been wrote successfully.
////////////////////////////////////////////////////////////////////////// */	

int AviPackerV3_GetWorkMemSize(void);
/*---------------------------------------------------------------------------
Description
	Working memory size of AVI-Packer V3

Return Value:
	Working memory size of AVI-Packer V3 , count in byte
////////////////////////////////////////////////////////////////////////// */	


void VdoFramNumsLowBoundReg(unsigned int fix_frame_cnts);
unsigned long current_movie_Byte_size_get(void);    
/*---------------------------------------------------------------------------
Description
	Working memory size of AVI-Packer V3

Return Value:
	Working memory size of AVI-Packer V3 , count in byte
////////////////////////////////////////////////////////////////////////// */	



#endif // __AVIPACKER_H__
