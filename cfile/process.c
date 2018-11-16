#include "process.h"
#include <stdio.h>
#include <string.h>

ImageData *imageData = NULL;
int videoStream = -1;
AVCodecContext *pNewCodecCtx = NULL;
AVFrame *frame = NULL;
AVFrame *pFrameRGB = NULL;
AVPacket pkt;
AVPacket orig_pkt;
struct SwsContext *sws_ctx = NULL;
uint8_t *frameBuffer;
int width = 0;
int height = 0;
//int video_frame_count = 0;

ImageData *process (AVFormatContext *pFormatCtx) {

    // find videoStream
	if (videoStream == -1) {
		
		if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
			videoStream = -1;
			return NULL;
		}
		videoStream = findVideoStream(pFormatCtx);
	}
	
	if(!pNewCodecCtx) {
		AVCodecContext *pCodecCtx = pFormatCtx->streams[videoStream]->codec;
		
		pNewCodecCtx = openCodec(pCodecCtx);
		if (!pNewCodecCtx) {
			fprintf(stderr, "openCodec failed, pNewCodecCtx is NULL\n");
			return NULL;
		}
		
		width = pNewCodecCtx->width;
		height = pNewCodecCtx->height;
		
	}
	
	if (!frame) {
		//printf("s1\n");
		frame = av_frame_alloc();
		av_init_packet(&pkt);
		pkt.data = NULL;
		pkt.size = 0;
		
		initAVFrame(pNewCodecCtx, &frameBuffer);
		
		// initialize SWS context for software scaling
		sws_ctx = sws_getContext(width,
			height,
			pNewCodecCtx->pix_fmt,
			width,
			height,
			AV_PIX_FMT_RGB24,
			SWS_BILINEAR,
			NULL,
			NULL,
			NULL
			);
	}
	//printf("read--");
    int ret = readAVFrame(pNewCodecCtx, pFormatCtx, pFrameRGB, videoStream); 
    if (ret < 0) {
        fprintf(stderr, "%s", "read AV frame failed\n");
        return NULL;
    } else if (ret==0){
		fprintf(stdout, "%s", "all frames decoded\n");
		return NULL;
	}
	
    imageData = (ImageData *)malloc(sizeof(ImageData));
    imageData->width = (uint32_t)width;
    imageData->height = (uint32_t)height;
    imageData->data = getFrameBuffer(pFrameRGB, pNewCodecCtx);
    
    return imageData;
      
}

uint8_t *getFrameBuffer(AVFrame *pFrame, AVCodecContext *pCodecCtx) {

    // Write pixel data
    uint8_t *buffer = (uint8_t *)malloc(height * width * 3);
    for (int y = 0; y < height; y++) {
        // fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);
        memcpy(buffer + y * pFrame->linesize[0], pFrame->data[0] + y * pFrame->linesize[0], width * 3);
    }
    return buffer;
}

int readAVFrame(AVCodecContext *pCodecCtx, AVFormatContext *pFormatCtx,
    AVFrame *pFrameRGB, int videoStream) {

    
    int frameFinished;
	int ret = -1;
	int done = 0;
	
	while (av_read_frame(pFormatCtx, &pkt) >= 0) {
		orig_pkt = pkt;
		if (pkt.stream_index == videoStream){
			
			ret = avcodec_decode_video2(pCodecCtx, frame, &frameFinished, &pkt);
			if (ret < 0) {
				fprintf(stderr, "error while decoding frame\n");
				return ret;
			}
			
			// Did we get a video frame?
			if(frameFinished) {
				//printf("video_frame n:%d coded_n:%d\n",
                   //video_frame_count++, frame->coded_picture_number);
				   
				sws_scale(sws_ctx, (uint8_t const * const *)frame->data,
					 frame->linesize, 0, height,
					 pFrameRGB->data, pFrameRGB->linesize);	 
				done = 1;
				ret = pkt.size;
				break;
			} 
			
		}
		av_packet_unref(&orig_pkt);
	}
	
	if(!done){
		pkt.data = NULL;
		pkt.size = 0;
		frameFinished = 1;
		while(frameFinished){
			ret = avcodec_decode_video2(pCodecCtx, frame, &frameFinished, &pkt);
			if (ret < 0) {
				fprintf(stderr, "error while decoding frame\n");
				return ret;
			}
			
			// Did we get a video frame?
			if(frameFinished) {
				//printf("video_frame_cached n:%d coded_n:%d\n",
                   //video_frame_count++, frame->coded_picture_number);
				   
				sws_scale(sws_ctx, (uint8_t const * const *)frame->data,
					 frame->linesize, 0, height,
					 pFrameRGB->data, pFrameRGB->linesize);
				done = 1;
				ret = 1;
				break;
			}
		}
	}
	if(!done) {
		ret = 0;
	}
	
	return ret;
}

void initAVFrame (AVCodecContext *pCodecCtx, uint8_t **pFrameBuffer) {
    // Allocate an AVFrame structure
    pFrameRGB = av_frame_alloc();
    if (pFrameRGB == NULL) {
        return;
    }
    int numBytes;
    // Determine required buffer size and allocate buffer
    numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, width,
                                height);
    *pFrameBuffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
    // Assign appropriate parts of buffer to image planes in pFrameRGB
    // Note that pFrameRGB is an AVFrame, but AVFrame is a superset
    // of AVPicture
    avpicture_fill((AVPicture *)pFrameRGB, *pFrameBuffer, AV_PIX_FMT_RGB24,
                width, height);

}

int findVideoStream (AVFormatContext *pFormatCtx) {
    // Find the first video stream
    int videoStream = -1;
    for (int i = 0; i < pFormatCtx -> nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }
    }
    return videoStream;
}

AVCodecContext *openCodec (AVCodecContext *pCodecCtx) {
    // Find the decoder for the video stream
    AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL) {
        fprintf(stderr, "%s", "Unsupported codec!\n");
        return NULL; // Codec not found
    }
    // Copy context
    AVCodecContext *pNewCodecCtx = avcodec_alloc_context3(pCodec);
    if (avcodec_copy_context(pNewCodecCtx, pCodecCtx) != 0) {
        fprintf(stderr, "Couldn't copy codec context");
        return NULL; // Error copying codec context
    }
    // Open codec
    if (avcodec_open2(pNewCodecCtx, pCodec, NULL) < 0) {
        return NULL; // Could not open codec
    }
    return pNewCodecCtx;
}

void clearData(){
	if (pNewCodecCtx) {
		avcodec_close(pNewCodecCtx);
		av_free(pNewCodecCtx);
		av_frame_free(&frame);
		av_free(frameBuffer);
		av_frame_free(&pFrameRGB);
		frameBuffer = NULL;
		pFrameRGB =NULL;
		frame = NULL;
		videoStream = -1;
		sws_ctx = NULL;
		pNewCodecCtx = NULL;
	}
}
