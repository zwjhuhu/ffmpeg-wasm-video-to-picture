#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

typedef struct {
    uint32_t width;
    uint32_t height;
    uint8_t *data;
} ImageData;

ImageData *process (AVFormatContext *pFormatCtx);
int findVideoStream (AVFormatContext *pFormatCtx);
AVCodecContext *openCodec (AVCodecContext *pCodecCtx);
void initAVFrame (AVCodecContext *pCodecCtx, uint8_t **frameBuffer);
int readAVFrame (AVCodecContext *pCodecCtx, AVFormatContext *pFormatCtx,
    AVFrame *pFrameRGB, int videoStream);
uint8_t *getFrameBuffer(AVFrame *pFrame, AVCodecContext *pCodecCtx);
void clearData();
