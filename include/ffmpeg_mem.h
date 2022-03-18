#pragma once
struct buffer_data
{
	uint8_t* ptr; /* �ļ��ж�Ӧλ��ָ�� */
	size_t size;  ///< size left in the buffer /* �ļ���ǰָ�뵽ĩβ */
};



// �ص㣬�Զ���buffer����Ҫ���������ﶨ��

class CAvExtractor
{
public:
	CAvExtractor(uint8_t* buf, int buf_size);
	CAvExtractor(const char* szFileName);
	CAvExtractor()=default;
	~CAvExtractor();

	static int read_packet(void* opaque, uint8_t* buf, int buf_size);
	int open_input_buffer(uint8_t* buf, int len);
	bool GetAudioStream(const char* szOutFileName, unsigned char* szOutBuf, int& nOutSize);
	bool OpenFile(const char* szFileName);
	void CloseAV();

	int  decode(AVCodecContext* dec_ctx, AVPacket* pkt, AVFrame* frame, FILE* outfile);
	bool decidemp3format(const char* inputfilename);

private:

	AVFormatContext* m_fmt_ctx;

	struct buffer_data m_bd = { 0 };


	bool m_bOpened = false;
	bool m_bMP3 = false;
};