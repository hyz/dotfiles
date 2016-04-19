#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <algorithm>
#include <boost/scope_exit.hpp>
#include <boost/range/size.hpp>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
//#  include "libavcodec/libavutil/mathematics.h"
}
int save_frame_as_jpeg(char const* odir, AVCodecContext *pCodecCtx, AVFrame *pFrame, int idxN);
static void yuv420p_save(char const* odir, AVFrame *pFrame, int width, int height, int idxN);

struct H264File 
{
    typedef std::pair<uint8_t*,uint8_t*> range;
    uint8_t *begin_, *end_;

    H264File(int fd) {
        struct stat st; // fd = open(fn, O_RDONLY);
        fstat(fd, &st); // printf("Size: %d\n", (int)st.st_size);
        begin_ = (uint8_t*)mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
        end_ = begin_ + st.st_size;
        assert(begin_[0]=='\0'&& begin_[1]=='\0'&& begin_[2]=='\0'&& begin_[3]==1);
    }
    //~H264File() { close(fd); }

    range begin() const { return find_(begin_); }
    range next(range const& prev) const { return find_(prev.second); }

    range find_(uint8_t* e) const {
        uint8_t* b = e;
        if (e+4 < end_) {
            uint8_t dx[] = {0, 0, 0, 1};
            e = std::search(e+4, end_, &dx[0], &dx[4]);
        }
        return std::make_pair(b,e);
    }
};
struct H264File_reader : H264File {
    H264File_reader(const char* fn) : H264File(fd_ = open(fn, O_RDONLY)) {}
    int fd_;
    ~H264File_reader() { close(fd_); }
};

//#define INBUF_SIZE 4096

static void init_packet(AVPacket& avpkt, std::vector<uint8_t>& vec, uint8_t* beg, uint8_t* end)
{
    av_init_packet(&avpkt);
    avpkt.size = end - beg; //fread(inbuf, 1, INBUF_SIZE, f);
    vec.resize(avpkt.size + FF_INPUT_BUFFER_PADDING_SIZE);
    memcpy(&vec[0], beg, avpkt.size);
    memset(&vec[avpkt.size], 0, FF_INPUT_BUFFER_PADDING_SIZE);
    avpkt.data = &vec[0];
    // printf("nalu size %d\n", int(avpkt.size));
}

void video_decode(char const *out_dir, char const *h264_fn)
{
    AVCodec *codec;
    AVCodecContext *cctx= NULL;
    AVPacket avpkt;
    AVFrame *avframe;
    int naluN = 0, gotN = 0, got_picture;

    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
        fprintf(stderr, "codec not found\n");
        exit(1);
    }

    cctx = avcodec_alloc_context3(codec);
    avframe = av_frame_alloc();

    if ((codec->capabilities)&CODEC_CAP_TRUNCATED)
        (cctx->flags) |= CODEC_FLAG_TRUNCATED;

    cctx->width = 480; //1920;
    cctx->height = 272; //1080;

    if (avcodec_open2(cctx, codec, NULL) < 0) {
        fprintf(stderr, "could not open codec\n");
        exit(1);
    }

    // f = fopen(h264_fn, "rb");
    //if (!f) {
    //    fprintf(stderr, "could not open %s\n", h264_fn);
    //    exit(1);
    //}
    //outf = fopen(outfilename,"w");
    //if(!outf){
    //    fprintf(stderr, "could not open %s\n", outfilename);
    //    exit(1);
    //}

    H264File_reader h264f(h264_fn);
    std::vector<uint8_t> vec;

    for (auto nalu = h264f.begin() ; boost::size(nalu)>4; nalu = h264f.next(nalu)) {
        ++naluN;
        init_packet(avpkt, vec, nalu.first, nalu.second);

        int len = avcodec_decode_video2(cctx, avframe, &got_picture, &avpkt);
        if (len < 0) {
            fprintf(stderr, "Error while decoding naluN %d\n", naluN);
            continue; //exit(1);
        }
        if (got_picture && gotN++ % 64 == 0) {
            avframe->height = cctx->height;
            avframe->width = cctx->width;
            avframe->format = cctx->pix_fmt;
            avframe->pts = 0;
yuv420p_save(out_dir, avframe, cctx->width, cctx->height, gotN);
//save_frame_as_jpeg(out_dir, cctx, avframe, gotN);
        //WriteJPEG(cctx, avframe, gotN);
            //for(i=0; i<cctx->height; i++)
            //    fwrite(avframe->data[0] + i * avframe->linesize[0], 1, cctx->width, outf  );
            //for(i=0; i<cctx->height/2; i++)
            //    fwrite(avframe->data[1] + i * avframe->linesize[1], 1, cctx->width/2, outf );
            //for(i=0; i<cctx->height/2; i++)
            //    fwrite(avframe->data[2] + i * avframe->linesize[2], 1, cctx->width/2, outf );
        }
        //avpkt.size -= len;
        //avpkt.data += len;
    }

    av_init_packet(&avpkt);
    avpkt.data = NULL;
    avpkt.size = 0;
    int len = avcodec_decode_video2(cctx, avframe, &got_picture, &avpkt);
    if (len >= 0 && got_picture) {
        ++gotN;
        printf("last, naluN %d, gotN %d\n", naluN, gotN);
    //WriteJPEG(cctx, avframe, gotN);
yuv420p_save(out_dir, avframe, cctx->width, cctx->height, gotN);
//save_frame_as_jpeg(out_dir, cctx, avframe, gotN);
    }

    //fclose(f);
    //fclose(outf);

    avcodec_close(cctx);
    av_free(cctx);
    av_frame_free(&avframe);
    printf("naluN %d, gotN %d\n", naluN, gotN);
}
        //void save_frame(AVFrame* avframe, int x)
        //{
        //    for(int i=0; i<cctx->height; i++)
        //        fwrite(avframe->data[0] + i * avframe->linesize[0], 1, cctx->width, outf );
        //    for(int i=0; i<cctx->height/2; i++)
        //        fwrite(avframe->data[1] + i * avframe->linesize[1], 1, cctx->width/2, outf );
        //    for(int i=0; i<cctx->height/2; i++)
        //        fwrite(avframe->data[2] + i * avframe->linesize[2], 1, cctx->width/2, outf );
        //}

int main(int argc, char **argv)
{
    BOOST_SCOPE_EXIT(void){ printf("\n"); }BOOST_SCOPE_EXIT_END ;

    avcodec_register_all();
    video_decode(argv[1], argv[2]);

    return 0;
}

static void yuv420p_save(char const* odir, AVFrame *pFrame, int width, int height, int idxN)
{
	int i = 0;

	int height_half = height / 2, width_half = width / 2;
	int y_wrap = pFrame->linesize[0];
	int u_wrap = pFrame->linesize[1];
	int v_wrap = pFrame->linesize[2];

	unsigned char *y_buf = pFrame->data[0];
	unsigned char *u_buf = pFrame->data[1];
	unsigned char *v_buf = pFrame->data[2];

	//char szFilename[32]; sprintf(szFilename, "/tmp/a/%d.yuv", idxN);
    char fn_out[256];
    snprintf(fn_out,sizeof(fn_out), "%s/%d.yuv", odir, idxN);
	FILE* pFile = fopen(fn_out, "wb");

	//save y
	for (i = 0; i <height; i++)
		fwrite(y_buf + i * y_wrap, 1, width, pFile);
	//fprintf(stderr, "===>save Y success\n");

	//save u
	for (i = 0; i <height_half; i++)
		fwrite(u_buf + i * u_wrap, 1, width_half, pFile);
	//fprintf(stderr, "===>save U success\n");

	//save v
	for (i = 0; i <height_half; i++)
		fwrite(v_buf + i * v_wrap, 1, width_half, pFile);
	//fprintf(stderr, "===>save V success\n");

	//fflush(pFile);
	fclose(pFile);
}

//void ffmpeg_dump_yuv(int fno, AVPicture *pic, int width,int height)
//{
//    FILE *fp =0;
//    int i,j,shift;
//
//    char fn[32];
//    snprintf(fn,sizeof(fn), "dump-%d.yuv", fno);
//
//    fp = fopen(fn,"wb");
//    if(fp) {
//        for(i = 0; i < 3; i++) {
//            shift = (i == 0 ? 0:1);
//            char* yuv_factor = pic->data[i];
//            for(j = 0; j < (height>>shift); j++) {
//                fwrite(yuv_factor,(width>>shift),1,fp);
//                yuv_factor += pic->linesize[i];
//            }
//        }
//        fclose(fp);
//    }
//}

//int WriteJPEG (AVCodecContext *pCodecCtx, AVFrame *pFrame, int FrameNo)
//{
//    AVCodecContext         *pOCodecCtx;
//    AVCodec                *pOCodec;
//    uint8_t                *Buffer;
//    int                     BufSiz;
//    int                     BufSizActual;
//    int                     ImgFmt = AV_PIX_FMT_YUVJ420P; //for the newer ffmpeg version, this int to pixelformat
//    FILE                   *JPEGFile;
//    char                    out_jpeg[256];
//
//    BufSiz = avpicture_get_size(AV_PIX_FMT_YUVJ420P, pCodecCtx->width, pCodecCtx->height);
//
//    Buffer = (uint8_t *)malloc ( BufSiz );
//    if ( Buffer == NULL )
//        return ( 0 );
//    memset ( Buffer, 0, BufSiz );
//
//    //pOCodecCtx = avcodec_alloc_context ( );
//    pOCodecCtx = avcodec_alloc_context3(codec);
//    if ( !pOCodecCtx ) {
//        free ( Buffer );
//        return ( 0 );
//    }
//
//    pOCodecCtx->bit_rate      = pCodecCtx->bit_rate;
//    pOCodecCtx->width         = pCodecCtx->width;
//    pOCodecCtx->height        = pCodecCtx->height;
//    pOCodecCtx->pix_fmt       = ImgFmt;
//    pOCodecCtx->codec_id      = CODEC_ID_MJPEG;
//    pOCodecCtx->codec_type    = CODEC_TYPE_VIDEO;
//    pOCodecCtx->time_base.num = pCodecCtx->time_base.num;
//    pOCodecCtx->time_base.den = pCodecCtx->time_base.den;
//
//    pOCodec = avcodec_find_encoder ( pOCodecCtx->codec_id );
//    if ( !pOCodec ) {
//        free ( Buffer );
//        return ( 0 );
//    }
//    if ( avcodec_open ( pOCodecCtx, pOCodec ) < 0 ) {
//        free ( Buffer );
//        return ( 0 );
//    }
//
//    pOCodecCtx->mb_lmin        = pOCodecCtx->lmin = pOCodecCtx->qmin * FF_QP2LAMBDA;
//    pOCodecCtx->mb_lmax        = pOCodecCtx->lmax = pOCodecCtx->qmax * FF_QP2LAMBDA;
//    pOCodecCtx->flags          = CODEC_FLAG_QSCALE;
//    pOCodecCtx->global_quality = pOCodecCtx->qmin * FF_QP2LAMBDA;
//
//    pFrame->pts     = 1;
//    pFrame->quality = pOCodecCtx->global_quality;
//    BufSizActual = avcodec_encode_video(pOCodecCtx,Buffer,BufSiz,pFrame);
//
//    sprintf ( out_jpeg, "%06d.jpg", FrameNo );
//    JPEGFile = fopen ( out_jpeg, "wb" );
//    fwrite ( Buffer, 1, BufSizActual, JPEGFile );
//    fclose ( JPEGFile );
//
//    avcodec_close ( pOCodecCtx );
//    free ( Buffer );
//    return ( BufSizActual );
//}

int save_frame_as_jpeg(char const* odir, AVCodecContext *pCodecCtx, AVFrame *pFrame, int idxN)
{
    AVOutputFormat* oformat = av_guess_format("mjpeg", NULL, NULL);
    assert(oformat);

    AVCodec *jpegCodec = avcodec_find_encoder(oformat->video_codec); //(AV_CODEC_ID_JPEG2000);
    assert(jpegCodec);

    AVCodecContext *jpegContext = avcodec_alloc_context3(jpegCodec);
    assert(jpegContext);
    //avcodec_free_context(jpegContext);

    jpegContext->pix_fmt = AV_PIX_FMT_YUVJ420P; //pCodecCtx->pix_fmt; // ;
    jpegContext->height = pFrame->height;
    jpegContext->width = pFrame->width;

    if (avcodec_open2(jpegContext, jpegCodec, NULL) < 0) {
        return -1;
    }

    //FILE *JPEGFile;

    AVPacket packet = {0,0};//{.data = NULL, .size = 0};
    av_init_packet(&packet);
    int gotFrame;

    if (avcodec_encode_video2(jpegContext, &packet, pFrame, &gotFrame) < 0) {
        return -1;
    }

    {
        AVFormatContext* pFormatCtx = avformat_alloc_context();
        pFormatCtx->oformat = oformat;

        char outfn[256];
        snprintf(outfn,sizeof(outfn), "%s/%d.jpeg", odir, idxN);

        av_dump_format(pFormatCtx, 0, outfn, 1);
        if (avio_open(&pFormatCtx->pb, outfn, AVIO_FLAG_READ_WRITE) < 0){
            printf("Couldn't open output file.");
            return -1;
        }
        avformat_write_header(pFormatCtx, NULL);
        av_write_frame(pFormatCtx, &packet);
        av_write_trailer(pFormatCtx);

        avio_close(pFormatCtx->pb);
        avformat_free_context(pFormatCtx);
    }

    //JPEGFile = fopen(outfn, "wb");
    //fwrite(packet.data, 1, packet.size, JPEGFile);
    //fclose(JPEGFile);

    av_packet_unref(&packet); // av_free_packet(&packet); //
    avcodec_close(jpegContext);
    return 0;
}

