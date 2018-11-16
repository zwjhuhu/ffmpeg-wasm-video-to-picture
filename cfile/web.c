#include <emscripten.h>
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "process.h"

int read_packet(void *opaque, uint8_t *buf, int buf_size);

typedef struct {
    uint8_t *ptr;
    size_t size;
} buffer_data;

buffer_data bufferData;

AVFormatContext *pFormatCtx = NULL;
uint8_t *avioCtxBuffer = NULL;
AVIOContext *avioCtx = NULL;

EMSCRIPTEN_KEEPALIVE //这个宏表示这个函数要作为导出的函数
int setFile(uint8_t *buff, const int buffLength) {

    unsigned char *avio_ctx_buffer = NULL;
    // 对于普通的mp4文件，这个size只要1MB就够了，但是对于mov/m4v需要和buff一样大
    size_t avio_ctx_buffer_size = buffLength;
    
    // AVInputFormat* in_fmt = av_find_input_format("h265");

    bufferData.ptr = buff;  /* will be grown as needed by the realloc above */
    bufferData.size = buffLength; /* no data at this point */

    pFormatCtx = avformat_alloc_context();

    avioCtxBuffer = (uint8_t *)av_malloc(avio_ctx_buffer_size);
    
    /* 读内存数据 */
    avioCtx = avio_alloc_context(avioCtxBuffer, avio_ctx_buffer_size, 0, NULL, read_packet, NULL, NULL);

    pFormatCtx->pb = avioCtx;
    pFormatCtx->flags = AVFMT_FLAG_CUSTOM_IO;

    /* 打开内存缓存文件, and allocate format context */
    // pFormatCtx->probesize = 10000 * 1024;
    // pFormatCtx->max_analyze_duration = 100 * AV_TIME_BASE;
    if (avformat_open_input(&pFormatCtx, "", NULL, NULL) < 0) {
        fprintf(stderr, "Could not open input\n");
        return 0;
    }
    av_dump_format(pFormatCtx, 0, "", 0);
    
    return 1;
}

EMSCRIPTEN_KEEPALIVE
void clearFile(){
	if(pFormatCtx){
		clearData();
		// free
		avformat_close_input(&pFormatCtx);
		av_free(avioCtx->buffer);
		av_free(avioCtx);
		//av_free(avioCtxBuffer);
		pFormatCtx = NULL;
	}

}

EMSCRIPTEN_KEEPALIVE
ImageData *findFrame(){
	ImageData *result = process(pFormatCtx);
	if (!result){
		clearFile();
	}
    return result;
}

int read_packet(void *opaque, uint8_t *buf, int buf_size) {

    buf_size = FFMIN(buf_size, bufferData.size);

    if (!buf_size)
        return AVERROR_EOF;
    // printf("ptr:%p size:%zu bz%zu\n", bufferData.ptr, bufferData.size, buf_size);

    /* copy internal buffer data to buf */
    memcpy(buf, bufferData.ptr, buf_size);
    bufferData.ptr += buf_size;
    bufferData.size -= buf_size;
    return buf_size;
}

/*
void test() {
	ImageData *data = NULL;
	FILE *f = fopen("/mnt/d/ffmpeg/ffmpeg-wasm-video-to-picture/s1.mp4", "rb");
	int buffLength = 31543747;
	uint8_t *buff = (uint8_t *)malloc(buffLength);
	fread(buff,1,buffLength,f);
	fclose(f);
	setFile(buff,buffLength);
	while(1){
		data = findFrame();
		if(!data){
			break;
		}
	}
	
	f = fopen("/mnt/d/ffmpeg/ffmpeg-wasm-video-to-picture/test.mp4", "rb");
	buffLength = 8911787;
	buff = (uint8_t *)malloc(buffLength);
	fread(buff,1,buffLength,f);
	fclose(f);
	setFile(buff,buffLength);
	while(1){
		data = findFrame();
		if(!data){
			break;
		}
	}
}*/

int main () {
    av_register_all();
    fprintf(stdout, "ffmpeg init done\n");
	//test();
    return 0;
}



