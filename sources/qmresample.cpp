#include "precomp.h"



#define LOGD  printf

CAudioResample::CAudioResample(const char* szSrcFile, const char* szDstFile, int dst_ch_layout, int dst_sample_rate, AVSampleFormat dst_fmt)
{

    m_strSrcFile = szSrcFile;
    m_strDstFile = szDstFile;
    m_dst_ch_layout = dst_ch_layout;  // AV_CH_LAYOUT_MONO

    m_dst_fmt = dst_fmt;

    m_bReady=getSrcInfo();
}


bool CAudioResample::getSrcInfo()
{
    // 检查文件中的流信息

    bool bResult = false;
    AVFormatContext* fmt_ctx;
    int ret = -1;
    const  AVCodec* codec;
    AVCodecContext* codecCtx;
    int audio_index;
    ret = avformat_open_input(&fmt_ctx, m_strSrcFile.c_str(), 0, 0);

    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "can't open file.\n");
        return false;
    }
  
    ret = avformat_find_stream_info(fmt_ctx, NULL);
    if (ret < 0) {
        //av_log("fail avformat avformat_find_stream_info result %s",   av_err2str(ret));
        goto END;
    }
    else {
        // av_log("avformat_find_stream_info success result is %s",  av_err2str(result));
    }

    ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "ret = %d\n", ret);

        goto END;
    }

    audio_index = ret;

    codec = avcodec_find_decoder(fmt_ctx->streams[audio_index]->codecpar->codec_id);
    if (codec == NULL) {
        printf("Codec not found.\n");
        goto END;
    }


    codecCtx = avcodec_alloc_context3(codec);
    codecCtx->thread_count = 1;
    avcodec_parameters_to_context(codecCtx, fmt_ctx->streams[audio_index]->codecpar);
    if (avcodec_open2(codecCtx, codec, NULL) < 0) {
        printf("Could not open codec.\n");
        goto END;
    }
    m_src_fmt = codecCtx->sample_fmt;
    m_src_ch_layout = av_get_default_channel_layout(codecCtx->channels); // AV_CH_LAYOUT_STEREO; // codecCtx->channel_layout;
    m_src_rate = codecCtx->sample_rate;
    m_src_bits=av_get_bytes_per_sample(codecCtx->sample_fmt) * 8;

    m_dst_bits = av_get_bytes_per_sample(m_dst_fmt) * 8;
    bResult = true;

 END:

    if(codecCtx)
        avcodec_close(codecCtx);

    if (fmt_ctx)
        avformat_close_input(&fmt_ctx);

    return bResult;

}
//----------------------

/** av_frame_get_buffer()给AVFrame分配的音频内存和av_samples_alloc_array_and_samples()分配的音频内存块的区别
 *  1、前者内存由引用计数管理，而且对于planner方式音频，是两个独立的内存块，内存块首地址不一样；
 *  2、后者内存和普通的内存管理方式一样，需要自己手动释放，对于planner方式音频，它也是一个连续的内存块，只不过每个planner对应的首地址分别存储于
 *  指向的数组指针中；释放由函数av_freep(dataAddr)，dataAddr指的连续内存块的首地址
 */
 /** 重采样实验：
  *  1、44100的采样率，升采样48000或者降采样24000 后对音质影响不大，存储到文件中的数值会变化
  *  2、float类型的采样数据，转换成32位整数型采样数据 对音质影响也不大，存储到文件中的数值会变化
  */
int  CAudioResample::doResample()
{
    int ret = -1;
    FILE* srcFile = fopen(m_strSrcFile.c_str(), "rb+");
    if (srcFile == NULL) {
       // LOGD("fopen srcFile fail");
        return ret;
    }
    FILE* dstFile = fopen(m_strDstFile.c_str(), "wb+");
    if (dstFile == NULL) {
      //  LOGD("fopen dstFile fail");
        return ret;
    }

    int nHeaderSize = get_wave_header_size();

    fseek(dstFile, nHeaderSize, SEEK_SET);

    // 音频数据
    uint8_t** src_data, ** dst_data;
    // 声道数
    int src_nb_channels = 0, dst_nb_channels = 0;
  
    // 采样率
    //int64_t src_rate = 44100, dst_rate = 16000;
    // 采样格式和音频数据存储方式
    //const enum AVSampleFormat src_sample_fmt = AV_SAMPLE_FMT_FLT, dst_sample_fmt = AV_SAMPLE_FMT_S16P;
    int src_nb_samples = 1024, dst_nb_samples, max_dst_nb_samples;
    int src_linesize, dst_linesize;

    // 1、创建上下文SwrContext
    SwrContext* swrCtx;
    swrCtx = swr_alloc();
    if (swrCtx == NULL) {
        LOGD("swr_allock fail");
        return ret;
    }

    // 2、设置重采样的相关参数 这些函数位于头文件 <libavutil/opt.h>
    av_opt_set_int(swrCtx, "in_channel_layout", m_src_ch_layout, 0);
    av_opt_set_int(swrCtx, "in_sample_rate", m_src_rate, 0);
    av_opt_set_sample_fmt(swrCtx, "in_sample_fmt", m_src_fmt, 0);

    av_opt_set_int(swrCtx, "out_channel_layout", m_dst_ch_layout, 0);
    av_opt_set_int(swrCtx, "out_sample_rate", m_dst_rate, 0);
    av_opt_set_sample_fmt(swrCtx, "out_sample_fmt", m_dst_fmt, 0);

    // 3、初始化上下文
  
    ret = swr_init(swrCtx);
    if (ret < 0) {
        LOGD("swr_init fail");
        return ret;
    }

    src_nb_channels = av_get_channel_layout_nb_channels(m_src_ch_layout);
    dst_nb_channels = av_get_channel_layout_nb_channels(m_dst_ch_layout);
    // 根据src_sample_fmt、src_nb_samples、src_nb_channels为src_data分配内存空间，和设置对应的的linesize的值；返回分配的总内存的大小
    int src_buf_size = av_samples_alloc_array_and_samples(&src_data, &src_linesize, src_nb_channels, src_nb_samples, m_src_fmt, 0);
    // 根据src_nb_samples*dst_rate/src_rate公式初步估算重采样后音频的nb_samples大小
    max_dst_nb_samples = dst_nb_samples = (int)av_rescale_rnd(src_nb_samples, m_dst_rate, m_src_rate, AV_ROUND_UP);
    int dst_buf_size = av_samples_alloc_array_and_samples(&dst_data, &dst_linesize, dst_nb_channels, dst_nb_samples,  m_dst_fmt, 0);

    size_t read_size = 0;
    size_t write_size = 0;
    while ((read_size = fread(src_data[0], 1, src_buf_size, srcFile)) > 0) {

        /** 因为有转换时有缓存，所以要不停的调整转换后的内存的大小。估算重采样后的的nb_samples的大小，这里swr_get_delay()用于获取重采样的缓冲延迟
         *  dst_nb_samples的值会经过多次调整后区域稳定
         */
        dst_nb_samples = (int)av_rescale_rnd(swr_get_delay(swrCtx, m_src_rate) + src_nb_samples, m_dst_rate, m_src_rate, AV_ROUND_UP);
        if (dst_nb_samples > max_dst_nb_samples) {
            LOGD("要重新分配内存了");
            // 先释放以前的内存，不管sample_fmt是planner还是packet方式，av_samples_alloc_array_and_samples()函数都是分配的一整块连续的内存
            av_freep(&dst_data[0]);
            // 再重新分配内存
            dst_buf_size = av_samples_alloc_array_and_samples(&dst_data, &dst_linesize, dst_nb_channels, dst_nb_samples, m_dst_fmt, 0);
            max_dst_nb_samples = dst_nb_samples;
        }

        // 开始重采样，重采样后的数据将根据前面指定的存储方式写入ds_data内存块中，返回每个声道实际的采样数
        /**
         *  遇到问题：转换后声音不对
         *  原因分析：swr_convert()函数返回的result是实际转换的采样个数，该值肯定小于等于预计采样数dst_nb_samples，所以写入文件的时候不能用dst_nb_samples的
         *  值，而应该用result值
         **/
        int result = swr_convert(swrCtx, dst_data, dst_nb_samples, (const uint8_t**)src_data, src_nb_samples);
        if (result < 0) {
            LOGD("swr_convert fail %d", result);
            break;
        }
        LOGD("read_size %d dst_nb_samples %d src_nb_samples %d result %d", read_size, dst_nb_samples, src_nb_samples, result);
        // 将音频数据写入pcm文件
        if (av_sample_fmt_is_planar(m_dst_fmt)) {  // planner方式，而pcm文件写入时一般都是packet方式，所以这里要注意转换一下
            int size = av_get_bytes_per_sample(m_dst_fmt);
            // 这里必须是result，而不能用dst_nb_samples，因为result才代表此次实际转换的采样数 它肯定小于等于dst_nb_samples
            for (int i = 0; i < result; i++) {
                for (int j = 0; j < dst_nb_channels; j++) {
                    write_size+= fwrite(dst_data[j] + i * size, 1, size, dstFile);
                }
            }
        }
        else {
            // 最后一个参数必须为1 否则会因为cpu对齐算出来的大小大于实际的数据大小 导致多写入数据 从而造成错误
            dst_buf_size = av_samples_get_buffer_size(&dst_linesize, dst_nb_channels, result, m_dst_fmt, 1);
            write_size+= fwrite(dst_data[0], 1, dst_buf_size, dstFile);
        }
    }

    // 还有剩余的缓存数据没有转换，第三个传递NULL，第四个传递0即可将缓存中的全部取出
    do {
        int real_nb_samples = swr_convert(swrCtx, dst_data, dst_nb_samples, NULL, 0);
        if (real_nb_samples <= 0) {
            break;
        }
        LOGD("余量 %d", real_nb_samples);
        if (av_sample_fmt_is_planar(m_dst_fmt)) {
            int size = av_get_bytes_per_sample(m_dst_fmt);
            for (int i = 0; i < real_nb_samples; i++) {
                for (int j = 0; j < dst_nb_channels; j++) {
                    write_size+=fwrite(dst_data[j] + i * size, 1, size, dstFile);
                }
            }
        }
        else {
            int size = av_samples_get_buffer_size(NULL, dst_nb_channels, real_nb_samples, m_dst_fmt, 1);
            write_size+=fwrite(dst_data[0], 1, size, dstFile);
        }

    } while (true);

    // 释放资源
    av_freep(&src_data[0]);
    av_freep(&dst_data[0]);
    swr_free(&swrCtx);
    fclose(srcFile);
    // write header
    head_pama hpInfo;

    hpInfo.format = GetAudioFormatType(m_src_fmt);
    hpInfo.channels = dst_nb_channels;
    hpInfo.rate = m_dst_rate;
    hpInfo.bits = m_dst_bits;
    fseek(dstFile, 0, SEEK_SET);
    wav_write_header(dstFile, hpInfo, write_size);
    fclose(dstFile);

    return ret;
}


int  CAudioResample::doResampleAVFrame()
{
    int ret = -1;
    uint64_t src_channel_layout = AV_CH_LAYOUT_STEREO, dst_channel_layout = AV_CH_LAYOUT_STEREO;
    int src_nb_channels = av_get_channel_layout_nb_channels(src_channel_layout);
    int dst_nb_channels = av_get_channel_layout_nb_channels(dst_channel_layout);
    int64_t src_nb_samples = 1024, dst_nb_samples, max_dst_nb_samples;
    //  int64_t src_sample_rate = 44100, dst_sample_rate = 44100;
    //const enum AVSampleFormat src_sample_fmt = AV_SAMPLE_FMT_FLT, dst_sample_fmt = AV_SAMPLE_FMT_S32P;

    FILE* srcFile, * dstFile;
    if ((srcFile = fopen(m_strSrcFile.c_str(), "rb")) == NULL) {
        LOGD("open srcpcm fail");
        return ret;
    }
    if ((dstFile = fopen(m_strDstFile.c_str(), "wb+")) == NULL) {
        LOGD("open dstpcm fail");
        return ret;
    }

    int nHeaderSize = get_wave_header_size();

    fseek(dstFile, nHeaderSize, SEEK_SET);

    

    AVFrame* srcFrame, * dstFrame;
    srcFrame = av_frame_alloc();
    srcFrame->nb_samples = (int)src_nb_samples;
    srcFrame->format = m_src_fmt;
    srcFrame->sample_rate = (int)m_src_rate;
    srcFrame->channel_layout = src_channel_layout;
    srcFrame->channels = src_nb_channels;
    ret = av_frame_get_buffer(srcFrame, 0);
    if (ret < 0) {
        LOGD("av_frame_get_buffer() return faile %d", ret);
        return ret;
    }
    ret = av_frame_make_writable(srcFrame);
    if (ret < 0) {
        LOGD("av_frame_make_writable() fail %d", ret);
        return ret;
    }

    // 采样数跟采样率有关 目标采样数根据由采样率根据比例公式计算出
    // R2 = R1*(β2/β1) R1 称作参考源 与R2是同一类型数据，β1、β2称作比例因子，影响系数。R1，β2，β1一次对应这个公式的第一二三个系数
    max_dst_nb_samples = dst_nb_samples = (int)av_rescale_rnd(src_nb_samples, m_dst_rate, m_src_rate, AV_ROUND_UP);

    dstFrame = av_frame_alloc();
    dstFrame->sample_rate = (int)m_dst_rate;
    dstFrame->format = m_dst_fmt;
    dstFrame->nb_samples = (int)dst_nb_samples;
    dstFrame->channel_layout = dst_channel_layout;
    dstFrame->channels = dst_nb_channels;
    ret = av_frame_get_buffer(dstFrame, 0);
    if (ret < 0) {
        LOGD("av_frame_get_buffer2 failt %d", ret);
        return ret;
    }
    ret = av_frame_make_writable(dstFrame);
    if (ret < 0) {
        LOGD("av_frame_make_writable() 2 failt %d", ret);
        return ret;
    }

    // 1、创建上下文
    SwrContext* swr_ctx = swr_alloc();
    if (swr_ctx == NULL) {
        LOGD("swr_alloc fail");
        return ret;
    }

    // 2、设置转换参数 在<libswresample/options.c>文件中可以找到定义
    av_opt_set_int(swr_ctx, "in_channel_layout", src_channel_layout, AV_OPT_SEARCH_CHILDREN);
    av_opt_set_int(swr_ctx, "in_sample_rate", m_src_rate, AV_OPT_SEARCH_CHILDREN);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", m_src_fmt, AV_OPT_SEARCH_CHILDREN);
    av_opt_set_int(swr_ctx, "out_channel_layout", dst_channel_layout, AV_OPT_SEARCH_CHILDREN);
    av_opt_set_int(swr_ctx, "out_sample_rate", m_dst_rate, AV_OPT_SEARCH_CHILDREN);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", m_dst_fmt, AV_OPT_SEARCH_CHILDREN);

    // 3、执行初始化
    ret = swr_init(swr_ctx);
    if (ret < 0) {
        LOGD("swr_init fail");
        return ret;
    }

    int require_size = av_samples_get_buffer_size(NULL, src_nb_channels, (int)src_nb_samples, m_src_fmt, 0);
    size_t read_size = 0;
    size_t write_size = 0;
    // 这里必须传递srcFrame->data[0]这个地址，而不能传递srcFrame->data
    while ((read_size = fread(srcFrame->data[0], 1, require_size, srcFile)) > 0) {

        /** 当转换后的采样率大于转换前的转换率时，内部会有缓冲，所以预计转换后的目标采样数会一直变化 所以这里目标音频内存大小也要跟着动态调整；当然也可以不做调整
         */
        dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, m_src_rate) + src_nb_samples, m_dst_rate, m_src_rate, AV_ROUND_UP);
        if (dst_nb_samples > max_dst_nb_samples) {
            max_dst_nb_samples = dst_nb_samples;

            av_frame_unref(dstFrame);
            LOGD("重新调整转换后存储音频数据内存大小 %d", max_dst_nb_samples);
            AVFrame* tmp = av_frame_alloc();
            tmp->format = m_dst_fmt;
            tmp->nb_samples = (int)dst_nb_samples;
            tmp->channel_layout = dst_channel_layout;
            tmp->channels = dst_nb_channels;
            tmp->sample_rate = (int)m_dst_rate;

            ret = av_frame_get_buffer(tmp, 0);
            if (ret < 0) {
                LOGD("av_frame_get_buffer fail %d", ret);
                break;
            }
            ret = av_frame_make_writable(tmp);
            if (ret < 0) {
                LOGD("av_frame_make_writable()11 fail %d", ret);
                break;
            }
            av_frame_move_ref(dstFrame, tmp);
        }

        // 4、音频格式转换 返回实际转换的采样数 <= dst_nb_samples
        // 对于音频 AVFrame中extended_data和data属性指向同一块内存
        int real_convert_nb_samples;
#define UseAVFrame
#ifndef UseAVFrame
        const uint8_t** srcdata = (const uint8_t**)srcFrame->extended_data;
        uint8_t** outdata = dstFrame->extended_data;
        real_convert_nb_samples = swr_convert(swr_ctx, outdata, (int)dst_nb_samples, srcdata, (int)src_nb_samples);
        LOGD("swr_convert nb_samples %d", real_convert_nb_samples);
#else
        ret = swr_convert_frame(swr_ctx, dstFrame, srcFrame);
        if (ret < 0) {
            LOGD("swr_convert_frame fail %d", ret);
            continue;
        }
        real_convert_nb_samples = dstFrame->nb_samples;
        LOGD("swr_convert_frame nb_samples %d", real_convert_nb_samples);
#endif

        // 5、存储;由于文件是planner格式，所以对于planner格式的存储要注意下
        if (av_sample_fmt_is_planar(m_dst_fmt)) {
            int size = av_get_bytes_per_sample(m_dst_fmt);
            for (int i = 0; i < real_convert_nb_samples; i++) {
                for (int j = 0; j < dst_nb_channels; j++) {
                    write_size+=fwrite(dstFrame->data[j] + i * size, 1, size, dstFile);
                }
            }
        }
        else {
            int size = av_samples_get_buffer_size(NULL, dst_nb_channels, real_convert_nb_samples, m_dst_fmt, 1);
            write_size+=fwrite(dstFrame->data[0], 1, size, dstFile);
        }

    }

    /** 当转换后的采样率大于转换前的转换率时，内部会有缓冲
     */
#ifdef UseAVFrame
     // 会有内部缓冲，这里传递NULL，将剩余的数据全部读取出来
    while (swr_convert_frame(swr_ctx, dstFrame, NULL) >= 0) {
        if (dstFrame->nb_samples <= 0) {
            break;
        }
       // LOGD("还有余量 %d", dstFrame->nb_samples);
#else
    uint8_t** outdata = dstFrame->extended_data;
    int real_convert_nb_samples = 1;

    while (real_convert_nb_samples > 0) {
        // 会有内部缓冲，这里第三个传递NULL，第四个传递0即可。将剩余的数据全部读取出来
        real_convert_nb_samples = swr_convert(swr_ctx, outdata, (int)dst_nb_samples, NULL, 0);
        LOGD("还有余量 %d", real_convert_nb_samples);
#endif
        if (av_sample_fmt_is_planar(m_dst_fmt)) {
            int size = av_get_bytes_per_sample((m_dst_fmt));
            for (int i = 0; i < dstFrame->nb_samples; i++) {
                for (int j = 0; j < dstFrame->channels; j++) {
                    write_size+=fwrite(dstFrame->data[j] + i * size, 1, size, dstFile);
                }
            }
        }
        else {
            int size = av_samples_get_buffer_size(NULL, dstFrame->channels, dstFrame->nb_samples, (enum AVSampleFormat)dstFrame->format, 1);
            write_size+=fwrite(dstFrame->data[0], 1, size, dstFile);
        }
    }

    swr_free(&swr_ctx);
    av_frame_unref(srcFrame);
    av_frame_unref(dstFrame);
    fclose(srcFile);

    // write header
    head_pama hpInfo;

    hpInfo.format = GetAudioFormatType(m_src_fmt);
    hpInfo.channels = dst_nb_channels;
    hpInfo.rate = m_dst_rate;
    hpInfo.bits = m_dst_bits;
    fseek(dstFile, 0, SEEK_SET);
    wav_write_header(dstFile,hpInfo, write_size);
    fclose(dstFile);

    return ret;
    }

int QmResqmple(const char* srcFile, const char* dstFile)
{

    
   CAudioResample ResampleObj(srcFile, dstFile, AV_CH_LAYOUT_MONO, 16000, AV_SAMPLE_FMT_S16P);

    return ResampleObj.doResample();

	
}
