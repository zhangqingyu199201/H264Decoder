#include "FileReader.h"

FileReader::FileReader(::std::string file_path) {
    fi_ = ::std::ifstream(file_path, ::std::ios::in | ::std::ios::binary);
    if (!fi_.is_open()) {
        ::std::cout << "Open " << file_path.c_str() << ::std::endl;
    }


}

FileReader::~FileReader() {
    if (fi_.is_open()) {
        fi_.close();
    }
}

bool FileReader::ReadData(char *buff, int len) {
    if (!fi_.is_open()) {
        return false;
    }

    // 遇到文件末尾
    fi_.read(buff, len);
    return true;
}

void FileReaderTest::Test() {
    START_TEST

    ::std::string test_file = ::std::string(MOVIE_PATH);
    FileReader fr(test_file);

    char d1[1];
    char d2[3];
    fr.ReadData(d1, 1);
    fr.ReadData(d2, 3);

    MYASSERT(d1[0] == 'F');
    MYASSERT(d2[0] == 'L');
    MYASSERT(d2[1] == 'V');
    MYASSERT(d2[2] == '\1');

    END_TEST
}
