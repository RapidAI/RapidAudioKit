#include "precomp.h"
#ifdef __cplusplus

extern "C"
{

#endif



	__QMVAD_API QMAVD_HANDLE QmVad_init()
	{

		VadInst* handle = WebRtcVad_Create();

		if (WebRtcVad_Init(handle)) {

			WebRtcVad_Free(handle);
			return NULL;
		}
		return handle;
	}

	__QMVAD_API void QmVad_Final(QMAVD_HANDLE Handle)
	{

		WebRtcVad_Free((VadInst * )Handle);
	}

	__QMVAD_API int QmVad_SetMode(QMAVD_HANDLE Handle, int Mode)
	{

		VadInst* handle = (VadInst * )Handle;

		if (!handle)
			return -1;
		return WebRtcVad_set_mode(handle, Mode);

	}
	
	__QMVAD_API int QmVad_ValidRateAndFrameLength(int rate, size_t frame_length)
	{

		if (WebRtcVad_ValidRateAndFrameLength(rate, frame_length))
		{
			return -1;
		}
		else {
			return 0;
		}

	}

	__QMVAD_API QMVAD_STATUS QmVad_Process(QMAVD_HANDLE Handle, long fs, int16_t * audio_frame_buf,long frame_length)
	{

		VadInst* handle = (VadInst*)Handle;

		if (!handle)
			return QVS_ERROR;

		int result = WebRtcVad_Process(handle, fs, audio_frame_buf,	frame_length);
		switch (result) {
		case 1:
			return QVS_ACTIVE;
			break;
		case 0:
			return QVS_SILENT;
			break;
		case -1:
			return QVS_ERROR; //error
		}

		return QVS_ERROR;

	}

#ifdef __cplusplus

}

#endif