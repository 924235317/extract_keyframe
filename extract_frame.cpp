#include "extract_frame.hpp"

//ffmpeg
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#ifdef __cplusplus
};
#endif

#include <pthread.h>

#define FMT_BGR AV_PIX_FMT_BGR24
#define FMT_YUV AV_PIX_FMT_YUV420P
#define FMT_RGB AV_PIX_FMT_RGB24

pthread_mutex_t m_mut = PTHREAD_MUTEX_INITIALIZER;

void erro_print (int err) {
    int bufsize = 1024;
    char *_buf = (char *)malloc(sizeof(char)*bufsize);
    av_strerror(err, _buf, bufsize);
    fprintf(stderr, _buf);
}

int extract_keyframes(std::string file_path, std::vector<cv::Mat> &frames)
{
    av_register_all();

    AVFormatContext *format_context = NULL;
    AVCodecContext *codec_context = NULL;
    AVCodec *codec = NULL;
    struct SwsContext *convert_context_yuv = NULL;
    struct SwsContext *convert_context_rgb = NULL;
    AVFrame *frame_org = NULL;
    AVPacket packet;
    int ret, video_stream;
    
    
    //open input and find nesserary info
    format_context = avformat_alloc_context(); 
    ret = avformat_open_input(&format_context, file_path.c_str(), NULL, NULL);
    if (ret < 0) {
        fprintf(stderr, "ERROR: Open input file FAILED!\n");
        goto end;
    }
    else {
        fprintf(stdout, "INFO: Open input file DONE!\n");
    }
    
    ret = avformat_find_stream_info(format_context, 0);
    if (ret < 0) {
        fprintf(stderr, "ERROR: Find stream info FAILED!\n");
        goto end;
    } 
    else {
        fprintf(stdout, "INFO: Find stream info DONE!\n");
    }

    for (int i = 0; i < format_context->nb_streams; i++) {
        if (format_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream = i;
            break;
        }
    }
    if (video_stream == -1) {
        fprintf(stderr, "ERROR: Find video stream FAILED!\n");
        goto end;
    }
    
    codec_context = format_context->streams[video_stream]->codec;
    codec = avcodec_find_decoder(codec_context->codec_id);
    ret = avcodec_open2(codec_context, codec, NULL);
    if (ret < 0) {
        fprintf(stderr, "ERROR: Open codec FAILED!\n");
        goto end;
    }      
    else {
        fprintf(stdout, "INFO: Open codec DONE!\n");
    }

    //init convert context 
    convert_context_yuv = sws_getCachedContext(convert_context_yuv, 
                                               codec_context->width, 
                                               codec_context->height,
                                               codec_context->pix_fmt, 
                                               codec_context->width, 
                                               codec_context->height,
                                               AV_PIX_FMT_YUV420P, 
                                               SWS_BICUBIC, NULL, NULL, NULL);
    if (!convert_context_yuv) {
         av_log(NULL, AV_LOG_ERROR, "Can't get sws context\n");
         goto end;
    } 

    convert_context_rgb = sws_getCachedContext(convert_context_rgb, 
                                               codec_context->width, 
                                               codec_context->height,
                                               AV_PIX_FMT_YUV420P, 
                                               codec_context->width, 
                                               codec_context->height,
                                               AV_PIX_FMT_BGR24, 
                                               SWS_BICUBIC, NULL, NULL, NULL);
    if (!convert_context_rgb) {
         av_log(NULL, AV_LOG_ERROR, "Can't get sws context\n");
         goto end;
    } 
    
    //decode
    int got_picture;
    frame_org = av_frame_alloc();
    while (1) {

        //test mutlithread

        ret = av_read_frame(format_context, &packet);
        if (ret == AVERROR_EOF) {
            fprintf(stdout, "INFO: End of the file!\n");
            goto end; 
        } else if (ret < 0) {
            fprintf(stderr, "ERRO: Read frame FAILED!\n");
            goto end; 
        } else {}

        if (packet.stream_index == video_stream && (packet.flags & AV_PKT_FLAG_KEY)) {
            
        	pthread_mutex_lock(&m_mut);
            avcodec_decode_video2(codec_context, frame_org, &got_picture, &packet);
		    pthread_mutex_unlock(&m_mut);
            
            if (got_picture) {
                
                //conver to yuv
                int buffer_size_yuv = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, 
                                                           codec_context->width, 
                                                           codec_context->height, 
                                                           codec_context->coded_width);
                uint8_t* pBuffer_yuv = (uint8_t *)av_malloc(buffer_size_yuv * sizeof(uint8_t));
                if (!pBuffer_yuv) {
                    av_log(NULL, AV_LOG_ERROR, "Can't allocate buffer\n");
                }
                AVFrame *frame_yuv = av_frame_alloc();
                avpicture_fill((AVPicture *)frame_yuv, pBuffer_yuv, AV_PIX_FMT_YUV420P, 
                               codec_context->width, codec_context->height);
                sws_scale(convert_context_yuv, (const uint8_t **)(frame_org->data), 
                          frame_org->linesize, 0, codec_context->height, 
                          frame_yuv->data, frame_yuv->linesize);
                
                //convert to rgb
                int buffer_size_rgb = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, 
                                                           codec_context->width, 
                                                           codec_context->height, 
                                                           codec_context->coded_width);
                uint8_t *pBuffer_rgb = (uint8_t *)av_malloc(buffer_size_rgb * sizeof(uint8_t));
                if (!pBuffer_rgb) {
                    av_log(NULL, AV_LOG_ERROR, "Can't allocate buffer\n");
                }
                AVFrame *frame_rgb = av_frame_alloc();
                avpicture_fill((AVPicture *)frame_rgb, pBuffer_rgb, AV_PIX_FMT_BGR24, 
                               codec_context->width, codec_context->height);
                sws_scale(convert_context_rgb, (const uint8_t **)(frame_yuv->data), 
                          frame_yuv->linesize, 0, codec_context->height, 
                          frame_rgb->data, frame_rgb->linesize);
                
                cv::Mat mat(codec_context->height, codec_context->width, 
                            CV_8UC3, frame_rgb->data[0], frame_rgb->linesize[0]);
                frames.push_back(mat);
            }
        }
        av_packet_unref(&packet);
    }

end:
    if (format_context) {
        avformat_close_input(&format_context);
    }
    if (codec_context) {
        avcodec_close(codec_context);
    }
    if (frame_org) {
        av_free(frame_org);
    }
    if (convert_context_yuv) {
        sws_freeContext(convert_context_yuv);
    }
    if (convert_context_rgb) {
        sws_freeContext(convert_context_rgb);
    }
    return ret;

}
