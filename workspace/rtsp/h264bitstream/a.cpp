//// https://github.com/aizvorski/h264bitstream
#include <stdio.h>
#include "h264_stream.h"

uint8_t filebuf[1024*1024*4];

int main()
{
    h264_stream_t* h = h264_new();

    int bufsiz = fread(filebuf, 1, sizeof(filebuf), stdin);
    uint8_t* e;
    uint8_t* p = filebuf;
    uint8_t* end = filebuf + bufsiz;

    for (int p2[2]; find_nal_unit(p, end-p, p2, p2+1) > 0; p = e) {
        e = p + p2[1];
        p += p2[0];

        if (FILE* fp = fopen("/tmp/nalu.0","w")) {
            fwrite(p, 1, e-p, fp);
            fclose(fp);
        }
        read_nal_unit(h, p, e-p);

        debug_nal(h, h->nal);
        printf("\n");

h->sps->vui_parameters_present_flag = 1;
h->sps->vui.bitstream_restriction_flag = 1;
h->sps->vui.max_dec_frame_buffering = 1;
        debug_nal(h, h->nal);

        uint8_t out[1024];
        int outlen = write_nal_unit(h, out, sizeof(out));
        if (FILE* fp = fopen("/tmp/nalu.1","w")) {
            fwrite(out+1, outlen-1, 1, fp);
            fclose(fp);
        }

        if (h->nal->nal_unit_type == NAL_UNIT_TYPE_SPS)
            break;
        p = e;
    }
}

//h264_stream_t* h = h264_new();
//
//h->nal->nal_ref_idc = 0x03;
//h->nal->nal_unit_type = NAL_UNIT_TYPE_SPS;
//
//h->sps->profile_idc = 0x42;
//h->sps->level_idc = 0x33;
//h->sps->log2_max_frame_num_minus4 = 0x05;
//h->sps->log2_max_pic_order_cnt_lsb_minus4 = 0x06;
//h->sps->num_ref_frames = 0x01;
//h->sps->pic_width_in_mbs_minus1 = 0x2c;
//h->sps->pic_height_in_map_units_minus1 = 0x1d;
//h->sps->frame_mbs_only_flag = 0x01;
//
//int len = write_nal_unit(h, buf, size);

