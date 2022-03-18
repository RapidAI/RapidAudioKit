#pragma once 

typedef std::vector<std::vector<int>>  QMAV_FRAMES;
size_t  QmAvkit_SeekPCMOffset(FILE* fp, WAVE_FMT& Format);

bool QmAvkit_SplitFrame(const char* szSrcWavFile, const char* szTmpFile, WAVE_FMT & Format, QMAV_FRAMES& Frames, int nFrameSizeMS);