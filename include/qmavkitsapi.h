#pragma once 
#include <stdint.h>

#ifdef WIN32
	#ifdef _QMAVKIT_EXPORT

	#define _QMAVKIT_API __declspec(dllexport)

	#else
	#define _QMAVKIT_API __declspec(dllimport)

	#endif

#else

#define _QMAVKIT_API 
#endif


#ifdef __cplusplus 

extern "C" {
#endif

	typedef void* QMAVHANDLER;

	_QMAVKIT_API QMAVHANDLER QmAVKitInit();
	_QMAVKIT_API int  QmAVKitExtractFromFile(QMAVHANDLER Handle, const char* szSrcFile, const char* szDstFile);
	_QMAVKIT_API int  QmAVKitExtractFromMem(QMAVHANDLER Handle, const unsigned char* SrcBuf, int nSrcLen, const char* szDstFile);
	_QMAVKIT_API int QmAVKitResample(const char* szSrcWAVFile, const char* szDstWavFile);
	_QMAVKIT_API void QmAVKitFinal(QMAVHANDLER Handle);


	// LengthMS maximum: 15s eg. 15000 ms  (QMAVKIT_MAX_LENG)
	
	_QMAVKIT_API int QmAVKitCutAudidoIntoFile(const char* szSrcWavFile, int startMS, int LengthMS, const char * szDstFile);
	_QMAVKIT_API void QmAVKitFreeBuffer(void* buf);
	_QMAVKIT_API int  QmAVKitCutAudidoAll(const char* szSrcWavFile, int LengthMS, const char* szTmpFile, const char* szDstDir);
#ifdef __cplusplus 

}
#endif