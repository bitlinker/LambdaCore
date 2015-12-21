#pragma once
#include <stdint.h>
#include <vector>
#include <map>
#include <Streams/IOStream.h>

namespace LambdaCore
{
    class WAD
    {
    private:
        struct Header
        {
            char szMagic[4];        // should be WAD2/WAD3
            uint32_t mNumEntries;   // number of directory entries
            uint32_t mDirOffset;    // offset into directory
        };

        static const uint32_t WAD_MAXTEXTURENAME = 16;

        struct Entry
        {
            uint32_t mFilePos;                   // offset in WAD
            uint32_t mDiskSize;                  // size in file
            uint32_t mSize;                      // uncompressed size
            uint8_t mType;                       // type of entry
            uint8_t mCompression;                // 0 if none
            uint16_t mDummy;                     // not used
            char mFilename[WAD_MAXTEXTURENAME];  // must be null terminated
        };

    public:
        WAD(const Commons::IOStreamPtr stream);

        uint32_t getNumEntries() const;
        std::string getEntryName(uint32_t index) const;
        Commons::IOStreamPtr getEntryStream(const std::string& name);
        Commons::IOStreamPtr getEntryStream(uint32_t index);
        int32_t getEntryIndexByName(const std::string& name) const;

    private:
        Commons::IOStreamPtr mStream;
        Header mHeader;
        std::vector<Entry> mEntries;
        std::map<std::string, uint32_t> mEntryNameMap;
    };

    typedef std::shared_ptr<WAD> WADPtr;
}