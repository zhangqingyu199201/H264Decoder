#include "PublicHeader.h"

#include "FileReader.h"
#include "BitReader.h"
#include "FlvDemuxer.h"
#include "AvcDecoder.h"

using namespace std;


int main(int argn, char** argv) {
    FileReaderTest::Test();
    FileBitReaderTest::Test();
    FlvDemuxerTest::Test();
    AvcDecoderTest::Test();
    return 0;
}
