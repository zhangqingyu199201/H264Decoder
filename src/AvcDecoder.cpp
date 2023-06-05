#include "AvcDecoder.h"

int ColumbDataUe(BitReader &br) {
  int leading_zero_bits = -1;
  for (int b = 0; !b; leading_zero_bits++) {
    b = br.ReadOneBits();
  }

  int code_num = (1 << leading_zero_bits) - 1 + br.ReadBits(leading_zero_bits);

  return code_num;
}

int ColumbDataSe(BitReader &br) {
  int code_num = ColumbDataUe(br);

  return ((code_num & 1) ? 1 : -1) * (code_num / 2);
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

void DeleteCompeteCode(char *buff_ori, int len_ori, char *&buff_dst,
                       int &len_dst) {
  if (buff_dst != nullptr) {
    free(buff_dst);
    buff_dst = nullptr;
    len_dst = 0;
  }

  char *buff_tmp = (char *)malloc(len_ori);
  int len_tmp = 0;

  for (int i = 0; i < len_ori; i++) {
    if (buff_ori[i] == 3 && i >= 2 && buff_ori[i - 1] == 0 &&
        buff_ori[i - 2] == 0) {
      // skip this
    } else {
      buff_tmp[len_tmp] = buff_ori[i];
      len_tmp++;
    }
  }

  buff_dst = (char *)malloc(len_tmp);
  memcpy(buff_dst, buff_tmp, len_tmp);
  len_dst = len_tmp;
  free(buff_tmp);
}

SPS *AvcDecoder::ParseSps(BitReader &br_tmp) {
  SPS *sps = new SPS();
  int len = br_tmp.ReadBits(16);

  char *buff_ori = br_tmp.buff_ + br_tmp.loc_;
  int len_ori = br_tmp.buff_size_ - br_tmp.loc_ + 1;
  char *buff_dst = nullptr;
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
      sps->vui_parameters_.chroma_sample_loc_type_bottom_field =
          ColumbDataUe(br);
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
    if (sps->vui_parameters_.nal_hrd_parameters_present_flag ||
        sps->vui_parameters_.vcl_hrd_parameters_present_flag) {
      sps->vui_parameters_.low_delay_hrd_flag = br.ReadBits(1);
    }

    sps->vui_parameters_.pic_struct_present_flag = br.ReadBits(1);
    sps->vui_parameters_.bitstream_restriction_flag = br.ReadBits(1);

    if (sps->vui_parameters_.bitstream_restriction_flag) {
      sps->vui_parameters_.motion_vectors_over_pic_boundaries_flag =
          br.ReadBits(1);
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

PPS *AvcDecoder::ParsePps(BitReader &br_tmp) {
  PPS *pps = new PPS();
  int len = br_tmp.ReadBits(16);

  char *buff_ori = br_tmp.buff_ + br_tmp.loc_;
  int len_ori = br_tmp.buff_size_ - br_tmp.loc_ + 1;
  char *buff_dst = nullptr;
  int len_dst = 0;

  DeleteCompeteCode(buff_ori, len_ori, buff_dst, len_dst);
  br_tmp.ReadBits(len * 8);

  BitReader br(buff_dst, len_dst);
  ParseNalHeader(br, pps->header_);

  pps->pic_parameter_set_id_ = ColumbDataUe(br);
  pps->seq_parameter_set_id_ = ColumbDataUe(br);
  pps->entropy_coding_mode_flag_ = br.ReadBits(1);
  pps->pic_order_present_flag_ = br.ReadBits(1);
  pps->num_slice_groups_minus1_ = ColumbDataUe(br);

  if (pps->num_slice_groups_minus1_ > 0) {
    pps->slice_group_map_type_ = ColumbDataUe(br);

    if (pps->slice_group_map_type_ == 0) {
      for (int i = 0; i <= pps->num_slice_groups_minus1_; i++) {
        pps->run_length_minus1_.push_back(ColumbDataUe(br));
      }

    } else if (pps->slice_group_map_type_ == 2) {
      for (int i = 0; i <= pps->num_slice_groups_minus1_; i++) {
        pps->top_left_.push_back(ColumbDataUe(br));
        pps->bottom_right_.push_back(ColumbDataUe(br));
      }

    } else if ((pps->slice_group_map_type_ == 3) ||
               (pps->slice_group_map_type_ == 4) ||
               (pps->slice_group_map_type_ == 5)) {
      pps->slice_group_change_direction_flag_ = br.ReadBits(1);
      pps->slice_group_change_rate_minus1_ = ColumbDataUe(br);
    } else if (pps->slice_group_map_type_ == 6) {
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

class VideoData {
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
    int ref_pic_list_reordering_flag_l0;

    struct RefPicList {
      int reordering_of_pic_nums_idc;
      int abs_diff_pic_num_minus1;
      int long_term_pic_num;
    };

    ::std::vector<RefPicList> l0;

    int ref_pic_list_reordering_flag_l1;
    ::std::vector<RefPicList> l1;

  } ref_pic_list_reordering;

  struct PredWeightTable {
    int luma_log2_weight_denom;
    int chroma_log2_weight_denom;

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

    ::std::vector<LumaWeightData> luma_weight_l0;
    ::std::vector<ChromaWeightData> chroma_weight_l0;
    ::std::vector<LumaWeightData> luma_weight_l1;
    ::std::vector<ChromaWeightData> chroma_weight_l1;
  } pred_weight_table_;

  struct DefRefPicMarking {
    int no_output_of_prior_pics_flag;
    int long_term_reference_flag;
    int adaptive_ref_pic_marking_mode_flag;

    struct AdaptiveRefPicMarking {
      int memory_management_control_operation;
      int difference_of_pic_nums_minus1;
      int long_term_pic_num;
      int long_term_frame_idx;
      int max_long_term_frame_idx_plus1;
    };
    ::std::vector<AdaptiveRefPicMarking> ref_pic_marking;
  } dec_ref_marking_;

  int cabac_init_idc_;
  int slice_qp_delta_;

  int sp_for_switch_flag_;
  int slice_qs_delta_;
  int disable_deblocking_filter_idc_;
  int slice_alpha_c0_offset_div2_;
  int slice_beta_offset_div2_;
  int slice_group_change_cycle_;
};

void AvcDecoder::ParseNormal(BitReader &br_tmp) {
  while (br_tmp.buff_size_ - 1 >= br_tmp.loc_) {

    int len = br_tmp.ReadBits((configure_.len_minus_one_ + 1) * 8);

    char *buff_ori = br_tmp.buff_ + br_tmp.loc_;
    int len_ori = br_tmp.buff_size_ - br_tmp.loc_ + 1;
    char *buff_dst = nullptr;
    int len_dst = 0;

    DeleteCompeteCode(buff_ori, len_ori, buff_dst, len_dst);
    br_tmp.ReadBits(len * 8);

    BitReader br(buff_dst, len_dst);

    VideoData *v_data = new VideoData();
    ParseNalHeader(br, v_data->header_);

    if (v_data->header_.unit_type_ == 6) {
      // sei
      delete v_data;
    } else{

      v_data->first_mb_in_slice_ = ColumbDataUe(br);
      v_data->slice_type_ = ColumbDataUe(br);
      v_data->pic_parameter_set_id_ = ColumbDataUe(br);

      SPS &sps = *(configure_.sps_list_[0]);
      PPS &pps = *(configure_.pps_list_[0]);
      int frame_bits = sps.log2_max_frame_num_minus4_ + 4;
      v_data->frame_num_ = br.ReadBits(sps.log2_max_frame_num_minus4_ + 4);

      if (!sps.frame_mbs_only_flag_) {

        v_data->field_pic_flag_ = br.ReadBits(1);
        if (v_data->field_pic_flag_) {
          v_data->bottom_field_flag_ = br.ReadBits(1);
        }
      }

      if (v_data->header_.unit_type_ == 5) {
        v_data->idr_pic_id_ = ColumbDataUe(br);
      }

      if (sps.pic_order_cnt_type_ == 0) {
        v_data->pic_order_cnt_lsb_ =
            br.ReadBits(sps.log2_max_pic_order_cnt_lsb_minus4_ + 4);

        if (pps.pic_order_present_flag_ && !v_data->field_pic_flag_) {
          v_data->delta_pic_order_cnt_bottom_ = ColumbDataSe(br);
        }
      }

      if (sps.pic_order_cnt_type_ == 1 &&
          !sps.delta_pic_order_always_zero_flag_) {
        v_data->delta_pic_order_cnt_[0] = ColumbDataSe(br);
        if (pps.pic_order_present_flag_ && !v_data->field_pic_flag_) {
          v_data->delta_pic_order_cnt_[1] = ColumbDataSe(br);
        }
      }

      if (pps.redundant_pic_cnt_present_flag_) {
        v_data->redundant_pic_cnt_ = ColumbDataUe(br);
      }

      if (v_data->slice_type_ == 1 || v_data->slice_type_ == 6) {
        v_data->direct_spatial_mv_pred_flag_ = br.ReadBits(1);
      }

      if (v_data->slice_type_ == 0 || v_data->slice_type_ == 5 ||
          v_data->slice_type_ == 1 || v_data->slice_type_ == 6 ||
          v_data->slice_type_ == 3 || v_data->slice_type_ == 8) {
        v_data->num_ref_idx_active_override_flag_ = br.ReadBits(1);
        if (v_data->num_ref_idx_active_override_flag_) {
          v_data->num_ref_idx_l0_active_minus1_ = ColumbDataUe(br);
          if (v_data->slice_type_ == 1 || v_data->slice_type_ == 6) {
            v_data->num_ref_idx_l1_active_minus1_ = ColumbDataUe(br);
          }
        }
      }

      if (v_data->slice_type_ != 2 && v_data->slice_type_ != 4 &&
          v_data->slice_type_ != 7 && v_data->slice_type_ != 9) {
        v_data->ref_pic_list_reordering.ref_pic_list_reordering_flag_l0 =
            br.ReadBits(1);
        if (v_data->ref_pic_list_reordering.ref_pic_list_reordering_flag_l0) {
          int stop = 0;
          do {
            VideoData::RefPicListReordering::RefPicList ref;
            ref.reordering_of_pic_nums_idc = ColumbDataUe(br);
            stop = ref.reordering_of_pic_nums_idc;
            if (ref.reordering_of_pic_nums_idc == 0 ||
                ref.reordering_of_pic_nums_idc == 1) {
              ref.abs_diff_pic_num_minus1 = ColumbDataUe(br);
            } else if (ref.reordering_of_pic_nums_idc == 2) {
              ref.long_term_pic_num = ColumbDataUe(br);
            }
            v_data->ref_pic_list_reordering.l0.push_back(ref);
          } while (stop != 3);
        }
      }

      if (v_data->slice_type_ == 1 || v_data->slice_type_ == 6) {
        v_data->ref_pic_list_reordering.ref_pic_list_reordering_flag_l1 =
            br.ReadBits(1);
        if (v_data->ref_pic_list_reordering.ref_pic_list_reordering_flag_l1) {
          int stop = 0;
          do {
            VideoData::RefPicListReordering::RefPicList ref;
            ref.reordering_of_pic_nums_idc = ColumbDataUe(br);
            stop = ref.reordering_of_pic_nums_idc;
            if (ref.reordering_of_pic_nums_idc == 0 ||
                ref.reordering_of_pic_nums_idc == 1) {
              ref.abs_diff_pic_num_minus1 = ColumbDataUe(br);
            } else if (ref.reordering_of_pic_nums_idc == 2) {
              ref.long_term_pic_num = ColumbDataUe(br);
            }
            v_data->ref_pic_list_reordering.l1.push_back(ref);
          } while (stop != 3);
        }
      }

      if ((pps.weighted_pred_flag_ &&
           (v_data->slice_type_ == 0 || v_data->slice_type_ == 3 ||
            v_data->slice_type_ == 5 || v_data->slice_type_ == 8)) ||
          (pps.weighted_bipred_idc_ &&
           (v_data->slice_type_ == 1 || v_data->slice_type_ == 6))) {

        v_data->pred_weight_table_.luma_log2_weight_denom = ColumbDataUe(br);
        if (sps.chroma_format_idc_ != 0) {
          v_data->pred_weight_table_.chroma_log2_weight_denom =
              ColumbDataUe(br);
        }

        for (int i = 0; i <= pps.num_ref_idx_l0_active_minus1_; i++) {

          VideoData::PredWeightTable::LumaWeightData luma;
          luma.weight_flag = br.ReadBits(1);

          if (luma.weight_flag) {
            luma.weight = ColumbDataSe(br);
            luma.offset = ColumbDataSe(br);
          }

          VideoData::PredWeightTable::ChromaWeightData chroma;
          if (sps.chroma_format_idc_ != 0) {
            chroma.weight_flag = br.ReadBits(1);

            if (chroma.weight_flag) {
              for (int j = 0; j < 2; j++) {
                chroma.weight.push_back(ColumbDataSe(br));
                chroma.offset.push_back(ColumbDataSe(br));
              }
            }
          }

          v_data->pred_weight_table_.luma_weight_l0.push_back(luma);
          v_data->pred_weight_table_.chroma_weight_l0.push_back(chroma);
        }

        if (v_data->slice_type_ == 1 || v_data->slice_type_ == 6) {
          for (int i = 0; i <= pps.num_ref_idx_l1_active_minus1_; i++) {

            VideoData::PredWeightTable::LumaWeightData luma;
            luma.weight_flag = br.ReadBits(1);

            if (luma.weight_flag) {
              luma.weight = ColumbDataSe(br);
              luma.offset = ColumbDataSe(br);
            }

            VideoData::PredWeightTable::ChromaWeightData chroma;
            if (sps.chroma_format_idc_ != 0) {
              chroma.weight_flag = br.ReadBits(1);

              if (chroma.weight_flag) {
                for (int j = 0; j < 2; j++) {
                  chroma.weight.push_back(ColumbDataSe(br));
                  chroma.offset.push_back(ColumbDataSe(br));
                }
              }
            }
            v_data->pred_weight_table_.luma_weight_l1.push_back(luma);
            v_data->pred_weight_table_.chroma_weight_l1.push_back(chroma);
          }
        }
      }

      if (v_data->header_.ref_idc_ != 0) {

        if (v_data->header_.unit_type_ == 5) {
          v_data->dec_ref_marking_.no_output_of_prior_pics_flag =
              br.ReadBits(1);
          v_data->dec_ref_marking_.long_term_reference_flag = br.ReadBits(1);
        } else {
          v_data->dec_ref_marking_.adaptive_ref_pic_marking_mode_flag =
              br.ReadBits(1);
          if (v_data->dec_ref_marking_.adaptive_ref_pic_marking_mode_flag) {

            int op = 0;
            do {
              VideoData::DefRefPicMarking::AdaptiveRefPicMarking
                  ref_pic_marking;
              ref_pic_marking.memory_management_control_operation =
                  ColumbDataUe(br);
              op = ref_pic_marking.memory_management_control_operation;
              if (op == 1 || op == 3) {

                ref_pic_marking.difference_of_pic_nums_minus1 =
                    ColumbDataUe(br);
              }

              if (op == 2) {

                ref_pic_marking.long_term_pic_num = ColumbDataUe(br);
              }

              if (op == 3 || op == 6) {

                ref_pic_marking.long_term_frame_idx = ColumbDataUe(br);
              }

              if (op == 4) {

                ref_pic_marking.max_long_term_frame_idx_plus1 =
                    ColumbDataUe(br);
              }

              v_data->dec_ref_marking_.ref_pic_marking.push_back(
                  ref_pic_marking);

            } while (op != 0);
          }
        }
      }
      /*
      0 p
      1 b
      2 i
      3 sp
      4 si
      5 p
      6 b
      7 i
      8 sp
      9 si
      */
      if (pps.entropy_coding_mode_flag_ &&
          (v_data->slice_type_ != 2 && v_data->slice_type_ != 4 &&
           v_data->slice_type_ != 7 && v_data->slice_type_ != 9)

      ) {

        v_data->cabac_init_idc_ = ColumbDataUe(br);
      }

      v_data->slice_qp_delta_ = ColumbDataSe(br);
      if ((v_data->slice_type_ == 3 || v_data->slice_type_ == 4 ||
           v_data->slice_type_ == 8 || v_data->slice_type_ == 9)

      ) {

        if (v_data->slice_type_ == 3 || v_data->slice_type_ == 8) {

          v_data->sp_for_switch_flag_ = br.ReadBits(1);
          v_data->slice_qs_delta_ = ColumbDataSe(br);
        }
        if (pps.deblocking_filter_control_present_flag_) {
          v_data->disable_deblocking_filter_idc_ = ColumbDataUe(br);
          if (v_data->disable_deblocking_filter_idc_ != 1) {

            v_data->slice_alpha_c0_offset_div2_ = ColumbDataSe(br);
            v_data->slice_beta_offset_div2_ = ColumbDataSe(br);
          }
        }

        if (pps.num_slice_groups_minus1_ > 0 &&
            pps.slice_group_map_type_ >= 3 && pps.slice_group_map_type_ <= 5) {
          v_data->slice_group_change_cycle_ =
              br.ReadBits(pps.num_slice_groups_minus1_ + 1);
        }
      }
    }

    //  br.ReadBits(pps->header_);

    free(buff_dst);
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
          configure_.sps_list_.push_back(ParseSps(br));
        }
        int pps_size = br.ReadBits(8);
        for (int i = 0; i < pps_size; i++) {
          configure_.pps_list_.push_back(ParsePps(br));
        }
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