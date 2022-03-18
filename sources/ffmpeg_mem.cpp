#include "precomp.h"


CAvExtractor::CAvExtractor(uint8_t* buf, int buf_size)
{


}

CAvExtractor::CAvExtractor(const char * szFileName)
{


}


CAvExtractor::~CAvExtractor()
{

    CloseAV();

}


bool CAvExtractor::decidemp3format(const char* inputfilename)
{
    FILE* fp;
    char id[3];

    fp = fopen(inputfilename, "rb+");
    if (fp == NULL)
    {
        return 1;
    }
    fread(id, 1, 3, fp);

    fclose(fp);
    if ((id[0] == 'I' || id[0] == 'i')
        && (id[1] == 'D' || id[1] == 'd')
        && (id[2] == '3'))
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool CAvExtractor::OpenFile(const char* szFileName)
{
    int ret = -1;

    ret = avformat_open_input(&m_fmt_ctx, szFileName, 0, 0);

    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "can't open file.\n");
        return false;
    }
    return true;
}

void CAvExtractor::CloseAV()
{
    if(m_fmt_ctx)
        avformat_close_input(&m_fmt_ctx);
}
//用来将内存buffer的数据拷贝到buf
int CAvExtractor::read_packet(void* opaque, uint8_t* buf, int buf_size)
{
    struct buffer_data* bd = (struct buffer_data*)opaque;
    buf_size = FFMIN(buf_size, bd->size);

    if (!buf_size)
        return AVERROR_EOF;
   // printf("ptr:%p size:%zu bz%zu\n", bd->ptr, bd->size, buf_size);

    /* copy internal buffer data to buf */
    memcpy(buf, bd->ptr, buf_size);
    bd->ptr += buf_size;
    bd->size -= buf_size;

    return buf_size;
}

/* 打开前端传来的视频buffer */
int CAvExtractor::open_input_buffer(uint8_t* buf, int len)
{
    unsigned char* avio_ctx_buffer = NULL;
    size_t avio_ctx_buffer_size = 32768;

    const AVInputFormat* in_fmt = av_find_input_format("h265");

    m_bd.ptr = buf;  /* will be grown as needed by the realloc above */
    m_bd.size = len; /* no data at this point */

    m_fmt_ctx = avformat_alloc_context();

    avio_ctx_buffer = (unsigned char*)av_malloc(avio_ctx_buffer_size);

    /* 读内存数据 */
    auto avio_ctx = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size, 0, &m_bd, read_packet, NULL, NULL);

    m_fmt_ctx->pb = avio_ctx;
    m_fmt_ctx->flags = AVFMT_FLAG_CUSTOM_IO;

    /* 打开内存缓存文件, and allocate format context */
    if (avformat_open_input(&m_fmt_ctx, "", in_fmt, NULL) < 0)
    {
        fprintf(stderr, "Could not open input\n");
        return -1;
    }
    return 0;
}
// save as wav : https://blog.csdn.net/bixinwei22/article/details/78604360
// https://www.cnblogs.com/lifexy/p/13639409.html
bool CAvExtractor::GetAudioStream(const char * szOutFileName, unsigned char * szOutBuf, int & nOutSize)
{
    

    int data_size = 0;
    head_pama pt = { 0 };
    FILE* dst_fd;

    dst_fd = fopen(szOutFileName, "wb");
    if (!dst_fd)
        return false;
    int nHeaderSize = get_wave_header_size();
    if (fseek(dst_fd, nHeaderSize, SEEK_SET) != 0)
        return false;

    int ret = -1;
    int audio_index =-1;
    
    // 检查文件中的流信息
    ret = avformat_find_stream_info(m_fmt_ctx, NULL);
    if (ret < 0) {
        //av_log("fail avformat avformat_find_stream_info result %s",   av_err2str(ret));
        return false;
    }
    else {
       // av_log("avformat_find_stream_info success result is %s",  av_err2str(result));
    }

    ret = av_find_best_stream(m_fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "ret = %d\n", ret);

        fclose(dst_fd);
        return false;
    }

    audio_index = ret;
    
   
    AVPacket* packet = av_packet_alloc();
    /*Initialize optional fields of a packet with default values.*/
    

   const  AVCodec* codec = avcodec_find_decoder(m_fmt_ctx->streams[audio_index]->codecpar->codec_id);
   if (codec == NULL) {
       printf("Codec not found.\n");
       fclose(dst_fd);
       return false;
   }

   AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
   codecCtx->thread_count = 1;
   avcodec_parameters_to_context(codecCtx, m_fmt_ctx->streams[audio_index]->codecpar);
   if (avcodec_open2(codecCtx, codec, NULL) < 0) {
        printf("Could not open codec.\n");
        fclose(dst_fd);
        return false ;
    }

   pt.channels =  codecCtx->channels;
   pt.rate = codecCtx->sample_rate;
   pt.bits = av_get_bytes_per_sample(codecCtx->sample_fmt)*8; 
   pt.format = GetAudioFormatType(codecCtx->sample_fmt);

   auto var = codecCtx->bits_per_raw_sample;
   //AV_SAMPLE_FMT_FLTP


    //AVSampleFormat in_sample_fmt = codecCtx->sample_fmt;//输入的采样格式
    //int in_sample_rate = codecCtx->sample_rate;//输入的采样率
    //int channels = codecCtx->channels;

    //int got_frame = 0, framecnt = 0;
   
    AVFrame* frame = av_frame_alloc();
 
 
    
    while (av_read_frame(m_fmt_ctx, packet) >= 0)
    {
        if (packet->stream_index == audio_index)
        {
       
            if(packet->size > 0)
            {
                int len =decode(codecCtx, packet, frame, dst_fd);
               
                data_size += len;
            }

        }
        av_packet_unref(packet);
    }
    /* flush the decoder */
    packet->data = NULL;
    packet->size = 0;
    decode(codecCtx, packet, frame, dst_fd);

    fseek(dst_fd, 0, SEEK_SET);
    wav_write_header(dst_fd, pt, data_size);
  
    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_close(codecCtx);
    fclose(dst_fd);
    return true;
}




int CAvExtractor::decode(AVCodecContext* dec_ctx, AVPacket* pkt, AVFrame* frame, FILE* outfile)
{
    int i, ch;
    int ret, data_size;
    int nTotalSize = 0;

    /* send the packet with the compressed data to the decoder */
    ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) {
        fprintf(stderr, "Error submitting the packet to the decoder\n");
        return -1;
    }
  
    /* read all the output frames (in general there may be any number of them */
    while (ret >= 0 )
    {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return nTotalSize;
        else if (ret < 0) {
            fprintf(stderr, "Error during decoding\n");
            return -1;
        }

        data_size = av_get_bytes_per_sample(dec_ctx->sample_fmt);
        if (data_size < 0) {
            /* This should not occur, checking just for paranoia */
            fprintf(stderr, "Failed to calculate data size\n");
            return false;
        }
       
        
        if (dec_ctx->sample_fmt == AV_SAMPLE_FMT_S16) // AV_SAMPLE_FMT_S16 所有声道的数据放在一个buffer中，左右声道采样点交叉存放，每个采样值为一个signed 16位(范围为-32767 to +32767)
        {
            int linesize = frame->linesize[0];
            int nwrite = fwrite(frame->data[0], 1, linesize, outfile);
            nTotalSize += nwrite;
        }
        else
        {
            for (i = 0; i < frame->nb_samples; i++)
                for (ch = 0; ch < dec_ctx->channels; ch++)
                {
                    fwrite(frame->data[ch] + data_size * i, 1, data_size, outfile); //  https://blog.csdn.net/zhuweigangzwg/article/details/53335941
                    nTotalSize += data_size;
                }
        }

    }

    return nTotalSize;
}

