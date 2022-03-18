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
    // ����ļ��е�����Ϣ

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

/** av_frame_get_buffer()��AVFrame�������Ƶ�ڴ��av_samples_alloc_array_and_samples()�������Ƶ�ڴ�������
 *  1��ǰ���ڴ������ü����������Ҷ���planner��ʽ��Ƶ���������������ڴ�飬�ڴ���׵�ַ��һ����
 *  2�������ڴ����ͨ���ڴ����ʽһ������Ҫ�Լ��ֶ��ͷţ�����planner��ʽ��Ƶ����Ҳ��һ���������ڴ�飬ֻ����ÿ��planner��Ӧ���׵�ַ�ֱ�洢��
 *  ָ�������ָ���У��ͷ��ɺ���av_freep(dataAddr)��dataAddrָ�������ڴ����׵�ַ
 */
 /** �ز���ʵ�飺
  *  1��44100�Ĳ����ʣ�������48000���߽�����24000 �������Ӱ�첻�󣬴洢���ļ��е���ֵ��仯
  *  2��float���͵Ĳ������ݣ�ת����32λ�����Ͳ������� ������Ӱ��Ҳ���󣬴洢���ļ��е���ֵ��仯
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

    // ��Ƶ����
    uint8_t** src_data, ** dst_data;
    // ������
    int src_nb_channels = 0, dst_nb_channels = 0;
  
    // ������
    //int64_t src_rate = 44100, dst_rate = 16000;
    // ������ʽ����Ƶ���ݴ洢��ʽ
    //const enum AVSampleFormat src_sample_fmt = AV_SAMPLE_FMT_FLT, dst_sample_fmt = AV_SAMPLE_FMT_S16P;
    int src_nb_samples = 1024, dst_nb_samples, max_dst_nb_samples;
    int src_linesize, dst_linesize;

    // 1������������SwrContext
    SwrContext* swrCtx;
    swrCtx = swr_alloc();
    if (swrCtx == NULL) {
        LOGD("swr_allock fail");
        return ret;
    }

    // 2�������ز�������ز��� ��Щ����λ��ͷ�ļ� <libavutil/opt.h>
    av_opt_set_int(swrCtx, "in_channel_layout", m_src_ch_layout, 0);
    av_opt_set_int(swrCtx, "in_sample_rate", m_src_rate, 0);
    av_opt_set_sample_fmt(swrCtx, "in_sample_fmt", m_src_fmt, 0);

    av_opt_set_int(swrCtx, "out_channel_layout", m_dst_ch_layout, 0);
    av_opt_set_int(swrCtx, "out_sample_rate", m_dst_rate, 0);
    av_opt_set_sample_fmt(swrCtx, "out_sample_fmt", m_dst_fmt, 0);

    // 3����ʼ��������
  
    ret = swr_init(swrCtx);
    if (ret < 0) {
        LOGD("swr_init fail");
        return ret;
    }

    src_nb_channels = av_get_channel_layout_nb_channels(m_src_ch_layout);
    dst_nb_channels = av_get_channel_layout_nb_channels(m_dst_ch_layout);
    // ����src_sample_fmt��src_nb_samples��src_nb_channelsΪsrc_data�����ڴ�ռ䣬�����ö�Ӧ�ĵ�linesize��ֵ�����ط�������ڴ�Ĵ�С
    int src_buf_size = av_samples_alloc_array_and_samples(&src_data, &src_linesize, src_nb_channels, src_nb_samples, m_src_fmt, 0);
    // ����src_nb_samples*dst_rate/src_rate��ʽ���������ز�������Ƶ��nb_samples��С
    max_dst_nb_samples = dst_nb_samples = (int)av_rescale_rnd(src_nb_samples, m_dst_rate, m_src_rate, AV_ROUND_UP);
    int dst_buf_size = av_samples_alloc_array_and_samples(&dst_data, &dst_linesize, dst_nb_channels, dst_nb_samples,  m_dst_fmt, 0);

    size_t read_size = 0;
    size_t write_size = 0;
    while ((read_size = fread(src_data[0], 1, src_buf_size, srcFile)) > 0) {

        /** ��Ϊ��ת��ʱ�л��棬����Ҫ��ͣ�ĵ���ת������ڴ�Ĵ�С�������ز�����ĵ�nb_samples�Ĵ�С������swr_get_delay()���ڻ�ȡ�ز����Ļ����ӳ�
         *  dst_nb_samples��ֵ�ᾭ����ε����������ȶ�
         */
        dst_nb_samples = (int)av_rescale_rnd(swr_get_delay(swrCtx, m_src_rate) + src_nb_samples, m_dst_rate, m_src_rate, AV_ROUND_UP);
        if (dst_nb_samples > max_dst_nb_samples) {
            LOGD("Ҫ���·����ڴ���");
            // ���ͷ���ǰ���ڴ棬����sample_fmt��planner����packet��ʽ��av_samples_alloc_array_and_samples()�������Ƿ����һ�����������ڴ�
            av_freep(&dst_data[0]);
            // �����·����ڴ�
            dst_buf_size = av_samples_alloc_array_and_samples(&dst_data, &dst_linesize, dst_nb_channels, dst_nb_samples, m_dst_fmt, 0);
            max_dst_nb_samples = dst_nb_samples;
        }

        // ��ʼ�ز������ز���������ݽ�����ǰ��ָ���Ĵ洢��ʽд��ds_data�ڴ���У�����ÿ������ʵ�ʵĲ�����
        /**
         *  �������⣺ת������������
         *  ԭ�������swr_convert()�������ص�result��ʵ��ת���Ĳ�����������ֵ�϶�С�ڵ���Ԥ�Ʋ�����dst_nb_samples������д���ļ���ʱ������dst_nb_samples��
         *  ֵ����Ӧ����resultֵ
         **/
        int result = swr_convert(swrCtx, dst_data, dst_nb_samples, (const uint8_t**)src_data, src_nb_samples);
        if (result < 0) {
            LOGD("swr_convert fail %d", result);
            break;
        }
        LOGD("read_size %d dst_nb_samples %d src_nb_samples %d result %d", read_size, dst_nb_samples, src_nb_samples, result);
        // ����Ƶ����д��pcm�ļ�
        if (av_sample_fmt_is_planar(m_dst_fmt)) {  // planner��ʽ����pcm�ļ�д��ʱһ�㶼��packet��ʽ����������Ҫע��ת��һ��
            int size = av_get_bytes_per_sample(m_dst_fmt);
            // ���������result����������dst_nb_samples����Ϊresult�Ŵ���˴�ʵ��ת���Ĳ����� ���϶�С�ڵ���dst_nb_samples
            for (int i = 0; i < result; i++) {
                for (int j = 0; j < dst_nb_channels; j++) {
                    write_size+= fwrite(dst_data[j] + i * size, 1, size, dstFile);
                }
            }
        }
        else {
            // ���һ����������Ϊ1 �������Ϊcpu����������Ĵ�С����ʵ�ʵ����ݴ�С ���¶�д������ �Ӷ���ɴ���
            dst_buf_size = av_samples_get_buffer_size(&dst_linesize, dst_nb_channels, result, m_dst_fmt, 1);
            write_size+= fwrite(dst_data[0], 1, dst_buf_size, dstFile);
        }
    }

    // ����ʣ��Ļ�������û��ת��������������NULL�����ĸ�����0���ɽ������е�ȫ��ȡ��
    do {
        int real_nb_samples = swr_convert(swrCtx, dst_data, dst_nb_samples, NULL, 0);
        if (real_nb_samples <= 0) {
            break;
        }
        LOGD("���� %d", real_nb_samples);
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

    // �ͷ���Դ
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

    // ���������������й� Ŀ������������ɲ����ʸ��ݱ�����ʽ�����
    // R2 = R1*(��2/��1) R1 �����ο�Դ ��R2��ͬһ�������ݣ���1����2�����������ӣ�Ӱ��ϵ����R1����2����1һ�ζ�Ӧ�����ʽ�ĵ�һ������ϵ��
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

    // 1������������
    SwrContext* swr_ctx = swr_alloc();
    if (swr_ctx == NULL) {
        LOGD("swr_alloc fail");
        return ret;
    }

    // 2������ת������ ��<libswresample/options.c>�ļ��п����ҵ�����
    av_opt_set_int(swr_ctx, "in_channel_layout", src_channel_layout, AV_OPT_SEARCH_CHILDREN);
    av_opt_set_int(swr_ctx, "in_sample_rate", m_src_rate, AV_OPT_SEARCH_CHILDREN);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", m_src_fmt, AV_OPT_SEARCH_CHILDREN);
    av_opt_set_int(swr_ctx, "out_channel_layout", dst_channel_layout, AV_OPT_SEARCH_CHILDREN);
    av_opt_set_int(swr_ctx, "out_sample_rate", m_dst_rate, AV_OPT_SEARCH_CHILDREN);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", m_dst_fmt, AV_OPT_SEARCH_CHILDREN);

    // 3��ִ�г�ʼ��
    ret = swr_init(swr_ctx);
    if (ret < 0) {
        LOGD("swr_init fail");
        return ret;
    }

    int require_size = av_samples_get_buffer_size(NULL, src_nb_channels, (int)src_nb_samples, m_src_fmt, 0);
    size_t read_size = 0;
    size_t write_size = 0;
    // ������봫��srcFrame->data[0]�����ַ�������ܴ���srcFrame->data
    while ((read_size = fread(srcFrame->data[0], 1, require_size, srcFile)) > 0) {

        /** ��ת����Ĳ����ʴ���ת��ǰ��ת����ʱ���ڲ����л��壬����Ԥ��ת�����Ŀ���������һֱ�仯 ��������Ŀ����Ƶ�ڴ��СҲҪ���Ŷ�̬��������ȻҲ���Բ�������
         */
        dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, m_src_rate) + src_nb_samples, m_dst_rate, m_src_rate, AV_ROUND_UP);
        if (dst_nb_samples > max_dst_nb_samples) {
            max_dst_nb_samples = dst_nb_samples;

            av_frame_unref(dstFrame);
            LOGD("���µ���ת����洢��Ƶ�����ڴ��С %d", max_dst_nb_samples);
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

        // 4����Ƶ��ʽת�� ����ʵ��ת���Ĳ����� <= dst_nb_samples
        // ������Ƶ AVFrame��extended_data��data����ָ��ͬһ���ڴ�
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

        // 5���洢;�����ļ���planner��ʽ�����Զ���planner��ʽ�Ĵ洢Ҫע����
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

    /** ��ת����Ĳ����ʴ���ת��ǰ��ת����ʱ���ڲ����л���
     */
#ifdef UseAVFrame
     // �����ڲ����壬���ﴫ��NULL����ʣ�������ȫ����ȡ����
    while (swr_convert_frame(swr_ctx, dstFrame, NULL) >= 0) {
        if (dstFrame->nb_samples <= 0) {
            break;
        }
       // LOGD("�������� %d", dstFrame->nb_samples);
#else
    uint8_t** outdata = dstFrame->extended_data;
    int real_convert_nb_samples = 1;

    while (real_convert_nb_samples > 0) {
        // �����ڲ����壬�������������NULL�����ĸ�����0���ɡ���ʣ�������ȫ����ȡ����
        real_convert_nb_samples = swr_convert(swr_ctx, outdata, (int)dst_nb_samples, NULL, 0);
        LOGD("�������� %d", real_convert_nb_samples);
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
