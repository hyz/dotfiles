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

    for (int pp[2]; find_nal_unit(p, end-p, pp, pp+1) > 0; p = e) {
        e = p + pp[1];
        p += pp[0];

        if (FILE* fp = fopen("sps.0","w")) {
            fwrite(p, 1, e-p, fp);
            fclose(fp);
        }
        read_nal_unit(h, p, e-p);

        debug_nal(h, h->nal);
        printf("\n");

//h->sps->profile_idc = 0x42;
//h->sps->level_idc = 0x33;
//h->sps->log2_max_frame_num_minus4 = 0x05;
//h->sps->log2_max_pic_order_cnt_lsb_minus4 = 0x06;
//h->sps->num_ref_frames = 0x01;

//h->sps->profile_idc = 66;
//h->sps->level_idc = 10;
h->sps->num_ref_frames = 0;
//h->sps->log2_max_frame_num_minus4 = 0x05;
//h->sps->log2_max_pic_order_cnt_lsb_minus4 = 0x06;
//h->sps->direct_8x8_inference_flag = 0;

#if 1
h->sps->vui_parameters_present_flag = 1;
    h->sps->vui.bitstream_restriction_flag = 1;
        //h->sps->vui.motion_vectors_over_pic_boundaries_flag = 1;
        //h->sps->vui.log2_max_mv_length_horizontal = 10;
        //h->sps->vui.log2_max_mv_length_vertical = 10;
        h->sps->vui.num_reorder_frames = 1;
        h->sps->vui.max_dec_frame_buffering = 2;
#endif
        debug_nal(h, h->nal);

        uint8_t out[1024];
        int outlen = write_nal_unit(h, out, sizeof(out));
        if (FILE* fp = fopen("sps.1","w")) {
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

#if 0
==================== NAL ====================
 forbidden_zero_bit : 0 
 nal_ref_idc : 3 
 nal_unit_type : 7 ( Sequence parameter set ) 
======= SPS =======
 profile_idc : 100 
 constraint_set0_flag : 0 
 constraint_set1_flag : 0 
 constraint_set2_flag : 0 
 constraint_set3_flag : 0 
 constraint_set4_flag : 0 
 constraint_set5_flag : 0 
 reserved_zero_2bits : 0 
 level_idc : 40 
 seq_parameter_set_id : 0 
 chroma_format_idc : 1 
 residual_colour_transform_flag : 0 
 bit_depth_luma_minus8 : 0 
 bit_depth_chroma_minus8 : 0 
 qpprime_y_zero_transform_bypass_flag : 0 
 seq_scaling_matrix_present_flag : 0 
 log2_max_frame_num_minus4 : 0 
 pic_order_cnt_type : 0 
   log2_max_pic_order_cnt_lsb_minus4 : 0 
   delta_pic_order_always_zero_flag : 0 
   offset_for_non_ref_pic : 0 
   offset_for_top_to_bottom_field : 0 
   num_ref_frames_in_pic_order_cnt_cycle : 0 
 num_ref_frames : 1 
 gaps_in_frame_num_value_allowed_flag : 0 
 pic_width_in_mbs_minus1 : 119 
 pic_height_in_map_units_minus1 : 67 
 frame_mbs_only_flag : 1 
 mb_adaptive_frame_field_flag : 0 
 direct_8x8_inference_flag : 1 
 frame_cropping_flag : 1 
   frame_crop_left_offset : 0 
   frame_crop_right_offset : 0 
   frame_crop_top_offset : 0 
   frame_crop_bottom_offset : 4 
 vui_parameters_present_flag : 0 
=== VUI ===
 aspect_ratio_info_present_flag : 0 
   aspect_ratio_idc : 0 
     sar_width : 0 
     sar_height : 0 
 overscan_info_present_flag : 0 
   overscan_appropriate_flag : 0 
 video_signal_type_present_flag : 0 
   video_format : 0 
   video_full_range_flag : 0 
   colour_description_present_flag : 0 
     colour_primaries : 0 
   transfer_characteristics : 0 
   matrix_coefficients : 0 
 chroma_loc_info_present_flag : 0 
   chroma_sample_loc_type_top_field : 0 
   chroma_sample_loc_type_bottom_field : 0 
 timing_info_present_flag : 0 
   num_units_in_tick : 0 
   time_scale : 0 
   fixed_frame_rate_flag : 0 
 nal_hrd_parameters_present_flag : 0 
 vcl_hrd_parameters_present_flag : 0 
   low_delay_hrd_flag : 0 
 pic_struct_present_flag : 0 
 bitstream_restriction_flag : 0 
   motion_vectors_over_pic_boundaries_flag : 0 
   max_bytes_per_pic_denom : 0 
   max_bits_per_mb_denom : 0 
   log2_max_mv_length_horizontal : 0 
   log2_max_mv_length_vertical : 0 
   num_reorder_frames : 0 
   max_dec_frame_buffering : 0 
=== HRD ===
 cpb_cnt_minus1 : 0 
 bit_rate_scale : 0 
 cpb_size_scale : 0 
   bit_rate_value_minus1[0] : 0 
   cpb_size_value_minus1[0] : 0 
   cbr_flag[0] : 0 
 initial_cpb_removal_delay_length_minus1 : 0 
 cpb_removal_delay_length_minus1 : 0 
 dpb_output_delay_length_minus1 : 0 
 time_offset_length : 0 

==================== NAL ====================
 forbidden_zero_bit : 0 
 nal_ref_idc : 3 
 nal_unit_type : 7 ( Sequence parameter set ) 
======= SPS =======
 profile_idc : 66 
 constraint_set0_flag : 0 
 constraint_set1_flag : 0 
 constraint_set2_flag : 0 
 constraint_set3_flag : 0 
 constraint_set4_flag : 0 
 constraint_set5_flag : 0 
 reserved_zero_2bits : 0 
 level_idc : 10 
 seq_parameter_set_id : 0 
 chroma_format_idc : 1 
 residual_colour_transform_flag : 0 
 bit_depth_luma_minus8 : 0 
 bit_depth_chroma_minus8 : 0 
 qpprime_y_zero_transform_bypass_flag : 0 
 seq_scaling_matrix_present_flag : 0 
 log2_max_frame_num_minus4 : 0 
 pic_order_cnt_type : 0 
   log2_max_pic_order_cnt_lsb_minus4 : 0 
   delta_pic_order_always_zero_flag : 0 
   offset_for_non_ref_pic : 0 
   offset_for_top_to_bottom_field : 0 
   num_ref_frames_in_pic_order_cnt_cycle : 0 
 num_ref_frames : 0 
 gaps_in_frame_num_value_allowed_flag : 0 
 pic_width_in_mbs_minus1 : 119 
 pic_height_in_map_units_minus1 : 67 
 frame_mbs_only_flag : 1 
 mb_adaptive_frame_field_flag : 0 
 direct_8x8_inference_flag : 0 
 frame_cropping_flag : 1 
   frame_crop_left_offset : 0 
   frame_crop_right_offset : 0 
   frame_crop_top_offset : 0 
   frame_crop_bottom_offset : 4 
 vui_parameters_present_flag : 0 
=== VUI ===
 aspect_ratio_info_present_flag : 0 
   aspect_ratio_idc : 0 
     sar_width : 0 
     sar_height : 0 
 overscan_info_present_flag : 0 
   overscan_appropriate_flag : 0 
 video_signal_type_present_flag : 0 
   video_format : 0 
   video_full_range_flag : 0 
   colour_description_present_flag : 0 
     colour_primaries : 0 
   transfer_characteristics : 0 
   matrix_coefficients : 0 
 chroma_loc_info_present_flag : 0 
   chroma_sample_loc_type_top_field : 0 
   chroma_sample_loc_type_bottom_field : 0 
 timing_info_present_flag : 0 
   num_units_in_tick : 0 
   time_scale : 0 
   fixed_frame_rate_flag : 0 
 nal_hrd_parameters_present_flag : 0 
 vcl_hrd_parameters_present_flag : 0 
   low_delay_hrd_flag : 0 
 pic_struct_present_flag : 0 
 bitstream_restriction_flag : 0 
   motion_vectors_over_pic_boundaries_flag : 0 
   max_bytes_per_pic_denom : 0 
   max_bits_per_mb_denom : 0 
   log2_max_mv_length_horizontal : 0 
   log2_max_mv_length_vertical : 0 
   num_reorder_frames : 0 
   max_dec_frame_buffering : 0 
=== HRD ===
 cpb_cnt_minus1 : 0 
 bit_rate_scale : 0 
 cpb_size_scale : 0 
   bit_rate_value_minus1[0] : 0 
   cpb_size_value_minus1[0] : 0 
   cbr_flag[0] : 0 
 initial_cpb_removal_delay_length_minus1 : 0 
 cpb_removal_delay_length_minus1 : 0 
 dpb_output_delay_length_minus1 : 0 
 time_offset_length : 0 
#endif

