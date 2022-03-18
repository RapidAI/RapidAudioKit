#pragma once 
//通过一个guard对象来确保自源释放
struct AudioInfoGuard {
    //格式化I/O上下文
    AVFormatContext* formatCtx = NULL;
    //解码器
    AVCodec* codec = NULL;
    //解码器上下文
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

//是在Qt中使用的，所以传递的QString
bool getAudioInfo(const QString& filepath)
{
       //const char *filepath="D:/Download/12.wav";

    //借助析构函数来释放自源
    AudioInfoGuard guard;

    //打开输入流并读取头
    //流要使用avformat_close_input关闭
    //成功时返回=0
    int result = avformat_open_input(&guard.formatCtx, path, NULL, NULL);
    if (result != 0 || guard.formatCtx == NULL) {
        return false;
    }

    //读取文件获取流信息，把它存入AVFormatContext中
    //正常时返回>=0
    if (avformat_find_stream_info(guard.formatCtx, NULL) < 0) {
        return false;
    }

    //获取元信息，曲名，歌手等
    //AVDictionaryEntry *tag = NULL;
    //while (tag = av_dict_get(fmt_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))
    //{
    //    qDebug()<<tag->key<<tag->value;
    //}

    qDebug() << "filepath" << filepath;
    //时长
    //duration/AV_TIME_BASE单位为秒
    qDebug() << "duration" << guard.formatCtx->duration / (AV_TIME_BASE / 1000.0) << "ms";
    //文件格式，如wav
    qDebug() << "format" << guard.formatCtx->iformat->name << ":" << guard.formatCtx->iformat->long_name;
    //这是容器的比特率
    qDebug() << "bit rate" << guard.formatCtx->bit_rate << "bps";
    qDebug() << "n stream" << guard.formatCtx->nb_streams;

    for (unsigned int i = 0; i < guard.formatCtx->nb_streams; i++)
    {
#if 1
        //AVStream是存储每一个视频/音频流信息的结构体
        AVStream* in_stream = guard.formatCtx->streams[i];

        //类型为音频
        if (in_stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            //查找具有匹配编解码器ID的已注册解码器
            //失败返回NULL
            guard.codec = avcodec_find_decoder(in_stream->codecpar->codec_id);
            if (guard.codec == NULL) {
                return false;
            }

            //分配AVCodecContext并将其字段设置为默认值
            //需要使用avcodec_free_context释放生成的对象
            //如果失败，则返回默认填充或者 NULL
            guard.codecCtx = avcodec_alloc_context3(guard.codec);
            if (guard.codecCtx == NULL) {
                return false;
            }

            //根据编码器填充上下文参数
            //事实上codecpar包含了大部分解码器相关的信息，这里是直接从AVCodecParameters复制到AVCodecContext
            //成功时返回值>=0
            if (avcodec_parameters_to_context(guard.codecCtx, in_stream->codecpar) < 0) {
                return false;
            }

            //某些AVCodecContext字段的访问器，已弃用
            //av_codec_set_pkt_timebase(codec_ctx, in_stream->time_base);

            //打开解码器
            //使用给定的AVCodec初始化AVCodecContext
            //在之前必须使用avcodec_alloc_context3()分配上下文
            //成功时返回值=0
            if (avcodec_open2(guard.codecCtx, guard.codec, nullptr) != 0) {
                return false;
            }

            //采样率
            qDebug() << "sample rate" << guard.codecCtx->sample_rate;
            //通道数
            qDebug() << "channels" << guard.codecCtx->channels;
            //采样精度（暂时没找到获取采样精度的接口，用fmt来算的有些对不上）
            //in_stream->codec->sample_fmt枚举AVSampleFormat表示数据存储格式，如16位无符号
            //av_get_bytes_per_sample返回AVSampleFormat对应的字节大小
            qDebug() << "sample bit" << guard.codecCtx->sample_fmt << ":" << (av_get_bytes_per_sample(guard.codecCtx->sample_fmt) << 3);
            //音频的比特率
            qDebug() << "bit rate" << guard.codecCtx->bit_rate;
            //编码，如pcm
            qDebug() << "codec name" << guard.codec->name << ":" << guard.codec->long_name;
            return true;
        }
#else
        //新的版本这种获取方式已弃用
        AVStream* in_stream = fmt_ctx->streams[i];
        AVCodecContext* avctx = in_stream->codec;
        if (avctx->codec_type == AVMEDIA_TYPE_VIDEO) {
            //视频信息略
        }
        else if (avctx->codec_type == AVMEDIA_TYPE_AUDIO) {
            //音频信息
            qDebug() << "sample rate" << in_stream->codec->sample_rate;
            qDebug() << "channels" << in_stream->codec->channels;
            //in_stream->codec->sample_fmt枚举AVSampleFormat表示数据存储格式，如16位无符号
            //av_get_bytes_per_sample返回AVSampleFormat对应的字节大小
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