#include "precomp.h"


int get_wave_header_size()
{

    return sizeof(WAVE_HEADER) + sizeof(WAVE_FMT) + sizeof(WAVE_DATA);
}

int  wav_write_header(FILE* fp, head_pama& pt, const int datasize)
{
    // 野割 WAVE_HEADER

//#define WAV_RIFF COMPOSE_ID('R','I','F','F')
//#define WAV_WAVE COMPOSE_ID('W','A','V','E')
//#define WAV_FMT COMPOSE_ID('f','m','t',' ')
//#define WAV_DATA COMPOSE_ID('d','a','t','a')

    WAVE_HEADER pcmHEADER;
    pcmHEADER.ChunkID = WAV_RIFF;
    pcmHEADER.Format = WAV_WAVE;


    //野割 WAVE_FMT 


    WAVE_FMT pcmFMT;
    pcmFMT.SampleRate = pt.rate;
    pcmFMT.ByteRate = pt.rate * (pt.bits / 8);
    pcmFMT.BitsPerSample = 8 * (pt.bits / 8);
    pcmFMT.Subchunk1ID = WAV_FMT;
    pcmFMT.Subchunk1Size = 16; // 徨翠海業
    pcmFMT.BlockAlign = pt.channels * (pt.bits / 8);
    pcmFMT.NumChannels = pt.channels;
    pcmFMT.AudioFormat = pt.format; //3, float 32  ;  1 int


    //野割 WAVE_DATA;
    WAVE_DATA pcmDATA;
    pcmDATA.Subchunk2ID = WAV_DATA;

    pcmDATA.Subchunk2Size = datasize;
    int headerSize = sizeof(pcmHEADER.Format) + sizeof(WAVE_FMT) + sizeof(WAVE_DATA); // 36
    pcmHEADER.ChunkSize = headerSize + pcmDATA.Subchunk2Size;

    fwrite(&pcmHEADER, sizeof(WAVE_HEADER), 1, fp);
    fwrite(&pcmFMT, sizeof(WAVE_FMT), 1, fp);
    fwrite(&pcmDATA, sizeof(WAVE_DATA), 1, fp);

    return sizeof(WAVE_HEADER) + sizeof(WAVE_FMT) + sizeof(WAVE_DATA);
}

int  GetAudioFormatType(AVSampleFormat format)
{

    /*
    enum AVSampleFormat {
    AV_SAMPLE_FMT_NONE = -1,
    AV_SAMPLE_FMT_U8,          ///< unsigned 8 bits
    AV_SAMPLE_FMT_S16,         ///< signed 16 bits
    AV_SAMPLE_FMT_S32,         ///< signed 32 bits
    AV_SAMPLE_FMT_FLT,         ///< float
    AV_SAMPLE_FMT_DBL,         ///< double

    AV_SAMPLE_FMT_U8P,         ///< unsigned 8 bits, planar
    AV_SAMPLE_FMT_S16P,        ///< signed 16 bits, planar
    AV_SAMPLE_FMT_S32P,        ///< signed 32 bits, planar
    AV_SAMPLE_FMT_FLTP,        ///< float, planar
    AV_SAMPLE_FMT_DBLP,        ///< double, planar

    AV_SAMPLE_FMT_NB           ///< Number of sample formats. DO NOT USE if linking dynamically
};
    */

    switch (format)
    {
    case AV_SAMPLE_FMT_DBL:
    case AV_SAMPLE_FMT_FLTP:
    case AV_SAMPLE_FMT_DBLP:

        return 3; //float

    case AV_SAMPLE_FMT_S16P:
    case AV_SAMPLE_FMT_S32P:
    case AV_SAMPLE_FMT_S16:
    case AV_SAMPLE_FMT_U8:
    case AV_SAMPLE_FMT_S32:

        return 1;

    default:
        return 1;
    }
}