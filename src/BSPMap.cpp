#include <BSPMap.h>
#include <Exception/Exception.h>
#include <Common/StringUtils.h>
#include <algorithm>

using namespace Commons;

namespace LambdaCore
{  
    BSPMap::BSPMap(std::ifstream& stream)
        : mHeader()
        // TODO: init members
    {
        initFromStream(stream);
    }

    
    void BSPMap::initFromStream(std::ifstream& stream)
    {
        stream.read(reinterpret_cast<char*>(&mHeader), sizeof(mHeader));
        checkVersion();
        
        std::vector<uint8_t> entities(getLumpSize(LUMP_ENTITIES));
        readLump(LUMP_ENTITIES, stream, &entities[0], entities.size());
        
        readPlanes(stream);
        readTextures(stream);
        readVertices(stream);

        int k = 0;
    }

    void BSPMap::checkVersion()
    {
        if (mHeader.mVersion != HL_BSP_VERSION)
        {
            // TODO: alias?
            throw SerializationException(StringUtils::FormatString("Wrong BSP version: %d", mHeader.mVersion));
        }
    }

    void BSPMap::readPlanes(std::ifstream& stream)
    {
        uint32_t len = getLumpSize(LUMP_PLANES);
        uint32_t cnt = len / sizeof(BSPPlane);
        if (len != cnt * sizeof(BSPPlane))
        {
            throw SerializationException(StringUtils::FormatString("Wrong Planes array size: %d", len));
        }

        mPlanes.resize(cnt);
        readLump(LUMP_PLANES, stream, reinterpret_cast<uint8_t*>(&mPlanes[0]), len);
    }

    void BSPMap::readTextures(std::ifstream& stream)
    {
        // TODO
    }

    void BSPMap::readVertices(std::ifstream& stream)
    {
        uint32_t len = getLumpSize(LUMP_VERTICES);
        uint32_t cnt = len / sizeof(glm::vec3);
        if (len != cnt * sizeof(glm::vec3))
        {
            throw SerializationException(StringUtils::FormatString("Wrong Vertices array size: %d", len));
        }
        mVertices.resize(cnt);
        readLump(LUMP_VERTICES, stream, reinterpret_cast<uint8_t*>(&mVertices[0]), len);
    }

    void BSPMap::readLump(ELumps lump, std::ifstream& stream, uint8_t* data, uint32_t maxLength) const
    {
        const BSPLump& lmp = mHeader.mLumps[lump];

        uint32_t length = std::min(lmp.mLength, maxLength);
        if (length > 0)
        {
            stream.seekg(lmp.mOffset, SEEK_SET);
            stream.read(reinterpret_cast<char*>(data), length); // TODO: stream
        }
    }

    uint32_t BSPMap::getLumpSize(ELumps lump) const
    {
        return mHeader.mLumps[lump].mLength;
    }
}