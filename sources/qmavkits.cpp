#include "precomp.h"

size_t  QmAvkit_SeekPCMOffset(FILE* fp, WAVE_FMT & Format)
{

	WAVE_HEADER Header;
	if (fread(&Header, 1,sizeof(Header), fp) != sizeof(Header))
		return 0;

	if (Header.ChunkID != WAV_RIFF || Header.Format != WAV_WAVE)
	{
		return 0;

	}

	
	if (fread(&Format, 1,sizeof(Format), fp) != sizeof(Format))
		return 0;

	if (Format.Subchunk1ID != WAV_FMT)
		return 0;


	WAVE_DATA  Data;

	if (fread(&Data, 1, sizeof(Data), fp) != sizeof(Data))
		return false;

	size_t nDataLen = Data.Subchunk2Size;

	fseek(fp, 0-nDataLen, SEEK_END);

	return nDataLen;
}


bool QmAvkit_SplitFrame(const char* szSrcWavFile, const char* szTmpFile, WAVE_FMT &Format, QMAV_FRAMES& Frames, int nFrameSizeMS)
{

	FILE* srcFP = fopen(szSrcWavFile, "rb");
	FILE* dstFP = fopen(szTmpFile, "wb");
	if (srcFP == nullptr || dstFP == nullptr)
	{

		fclose(srcFP);
		fclose(dstFP);
		return -1;

	}


	size_t nDataLen = 0;



	if (!(nDataLen = QmAvkit_SeekPCMOffset(srcFP, Format)))
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


	
	size_t nFrameLen = QMVAD_GET_FRAME_SIZE(Format.SampleRate, Format.BitsPerSample, uint16_t, nFrameSizeMS); // the number of unint16_t

	size_t nFrameCount = 0;

	int16_t* pData = new  int16_t[nFrameLen];
		 

	auto Handle = QmVad_init();

	QmVad_SetMode(Handle, 3);

	int nStartPos = -1, nEndPos=-1;
	int nCount = 0;
	while (!feof(srcFP))
	{
		size_t nRead = fread(pData, 1, sizeof(uint16_t) * nFrameLen, srcFP);

		if (nRead == sizeof(uint16_t) * nFrameLen)
		{
			auto Status = QmVad_Process(Handle,Format.SampleRate, pData, nFrameLen);

			if (Status == QVS_ACTIVE)
			{
				int nWrite = fwrite(pData, 1, sizeof(uint16_t) * nFrameLen, dstFP);

				if (nStartPos == -1)
				{
			
				nStartPos = nCount;
				}

				nCount++;

			}
			else
			if (Status == QVS_SILENT)
			{

				if (nStartPos > -1)
				{
					nEndPos = nCount;

					std::vector<int> FramePos;
					FramePos.push_back(nStartPos);
					FramePos.push_back(nEndPos);
					Frames.push_back(FramePos);
					nStartPos = -1;
					nEndPos = -1;

				}
			}
			else
			{
				printf("error!");
			}
		
		}


	


	}
	
	QmVad_Final(Handle);

	fclose(srcFP);
	fclose(dstFP);
	return 0;


}