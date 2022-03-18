#include "qmavkitsapi.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#define TEST_FILE  "e:\\workprj\\mousika\\asr\\qmavkit\\samples\\test.flac"
#define	DST_FILE   "e:\\workprj\\mousika\\asr\\qmavkit\\samples\\test.wav"
#define FINAL_FILE "e:\\workprj\\mousika\\asr\\qmavkit\\samples\\final.wav"
#else
#define TEST_FILE  "/opt/test/test7.ogg"
#define	DST_FILE  "/opt/test/test7.wav"
#define FINAL_FILE "/opt/test/resample.wav"
#define CUT_FILE  "/opt/test/cut.wav"
#define TMP_FILE  "/opt/test/tmp.wav"
#define OUT_DIR "/opt/test/out"
#endif
int main(int argc, char * argv[])
{

	QMAVHANDLER  Handle=QmAVKitInit();
	if (!Handle)
		return -1;

	//if (QmAVKitExtractFromFile(Handle,TEST_FILE, DST_FILE))
	//	printf("failed");
	

	//if (QmAVKitResample(DST_FILE, FINAL_FILE))
	//	printf("Resampling failed!\n");


	//if (QmAVKitCutAudidoIntoFile(FINAL_FILE, 0, 5000, CUT_FILE))
	//{
	//	printf("error!!");
	//}

	if (QmAVKitCutAudidoAll(FINAL_FILE, 30000, TMP_FILE, OUT_DIR))
	{
		printf("error!!");
	}


	QmAVKitFinal(Handle);

	return 0;
}