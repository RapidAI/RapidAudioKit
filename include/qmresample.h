#pragma once

/** 广义的音频的重采样包括：
 *  1、采样格式转化：比如采样格式从16位整形变为浮点型
 *  2、采样率的转换：降采样和升采样，比如44100采样率降为2000采样率
 *  3、存放方式转化：音频数据从packet方式变为planner方式。有的硬件平台在播放声音时需要的音频数据是
 *  planner格式的，而有的可能又是packet格式的，或者其它需求原因经常需要进行这种存放方式的转化
 */
class CAudioResample
{
private:
    std::string m_strSrcFile, m_strDstFile;
    int64_t m_src_ch_layout, m_dst_ch_layout = AV_CH_LAYOUT_MONO;  //AV_CH_LAYOUT_STEREO
    int m_src_rate,m_dst_rate = 16000;
    int m_src_bits, m_dst_bits;
    enum AVSampleFormat m_src_fmt, m_dst_fmt;

    bool m_bReady = false;
public:
    CAudioResample(const char* szSrcFile, const char* szDstFile,int dst_ch_layout,int dst_sample_rate, enum AVSampleFormat dst_fmt);
    ~CAudioResample() = default;

    int doResample();
    int doResampleAVFrame();

    bool getSrcInfo();
private:
};




int QmResqmple(const char* srcFile, const char* dstFile);