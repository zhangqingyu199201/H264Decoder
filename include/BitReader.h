#pragma once

#include "PublicHeader.h"
#include "FileReader.h"

/**
 * @brief 读取bit
 *
 */
class BitReader {
public:
    char *buff_;
    int buff_size_;
    int loc_{0};
    int bit_off_{0};


    /**
     * @brief Construct a new Bit Reader object
     *
     * @param file_path
     */
    explicit BitReader(char* buff, int buff_size);

    /**
     * @brief Destroy the Bit Reader object
     *
     */
    ~BitReader();

    /**
     * @brief
     *
     * @param len
     */
    int64_t ReadBits(int32_t len);

    /**
     * @brief
     *
     * @return int64_t
     */
    int64_t ReadOneBits();

    /**
     * @brief
     *
     */
    void SkipReserveBits();

    /**
     * @brief
     *
     * @param buff
     * @param len
     */
    void ReadBuffer(char *buff, int len);
};


class FileBitReader {
public:
    char *buff_;
    int buff_size_;
    int loc_{0};
    int bit_off_{0};

    FileReader fr_;

    /**
     * @brief
     *
     */
    void CheckAndRead();

        /**
     * @brief Construct a new Bit Reader object
     *
     * @param file_path
     */
    explicit FileBitReader(::std::string file_path, int buff_size = 1024);

    /**
     * @brief Destroy the Bit Reader object
     *
     */
    ~FileBitReader();

    /**
     * @brief
     *
     * @param len
     */
    int64_t ReadBits(int32_t len);

    /**
     * @brief
     *
     * @return int64_t
     */
    int64_t ReadOneBits();

    /**
     * @brief
     *
     */
    void SkipReserveBits();

    /**
     * @brief
     *
     * @param buff
     * @param len
     */
    void ReadBuffer(char *buff, int len);

};




class FileBitReaderTest {
public:
    static void Test();
};