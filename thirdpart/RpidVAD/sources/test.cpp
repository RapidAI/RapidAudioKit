#include "qmvad_api.h"


#define FRAME_SIZE 320

#define SAMPLE_RATE  16000
int main(int argc, char * argv[])
{


	int16_t frame[FRAME_SIZE] = { 0 };
	QMAVD_HANDLE Handle = QmVad_init();


	int ret=QmVad_SetMode(Handle,3);

	// PCM Buffersize=采样率*采样时间*采样位深/8*通道数（Bytes）

	// for 16000hz 

	int frame_size = QMVAD_GET_FRAME_SIZE(SAMPLE_RATE,16,int16_t,20); // 16000 * 10 * (16 / 8) * 1 / 1000; // bytes



	ret=QmVad_ValidRateAndFrameLength(SAMPLE_RATE, FRAME_SIZE);

	//  rate: sample rate,   8000, 16000 , 32000 or 48000;   time: 10, 20 or 30 ms frames
	// for 8000hz, valid length for 10ms,20ms,30ms is: 80,160,240
	
	//  16000hz , 10ms/20ms/30ms  length 160, 320, 480 


	for (int i = 0; i < FRAME_SIZE; i++)
		frame[i] = 3000;
	
	ret = QmVad_ValidRateAndFrameLength(SAMPLE_RATE, FRAME_SIZE);

	QMVAD_STATUS status = QmVad_Process(Handle, 16000, frame, FRAME_SIZE);





	if (Handle)
		QmVad_Final(Handle);


	return 0;
}