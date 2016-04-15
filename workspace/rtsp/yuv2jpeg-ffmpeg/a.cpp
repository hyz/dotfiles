#include <stdio.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
};

template <typename... Args> void err_exit_(int lin_, char const* fmt, Args... a) {
    fprintf(stderr, fmt, lin_, a...);
    exit(127);
}
template <typename... Args> void err_msg_(int lin_, char const* fmt, Args... a) {
    fprintf(stderr, fmt, lin_, a...); //fflush(stderr);
}
#define ERR_EXIT(...) err_exit_(__LINE__, "%d: " __VA_ARGS__)
#define ERR_MSG(...) err_msg_(__LINE__, "%d: " __VA_ARGS__)
#define ensure(c, ...) if(!(c))ERR_EXIT(__VA_ARGS__)

int main(int argc, char* argv[])
{
    const char* out_file = "out.jpg";    //Output file
    int ret=0;
    int in_w=480, in_h=272;                           //YUV's width and height

    av_register_all();

    AVOutputFormat* oformat = av_guess_format("mjpeg", NULL, NULL);
    ensure(oformat, "av_guess_format");

    AVFormatContext* format_ctx = avformat_alloc_context();
    format_ctx->oformat = oformat;
        //Method 2. More simple
        //avformat_alloc_output_context2(&format_ctx, NULL, NULL, out_file);
        //AVOutputFormat* oformat = format_ctx->oformat;

    AVCodecContext* codec_ctx;
    AVStream* stream = avformat_new_stream(format_ctx, 0);
    ensure(stream, "avformat_new_stream");
    {
        codec_ctx = stream->codec;
        codec_ctx->codec_id = oformat->video_codec;
        codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;

        codec_ctx->pix_fmt = AV_PIX_FMT_YUVJ420P;
        codec_ctx->width = in_w;  
        codec_ctx->height = in_h;

        codec_ctx->time_base.num = 1;  
        codec_ctx->time_base.den = 25;   

        AVCodec* codec = avcodec_find_encoder(codec_ctx->codec_id);
        ensure(codec, "avcodec_find_encoder");

        if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
            ERR_EXIT("avcodec_open2");
        }
    }

    int y_size = codec_ctx->width * codec_ctx->height;

    AVPacket jpeg_packet;
    av_new_packet(&jpeg_packet, y_size*3);

    int got_jpeg = 0;
    {
        AVFrame* yuv_frame = av_frame_alloc();

        int size = avpicture_get_size(codec_ctx->pix_fmt, codec_ctx->width, codec_ctx->height);
        uint8_t* yuv_frame_buf = (uint8_t *)av_malloc(size);
        ensure(yuv_frame_buf, "av_malloc");
        avpicture_fill( (AVPicture*)yuv_frame, yuv_frame_buf, codec_ctx->pix_fmt, codec_ctx->width, codec_ctx->height);

        //Read YUV
        FILE * in_file = fopen(argv[1], "rb");
        if (fread(yuv_frame_buf, 1, y_size*3/2, in_file) <= 0) {
            ERR_EXIT("fread");
        }
        fclose(in_file);

        yuv_frame->data[0] = yuv_frame_buf;              // Y
        yuv_frame->data[1] = yuv_frame_buf+ y_size;      // U 
        yuv_frame->data[2] = yuv_frame_buf+ y_size*5/4;  // V
        //Encode
        ret = avcodec_encode_video2(codec_ctx, &jpeg_packet, yuv_frame, &got_jpeg);
        ensure(ret>=0, "avcodec_encode_video2");

        av_free(yuv_frame_buf);
        av_free(yuv_frame);
    }

    if (got_jpeg > 0){
        //jpeg_packet.stream_index = stream->index;
        ///Output some information
        av_dump_format(format_ctx, 0, out_file, 1);

        if (avio_open(&format_ctx->pb, out_file, AVIO_FLAG_READ_WRITE) < 0){
            ERR_EXIT("avio_open");
        }
        avformat_write_header(format_ctx, NULL);
        ret = av_write_frame(format_ctx, &jpeg_packet);
        av_write_trailer(format_ctx);
        avio_close(format_ctx->pb);
    }
    av_packet_unref(&jpeg_packet); // av_free_packet(&jpeg_packet);

    if (stream) {
        avcodec_close(stream->codec);
    }
    avformat_free_context(format_ctx);

    printf("%s\n", out_file);
    return 0;
}

