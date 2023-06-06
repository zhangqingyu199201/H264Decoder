#include "FlvDemuxer.h"

void FlvScriptNode::ParseHead(unsigned char *buff, int len) {
    int offset = 0;
    int key_type = buff[offset];
    offset++;
    MYASSERT(key_type == 2);

    ParseBody(buff + offset, len - offset);
}

int FlvScriptNode::ParseDataValue(unsigned char *buff, int len) {
    int offset = 0;
    int type = buff[offset];
    type_ = type;
    offset++;

    switch (type) {
    case 0: {
        // (uint32_t)
        uint64_t l = (uint32_t)(buff[offset] << 24) | (buff[offset + 1] << 16) | (buff[offset + 2] << 8) | (buff[offset + 3]);
        offset += 4;
        uint64_t r = (uint32_t)(buff[offset] << 24) | (buff[offset + 1] << 16) | (buff[offset + 2] << 8) | (buff[offset + 3]);
        offset += 4;
        uint64_t v = ((uint64_t)l << 32) | r;
        value_.d_64_ = *((double *)&v);
        break;
    }
    case 1: {
        value_.b_8_ = buff[offset];
        offset++;
        break;
    }
    case 2: {
        int str_len = (buff[offset] << 8) | buff[offset + 1];
        offset += 2;
        char *str = (char *)malloc(sizeof(char) * (str_len + 1));
        memcpy(str, buff + offset, str_len);
        offset += str_len;
        str[str_len] = 0;
        value_.str_ = ::std::string(str);
        free(str);
        break;
    }
    case 7: {
        value_.i_16_ = (buff[offset] << 8) | (buff[offset + 1]);
        offset += 2;
        break;
    }
    case 12: {
        int str_len = (buff[offset] << 24) | (buff[offset + 1] << 16) | (buff[offset + 2] << 8) | (buff[offset + 3]);
        offset += 4;
        char *str = (char *)malloc(sizeof(char) * (str_len + 1));
        memcpy(str, buff + offset, str_len);
        offset += str_len;
        str[str_len] = 0;
        value_.str_ = ::std::string(str);
        free(str);
    }
    case 11: {
        uint64_t l = (uint32_t)(buff[offset] << 24) | (buff[offset + 1] << 16) | (buff[offset + 2] << 8) | (buff[offset + 3]);
        offset += 4;
        uint64_t r = (uint32_t)(buff[offset] << 24) | (buff[offset + 1] << 16) | (buff[offset + 2] << 8) | (buff[offset + 3]);
        offset += 4;
        uint64_t v = ((uint64_t)l << 32) | r;
        value_.time_ = *((double *)&v);

        value_.i_16_ = (buff[offset] << 8) | (buff[offset + 1]);
        offset += 2;
        break;
    }
    case 8: {
        int32_t ecma_len = (buff[offset] << 24) | (buff[offset + 1] << 16) | (buff[offset + 2] << 8) | (buff[offset + 3]);
        offset += 4;

        for (int i = 0; i < ecma_len; i++) {
            FlvScriptNode *node = new FlvScriptNode();
            int offset_inner = node->ParseBody(buff + offset, len - offset);
            offset += offset_inner;
            value_.obj_.push_back(node);
        }
        break;
    }
    case 10: {
        int32_t strict_len = (buff[offset] << 24) | (buff[offset + 1] << 16) | (buff[offset + 2] << 8) | (buff[offset + 3]);
        offset += 4;

        for (int i = 0; i < strict_len; i++) {
            FlvScriptNode *node = new FlvScriptNode();
            int inner_offset = node->ParseDataValue(buff + offset, len - offset);
            offset += inner_offset;
            value_.obj_.push_back(node);
        }

        break;
    }
    case 3: {
        while (true) {
            int str_len = (buff[offset] << 8) | buff[offset + 1];
            offset += 2;
            if (str_len == 0) {
                MYASSERT(buff[offset] == 9);
                offset++;
                break;
            }

            offset -= 2;
            FlvScriptNode *node = new FlvScriptNode();
            int inner_offset = node->ParseBody(buff + offset, len - offset);
            offset += inner_offset;
            value_.obj_.push_back(node);
        }

        break;
    }
    default: {
        ::std::cout << "MAF Unsupport type" << ::std::endl;
        break;
    }
    }

    return offset;
}

int FlvScriptNode::ParseBody(unsigned char *buff, int len) {
    int offset = 0;
    int key_len = (buff[offset] << 8) | buff[offset + 1];
    offset += 2;
    char *key = (char *)malloc(sizeof(char) * (key_len + 1));
    memcpy(key, buff + offset, key_len);
    offset += key_len;
    key[key_len] = 0;
    key_ = ::std::string(key);
    free(key);

    int inner_offset = ParseDataValue(buff + offset, len - offset);

    return offset + inner_offset;
}

FlvDemuxer::FlvDemuxer(::std::string file_path) :
    br_(file_path) {
}

void FlvDemuxer::ReadHeader() {
    MYASSERT(br_.ReadBits(8) == 'F');
    MYASSERT(br_.ReadBits(8) == 'L');
    MYASSERT(br_.ReadBits(8) == 'V');

    header_.version_ = br_.ReadBits(8);

    MYASSERT(br_.ReadBits(5) == 0);
    header_.has_audio_ = br_.ReadOneBits();
    MYASSERT(br_.ReadBits(1) == 0);
    header_.has_video_ = br_.ReadOneBits();
    header_.data_offset_ = br_.ReadBits(32);
}

FlvTag *FlvDemuxer::ReadTag() {
    FlvTag *tag = new FlvTag();
    tag->tag_header_.pre_tag_size_ = br_.ReadBits(32);
    br_.ReadBits(2);
    tag->tag_header_.is_filter_ = br_.ReadOneBits();
    tag->tag_header_.type_ = br_.ReadBits(5);
    tag->tag_header_.data_size_ = br_.ReadBits(24);
    tag->tag_header_.timestamp_ = br_.ReadBits(24);
    tag->tag_header_.timestamp_extended_ = br_.ReadBits(8);
    tag->tag_header_.stream_id_ = br_.ReadBits(24);

    int data_size = tag->tag_header_.data_size_;
    if (tag->tag_header_.type_ == FlvTag::TAG_TYPE_AUDIO) {
        // read audio tag header
    } else if (tag->tag_header_.type_ == FlvTag::TAG_TYPE_VIDEO) {
        // read video tag header
        tag->tag_header_.video_tag_header_ = (FlvTag::VideoTagHeader *)malloc(sizeof(FlvTag::VideoTagHeader));

        tag->tag_header_.video_tag_header_->frame_type_ = br_.ReadBits(4);
        tag->tag_header_.video_tag_header_->codec_id_ = br_.ReadBits(4);
        data_size--;
        if (tag->tag_header_.video_tag_header_->codec_id_ == 7) {
            tag->tag_header_.video_tag_header_->packet_type_ = br_.ReadBits(8);
            tag->tag_header_.video_tag_header_->composition_time_ = br_.ReadBits(24);
            data_size -= 4;
        }
    }

    if (tag->tag_header_.is_filter_) {
        // read encryption header
        // read filter param
    }

    // readData
    tag->data_size_ = data_size;
    tag->data_ = (unsigned char *)malloc(data_size);
    br_.ReadBuffer((char *)(tag->data_), data_size);

    if (tag->tag_header_.type_ == FlvTag::TAG_TYPE_SCRIPT) {
        script_.ParseHead(tag->data_, tag->data_size_);
    }
    return tag;
}

void FlvDemuxerTest::Test() {
    START_TEST

    FlvDemuxer flv(::std::string(MOVIE_PATH));
    flv.ReadHeader();

    MYASSERT(flv.header_.has_audio_ == 0);
    MYASSERT(flv.header_.has_video_ == 1);
    MYASSERT(flv.header_.version_ == 1);
    MYASSERT(flv.header_.data_offset_ == 9);

    FlvTag *tag_script = flv.ReadTag();
    FlvTag *tag_video_0 = flv.ReadTag();
    FlvTag *tag_video_1 = flv.ReadTag();
    FlvTag *tag_video_2 = flv.ReadTag();
    MYASSERT(tag_script->tag_header_.data_size_ == 313);
    MYASSERT(tag_video_0->tag_header_.data_size_ == 52);
    MYASSERT(tag_video_1->tag_header_.data_size_ == 63213);
    MYASSERT(tag_video_2->tag_header_.data_size_ == 29786);

    END_TEST
}
