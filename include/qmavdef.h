#pragma once

#define AVCODEC_MAX_AUDIO_FRAME_SIZE   192000
//�������ĸ��ṹ����Ϊ�˷���wavͷ��
typedef struct {
    uint32_t          ChunkID; //����Ϊ"RIFF"
    uint32_t          ChunkSize;  //�洢�ļ����ֽ�����������ChunkID��ChunkSize��8���ֽڣ�
    uint32_t          Format;  //����Ϊ"WAVE��
} WAVE_HEADER;

typedef struct {
    uint32_t  Subchunk1ID; //����Ϊ"fmt"
    uint32_t  Subchunk1Size;  //�洢���ӿ���ֽ���������ǰ���Subchunk1ID��Subchunk1Size��8���ֽڣ�
    uint16_t AudioFormat;    //�洢��Ƶ�ļ��ı����ʽ��������ΪPCM����洢ֵΪ1��
    uint16_t NumChannels;    //��������������(Mono)ֵΪ1��˫����(Stereo)ֵΪ2���ȵ�
    uint32_t  SampleRate;     //�����ʣ���8k��44.1k��
    uint32_t  ByteRate;       //ÿ��洢��bit������ֵ = SampleRate * NumChannels * BitsPerSample / 8
    uint16_t BlockAlign;     //������С����ֵ = NumChannels * BitsPerSample / 8
    uint16_t BitsPerSample;  //ÿ���������bit����һ��Ϊ8,16,32�ȡ�
} WAVE_FMT;

typedef struct {
    uint32_t          Subchunk2ID; //����Ϊ��data��
    uint32_t          Subchunk2Size;  //����������ʽ�����ݲ��ֵ��ֽ�������ֵ = NumSamples * NumChannels * BitsPerSample / 8
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