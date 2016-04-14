#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <algorithm>

extern "C" {
#include "libavcodec/avcodec.h"
//#  include "libavcodec/libavutil/mathematics.h"
}
int WriteJPEG (AVCodecContext *pCodecCtx, AVFrame *pFrame, int FrameNo);
static void yuv420p_save(AVFrame *pFrame, int width, int height, int iFrame);

struct H264File 
{
    uint8_t *nalu_begin, *nalu_end;
    int fd;
    uint8_t *m_begin, *m_end;

    H264File(const char* fn) {
        struct stat st;
        fd = open(fn, O_RDONLY);
        fstat(fd, &st); // printf("Size: %d\n", (int)st.st_size);
        m_begin = (uint8_t*)mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
        m_end = m_begin + st.st_size;

        nalu_begin = m_begin+4;
        nalu_end = find_(nalu_begin);
    }
    ~H264File() { close(fd); }

    bool next() {
        if (nalu_end+4 < m_end) {
            nalu_begin = nalu_end+4;
            nalu_end = find_(nalu_begin);
            return 1;
        }
        nalu_begin = nalu_end;
        return 0;
    }

    uint8_t* find_(uint8_t* pos) {
        uint8_t dx[] = {0, 0, 0, 1};
        return std::search(pos, m_end, &dx[0], &dx[4]);
    }
};

#define INBUF_SIZE 4096

void video_decode(char const *outfilename, char const *filename)
{
    AVCodec *codec;
    AVCodecContext *c= NULL;
    int frame, got_picture, len;
    //FILE *outf; //*f, 
    AVFrame *picture;
    //uint8_t inbuf[INBUF_SIZE + FF_INPUT_BUFFER_PADDING_SIZE];
    AVPacket avpkt;
    //int i;

    FF_INPUT_BUFFER_PADDING_SIZE;//memset(inbuf + INBUF_SIZE, 0, FF_INPUT_BUFFER_PADDING_SIZE);

    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
        fprintf(stderr, "codec not found\n");
        exit(1);
    }

    c = avcodec_alloc_context3(codec);
    picture = av_frame_alloc();

    if ((codec->capabilities)&CODEC_CAP_TRUNCATED)
        (c->flags) |= CODEC_FLAG_TRUNCATED;

    c->width = 480; //1920;
    c->height = 272; //1080;

    if (avcodec_open2(c, codec, NULL) < 0) {
        fprintf(stderr, "could not open codec\n");
        exit(1);
    }

    H264File h264f(filename);

    // f = fopen(filename, "rb");
    //if (!f) {
    //    fprintf(stderr, "could not open %s\n", filename);
    //    exit(1);
    //}
    //outf = fopen(outfilename,"w");
    //if(!outf){
    //    fprintf(stderr, "could not open %s\n", outfilename);
    //    exit(1);
    //}

    frame = 0;
    while (h264f.nalu_begin < h264f.nalu_end) {
        av_init_packet(&avpkt);
        avpkt.size = h264f.nalu_end - h264f.nalu_begin; //fread(inbuf, 1, INBUF_SIZE, f);
        avpkt.data = h264f.nalu_begin; //inbuf;

        len = avcodec_decode_video2(c, picture, &got_picture, &avpkt);
        if (len < 0) {
            fprintf(stderr, "Error while decoding frame %d\n", frame);
            exit(1);
        }
        if(got_picture && ++frame % 32 == 0) {
            printf("saving frame %3d\n", frame);
            fflush(stdout);
yuv420p_save(picture, c->width, c->height, frame);
        //WriteJPEG(c, picture, frame);
            //for(i=0; i<c->height; i++)
            //    fwrite(picture->data[0] + i * picture->linesize[0], 1, c->width, outf  );
            //for(i=0; i<c->height/2; i++)
            //    fwrite(picture->data[1] + i * picture->linesize[1], 1, c->width/2, outf );
            //for(i=0; i<c->height/2; i++)
            //    fwrite(picture->data[2] + i * picture->linesize[2], 1, c->width/2, outf );
            //frame++;
        }
        //avpkt.size -= len;
        //avpkt.data += len;
        if (!h264f.next())
            return;
    }

    av_init_packet(&avpkt);
    avpkt.data = NULL;
    avpkt.size = 0;
    len = avcodec_decode_video2(c,picture, &got_picture, &avpkt);
    if(got_picture && ++frame % 32 == 0) {
        printf("saving last frame %d\n",frame);
        fflush(stdout);
    //WriteJPEG(c, picture, frame);
yuv420p_save(picture, c->width, c->height, frame);
    }

    //fclose(f);
    //fclose(outf);

    avcodec_close(c);
    av_free(c);
    av_frame_free(&picture);
    printf(" %d\n", frame);
}
        //void save_frame(AVFrame* picture, int x)
        //{
        //    for(int i=0; i<c->height; i++)
        //        fwrite(picture->data[0] + i * picture->linesize[0], 1, c->width, outf );
        //    for(int i=0; i<c->height/2; i++)
        //        fwrite(picture->data[1] + i * picture->linesize[1], 1, c->width/2, outf );
        //    for(int i=0; i<c->height/2; i++)
        //        fwrite(picture->data[2] + i * picture->linesize[2], 1, c->width/2, outf );
        //}

int main(int argc, char **argv){
    avcodec_register_all();
    video_decode("test", "a.264");

    return 0;
}

static void yuv420p_save(AVFrame *pFrame, int width, int height, int iFrame)
{
	int i = 0;
	FILE *pFile;
	char szFilename[32];

	int height_half = height / 2, width_half = width / 2;
	int y_wrap = pFrame->linesize[0];
	int u_wrap = pFrame->linesize[1];
	int v_wrap = pFrame->linesize[2];

	unsigned char *y_buf = pFrame->data[0];
	unsigned char *u_buf = pFrame->data[1];
	unsigned char *v_buf = pFrame->data[2];
	sprintf(szFilename, "/tmp/a/%d.jpg", iFrame);
	pFile = fopen(szFilename, "wb");

	//save y
	for (i = 0; i <height; i++)
		fwrite(y_buf + i * y_wrap, 1, width, pFile);
	fprintf(stderr, "===>save Y success\n");

	//save u
	for (i = 0; i <height_half; i++)
		fwrite(u_buf + i * u_wrap, 1, width_half, pFile);
	fprintf(stderr, "===>save U success\n");

	//save v
	for (i = 0; i <height_half; i++)
		fwrite(v_buf + i * v_wrap, 1, width_half, pFile);
	fprintf(stderr, "===>save V success\n");

	fflush(pFile);
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
//    char                    JPEGFName[256];
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
//    sprintf ( JPEGFName, "%06d.jpg", FrameNo );
//    JPEGFile = fopen ( JPEGFName, "wb" );
//    fwrite ( Buffer, 1, BufSizActual, JPEGFile );
//    fclose ( JPEGFile );
//
//    avcodec_close ( pOCodecCtx );
//    free ( Buffer );
//    return ( BufSizActual );
//}
