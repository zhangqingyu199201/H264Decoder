#pragma once

#include "PublicHeader.h"

#include "BitReader.h"

class FlvTag {
public:
    enum {
        TAG_TYPE_AUDIO = 8,
        TAG_TYPE_VIDEO = 9,
        TAG_TYPE_SCRIPT = 18,
    };

    struct VideoTagHeader {
        int frame_type_;
        int codec_id_;
        int packet_type_;
        int composition_time_;
    };

    struct
    {
        int pre_tag_size_;
        int is_filter_;
        int type_;
        int data_size_;
        int timestamp_;
        int timestamp_extended_;
        int stream_id_;
        void *audio_tag_header_{nullptr};
        VideoTagHeader *video_tag_header_{nullptr};
        void *encryption_header_{nullptr};
        void *filter_param_{nullptr};
    } tag_header_;

    int data_size_;
    unsigned char *data_{nullptr};

    ~FlvTag() {
        if (tag_header_.audio_tag_header_) {
            free(tag_header_.audio_tag_header_);
            tag_header_.audio_tag_header_ = nullptr;
        }
        if (tag_header_.video_tag_header_) {
            free(tag_header_.video_tag_header_);
            tag_header_.video_tag_header_ = nullptr;
        }
        if (tag_header_.encryption_header_) {
            free(tag_header_.encryption_header_);
            tag_header_.encryption_header_ = nullptr;
        }
        if (tag_header_.filter_param_) {
            free(tag_header_.filter_param_);
            tag_header_.filter_param_ = nullptr;
        }
        if (data_) {
            free(data_);
            data_ = nullptr;
        }
    }
};

class FlvScriptNode {
public:
    ::std::string key_;
    int type_;

    class ScriptValue {
    public:
        double d_64_ = 0;
        int32_t i_32_ = 0;
        int32_t i_16_ = 0;
        int32_t b_8_ = 0;
        ::std::string str_;
        double time_ = 0;
        int time_off_ = 0;
        ::std::vector<FlvScriptNode *> obj_;
        ::std::vector<ScriptValue *> strict_;

        ~ScriptValue() {
            for (auto iter : obj_) {
                delete iter;
            }

            for (auto iter : strict_) {
                delete iter;
            }
        }
    } value_;

    void ParseHead(unsigned char *buff, int len);

    int ParseBody(unsigned char *buff, int len);

    int ParseDataValue(unsigned char *buff, int len);
};

class FlvDemuxer {
private:
    FileBitReader br_;
    FlvScriptNode script_;

public:
    explicit FlvDemuxer(::std::string file_path);

    struct {
        int version_;
        int has_audio_;
        int has_video_;
        int data_offset_;
    } header_;

    void ReadHeader();

    FlvTag *ReadTag();
};

class FlvDemuxerTest {
public:
    static void Test();
};