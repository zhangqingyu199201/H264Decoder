#pragma once

#include "PublicHeader.h"
#include "FlvDemuxer.h"




class Tool {
public:
    static int ColumbDataUe(BitReader& br);

    static int ColumbDataSe(BitReader& br);

    static void DeleteCompeteCode(char* buff_ori, int len_ori, char*& buff_dst,
        int& len_dst);

    static ::std::string GetSliceType(int slice_type);
};

class NALHeader {
public:
    int forbidden_zero_bit_;
    int nal_ref_idc_;
    int nal_unit_type_;

    void Parse(BitReader& br);
};

class Hrd {
public:
    int cpb_cnt_minus1_;
    int bit_rate_scale_;
    int cpb_size_scale_;

    struct SchedSei {
        int bit_rate_value_minus1_;
        int cpb_size_value_minus1_;
        int cbr_flag_;
    };
    ::std::vector<SchedSei> sched_sei_list_;

    int initial_cpb_removal_delay_length_minus1_;
    int cpb_removal_delay_length_minus1_;
    int dpb_output_delay_length_minus1_;
    int time_offset_length_;

    void Parse(BitReader& br);
};

class VuiParameters {
public:
    int aspect_ratio_info_present_flag_;
    int aspect_ratio_idc_;
    int sar_width_;
    int sar_height_;
    int overscan_info_present_flag_;
    int overscan_appropriate_flag_;
    int video_signal_type_present_flag_;
    int video_format_;
    int video_full_range_flag_;
    int colour_description_present_flag_;
    int colour_primaries_;
    int transfer_characteristics_;
    int matrix_coefficients_;
    int chroma_loc_info_present_flag_;
    int chroma_sample_loc_type_top_field_;
    int chroma_sample_loc_type_bottom_field_;
    int timing_info_present_flag_;
    int num_units_in_tick_;
    int time_scale_;
    int fixed_frame_rate_flag_;
    int nal_hrd_parameters_present_flag_;
    Hrd nal_hrd_;
    int vcl_hrd_parameters_present_flag_;
    Hrd vcl_hrd_;
    int low_delay_hrd_flag_;
    int pic_struct_present_flag_;
    int bitstream_restriction_flag_;
    int motion_vectors_over_pic_boundaries_flag_;
    int max_bytes_per_pic_denom_;
    int max_bits_per_mb_denom_;
    int log2_max_mv_length_horizontal_;
    int log2_max_mv_length_vertical_;
    int num_reorder_frames_;
    int max_dec_frame_buffering_;

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
    int reserved_zero_4bits_;
    int level_idc_;

    int seq_parameter_set_id_;

    int chroma_format_idc_;
    int residual_colour_transform_flag_;
    int bit_depth_luma_minus8_;
    int bit_depth_chroma_minus8_;
    int qpprime_y_zero_transform_bypass_flag_;
    int seq_scaling_matrix_present_flag_;

    // scaling list

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

    VuiParameters vui_parameters_;

    void Parse(BitReader& br);
};

class PPS {
public:
    NALHeader header_;

    int pic_parameter_set_id_;
    int seq_parameter_set_id_;
    int entropy_coding_mode_flag_;
    int pic_order_present_flag_;
    int num_slice_groups_minus1_;

    int slice_group_map_type_;
    struct SliceGroup {
        int top_left_;
        int bottom_right_;
    };

    ::std::vector<int> run_length_minus1_;
    ::std::vector<SliceGroup> slice_group_;
    int  slice_group_change_direction_flag_;
    int  slice_group_change_rate_minus1_;
    int  pic_size_in_map_units_minus1_;

    ::std::vector<int>   slice_group_id_;

    int num_ref_idx_l0_active_minus1_;
    int num_ref_idx_l1_active_minus1_;
    int weighted_pred_flag_;

    int weighted_bipred_idc_;
    int     pic_init_qp_minus26_;
    int     pic_init_qs_minus26_;

    int chroma_qp_index_offset_;
    int deblocking_filter_control_present_flag_;
    int constrained_intra_pred_flag_;
    int redundant_pic_cnt_present_flag_;
    int transfrom_8X8_mode_flag_;
    int pic_scaling_matrix_present_flag_;
    ::std::vector<int> pic_scaling_list_present_flag_;

    // scaling list
    int second_chroma_qp_index_offset_;

    void Parse(BitReader& br);
};

class AvcDecoderConfigure {
public:
    int version_;
    int profile_indict_;
    int profile_compatibility_;
    int level_indict_;
    int len_minus_one_;
    ::std::vector<SPS*> sps_list_;
    ::std::vector<PPS*> pps_list_;

    ~AvcDecoderConfigure() {
        for (auto it : sps_list_) {
            delete it;
        }
        for (auto it : pps_list_) {
            delete it;
        }
    }

    void Parse(BitReader& br);
};

class SliceHeader {
public:
    NALHeader header_;

    int first_mb_in_slice_;
    int slice_type_;
    int pic_parameter_set_id_;
    int frame_num_;
    int field_pic_flag_;
    int bottom_field_flag_;
    int idr_pic_id_;
    int pic_order_cnt_lsb_;
    int delta_pic_order_cnt_bottom_;
    ::std::array<int, 2> delta_pic_order_cnt_;
    int redundant_pic_cnt_;
    int direct_spatial_mv_pred_flag_;
    int num_ref_idx_active_override_flag_;
    int num_ref_idx_l0_active_minus1_;
    int num_ref_idx_l1_active_minus1_;

    //
    struct RefPicListReordering {
        struct PicNumsIdc {
            int reordering_of_pic_nums_idc_;
            int abs_diff_pic_num_minus1_;
            int long_term_pic_num_;
        };

        int ref_pic_list_reordering_flag_l0_;
        ::std::vector<PicNumsIdc> l0_;
        int ref_pic_list_reordering_flag_l1_;
        ::std::vector<PicNumsIdc> l1_;

        void Parse(BitReader& br, ::std::string& str_slice_type);
    } ref_pic_list_reordering_;

    struct PredWeightTable {
        struct LumaWeightData {
            int weight_flag;
            int weight;
            int offset;
        };

        struct ChromaWeightData {
            int weight_flag;
            ::std::vector<int> weight;
            ::std::vector<int> offset;
        };

        int luma_log2_weight_denom_;
        int chroma_log2_weight_denom_;

        ::std::vector<LumaWeightData> luma_weight_l0_;
        ::std::vector<ChromaWeightData> chroma_weight_l0_;
        ::std::vector<LumaWeightData> luma_weight_l1_;
        ::std::vector<ChromaWeightData> chroma_weight_l1_;

        void Parse(BitReader& br, SPS& sps, PPS& pps, ::std::string& str_slice_type);
    } pred_weight_table_;

    struct DefRefPicMarking {
        struct AdaptiveRefPicMarking {
            int memory_management_control_operation_;
            int difference_of_pic_nums_minus1_;
            int long_term_pic_num_;
            int long_term_frame_idx_;
            int max_long_term_frame_idx_plus1_;
        };

        int no_output_of_prior_pics_flag_;
        int long_term_reference_flag_;
        int adaptive_ref_pic_marking_mode_flag_;
        ::std::vector<AdaptiveRefPicMarking> ref_pic_marking_;

        void Parse(BitReader& br, int& nal_unit_type);
    } dec_ref_pic_marking_;

    int cabac_init_idc_;
    int slice_qp_delta_;

    int sp_for_switch_flag_;
    int slice_qs_delta_;
    int disable_deblocking_filter_idc_;
    int slice_alpha_c0_offset_div2_;
    int slice_beta_offset_div2_;
    int slice_group_change_cycle_;

    void Parse(BitReader& br, AvcDecoderConfigure& configure);
};
















class AvcDecoder {
public:
    AvcDecoderConfigure configure_;
    void ParseFirstPacket(FlvTag *tag);

    void ParseNormal(BitReader& br);
};

class AvcDecoderTest {
public:
    static void Test();
};