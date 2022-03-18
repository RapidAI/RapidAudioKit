#pragma once 
#include <stdint.h>
#include <string>
#include <vector>




// ffmpeg
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libavutil/avutil.h>
#include <libavutil/dict.h>
#include "libavformat/avio.h"
#include "libavdevice/avdevice.h"
#include "libavutil/audio_fifo.h"

}

#include "qmvad_api.h"

#include "qmavdef.h"
#include "qmavkits.h"
#include "qmavkitsapi.h"

#include "ffmpeg_mem.h"
#include "qmresample.h"

