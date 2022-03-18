#pragma once

#define AVCODEC_MAX_AUDIO_FRAME_SIZE   192000
//下面这四个结构体是为了分析wav头的
typedef struct {
    uint32_t          ChunkID; //内容为"RIFF"
    uint32_t          ChunkSize;  //存储文件的字节数（不包含ChunkID和ChunkSize这8个字节）
    uint32_t          Format;  //内容为"WAVE“
} WAVE_HEADER;

typedef struct {
    uint32_t  Subchunk1ID; //内容为"fmt"
    uint32_t  Subchunk1Size;  //存储该子块的字节数（不含前面的Subchunk1ID和Subchunk1Size这8个字节）
    uint16_t AudioFormat;    //存储音频文件的编码格式，例如若为PCM则其存储值为1。
    uint16_t NumChannels;    //声道数，单声道(Mono)值为1，双声道(Stereo)值为2，等等
    uint32_t  SampleRate;     //采样率，如8k，44.1k等
    uint32_t  ByteRate;       //每秒存储的bit数，其值 = SampleRate * NumChannels * BitsPerSample / 8
    uint16_t BlockAlign;     //块对齐大小，其值 = NumChannels * BitsPerSample / 8
    uint16_t BitsPerSample;  //每个采样点的bit数，一般为8,16,32等。
} WAVE_FMT;

typedef struct {
    uint32_t          Subchunk2ID; //内容为“data”
    uint32_t          Subchunk2Size;  //接下来的正式的数据部分的字节数，其值 = NumSamples * NumChannels * BitsPerSample / 8
} WAVE_DATA;



typedef struct {
    short channels;
    short bits;
    int   rate;
    int  format;

}head_pama;

int get_wave_header_size();
int  wav_write_header(FILE* outfile, head_pama& pt, const int datasize);
int  GetAudioFormatType(AVSampleFormat format);

#define COMPOSE_ID(a,b,c,d) ((a) | ((b)<<8) | ((c)<<16) | ((d)<<24))
#define WAV_RIFF COMPOSE_ID('R','I','F','F')
#define WAV_WAVE COMPOSE_ID('W','A','V','E')
#define WAV_FMT COMPOSE_ID('f','m','t',' ')
#define WAV_DATA COMPOSE_ID('d','a','t','a')