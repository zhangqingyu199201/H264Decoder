#pragma once

#include "PublicHeader.h"

/**
 * @brief 读取文件内容
 *
 */
class FileReader {
public:
    ::std::ifstream fi_;

    /**
     * @brief Construct a new File Reader object
     *
     * @param file_path
     */
    explicit FileReader(::std::string file_path);

    /**
     * @brief Destroy the File Reader object
     *
     */
    ~FileReader();

    /**
     * @brief
     *
     * @param buff
     * @param len
     * @return true
     * @return false
     */
    bool ReadData(char *buff, int32_t len);
};

class FileReaderTest {
public:
    static void Test();
};