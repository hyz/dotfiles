/// http://stackoverflow.com/questions/12018535/get-the-width-height-of-the-video-from-h-264-nalu/27636670#27636670
/// http://stackoverflow.com/questions/6394874/fetching-the-dimensions-of-a-h264video-stream/6477652#6477652
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

struct BitstreamReader
{
    BitstreamReader(uint8_t const* pStart, uint8_t const* pEnd) {
        begin_ = pStart;
        len_ = pEnd - pStart;
        bitx_ = 0;
    }

    unsigned int ReadBit() const
    {
        assert(bitx_ <= len_ * 8);
        int nIndex = bitx_ / 8;
        int nOffset = bitx_ % 8 + 1;

        bitx_ ++;
        return (begin_[nIndex] >> (8-nOffset)) & 0x01;
    }

    unsigned int ReadBits(int n) const
    {
        int r = 0;
        int i;
        for (i = 0; i < n; i++) {
            r |= ( ReadBit() << ( n - i - 1 ) );
        }
        return r;
    }

    unsigned int ReadExponentialGolombCode() const
    {
        int r = 0;
        int i = 0;

        while( (ReadBit() == 0) && (i < 32) ) {
            i++;
        }

        r = ReadBits(i);
        r += (1 << i) - 1;
        return r;
    }

    unsigned int ReadSE() const
    {
        int r = ReadExponentialGolombCode();
        if (r & 0x01) {
            r = (r+1)/2;
        } else {
            r = -(r/2);
        }
        return r;
    }

    mutable unsigned bitx_;
    uint8_t const* begin_;
    unsigned short len_;
};

struct frame_size { unsigned short width, height; };

frame_size Parse(BitstreamReader const& bs) //(const unsigned char * pStart, unsigned short nLen)
{
    int frame_crop_left_offset=0;
    int frame_crop_right_offset=0;
    int frame_crop_top_offset=0;
    int frame_crop_bottom_offset=0;

    int profile_idc = bs.ReadBits(8);
    int constraint_set0_flag = bs.ReadBit();
    int constraint_set1_flag = bs.ReadBit();
    int constraint_set2_flag = bs.ReadBit();
    int constraint_set3_flag = bs.ReadBit();
    int constraint_set4_flag = bs.ReadBit();
    int constraint_set5_flag = bs.ReadBit();
    int reserved_zero_2bits  = bs.ReadBits(2);
    int level_idc = bs.ReadBits(8);
    int seq_parameter_set_id = bs.ReadExponentialGolombCode();

    if (profile_idc == 100 || profile_idc == 110
            || profile_idc == 122 || profile_idc == 244
            || profile_idc == 44 || profile_idc == 83
            || profile_idc == 86 || profile_idc == 118 )
    {
        int chroma_format_idc = bs.ReadExponentialGolombCode();

        if( chroma_format_idc == 3 ) {
            int residual_colour_transform_flag = bs.ReadBit();
        }
        int bit_depth_luma_minus8 = bs.ReadExponentialGolombCode();
        int bit_depth_chroma_minus8 = bs.ReadExponentialGolombCode();
        int qpprime_y_zero_transform_bypass_flag = bs.ReadBit();
        int seq_scaling_matrix_present_flag = bs.ReadBit();

        if (seq_scaling_matrix_present_flag) {
            int i=0;
            for ( i = 0; i < 8; i++)
            {
                int seq_scaling_list_present_flag = bs.ReadBit();
                if (seq_scaling_list_present_flag)
                {
                    int sizeOfScalingList = (i < 6) ? 16 : 64;
                    int lastScale = 8;
                    int nextScale = 8;
                    int j=0;
                    for ( j = 0; j < sizeOfScalingList; j++)
                    {
                        if (nextScale != 0)
                        {
                            int delta_scale = bs.ReadSE();
                            nextScale = (lastScale + delta_scale + 256) % 256;
                        }
                        lastScale = (nextScale == 0) ? lastScale : nextScale;
                    }
                }
            }
        }
    }

    int log2_max_frame_num_minus4 = bs.ReadExponentialGolombCode();
    int pic_order_cnt_type = bs.ReadExponentialGolombCode();
    if( pic_order_cnt_type == 0 ) {
        int log2_max_pic_order_cnt_lsb_minus4 = bs.ReadExponentialGolombCode();

    } else if( pic_order_cnt_type == 1 ) {
        int delta_pic_order_always_zero_flag = bs.ReadBit();
        int offset_for_non_ref_pic = bs.ReadSE();
        int offset_for_top_to_bottom_field = bs.ReadSE();
        int num_ref_frames_in_pic_order_cnt_cycle = bs.ReadExponentialGolombCode();
        int i;
        for( i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++ ) {
            bs.ReadSE();
            //sps->offset_for_ref_frame[ i ] = bs.ReadSE();
        }
    }
    int max_num_ref_frames = bs.ReadExponentialGolombCode();
    int gaps_in_frame_num_value_allowed_flag = bs.ReadBit();
    int pic_width_in_mbs_minus1 = bs.ReadExponentialGolombCode();
    int pic_height_in_map_units_minus1 = bs.ReadExponentialGolombCode();
    int frame_mbs_only_flag = bs.ReadBit();
    if( !frame_mbs_only_flag ) {
        int mb_adaptive_frame_field_flag = bs.ReadBit();
    }
    int direct_8x8_inference_flag = bs.ReadBit();
    int frame_cropping_flag = bs.ReadBit();
    if( frame_cropping_flag ) {
        frame_crop_left_offset = bs.ReadExponentialGolombCode();
        frame_crop_right_offset = bs.ReadExponentialGolombCode();
        frame_crop_top_offset = bs.ReadExponentialGolombCode();
        frame_crop_bottom_offset = bs.ReadExponentialGolombCode();
    }
    int vui_parameters_present_flag = bs.ReadBit();
    //pStart++;

    unsigned short w,h;
    w = ((pic_width_in_mbs_minus1 +1)*16) - frame_crop_bottom_offset*2 - frame_crop_top_offset*2;
    h = ((2 - frame_mbs_only_flag)* (pic_height_in_map_units_minus1 +1) * 16) - (frame_crop_right_offset * 2) - (frame_crop_left_offset * 2);

    return frame_size{w,h};
}

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <boost/range/iterator_range.hpp>

struct h264file_mmap
{
    typedef boost::iterator_range<uint8_t*> range; // typedef std::pair<uint8_t*,uint8_t*> range;
    uint8_t *begin_, *end_;

    h264file_mmap(int fd) {
        struct stat st; // fd = open(fn, O_RDONLY);
        fstat(fd, &st); // printf("Size: %d\n", (int)st.st_size);
        void* p = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
        begin_ = (uint8_t*)p;
        end_ = begin_ + st.st_size;
        assert(begin_[0]=='\0'&& begin_[1]=='\0'&& begin_[2]=='\0'&& begin_[3]==1);
    }
    //~h264file_mmap() { if (begin_) free(begin_); } //munmap;

    range begin() const { return find_(begin_); }
    range next(range const& prev) const { return find_(prev.end()); }

    range find_(uint8_t* e) const {
        uint8_t* b = e;
        if (e+4 < end_) {
            uint8_t dx[] = {0, 0, 0, 1};
            b = e + 4;
            e = std::search(b, end_, &dx[0], &dx[4]);
        }
        return boost::make_iterator_range(b,e);
    }
};

int main(int argc, char* const argv[])
{
    int iHeight=0, iWidth=0;
    h264file_mmap fm(0);
    for (auto p = fm.begin(); !boost::empty(p); p = fm.next(p)) {
        if (p[0] == 0x67 || p[0] == 0x27) {
            BitstreamReader bs(p.begin()+1, boost::end(p));
            auto s = Parse(bs); //(p.begin()+1, boost::size(p)-1);
            printf("%dx%d\n",s.width,s.height);
        }
    }
    return 0;
}

