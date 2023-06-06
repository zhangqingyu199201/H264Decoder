#include "AvcDecoder.h"

int Tool::ColumbDataUe(BitReader &br) {
  int leading_zero_bits = -1;
  for (int b = 0; !b; leading_zero_bits++) {
    b = br.ReadOneBits();
  }

  int code_num = (1 << leading_zero_bits) - 1 + br.ReadBits(leading_zero_bits);

  return code_num;
}

int  Tool::ColumbDataSe(BitReader &br) {
  int code_num = ColumbDataUe(br);

  return ((code_num & 1) ? 1 : -1) * (code_num / 2);
}

void Tool::DeleteCompeteCode(char* buff_ori, int len_ori, char*& buff_dst,
    int& len_dst) {
    if (buff_dst != nullptr) {
        free(buff_dst);
        buff_dst = nullptr;
        len_dst = 0;
    }

    char* buff_tmp = (char*)malloc(len_ori);
    int len_tmp = 0;

    for (int i = 0; i < len_ori; i++) {
        if (buff_ori[i] == 3 && i >= 2 && buff_ori[i - 1] == 0 &&
            buff_ori[i - 2] == 0) {
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

::std::string Tool::GetSliceType(int slice_type) {
    static ::std::string s_type[] = {
        "P", "B", "I", "SP", "SI",
        "P", "B", "I", "SP", "SI"
    };

    return s_type[slice_type];
}

void NALHeader::Parse(BitReader& br) {
    forbidden_zero_bit_ = br.ReadBits(1);
    nal_ref_idc_ = br.ReadBits(2);
    nal_unit_type_ = br.ReadBits(5);
}

void Hrd::Parse(BitReader &br) {
  cpb_cnt_minus1_ = Tool::ColumbDataUe(br);
  bit_rate_scale_ = br.ReadBits(4);
  cpb_size_scale_ = br.ReadBits(4);

  for (int i = 0; i < cpb_cnt_minus1_; i++) {
      SchedSei sched_sei;
      sched_sei.bit_rate_value_minus1_ = Tool::ColumbDataUe(br);
      sched_sei.cpb_size_value_minus1_ = Tool::ColumbDataUe(br);
      sched_sei.cbr_flag_ = Tool::ColumbDataUe(br);
      sched_sei_list_.push_back(sched_sei);
  }

  initial_cpb_removal_delay_length_minus1_ = br.ReadBits(5);
  cpb_removal_delay_length_minus1_ = br.ReadBits(5);
  dpb_output_delay_length_minus1_ = br.ReadBits(5);
  time_offset_length_ = br.ReadBits(5);
}

void VuiParameters::Parse(BitReader& br) {
    aspect_ratio_info_present_flag_ = br.ReadOneBits();
    if (aspect_ratio_info_present_flag_) {
        aspect_ratio_idc_ = br.ReadBits(8);
        if (aspect_ratio_idc_ == 255) {
            sar_width_ = br.ReadBits(16);
            sar_height_ = br.ReadBits(16);
        }
    }

    overscan_info_present_flag_ = br.ReadBits(1);
    if (overscan_info_present_flag_) {
        overscan_appropriate_flag_ = br.ReadBits(1);
    }

    video_signal_type_present_flag_ = br.ReadBits(1);
    if (video_signal_type_present_flag_) {
        video_format_ = br.ReadBits(3);
        video_full_range_flag_ = br.ReadBits(1);
        colour_description_present_flag_ = br.ReadBits(1);
        if (colour_description_present_flag_) {
            colour_primaries_ = br.ReadBits(8);
            transfer_characteristics_ = br.ReadBits(8);
            matrix_coefficients_ = br.ReadBits(8);
        }
    }

    chroma_loc_info_present_flag_ = br.ReadBits(1);
    if (chroma_loc_info_present_flag_) {
        chroma_sample_loc_type_top_field_ = Tool::ColumbDataUe(br);
        chroma_sample_loc_type_bottom_field_ =
            Tool::ColumbDataUe(br);
    }

    timing_info_present_flag_ = br.ReadBits(1);
    if (timing_info_present_flag_) {
        num_units_in_tick_ = br.ReadBits(32);
        time_scale_ = br.ReadBits(32);
        fixed_frame_rate_flag_ = br.ReadBits(1);
    }
    nal_hrd_parameters_present_flag_ = br.ReadBits(1);
    if (nal_hrd_parameters_present_flag_) {
        nal_hrd_.Parse(br);
    }
    vcl_hrd_parameters_present_flag_ = br.ReadBits(1);
    if (vcl_hrd_parameters_present_flag_) {
        vcl_hrd_.Parse(br);
    }
    if (nal_hrd_parameters_present_flag_ ||
        vcl_hrd_parameters_present_flag_) {
        low_delay_hrd_flag_ = br.ReadBits(1);
    }

    pic_struct_present_flag_ = br.ReadBits(1);
    bitstream_restriction_flag_ = br.ReadBits(1);

    if (bitstream_restriction_flag_) {
        motion_vectors_over_pic_boundaries_flag_ =
            br.ReadBits(1);
        max_bytes_per_pic_denom_ = Tool::ColumbDataUe(br);
        max_bits_per_mb_denom_ = Tool::ColumbDataUe(br);
        log2_max_mv_length_horizontal_ = Tool::ColumbDataUe(br);
        log2_max_mv_length_vertical_ = Tool::ColumbDataUe(br);
        num_reorder_frames_ = Tool::ColumbDataUe(br);
        max_dec_frame_buffering_ = Tool::ColumbDataUe(br);
    }
}

void SPS::Parse(BitReader& br_tmp) {
    int len = br_tmp.ReadBits(16);

    char* buff_ori = br_tmp.buff_ + br_tmp.loc_;
    int len_ori = br_tmp.buff_size_ - br_tmp.loc_ + 1;
    char* buff_dst = nullptr;
    int len_dst = 0;

    Tool::DeleteCompeteCode(buff_ori, len_ori, buff_dst, len_dst);
    br_tmp.ReadBits(len * 8);

    BitReader br(buff_dst, len_dst);
    header_.Parse(br);

    profile_idc_ = br.ReadBits(8);
    constrint_set0_flag_ = br.ReadBits(1);
    constrint_set1_flag_ = br.ReadBits(1);
    constrint_set2_flag_ = br.ReadBits(1);
    constrint_set3_flag_ = br.ReadBits(1);
    br.ReadBits(4);
    level_idc_ = br.ReadBits(8);

    seq_parameter_set_id_ = Tool::ColumbDataUe(br);

    switch (profile_idc_) {
    case 100:
    case 110:
    case 122:
    case 144: {
        chroma_format_idc_ = Tool::ColumbDataUe(br);
        if (chroma_format_idc_ == 3) {
            residual_colour_transform_flag_ = br.ReadOneBits();
        }
        bit_depth_luma_minus8_ = Tool::ColumbDataUe(br);
        bit_depth_chroma_minus8_ = Tool::ColumbDataUe(br);
        qpprime_y_zero_transform_bypass_flag_ = br.ReadOneBits();
        seq_scaling_matrix_present_flag_ = br.ReadOneBits();

        if (seq_scaling_matrix_present_flag_) {
            WAIT_TODO
        }
    } break;
    default:
        break;
    }

    log2_max_frame_num_minus4_ = Tool::ColumbDataUe(br);
    pic_order_cnt_type_ = Tool::ColumbDataUe(br);
    if (pic_order_cnt_type_ == 0) {
        log2_max_pic_order_cnt_lsb_minus4_ = Tool::ColumbDataUe(br);

    }
    else if (pic_order_cnt_type_ == 1) {
        delta_pic_order_always_zero_flag_ = br.ReadOneBits();
        offset_for_non_ref_pic_ = Tool::ColumbDataSe(br);
        offset_for_top_to_bottom_field_ = Tool::ColumbDataSe(br);
        num_ref_frames_in_pic_order_cnt_cycle_ = Tool::ColumbDataSe(br);

        for (int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle_; i++) {
            offset_for_ref_frame_.push_back(Tool::ColumbDataSe(br));
        }
    }

    num_ref_frames_ = Tool::ColumbDataUe(br);
    gaps_in_frame_num_value_allowed_flag_ = br.ReadOneBits();
    pic_width_in_mbs_minus1_ = Tool::ColumbDataUe(br);
    pic_height_in_map_units_minus1_ = Tool::ColumbDataUe(br);
    frame_mbs_only_flag_ = br.ReadOneBits();
    if (!frame_mbs_only_flag_) {
        mb_adaptive_frame_field_flag_ = br.ReadOneBits();
    }

    direct_8x8_inference_flag_ = br.ReadOneBits();
    frame_cropping_flag_ = br.ReadOneBits();
    if (frame_cropping_flag_) {
        frame_crop_left_offset_ = Tool::ColumbDataUe(br);
        frame_crop_right_offset_ = Tool::ColumbDataUe(br);
        frame_crop_top_offset_ = Tool::ColumbDataUe(br);
        frame_crop_bottom_offset_ = Tool::ColumbDataUe(br);
    }
    vui_parameters_present_flag_ = br.ReadOneBits();
    if (vui_parameters_present_flag_) {
        vui_parameters_.Parse(br);
    }

    free(buff_dst);
}

void PPS::Parse(BitReader& br_tmp) {
    int len = br_tmp.ReadBits(16);

    char* buff_ori = br_tmp.buff_ + br_tmp.loc_;
    int len_ori = br_tmp.buff_size_ - br_tmp.loc_ + 1;
    char* buff_dst = nullptr;
    int len_dst = 0;

    Tool::DeleteCompeteCode(buff_ori, len_ori, buff_dst, len_dst);
    br_tmp.ReadBits(len * 8);

    BitReader br(buff_dst, len_dst);
    header_.Parse(br);

    pic_parameter_set_id_ = Tool::ColumbDataUe(br);
    seq_parameter_set_id_ = Tool::ColumbDataUe(br);
    entropy_coding_mode_flag_ = br.ReadBits(1);
    pic_order_present_flag_ = br.ReadBits(1);
    num_slice_groups_minus1_ = Tool::ColumbDataUe(br);

    if (num_slice_groups_minus1_ > 0) {
        slice_group_map_type_ = Tool::ColumbDataUe(br);

        if (slice_group_map_type_ == 0) {
            for (int i = 0; i <= num_slice_groups_minus1_; i++) {
                run_length_minus1_.push_back(Tool::ColumbDataUe(br));
            }

        }
        else if (slice_group_map_type_ == 2) {
            for (int i = 0; i <= num_slice_groups_minus1_; i++) {
                SliceGroup group;
                group. top_left_ =(Tool::ColumbDataUe(br));
                group.bottom_right_ = (Tool::ColumbDataUe(br));
                slice_group_.push_back(group);

            }

        }
        else if ((slice_group_map_type_ == 3) ||
            (slice_group_map_type_ == 4) ||
            (slice_group_map_type_ == 5)) {
            slice_group_change_direction_flag_ = br.ReadBits(1);
            slice_group_change_rate_minus1_ = Tool::ColumbDataUe(br);
        }
        else if (slice_group_map_type_ == 6) {
            pic_size_in_map_units_minus1_ = Tool::ColumbDataUe(br);
            for (int i = 0; i <= pic_size_in_map_units_minus1_; i++) {

                slice_group_id_.push_back(Tool::ColumbDataUe(br));
            }
        }
    }

    num_ref_idx_l0_active_minus1_ = Tool::ColumbDataUe(br);
    num_ref_idx_l1_active_minus1_ = Tool::ColumbDataUe(br);
    weighted_pred_flag_ = br.ReadBits(1);

    weighted_bipred_idc_ = br.ReadBits(2);
    pic_init_qp_minus26_ = Tool::ColumbDataSe(br);
    pic_init_qs_minus26_ = Tool::ColumbDataSe(br);

    chroma_qp_index_offset_ = Tool::ColumbDataSe(br);
    deblocking_filter_control_present_flag_ = br.ReadBits(1);
    constrained_intra_pred_flag_ = br.ReadBits(1);
    redundant_pic_cnt_present_flag_ = br.ReadBits(1);

    if (br.buff_size_ - br.loc_ + 1 > 0) {
        transfrom_8X8_mode_flag_ = br.ReadBits(1);
        pic_scaling_matrix_present_flag_ = br.ReadBits(1);

        if (pic_scaling_matrix_present_flag_) {
            WAIT_TODO
        }

        second_chroma_qp_index_offset_ = Tool::ColumbDataSe(br);
    }

    free(buff_dst);
}

void AvcDecoderConfigure::Parse(BitReader& br) {
    version_ = br.ReadBits(8);
    profile_indict_ = br.ReadBits(8);
    profile_compatibility_ = br.ReadBits(8);
    level_indict_ = br.ReadBits(8);
    br.ReadBits(6);
    len_minus_one_ = br.ReadBits(2);
    br.ReadBits(3);
    int sps_size = br.ReadBits(5);
    for (int i = 0; i < sps_size; i++) {
        SPS* sps = new SPS();
        sps->Parse(br);
        sps_list_.push_back(sps);
    }
    int pps_size = br.ReadBits(8);
    for (int i = 0; i < pps_size; i++) {
        PPS* pps = new PPS();
        pps->Parse(br);
        pps_list_.push_back(pps);
    }
}

void SliceHeader::RefPicListReordering::Parse(BitReader& br, ::std::string& str_slice_type) {
    if (str_slice_type != "I" && str_slice_type != "SI") {
        ref_pic_list_reordering_flag_l0_ = br.ReadBits(1);
        if (ref_pic_list_reordering_flag_l0_) {
            int stop = 0;
            do {
                PicNumsIdc ref;
                ref.reordering_of_pic_nums_idc_ = Tool::ColumbDataUe(br);
                stop = ref.reordering_of_pic_nums_idc_;
                if (ref.reordering_of_pic_nums_idc_ == 0 ||
                    ref.reordering_of_pic_nums_idc_ == 1) {
                    ref.abs_diff_pic_num_minus1_ = Tool::ColumbDataUe(br);
                }
                else if (ref.reordering_of_pic_nums_idc_ == 2) {
                    ref.long_term_pic_num_ = Tool::ColumbDataUe(br);
                }
                l0_.push_back(ref);
            } while (stop != 3);
        }
    }

    if (str_slice_type == "B") {
        ref_pic_list_reordering_flag_l1_ =  br.ReadBits(1);
        if (ref_pic_list_reordering_flag_l1_) {
            int stop = 0;
            do {
                PicNumsIdc ref;
                ref.reordering_of_pic_nums_idc_ = Tool::ColumbDataUe(br);
                stop = ref.reordering_of_pic_nums_idc_;
                if (ref.reordering_of_pic_nums_idc_ == 0 ||
                    ref.reordering_of_pic_nums_idc_ == 1) {
                    ref.abs_diff_pic_num_minus1_ = Tool::ColumbDataUe(br);
                }
                else if (ref.reordering_of_pic_nums_idc_ == 2) {
                    ref.long_term_pic_num_ = Tool::ColumbDataUe(br);
                }
                l1_.push_back(ref);
            } while (stop != 3);
        }
    }
}

void SliceHeader::PredWeightTable::Parse(BitReader& br, SPS& sps, PPS& pps, ::std::string& str_slice_type) {
    luma_log2_weight_denom_ = Tool::ColumbDataUe(br);
    if (sps.chroma_format_idc_ != 0) {
        chroma_log2_weight_denom_ =
            Tool::ColumbDataUe(br);
    }

    for (int i = 0; i <= pps.num_ref_idx_l0_active_minus1_; i++) {
        LumaWeightData luma;
        luma.weight_flag = br.ReadBits(1);

        if (luma.weight_flag) {
            luma.weight = Tool::ColumbDataSe(br);
            luma.offset = Tool::ColumbDataSe(br);
        }

        ChromaWeightData chroma;
        if (sps.chroma_format_idc_ != 0) {
            chroma.weight_flag = br.ReadBits(1);

            if (chroma.weight_flag) {
                for (int j = 0; j < 2; j++) {
                    chroma.weight.push_back(Tool::ColumbDataSe(br));
                    chroma.offset.push_back(Tool::ColumbDataSe(br));
                }
            }
        }

        luma_weight_l0_.push_back(luma);
        chroma_weight_l0_.push_back(chroma);
    }

    if (str_slice_type == "B") {
        for (int i = 0; i <= pps.num_ref_idx_l1_active_minus1_; i++) {
            LumaWeightData luma;
            luma.weight_flag = br.ReadBits(1);

            if (luma.weight_flag) {
                luma.weight = Tool::ColumbDataSe(br);
                luma.offset = Tool::ColumbDataSe(br);
            }

            ChromaWeightData chroma;
            if (sps.chroma_format_idc_ != 0) {
                chroma.weight_flag = br.ReadBits(1);

                if (chroma.weight_flag) {
                    for (int j = 0; j < 2; j++) {
                        chroma.weight.push_back(Tool::ColumbDataSe(br));
                        chroma.offset.push_back(Tool::ColumbDataSe(br));
                    }
                }
            }
            luma_weight_l1_.push_back(luma);
            chroma_weight_l1_.push_back(chroma);
        }
    }
}

void SliceHeader::DefRefPicMarking::Parse(BitReader& br, int& nal_unit_type) {
    if (nal_unit_type == 5) {
        no_output_of_prior_pics_flag_ =
            br.ReadBits(1);
        long_term_reference_flag_ = br.ReadBits(1);
    } else {
        adaptive_ref_pic_marking_mode_flag_ =
            br.ReadBits(1);
        if (adaptive_ref_pic_marking_mode_flag_) {

            int op = 0;
            do {
                AdaptiveRefPicMarking ref_pic;
                ref_pic.memory_management_control_operation_ =
                    Tool::ColumbDataUe(br);
                op = ref_pic.memory_management_control_operation_;
                if (op == 1 || op == 3) {
                    ref_pic.difference_of_pic_nums_minus1_ =
                        Tool::ColumbDataUe(br);
                }

                if (op == 2) {
                    ref_pic.long_term_pic_num_ = Tool::ColumbDataUe(br);
                }

                if (op == 3 || op == 6) {
                    ref_pic.long_term_frame_idx_ = Tool::ColumbDataUe(br);
                }

                if (op == 4) {
                    ref_pic.max_long_term_frame_idx_plus1_ = Tool::ColumbDataUe(br);
                }

                ref_pic_marking_.push_back(ref_pic);

            } while (op != 0);
        }
    }
}

void SliceHeader::Parse(BitReader& br, AvcDecoderConfigure& configure) {
     if (header_.nal_unit_type_ == 6) {
        // sei
        return;
    }

    first_mb_in_slice_ = Tool::ColumbDataUe(br);
    slice_type_ = Tool::ColumbDataUe(br);
    std::string str_slice_type = Tool::GetSliceType(slice_type_);
    pic_parameter_set_id_ = Tool::ColumbDataUe(br);

    SPS& sps = *(configure.sps_list_[0]);
    PPS& pps = *(configure.pps_list_[pic_parameter_set_id_]);
    int frame_bits = sps.log2_max_frame_num_minus4_ + 4;
    frame_num_ = br.ReadBits(sps.log2_max_frame_num_minus4_ + 4);

    if (!sps.frame_mbs_only_flag_) {
        field_pic_flag_ = br.ReadBits(1);
        if (field_pic_flag_) {
            bottom_field_flag_ = br.ReadBits(1);
        }
    }

    if (header_.nal_unit_type_ == 5) {
        idr_pic_id_ = Tool::ColumbDataUe(br);
    }

    if (sps.pic_order_cnt_type_ == 0) {
        pic_order_cnt_lsb_ =
            br.ReadBits(sps.log2_max_pic_order_cnt_lsb_minus4_ + 4);

        if (pps.pic_order_present_flag_ && !field_pic_flag_) {
            delta_pic_order_cnt_bottom_ = Tool::ColumbDataSe(br);
        }
    }

    if (sps.pic_order_cnt_type_ == 1 &&
        !sps.delta_pic_order_always_zero_flag_) {
        delta_pic_order_cnt_[0] = Tool::ColumbDataSe(br);
        if (pps.pic_order_present_flag_ && !field_pic_flag_) {
            delta_pic_order_cnt_[1] = Tool::ColumbDataSe(br);
        }
    }

    if (pps.redundant_pic_cnt_present_flag_) {
        redundant_pic_cnt_ = Tool::ColumbDataUe(br);
    }

    if (str_slice_type == "B") {
        direct_spatial_mv_pred_flag_ = br.ReadBits(1);
    }

    if (str_slice_type == "P" ||
        str_slice_type == "B" ||
        str_slice_type == "SP") {
        num_ref_idx_active_override_flag_ = br.ReadBits(1);
        if (num_ref_idx_active_override_flag_) {
            num_ref_idx_l0_active_minus1_ = Tool::ColumbDataUe(br);
            if (str_slice_type == "B") {
                num_ref_idx_l1_active_minus1_ = Tool::ColumbDataUe(br);
            }
        }
    }

    ref_pic_list_reordering_.Parse(br, str_slice_type);

    if ((pps.weighted_pred_flag_ && (str_slice_type == "P" || str_slice_type == "SP")) 
        || (pps.weighted_bipred_idc_ && (str_slice_type == "B"))) {
        pred_weight_table_.Parse(br, sps, pps, str_slice_type);
    }

    if (header_.nal_ref_idc_ != 0) {
        dec_ref_pic_marking_.Parse(br, header_.nal_unit_type_);
    }

    if (pps.entropy_coding_mode_flag_ &&
        (str_slice_type != "I" && str_slice_type != "SI")) {
        cabac_init_idc_ = Tool::ColumbDataUe(br);
    }

    slice_qp_delta_ = Tool::ColumbDataSe(br);
    if (str_slice_type == "SP" || str_slice_type == "SI") {
        if (str_slice_type == "SP") {
            sp_for_switch_flag_ = br.ReadBits(1);
            slice_qs_delta_ = Tool::ColumbDataSe(br);
        }

        if (pps.deblocking_filter_control_present_flag_) {
            disable_deblocking_filter_idc_ = Tool::ColumbDataUe(br);
            if (disable_deblocking_filter_idc_ != 1) {
                slice_alpha_c0_offset_div2_ = Tool::ColumbDataSe(br);
                slice_beta_offset_div2_ = Tool::ColumbDataSe(br);
            }
        }

        if (pps.num_slice_groups_minus1_ > 0 &&
            pps.slice_group_map_type_ >= 3 && pps.slice_group_map_type_ <= 5) {
            slice_group_change_cycle_ =
                br.ReadBits(pps.num_slice_groups_minus1_ + 1);
        }
    }
}



void AvcDecoder::ParseNormal(BitReader &br_tmp) {
    while (br_tmp.buff_size_ - 1 >= br_tmp.loc_) {
        int len = br_tmp.ReadBits((configure_.len_minus_one_ + 1) * 8);
        char *buff_ori = br_tmp.buff_ + br_tmp.loc_;
        int len_ori = br_tmp.buff_size_ - br_tmp.loc_ + 1;
        char *buff_dst = nullptr;
        int len_dst = 0;

        Tool::DeleteCompeteCode(buff_ori, len_ori, buff_dst, len_dst);
        br_tmp.ReadBits(len * 8);

        BitReader br(buff_dst, len_dst);

        SliceHeader *v_data = new SliceHeader();
        v_data->header_.Parse(br);

        if (v_data->header_.nal_unit_type_ == 6) {
            // sei
            delete v_data;
        } else{
            v_data->Parse(br, configure_);
        }

        // br.ReadBits(pps->header_);
        free(buff_dst);
    }
}

void AvcDecoder::ParseFirstPacket(FlvTag *tag) {
  if (tag->tag_header_.video_tag_header_->frame_type_ == 5) {
    WAIT_TODO
  } else {
    if (tag->tag_header_.video_tag_header_->codec_id_ == 7) {
      BitReader br((char *)tag->data_, tag->data_size_);
      int pkt_type = tag->tag_header_.video_tag_header_->packet_type_;
      if (pkt_type == 0) {
          configure_.Parse(br);
      } else if (pkt_type == 1) {

        ParseNormal(br);
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
    FlvTag *tag_video_1 = flv.ReadTag();
    FlvTag *tag_video_2 = flv.ReadTag();


    AvcDecoder decoder;
    decoder.ParseFirstPacket(tag_video_0);
    decoder.ParseFirstPacket(tag_video_1);
    decoder.ParseFirstPacket(tag_video_2);

    END_TEST
}