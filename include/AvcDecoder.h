#pragma once

#include "PublicHeader.h"
#include "FlvDemuxer.h"

class SPS;
class PPS;

class NALHeader {
public:
    int forbidden_zero_bit_;
    int ref_idc_;
    int unit_type_;
};

class Hrd {
public:
    int cpb_cnt_minus1;
    int bit_rate_scale;
    int cpb_size_scale;
    ::std::vector<int> bit_rate_value_minus1;
    ::std::vector<int> cpb_size_value_minus1;
    ::std::vector<int> cbr_flag;

    int initial_cpb_removal_delay_length_minus1;
    int cpb_removal_delay_length_minus1;
    int dpb_output_delay_length_minus1;
    int time_offset_length;

    void Parse(BitReader& br);

};

class SPS {
public:
    NALHeader header_;

    int profile_idc_;
    int constrint_set0_flag_;
    int constrint_set1_flag_;
    int constrint_set2_flag_;
    int constrint_set3_flag_;
    int level_idc_;

    int sps_id_;

    int chroma_format_idc_;
    int residual_colour_transform_flag_;
    int bit_depth_luma_minus8_;
    int bit_depth_chroma_minus8_;
    int qpprime_y_zero_transform_bypass_flag_;
    int seq_scaling_matrix_present_flag_;

    int log2_max_frame_num_minus4_;
    int pic_order_cnt_type_;
    int log2_max_pic_order_cnt_lsb_minus4_;
    int delta_pic_order_always_zero_flag_;
    int offset_for_non_ref_pic_;
    int offset_for_top_to_bottom_field_;
    int num_ref_frames_in_pic_order_cnt_cycle_;
    ::std::vector<int> offset_for_ref_frame_;
    int num_ref_frames_;
    int gaps_in_frame_num_value_allowed_flag_;
    int pic_width_in_mbs_minus1_;
    int pic_height_in_map_units_minus1_;
    int frame_mbs_only_flag_;
    int mb_adaptive_frame_field_flag_;
    int direct_8x8_inference_flag_;
    int frame_cropping_flag_;
    int frame_crop_left_offset_;
    int frame_crop_right_offset_;
    int frame_crop_top_offset_;
    int frame_crop_bottom_offset_;
    int vui_parameters_present_flag_;

    struct {
        int aspect_ratio_info_present_flag;
        int aspect_ratio_idc;
        int sar_width;
        int sar_height;
        int overscan_info_present_flag;
        int overscan_appropriate_flag;
        int video_signal_type_present_flag;
        int video_format;
        int video_full_range_flag;
        int colour_description_present_flag;
        int colour_primaries;
        int transfer_characteristics;
        int matrix_coefficients;
        int chroma_loc_info_present_flag;
        int chroma_sample_loc_type_top_field;
        int chroma_sample_loc_type_bottom_field;
        int timing_info_present_flag;
        int num_units_in_tick;
        int time_scale;
        int fixed_frame_rate_flag;
        int nal_hrd_parameters_present_flag;
        Hrd nal_hrd;
        int vcl_hrd_parameters_present_flag;
        Hrd vcl_hrd;
        int low_delay_hrd_flag;
        int pic_struct_present_flag;
        int bitstream_restriction_flag;
        int motion_vectors_over_pic_boundaries_flag;
        int max_bytes_per_pic_denom;
        int max_bits_per_mb_denom;
        int log2_max_mv_length_horizontal;
        int log2_max_mv_length_vertical;
        int num_reorder_frames;
        int max_dec_frame_buffering;

    } vui_parameters_;
};

class AvcDecoderConfigure {
public:
    int version_;
    int profile_indict_;
    int profile_compatibility_;
    int level_indict_;
    int len_minus_one_;
    ::std::vector<SPS *> sps_list_;
    ::std::vector<PPS *> pps_list_;

    ~AvcDecoderConfigure() {
        /*
        for (auto it : sps_list_) {
            delete it;
        }
        for (auto it : pps_list_) {
            delete it;
        }*/
    }
};

class AvcDecoder {
public:
    AvcDecoderConfigure configure_;
    void ParseFirstPacket(FlvTag *tag);

    SPS *ParseSps(BitReader &br);

    void ParseNalHeader(BitReader &br, NALHeader &header);
};

class AvcDecoderTest {
public:
    static void Test();
};