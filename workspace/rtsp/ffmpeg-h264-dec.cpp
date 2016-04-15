/// http://stackoverflow.com/questions/30510119/ffmpeg-c-api-decode-h264-error
//
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>

#include <opencv2/opencv.hpp>

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/avutil.h>
#include <libpostproc/postprocess.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#ifdef __cplusplus
} // end extern "C".
#endif // __cplusplus

int main(int argc, char* const argv[])
{
    AVCodec*            l_pCodec;
    AVCodecContext*     l_pAVCodecContext;
    SwsContext*         l_pSWSContext;
    AVFormatContext*    l_pAVFormatContext;
    AVFrame*            l_pAVFrame;
    AVFrame*            l_pAVFrameBGR;
    AVPacket            l_AVPacket;
    std::string         l_sFile;
    uint8_t*            l_puiBuffer;
    int                 l_iResult;
    int                 l_iFrameCount;
    int                 l_iGotFrame;
    int                 l_iDecodedBytes;
    int                 l_iVideoStreamIdx;
    int                 l_iNumBytes;
    cv::Mat             l_cvmImage;

    l_pCodec = NULL;
    l_pAVCodecContext = NULL;
    l_pSWSContext = NULL;
    l_pAVFormatContext = NULL;
    l_pAVFrame = NULL;
    l_pAVFrameBGR = NULL;
    l_puiBuffer = NULL;

    av_register_all();

    l_sFile = argv[1]; //"myvideo.ts";
    l_iResult = avformat_open_input(&l_pAVFormatContext, l_sFile.c_str(), NULL, NULL);

    if (l_iResult >= 0) {
        l_iResult = avformat_find_stream_info(l_pAVFormatContext, NULL);
        if (l_iResult >= 0) {
            for (int i=0; i<l_pAVFormatContext->nb_streams; i++) {
                if (l_pAVFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
                    l_iVideoStreamIdx = i;
                    l_pAVCodecContext = l_pAVFormatContext->streams[l_iVideoStreamIdx]->codec;
                    if (l_pAVCodecContext) {
                        l_pCodec = avcodec_find_decoder(l_pAVCodecContext->codec_id);
                    }
                    break;
                }
            }
        }
    }

    if (l_pCodec && l_pAVCodecContext)
    {
        l_iResult = avcodec_open2(l_pAVCodecContext, l_pCodec, NULL);

        if (l_iResult >= 0) {
            l_pAVFrame = av_frame_alloc();
            l_pAVFrameBGR = av_frame_alloc();

            l_iNumBytes = avpicture_get_size(PIX_FMT_BGR24,
                                             l_pAVCodecContext->width,
                                             l_pAVCodecContext->height);

            l_puiBuffer = (uint8_t *)av_malloc(l_iNumBytes*sizeof(uint8_t));

            avpicture_fill((AVPicture *)l_pAVFrameBGR,
                           l_puiBuffer,
                           PIX_FMT_RGB24,
                           l_pAVCodecContext->width,
                           l_pAVCodecContext->height);

            l_pSWSContext = sws_getContext(
                        l_pAVCodecContext->width,
                        l_pAVCodecContext->height,
                        l_pAVCodecContext->pix_fmt,
                        l_pAVCodecContext->width,
                        l_pAVCodecContext->height,
                        AV_PIX_FMT_BGR24,
                        SWS_BICUBIC,
                        NULL,
                        NULL,
                        NULL);

            while (av_read_frame(l_pAVFormatContext, &l_AVPacket) >= 0)
            {
                if (l_AVPacket.stream_index == l_iVideoStreamIdx)
                {
                    l_iDecodedBytes = avcodec_decode_video2(
                                l_pAVCodecContext,
                                l_pAVFrame,
                                &l_iGotFrame,
                                &l_AVPacket);

                    if (l_iGotFrame)
                    {
                        if (l_pSWSContext)
                        {
                            l_iResult = sws_scale(
                                        l_pSWSContext,
                                        l_pAVFrame->data,
                                        l_pAVFrame->linesize,
                                        0,
                                        l_pAVCodecContext->height,
                                        l_pAVFrameBGR->data,
                                        l_pAVFrameBGR->linesize);

                            if (l_iResult > 0)
                            {
                                l_cvmImage = cv::Mat(
                                            l_pAVFrame->height,
                                            l_pAVFrame->width,
                                            CV_8UC3,
                                            l_pAVFrameBGR->data[0],
                                        l_pAVFrameBGR->linesize[0]);

                                if (l_cvmImage.empty() == false) {
                                    cv::imshow("image", l_cvmImage);
                                    cv::waitKey(1);
                                }
                            }
                        }

                        l_iFrameCount++;
                    }
                }
            }
        }
    }
}

