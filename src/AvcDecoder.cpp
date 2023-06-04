#include "AvcDecoder.h"

int ColumbDataUe(BitReader &br) {
    int leading_zero_bits = -1;
    for (int b = 0; !b; leading_zero_bits++) {
        b = br.ReadOneBits();
    }

    int code_num = (1 << leading_zero_bits) - 1 + br.ReadBits(leading_zero_bits);

    return code_num;
}

int ColumbDataSe(BitReader& br) {
    int code_num = ColumbDataUe(br);

    return ((code_num & 1) ? 1 : -1) *(code_num / 2);
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

void DeleteCompeteCode(char* buff_ori, int len_ori, char* &buff_dst, int &len_dst ) {
    if (buff_dst != nullptr) {
        free(buff_dst);
        buff_dst = nullptr;
        len_dst = 0;
    }
    
    char* buff_tmp = (char*)malloc(len_ori);
    int len_tmp = 0;

    for (int i = 0; i < len_ori; i++) {
        if (buff_ori[i] == 3 && i >= 2 && buff_ori[i - 1] == 0 && buff_ori[i - 2] == 0) {
            // skip this
        }
        else {
            buff_tmp[len_tmp] = buff_ori[i];
            len_tmp++;
        }
    }

    buff_dst = (char*)malloc(len_tmp);
    memcpy(buff_dst, buff_tmp, len_tmp);
    len_dst = len_tmp;
    free(buff_tmp);
}


SPS *AvcDecoder::ParseSps(BitReader &br_tmp) {
    SPS* sps = new SPS();
    int len = br_tmp.ReadBits(16);

    char* buff_ori = br_tmp.buff_ + br_tmp.loc_;
    int len_ori = br_tmp.buff_size_ - br_tmp.loc_ + 1;
    char* buff_dst = nullptr;
    int len_dst = 0;

    DeleteCompeteCode(buff_ori, len_ori, buff_dst, len_dst);
    br_tmp.ReadBits(len * 8);


    BitReader br(buff_dst, len_dst);
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
        sps->offset_for_non_ref_pic_ = ColumbDataSe(br);         
        sps->offset_for_top_to_bottom_field_ = ColumbDataSe(br); 
        sps->num_ref_frames_in_pic_order_cnt_cycle_ = ColumbDataSe(br);

        for (int i = 0; i < sps->num_ref_frames_in_pic_order_cnt_cycle_; i++) {
            sps->offset_for_ref_frame_.push_back(ColumbDataSe(br));
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

    free(buff_dst);

    return sps;
}


PPS* AvcDecoder::ParsePps(BitReader& br_tmp) {
    PPS* pps = new PPS();
    int len = br_tmp.ReadBits(16);

    char* buff_ori = br_tmp.buff_ + br_tmp.loc_;
    int len_ori = br_tmp.buff_size_ - br_tmp.loc_ + 1;
    char* buff_dst = nullptr;
    int len_dst = 0;

    DeleteCompeteCode(buff_ori, len_ori, buff_dst, len_dst);
    br_tmp.ReadBits(len * 8);


    BitReader br(buff_dst, len_dst);
    ParseNalHeader(br, pps->header_);


    pps->pic_parameter_set_id_ = ColumbDataUe(br);
    pps->seq_parameter_set_id_ = ColumbDataUe(br);
    pps->entropy_coding_mode_flag_ = br.ReadBits(1);
    pps->bottom_field_pic_order_in_frame_present_flag_ = br.ReadBits(1);
    pps->num_slice_groups_minus1_ = ColumbDataUe(br);

    if (pps->num_slice_groups_minus1_ > 0) {
        pps->slice_group_map_type_ = ColumbDataUe(br);
            
        if (pps->slice_group_map_type_ == 0) {
            for (int i = 0; i <= pps->num_slice_groups_minus1_; i++) {
                pps->run_length_minus1_.push_back(ColumbDataUe(br));

            }
            

        } else  if (pps->slice_group_map_type_ == 2) {
            for (int i = 0; i <= pps->num_slice_groups_minus1_; i++) {
                pps->top_left_.push_back(ColumbDataUe(br));
                pps->bottom_right_.push_back(ColumbDataUe(br));

            }


        }
        else  if ((pps->slice_group_map_type_ == 3)
            || (pps->slice_group_map_type_ == 4)
            || (pps->slice_group_map_type_ == 5)) {
    pps->slice_group_change_direction_flag_ = br.ReadBits(1);
    pps->slice_group_change_rate_minus1_ = ColumbDataUe(br);
        }
        else  if (pps->slice_group_map_type_ == 6) {
            pps->pic_size_in_map_units_minus1_ = ColumbDataUe(br);
            for (int i = 0; i <= pps->pic_size_in_map_units_minus1_; i++) {

                pps->slice_group_id_.push_back(ColumbDataUe(br));

            }
        }

    }

  
    pps->num_ref_idx_l0_active_minus1_ = ColumbDataUe(br);
    pps->num_ref_idx_l1_active_minus1_ = ColumbDataUe(br);
    pps->weighted_pred_flag_ = br.ReadBits(1);

    pps->weighted_bipred_idc_ = br.ReadBits(2);
    pps->pic_init_qp_minus26_ = ColumbDataSe(br); 
    pps->pic_init_qs_minus26_ = ColumbDataSe(br); 

    pps->chroma_qp_index_offset_ = ColumbDataSe(br); 
    pps->deblocking_filter_control_present_flag_ = br.ReadBits(1);
    pps->constrained_intra_pred_flag_ = br.ReadBits(1);
    pps->redundant_pic_cnt_present_flag_ = br.ReadBits(1);

    if (br.buff_size_ - br.loc_ + 1 > 0) {
        pps->transfrom_8X8_mode_flag_ = br.ReadBits(1);
        pps->pic_scaling_matrix_present_flag_ = br.ReadBits(1);

        if (pps->pic_scaling_matrix_present_flag_) {
            WAIT_TODO
        }

        pps->second_chroma_qp_index_offset_ = ColumbDataSe(br); 
    }






    free(buff_dst);
    return pps;
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
                        configure_.sps_list_.push_back( ParseSps(br));
                }
                int pps_size = br.ReadBits(8);
                for (int i = 0; i < pps_size; i++) {
                    configure_.pps_list_.push_back(ParsePps(br));
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