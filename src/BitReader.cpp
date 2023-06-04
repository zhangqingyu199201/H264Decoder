#include "BitReader.h"

#include "PublicHeader.h"
#include "FileReader.h"

BitReader::BitReader(char *buff, int buff_size) :
    buff_(buff), buff_size_(buff_size) {
}

BitReader::~BitReader() {
}

int64_t BitReader::ReadBits(int32_t len) {
    int res = 0;
    for (int i = 0; i < len; i++) {
        res <<= 1;
        res |= ReadOneBits();
    }
    return res;
}

int64_t BitReader::ReadOneBits() {
    int64_t res = buff_[loc_];
    res = (res >> (7 - bit_off_)) & 0x1;
    bit_off_++;
    if (bit_off_ == 8) {
        bit_off_ = 0;
        loc_++;
    }

    return res;
}

void BitReader::SkipReserveBits() {
    if (bit_off_ != 0) {
        bit_off_ = 0;
        loc_++;
    }
}

void BitReader::ReadBuffer(char *buff, int len) {
    int buff_loc = 0;
    while (len) {
        if (buff_size_ - loc_ >= len) {
            memcpy(buff + buff_loc, buff_ + loc_, len);
            loc_ += len;
            buff_loc += len;
            len -= len;
            break;
        } else {
            int copy_size = buff_size_ - loc_;
            memcpy(buff + buff_loc, buff_ + loc_, copy_size);
            loc_ += copy_size;
            buff_loc += copy_size;
            len -= copy_size;
        }
    }
}

FileBitReader::FileBitReader(::std::string file_path, int buff_size) :
    fr_(file_path), buff_size_(buff_size) {
    buff_ = (char *)malloc(sizeof(char) * buff_size_);
    fr_.ReadData(buff_, buff_size_);
}

FileBitReader::~FileBitReader() {
    if (buff_) {
        free(buff_);
        buff_ = nullptr;
    }
}

void FileBitReader::CheckAndRead() {
    if (buff_size_ - loc_ <= 9) {
        // 不考虑重叠
        memcpy(buff_, buff_ + loc_, buff_size_ - loc_);
        fr_.ReadData((char *)buff_ + buff_size_ - loc_, loc_);
        loc_ = 0;
    }
}

int64_t FileBitReader::ReadOneBits() {
    int64_t res = buff_[loc_];
    res = (res >> (7 - bit_off_)) & 0x1;
    bit_off_++;
    if (bit_off_ == 8) {
        bit_off_ = 0;
        loc_++;
    }

    return res;
}

int64_t FileBitReader::ReadBits(int32_t len) {
    CheckAndRead();
    int res = 0;
    for (int i = 0; i < len; i++) {
        res <<= 1;
        res |= ReadOneBits();
    }
    return res;
}

void FileBitReader::SkipReserveBits() {
    if (bit_off_ != 0) {
        bit_off_ = 0;
        loc_++;
    }
}

void FileBitReader::ReadBuffer(char *buff, int len) {
    int buff_loc = 0;
    while (len) {
        CheckAndRead();
        if (buff_size_ - loc_ >= len) {
            memcpy(buff + buff_loc, buff_ + loc_, len);
            loc_ += len;
            buff_loc += len;
            len -= len;
            break;
        } else {
            int copy_size = buff_size_ - loc_;
            memcpy(buff + buff_loc, buff_ + loc_, copy_size);
            loc_ += copy_size;
            buff_loc += copy_size;
            len -= copy_size;
        }
    }
}

void FileBitReaderTest::Test() {
    START_TEST

    ::std::string file_path = ::std::string(MOVIE_PATH);
    FileBitReader br(file_path, 12);

    char type[4] = {0};
    br.ReadBuffer(type, 3);
    int d_0 = br.ReadBits(4);
    int d_1 = br.ReadBits(3);
    int d_2 = br.ReadBits(5);
    br.SkipReserveBits();
    char data[22];
    br.ReadBuffer(data, 22);
    char meta[11] = {0};
    br.ReadBuffer(meta, 10);

    MYASSERT(::std::string(type) == "FLV");
    MYASSERT(d_0 == 0);
    MYASSERT(d_1 == 0);
    MYASSERT(d_2 == 0x10);
    MYASSERT(::std::string(meta) == "onMetaData");

    END_TEST
}
