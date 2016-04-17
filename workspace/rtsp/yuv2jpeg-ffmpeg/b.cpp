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

    AVCodec *codec = avcodec_find_encoder(oformat->video_codec); //(AV_CODEC_ID_JPEG2000);
    ensure(codec, "avcodec_find_encoder");

    AVFormatContext* format_ctx = avformat_alloc_context();
    format_ctx->oformat = oformat;
        //Method 2. More simple
        //avformat_alloc_output_context2(&format_ctx, NULL, NULL, out_file);
        //AVOutputFormat* oformat = format_ctx->oformat;

    // AVStream* stream = avformat_new_stream(format_ctx, 0); ensure(stream, "avformat_new_stream");
    // codec_ctx = stream->codec;
    AVCodecContext* codec_ctx = avcodec_alloc_context3(codec); //
    ensure(codec_ctx, "avcodec_alloc_context3");
    {
        codec_ctx->codec_id = oformat->video_codec;
        codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;

        codec_ctx->pix_fmt = AV_PIX_FMT_YUVJ420P;
        codec_ctx->width = in_w;  
        codec_ctx->height = in_h;

        codec_ctx->time_base.num = 1;  
        codec_ctx->time_base.den = 25;   

        if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
            ERR_EXIT("avcodec_open2");
        }
    }
    AVFrame* picture = av_frame_alloc();
    picture->pts = 0;
    picture->height = codec_ctx->height;
    picture->width = codec_ctx->width;
    picture->format = codec_ctx->pix_fmt;

    int size = avpicture_get_size(codec_ctx->pix_fmt, codec_ctx->width, codec_ctx->height);
    uint8_t* picture_buf = (uint8_t *)av_malloc(size);
    ensure(picture_buf, "av_malloc");
    avpicture_fill((AVPicture *)picture, picture_buf, codec_ctx->pix_fmt, codec_ctx->width, codec_ctx->height);

    int y_size = codec_ctx->width * codec_ctx->height;

    AVPacket pkt;
    av_new_packet(&pkt, y_size*3);

    //Read YUV
    FILE * in_file = fopen(argv[1], "rb");
    if (fread(picture_buf, 1, y_size*3/2, in_file) <= 0) {
        ERR_EXIT("fread");
    }
    picture->data[0] = picture_buf;              // Y
    picture->data[1] = picture_buf+ y_size;      // U 
    picture->data[2] = picture_buf+ y_size*5/4;  // V

    //Encode
    int got_picture=0;
    ret = avcodec_encode_video2(codec_ctx, &pkt, picture, &got_picture);
    ensure(ret>=0, "avcodec_encode_video2");

    if (got_picture > 0){
        //pkt.stream_index = stream->index;
        ///Output some information
        av_dump_format(format_ctx, 0, out_file, 1);
        ///Output URL
        if (avio_open(&format_ctx->pb, out_file, AVIO_FLAG_READ_WRITE) < 0){
            ERR_EXIT("avio_open");
        }
        ///Write Header
        avformat_write_header(format_ctx, NULL);
        ///Write Frame
        ret = av_write_frame(format_ctx, &pkt);
        ///Write Trailer
        av_write_trailer(format_ctx);
    }

    av_free_packet(&pkt);

    printf("Encode Successful.\n");

    {
        avcodec_close(codec_ctx);//(stream->codec);
        av_free(picture);
        av_free(picture_buf);
    }
    avio_close(format_ctx->pb);
    avformat_free_context(format_ctx);

    fclose(in_file);

    return 0;
}

