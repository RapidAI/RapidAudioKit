#pragma once

/** �������Ƶ���ز���������
 *  1��������ʽת�������������ʽ��16λ���α�Ϊ������
 *  2�������ʵ�ת������������������������44100�����ʽ�Ϊ2000������
 *  3����ŷ�ʽת������Ƶ���ݴ�packet��ʽ��Ϊplanner��ʽ���е�Ӳ��ƽ̨�ڲ�������ʱ��Ҫ����Ƶ������
 *  planner��ʽ�ģ����еĿ�������packet��ʽ�ģ�������������ԭ�򾭳���Ҫ�������ִ�ŷ�ʽ��ת��
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