#include "precomp.h"

// 15s
#define QMAVKIT_MAX_LENG 15000  

#ifdef __cplusplus 

extern "C" {
#endif

	_QMAVKIT_API QMAVHANDLER QmAVKitInit()
	{

		return new CAvExtractor;
	}
	_QMAVKIT_API int  QmAVKitExtractFromFile(QMAVHANDLER Handle,const char* szSrcFile, const char* szDstFile)
	{
		CAvExtractor* pAvClass = (CAvExtractor*)Handle;
		if (!pAvClass)
			return -1;

		if (!pAvClass->OpenFile(szSrcFile))
			return -1;
		int nOutLen = 0;
		pAvClass->GetAudioStream(szDstFile, NULL, nOutLen);

		return 0;

	}
	_QMAVKIT_API int  QmAVKitExtractFromMem(QMAVHANDLER Handle, const unsigned char* SrcBuf, int nSrcLen, const char* szDstFile)
	{

		return 0;

	}

	_QMAVKIT_API int QmAVKitResample(const char* szSrcWAVFile, const char* szDstWavFile)
	{

		return QmResqmple(szSrcWAVFile, szDstWavFile);
		

	}

	_QMAVKIT_API void QmAVKitFinal(QMAVHANDLER Handle)
	{

		CAvExtractor* pAvClass = (CAvExtractor*)Handle;
		if (!pAvClass)
			return ;

		delete pAvClass;

	}






	_QMAVKIT_API int QmAVKitCutAudidoIntoFile(const char* szSrcWavFile, int startMS, int LengthMS, const char* szDstFile)
	{


		FILE* srcFP = fopen(szSrcWavFile, "rb");
		FILE* dstFP = fopen(szDstFile, "wb");
		if (srcFP == nullptr || dstFP == nullptr)
		{

			fclose(srcFP);
			fclose(dstFP);
			return -1;

		}

	

		int HeaderSize = get_wave_header_size();
		fseek(dstFP, HeaderSize, SEEK_SET);
	
		size_t nDataLen = 0;

		WAVE_FMT Format;

		if (!(nDataLen=QmAvkit_SeekPCMOffset(srcFP, Format)))
		{
			fclose(srcFP);
			fclose(dstFP);
			return -1;
		}

		if (Format.NumChannels != 1 || Format.SampleRate != 16000 || Format.BitsPerSample != 16)
		{
			fclose(srcFP);
			fclose(dstFP);
			return -1;
		}

		size_t nOffset = QMVAD_GET_FRAME_SIZE(Format.SampleRate, Format.BitsPerSample, uint16_t, startMS);
		size_t nLen= QMVAD_GET_FRAME_SIZE(Format.SampleRate, Format.BitsPerSample, uint16_t, LengthMS); // the number of unint16_t
		if (nLen <= 0)
		{
			fclose(srcFP);
			fclose(dstFP);
			return -1;
		}
		uint16_t* pData = new uint16_t[nLen];

		if(nOffset>0)
			fseek(srcFP, nOffset*sizeof(uint16_t), SEEK_CUR);
		size_t nRead = fread(pData, 1, sizeof(uint16_t) * nLen, srcFP);
		if (nRead > 0)
		{
			fwrite(pData, 1, sizeof(uint16_t) * nLen, dstFP);
		}

		fseek(dstFP, 0, SEEK_SET);
		head_pama hp;
		hp.bits = Format.BitsPerSample;
		hp.channels = 1;
		hp.format = Format.AudioFormat;
		hp.rate = Format.SampleRate;
		wav_write_header(dstFP, hp, sizeof(uint16_t) * nLen);
		delete[]pData;
	
		fclose(srcFP);
		fclose(dstFP);
		return 0;
	}
	_QMAVKIT_API void QmAVKitFreeBuffer(void* buf)
	{
		if(buf)
			delete buf;
	}
	_QMAVKIT_API int  QmAVKitCutAudidoAll(const char* szSrcWavFile, int LengthMS, const char * szTmpFile, const char* szDstDir)
	{


		if (LengthMS < 1000)// 小于1秒
			return -1; 
		
		QMAV_FRAMES Frames;
		WAVE_FMT Format;
		int nPerFrameLen = 30; // 30ms
		if (QmAvkit_SplitFrame(szSrcWavFile, szTmpFile, Format, Frames, nPerFrameLen))
			return -2;

		FILE* tmpFP = fopen(szTmpFile, "rb");

		if (tmpFP == nullptr)
			return -3;


		size_t nFrameDataLen = QMVAD_GET_FRAME_SIZE(Format.SampleRate, Format.BitsPerSample, uint16_t, nPerFrameLen);
		nFrameDataLen *= sizeof(uint16_t);


		unsigned char* pFrameBuf = new unsigned char[nFrameDataLen];
		//int nFrameNumber = Frames.size();

		int nTotalTime = 0;
		int nFileCount = 0;

		char szFileName[512] = { 0 };

		int nDataLen = 0;
		int nTotalFrames = 0;
		int nHeaderSize = get_wave_header_size();
		for (auto& item : Frames)
		{

			int nFrames = item[1]-item[0];
			int nTime = nFrames * nPerFrameLen; // ms

			nTotalTime += nTime;

			nTotalFrames += nFrames;

		
			if (nTotalTime > LengthMS - 500) // 大于 LengthMS -500
			{


				
				sprintf(szFileName, "%s/section%d.wav", szDstDir, nFileCount++);

				FILE* fp = fopen(szFileName, "wb");
				if (!fp)
				{
					printf("cannot open file: %s\n", szFileName);
					break;
				}

				fseek(fp, nHeaderSize, SEEK_SET);
				if (fp != nullptr)
				{
					nDataLen = nTotalFrames * nFrameDataLen;
					for (int i = 0; i < nTotalFrames; i++)
					{
						
						if (fread(pFrameBuf, 1, nFrameDataLen, tmpFP) == nFrameDataLen)
						{
							if (fwrite(pFrameBuf, 1, nFrameDataLen, fp) != nFrameDataLen)
								printf("write error!\n");
						}
					}

					
					head_pama hp;
					hp.bits = Format.BitsPerSample;
					hp.channels = Format.NumChannels;
					hp.format = Format.AudioFormat;
					hp.rate = Format.SampleRate;
					fseek(fp, 0, SEEK_SET);
					wav_write_header(fp, hp, nDataLen);

					fclose(fp);
				}
				printf("the section is : %d ,filename: %s\n", nTotalTime, szFileName);

				

				nTotalTime = 0;
				nTotalFrames = 0;
				nDataLen = 0;
			}

			

		}

		fclose(tmpFP);

		delete  []pFrameBuf;
		return 0;
	}
#ifdef __cplusplus 

}
#endif