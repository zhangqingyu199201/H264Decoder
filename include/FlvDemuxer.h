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
        int frame_type;
        int codec_id;
        int packet_type;
        int composition_time;
    };

    struct
    {
        int pre_tag_size;
        int is_filter;
        int type;
        int data_size;
        int timestamp;
        int timestamp_extended;
        int stream_id;
        void *audio_tag_header{nullptr};
        VideoTagHeader *video_tag_header{nullptr};
        void *encryption_header{nullptr};
        void *filter_param{nullptr};
    } tag_header_;

    int data_size_;
    unsigned char *data_{nullptr};

    ~FlvTag() {
        if (tag_header_.audio_tag_header) {
            free(tag_header_.audio_tag_header);
            tag_header_.audio_tag_header = nullptr;
        }
        if (tag_header_.video_tag_header) {
            free(tag_header_.video_tag_header);
            tag_header_.video_tag_header = nullptr;
        }
        if (tag_header_.encryption_header) {
            free(tag_header_.encryption_header);
            tag_header_.encryption_header = nullptr;
        }
        if (tag_header_.filter_param) {
            free(tag_header_.filter_param);
            tag_header_.filter_param = nullptr;
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
        double d_64 = 0;
        int32_t i_32 = 0;
        int32_t i_16 = 0;
        int32_t b_8 = 0;
        ::std::string str;
        double time = 0;
        int time_off = 0;
        ::std::vector<FlvScriptNode *> obj;
        ::std::vector<ScriptValue *> strict;

        ~ScriptValue() {
            for (auto iter : obj) {
                delete iter;
            }

            for (auto iter : strict) {
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
        int version;
        int has_audio;
        int has_video;
        int data_offset;
    } header_;

    void ReadHeader();

    FlvTag *ReadTag();
};

class FlvDemuxerTest {
public:
    static void Test();
};