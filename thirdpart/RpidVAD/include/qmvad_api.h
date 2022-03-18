#pragma once 

#include <stdint.h>
#ifndef _WIN32
#include <stddef.h>
#endif
#ifdef _WIN32
#ifdef _EXPORT_QMVAD_API
#define __QMVAD_API  __declspec(dllexport)
#else
#define __QMVAD_API  __declspec(dllimport)
#endif

#else
#define __QMVAD_API 
#endif


typedef void* QMAVD_HANDLE;

typedef enum
{

	QVS_SILENT = 0,
	QVS_ACTIVE = 1,
	QVS_ERROR = -1,

}QMVAD_STATUS;
#ifdef __cplusplus

extern "C"
{

#endif

// PCM Buffersize=采样率*采样时间*采样位深/8*通道数（Bytes）

#define QMVAD_GET_FRAME_SIZE(HZ,BYTEDEEP,DATA_TYPE,MS)  	( (HZ) * (MS) * (BYTEDEEP /( sizeof(DATA_TYPE)*8) ) * 1 / 1000   ) // bytes

	__QMVAD_API QMAVD_HANDLE QmVad_init();


	__QMVAD_API void QmVad_Final(QMAVD_HANDLE Handle);


	// Mode  0,1,2,3  , 3 is the strongest
	
	__QMVAD_API int QmVad_SetMode(QMAVD_HANDLE Handle, int Mode);

	//  rate: sample rate,   8000, 16000 , 32000 or 48000;   time: 10, 20 or 30 ms frames

	//  16000hz , 10ms/20ms/30ms  length 160, 320, 480 
	// for 8000hz, valid length for 10ms,20ms,30ms is: 80,160,240

	__QMVAD_API int QmVad_ValidRateAndFrameLength(int rate, size_t frame_length);
	
	// fs: sample rate,   8000, 16000 , 32000   
	__QMVAD_API QMVAD_STATUS QmVad_Process(QMAVD_HANDLE Handle, long fs, int16_t* audio_frame_buf, long frame_length);


#ifdef __cplusplus
}
#endif