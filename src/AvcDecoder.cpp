#include "AvcDecoder.h"

int ColumbDataUe(BitReader &br) {
    int leading_zero_bits = -1;
    for (int b = 0; !b; leading_zero_bits++) {
        b = br.ReadOneBits();
    }

    int code_num = (1 << leading_zero_bits) - 1 + br.ReadBits(leading_zero_bits);

    return code_num;
}

void Hrd::Parse(BitReader &br) {
    cpb_cnt_minus1 = ColumbDataUe(br);
    bit_rate_scale = br.ReadBits(4);
    cpb_size_scale = br.ReadBits(4);

    for (int i = 0; i < cpb_cnt_minus1; i++) {
        bit_rate_value_minus1.push_back(ColumbDataUe(br));
        cpb_size_value_minus1.push_back(ColumbDataUe(br));
        cbr_flag.push_back(br.ReadBits(4));
    }

    initial_cpb_removal_delay_length_minus1 = br.ReadBits(5);
    cpb_removal_delay_length_minus1 = br.ReadBits(5);
    dpb_output_delay_length_minus1 = br.ReadBits(5);
    time_offset_length = br.ReadBits(5);
}

void AvcDecoder::ParseNalHeader(BitReader &br, NALHeader &header) {
    header.forbidden_zero_bit_ = br.ReadBits(1);
    header.ref_idc_ = br.ReadBits(2);
    header.unit_type_ = br.ReadBits(5);
}

SPS *AvcDecoder::ParseSps(BitReader &br) {
    SPS *sps = new SPS();

    int len = br.ReadBits(16);

    ParseNalHeader(br, sps->header_);

    sps->profile_idc_ = br.ReadBits(8);
    sps->constrint_set0_flag_ = br.ReadBits(1);
    sps->constrint_set1_flag_ = br.ReadBits(1);
    sps->constrint_set2_flag_ = br.ReadBits(1);
    sps->constrint_set3_flag_ = br.ReadBits(1);
    br.ReadBits(4);
    sps->level_idc_ = br.ReadBits(8);

    sps->sps_id_ = ColumbDataUe(br);

    switch (sps->profile_idc_) {
    case 100:
    case 110:
    case 122:
    case 144: {
        sps->chroma_format_idc_ = ColumbDataUe(br);
        if (sps->chroma_format_idc_ == 3) {
            sps->residual_colour_transform_flag_ = br.ReadOneBits();
        }
        sps->bit_depth_luma_minus8_ = ColumbDataUe(br);
        sps->bit_depth_chroma_minus8_ = ColumbDataUe(br);
        sps->qpprime_y_zero_transform_bypass_flag_ = br.ReadOneBits();
        sps->seq_scaling_matrix_present_flag_ = br.ReadOneBits();

        if (sps->seq_scaling_matrix_present_flag_) {
            WAIT_TODO
        }
    } break;
    default:
        break;
    }

    sps->log2_max_frame_num_minus4_ = ColumbDataUe(br);
    sps->pic_order_cnt_type_ = ColumbDataUe(br);
    if (sps->pic_order_cnt_type_ == 0) {
        sps->log2_max_pic_order_cnt_lsb_minus4_ = ColumbDataUe(br);

    } else if (sps->pic_order_cnt_type_ == 1) {
        sps->delta_pic_order_always_zero_flag_ = br.ReadOneBits();
        sps->offset_for_non_ref_pic_ = ColumbDataUe(br);         //
        sps->offset_for_top_to_bottom_field_ = ColumbDataUe(br); //
        sps->num_ref_frames_in_pic_order_cnt_cycle_ = ColumbDataUe(br);

        for (int i = 0; i < sps->num_ref_frames_in_pic_order_cnt_cycle_; i++) {
            sps->offset_for_ref_frame_.push_back(ColumbDataUe(br)); //
        }
    }

    sps->num_ref_frames_ = ColumbDataUe(br);
    sps->gaps_in_frame_num_value_allowed_flag_ = br.ReadOneBits();
    sps->pic_width_in_mbs_minus1_ = ColumbDataUe(br);
    sps->pic_height_in_map_units_minus1_ = ColumbDataUe(br);
    sps->frame_mbs_only_flag_ = br.ReadOneBits();
    if (!sps->frame_mbs_only_flag_) {
        sps->mb_adaptive_frame_field_flag_ = br.ReadOneBits();
    }

    sps->direct_8x8_inference_flag_ = br.ReadOneBits();
    sps->frame_cropping_flag_ = br.ReadOneBits();
    if (sps->frame_cropping_flag_) {
        sps->frame_crop_left_offset_ = ColumbDataUe(br);
        sps->frame_crop_right_offset_ = ColumbDataUe(br);
        sps->frame_crop_top_offset_ = ColumbDataUe(br);
        sps->frame_crop_bottom_offset_ = ColumbDataUe(br);
    }
    sps->vui_parameters_present_flag_ = br.ReadOneBits();
    if (sps->vui_parameters_present_flag_) {
        sps->vui_parameters_.aspect_ratio_info_present_flag = br.ReadOneBits();
        if (sps->vui_parameters_.aspect_ratio_info_present_flag) {
            sps->vui_parameters_.aspect_ratio_idc = br.ReadBits(8);
            if (sps->vui_parameters_.aspect_ratio_idc == 255) {
                sps->vui_parameters_.sar_width = br.ReadBits(16);
                sps->vui_parameters_.sar_height = br.ReadBits(16);
            }
        }

        sps->vui_parameters_.overscan_info_present_flag = br.ReadBits(1);
        if (sps->vui_parameters_.overscan_info_present_flag) {
            sps->vui_parameters_.overscan_appropriate_flag = br.ReadBits(1);
        }

        sps->vui_parameters_.video_signal_type_present_flag = br.ReadBits(1);
        if (sps->vui_parameters_.video_signal_type_present_flag) {
            sps->vui_parameters_.video_format = br.ReadBits(3);
            sps->vui_parameters_.video_full_range_flag = br.ReadBits(1);
            sps->vui_parameters_.colour_description_present_flag = br.ReadBits(1);
            if (sps->vui_parameters_.colour_description_present_flag) {
                sps->vui_parameters_.colour_primaries = br.ReadBits(8);
                sps->vui_parameters_.transfer_characteristics = br.ReadBits(8);
                sps->vui_parameters_.matrix_coefficients = br.ReadBits(8);
            }
        }

        sps->vui_parameters_.chroma_loc_info_present_flag = br.ReadBits(1);
        if (sps->vui_parameters_.chroma_loc_info_present_flag) {
            sps->vui_parameters_.chroma_sample_loc_type_top_field = ColumbDataUe(br);
            sps->vui_parameters_.chroma_sample_loc_type_bottom_field = ColumbDataUe(br);
        }

        sps->vui_parameters_.timing_info_present_flag = br.ReadBits(1);
        if (sps->vui_parameters_.timing_info_present_flag) {
            sps->vui_parameters_.num_units_in_tick = br.ReadBits(32);
            sps->vui_parameters_.time_scale = br.ReadBits(32);
            sps->vui_parameters_.fixed_frame_rate_flag = br.ReadBits(1);
        }
        sps->vui_parameters_.nal_hrd_parameters_present_flag = br.ReadBits(1);
        if (sps->vui_parameters_.nal_hrd_parameters_present_flag) {
            sps->vui_parameters_.nal_hrd.Parse(br);
        }
        sps->vui_parameters_.vcl_hrd_parameters_present_flag = br.ReadBits(1);
        if (sps->vui_parameters_.vcl_hrd_parameters_present_flag) {
            sps->vui_parameters_.vcl_hrd.Parse(br);
        }
        if (sps->vui_parameters_.nal_hrd_parameters_present_flag
            || sps->vui_parameters_.vcl_hrd_parameters_present_flag) {
            sps->vui_parameters_.low_delay_hrd_flag = br.ReadBits(1);
        }

        sps->vui_parameters_.pic_struct_present_flag = br.ReadBits(1);
        sps->vui_parameters_.bitstream_restriction_flag = br.ReadBits(1);

        if (sps->vui_parameters_.bitstream_restriction_flag) {
            sps->vui_parameters_.motion_vectors_over_pic_boundaries_flag = br.ReadBits(1);
            sps->vui_parameters_.max_bytes_per_pic_denom = ColumbDataUe(br);
            sps->vui_parameters_.max_bits_per_mb_denom = ColumbDataUe(br);
            sps->vui_parameters_.log2_max_mv_length_horizontal = ColumbDataUe(br);
            sps->vui_parameters_.log2_max_mv_length_vertical = ColumbDataUe(br);
            sps->vui_parameters_.num_reorder_frames = ColumbDataUe(br);
            sps->vui_parameters_.max_dec_frame_buffering = ColumbDataUe(br);
        }
    }
}

void AvcDecoder::ParseFirstPacket(FlvTag *tag) {
    if (tag->tag_header_.video_tag_header->frame_type == 5) {
        WAIT_TODO
    } else {
        if (tag->tag_header_.video_tag_header->codec_id == 7) {
            BitReader br((char *)tag->data_, tag->data_size_);
            int pkt_type = tag->tag_header_.video_tag_header->packet_type;
            if (pkt_type == 0) {
                configure_.version_ = br.ReadBits(8);
                configure_.profile_indict_ = br.ReadBits(8);
                configure_.profile_compatibility_ = br.ReadBits(8);
                configure_.level_indict_ = br.ReadBits(8);
                br.ReadBits(6);
                configure_.len_minus_one_ = br.ReadBits(2);
                br.ReadBits(3);
                int sps_size = br.ReadBits(5);

                for (int i = 0; i < sps_size; i++) {
                    ParseSps(br);
                }

            } else {
                WAIT_TODO
            }

        } else {
            WAIT_TODO
        }
    }
}

void AvcDecoderTest::Test() {
    START_TEST

    FlvDemuxer flv(::std::string(MOVIE_PATH));
    flv.ReadHeader();

    FlvTag *tag_script = flv.ReadTag();
    FlvTag *tag_video_0 = flv.ReadTag();

    AvcDecoder decoder;
    decoder.ParseFirstPacket(tag_video_0);

    FlvTag *tag_video_1 = flv.ReadTag();
    FlvTag *tag_video_2 = flv.ReadTag();

    END_TEST
}