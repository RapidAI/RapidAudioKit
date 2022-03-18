#pragma once 
//ͨ��һ��guard������ȷ����Դ�ͷ�
struct AudioInfoGuard {
    //��ʽ��I/O������
    AVFormatContext* formatCtx = NULL;
    //������
    AVCodec* codec = NULL;
    //������������
    AVCodecContext* codecCtx = NULL;

    ~AudioInfoGuard() {
        if (codecCtx) {
            avcodec_free_context(&codecCtx);
        }
        if (formatCtx) {
            avformat_close_input(&formatCtx);
            avformat_free_context(formatCtx);
        }
    }
};

//����Qt��ʹ�õģ����Դ��ݵ�QString
bool getAudioInfo(const QString& filepath)
{
       //const char *filepath="D:/Download/12.wav";

    //���������������ͷ���Դ
    AudioInfoGuard guard;

    //������������ȡͷ
    //��Ҫʹ��avformat_close_input�ر�
    //�ɹ�ʱ����=0
    int result = avformat_open_input(&guard.formatCtx, path, NULL, NULL);
    if (result != 0 || guard.formatCtx == NULL) {
        return false;
    }

    //��ȡ�ļ���ȡ����Ϣ����������AVFormatContext��
    //����ʱ����>=0
    if (avformat_find_stream_info(guard.formatCtx, NULL) < 0) {
        return false;
    }

    //��ȡԪ��Ϣ�����������ֵ�
    //AVDictionaryEntry *tag = NULL;
    //while (tag = av_dict_get(fmt_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))
    //{
    //    qDebug()<<tag->key<<tag->value;
    //}

    qDebug() << "filepath" << filepath;
    //ʱ��
    //duration/AV_TIME_BASE��λΪ��
    qDebug() << "duration" << guard.formatCtx->duration / (AV_TIME_BASE / 1000.0) << "ms";
    //�ļ���ʽ����wav
    qDebug() << "format" << guard.formatCtx->iformat->name << ":" << guard.formatCtx->iformat->long_name;
    //���������ı�����
    qDebug() << "bit rate" << guard.formatCtx->bit_rate << "bps";
    qDebug() << "n stream" << guard.formatCtx->nb_streams;

    for (unsigned int i = 0; i < guard.formatCtx->nb_streams; i++)
    {
#if 1
        //AVStream�Ǵ洢ÿһ����Ƶ/��Ƶ����Ϣ�Ľṹ��
        AVStream* in_stream = guard.formatCtx->streams[i];

        //����Ϊ��Ƶ
        if (in_stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            //���Ҿ���ƥ��������ID����ע�������
            //ʧ�ܷ���NULL
            guard.codec = avcodec_find_decoder(in_stream->codecpar->codec_id);
            if (guard.codec == NULL) {
                return false;
            }

            //����AVCodecContext�������ֶ�����ΪĬ��ֵ
            //��Ҫʹ��avcodec_free_context�ͷ����ɵĶ���
            //���ʧ�ܣ��򷵻�Ĭ�������� NULL
            guard.codecCtx = avcodec_alloc_context3(guard.codec);
            if (guard.codecCtx == NULL) {
                return false;
            }

            //���ݱ�������������Ĳ���
            //��ʵ��codecpar�����˴󲿷ֽ�������ص���Ϣ��������ֱ�Ӵ�AVCodecParameters���Ƶ�AVCodecContext
            //�ɹ�ʱ����ֵ>=0
            if (avcodec_parameters_to_context(guard.codecCtx, in_stream->codecpar) < 0) {
                return false;
            }

            //ĳЩAVCodecContext�ֶεķ�������������
            //av_codec_set_pkt_timebase(codec_ctx, in_stream->time_base);

            //�򿪽�����
            //ʹ�ø�����AVCodec��ʼ��AVCodecContext
            //��֮ǰ����ʹ��avcodec_alloc_context3()����������
            //�ɹ�ʱ����ֵ=0
            if (avcodec_open2(guard.codecCtx, guard.codec, nullptr) != 0) {
                return false;
            }

            //������
            qDebug() << "sample rate" << guard.codecCtx->sample_rate;
            //ͨ����
            qDebug() << "channels" << guard.codecCtx->channels;
            //�������ȣ���ʱû�ҵ���ȡ�������ȵĽӿڣ���fmt�������Щ�Բ��ϣ�
            //in_stream->codec->sample_fmtö��AVSampleFormat��ʾ���ݴ洢��ʽ����16λ�޷���
            //av_get_bytes_per_sample����AVSampleFormat��Ӧ���ֽڴ�С
            qDebug() << "sample bit" << guard.codecCtx->sample_fmt << ":" << (av_get_bytes_per_sample(guard.codecCtx->sample_fmt) << 3);
            //��Ƶ�ı�����
            qDebug() << "bit rate" << guard.codecCtx->bit_rate;
            //���룬��pcm
            qDebug() << "codec name" << guard.codec->name << ":" << guard.codec->long_name;
            return true;
        }
#else
        //�µİ汾���ֻ�ȡ��ʽ������
        AVStream* in_stream = fmt_ctx->streams[i];
        AVCodecContext* avctx = in_stream->codec;
        if (avctx->codec_type == AVMEDIA_TYPE_VIDEO) {
            //��Ƶ��Ϣ��
        }
        else if (avctx->codec_type == AVMEDIA_TYPE_AUDIO) {
            //��Ƶ��Ϣ
            qDebug() << "sample rate" << in_stream->codec->sample_rate;
            qDebug() << "channels" << in_stream->codec->channels;
            //in_stream->codec->sample_fmtö��AVSampleFormat��ʾ���ݴ洢��ʽ����16λ�޷���
            //av_get_bytes_per_sample����AVSampleFormat��Ӧ���ֽڴ�С
            qDebug() << "sample bit" << in_stream->codec->sample_fmt << ":" << (av_get_bytes_per_sample(in_stream->codec->sample_fmt) << 3);

            AVCodec* codec = avcodec_find_decoder(avctx->codec_id);
            if (codec == NULL) {
                return;
            }
            qDebug() << "codec name" << codec->name << ":" << codec->long_name;
            return true;
        }
#endif
    }
    return false;
}