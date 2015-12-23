#include <WAD.h>
#include <cassert>
#include <Exception/Exception.h>
#include <Common/StringUtils.h>
#include <Streams/MemoryStream.h>
#include <algorithm>

using namespace Commons;

namespace LambdaCore
{
    static const char* WAD3_MAGIC = "WAD3";

    WAD::WAD(const Commons::IOStreamPtr stream)
        : mStream(stream)
        , mHeader()
        , mEntries()
        , mEntryNameMap()
    {
        // Header
        stream->read(&mHeader, sizeof(Header));
        if (::strncmp(mHeader.szMagic, WAD3_MAGIC, sizeof(WAD3_MAGIC)) != 0)
        {
            throw SerializationException(StringUtils::FormatString("Not a valid WAD 3 file: %s", mHeader.szMagic));
        }

        // Directory
        mEntries.resize(mHeader.mNumEntries);
        stream->seek(mHeader.mDirOffset, IOStream::ORIGIN_SET);
        stream->read(&mEntries[0], sizeof(Entry) * mEntries.size());
        // Some entries in WAD  probably could be duplicated, so entries should be also kept in original array...

        for (uint32_t i = 0; i < mEntries.size(); ++i)
        {
            mEntryNameMap.insert(make_pair(getEntryName(i), i));
        }
    }

    uint32_t WAD::getNumEntries() const
    {
        return mEntries.size();
    }

    std::string WAD::getEntryName(uint32_t index) const
    {
        assert(index < mEntries.size());
        std::string upperName(mEntries[index].mFilename);
        std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);
        return upperName;
    }

    int32_t WAD::getEntryIndexByName(const std::string& name) const
    {
        std::string upperName(name);
        std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);

        auto it = mEntryNameMap.find(upperName);
        if (it != mEntryNameMap.end())
        {
            return it->second;
        }
        return -1;
    }

    Commons::MemoryStreamPtr WAD::getEntryStream(const std::string& name)
    {
        int32_t id = getEntryIndexByName(name);
        if (id >= 0)
        {
            return getEntryStream(id);
        }
        return nullptr;
    }

    Commons::MemoryStreamPtr WAD::getEntryStream(uint32_t index)
    {
        assert(index < mEntries.size());
        const Entry& entry = mEntries[index];

        if (entry.mCompression != 0)
        {
            throw SerializationException("Unsupported WAD file: compression enabled");
        }

        mStream->seek(entry.mFilePos, IOStream::ORIGIN_SET);

        uint32_t length = std::min(entry.mDiskSize, entry.mSize);
        MemoryStreamPtr result(new MemoryStream(entry.mSize));
        mStream->read(result->getPtr(), length);

        return result;
    }
}
