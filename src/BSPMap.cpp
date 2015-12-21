#include <BSPMap.h>
#include <Exception/Exception.h>
#include <Common/StringUtils.h>
#include <algorithm>

using namespace Commons;

namespace LambdaCore
{  
    BSPMap::BSPMap(const Commons::IOStreamPtr stream)
        : mHeader()
        // TODO: init members
    {
        initFromStream(stream);
    }

    
    void BSPMap::initFromStream(const Commons::IOStreamPtr stream)
    {
        stream->read(&mHeader, sizeof(mHeader));
        checkVersion();
        
        std::vector<uint8_t> entities(getLumpSize(LUMP_ENTITIES));
        readLump(LUMP_ENTITIES, stream, &entities[0], entities.size());
        
        readEntities(stream);
        readPlanes(stream);
        readTextures(stream);
        readVertices(stream);
        readVIS(stream);
        readNodes(stream);
        readTexInfo(stream);
        readFaces(stream);
        readLightmaps(stream);
        readClipNodes(stream);
        readLeafs(stream);
        readMarkSurfaces(stream);
        readEdges(stream);
        readSurfEdges(stream);
        readModels(stream);        
    }

    void BSPMap::checkVersion()
    {
        if (mHeader.mVersion != HL_BSP_VERSION)
        {
            // TODO: alias?
            throw SerializationException(StringUtils::FormatString("Wrong BSP version: %d", mHeader.mVersion));
        }
    }

    void BSPMap::readEntities(const Commons::IOStreamPtr stream)
    {
        mEntities.resize(getLumpSize(LUMP_ENTITIES));
        readLump(LUMP_ENTITIES, stream, &mEntities[0], mEntities.size());
    }

    template<typename T>
    void BSPMap::readTypedVec(BSPMap::ELumps lump, const Commons::IOStreamPtr stream, std::vector<T>& vec)
    {
        uint32_t len = getLumpSize(lump);
        uint32_t cnt = len / sizeof(T);
        uint32_t arrayLen = cnt * sizeof(T);
        if (arrayLen != len)
        {
            throw SerializationException(StringUtils::FormatString("Lump length mismatch for lump %d: expected %d, actual %d", lump, arrayLen, len));
        }
        vec.resize(len);
        readLump(lump, stream, &vec[0], len);
    }

    void BSPMap::readPlanes(const Commons::IOStreamPtr stream)
    {
        readTypedVec(LUMP_PLANES, stream, mPlanes);
    }

    void BSPMap::readTextures(const Commons::IOStreamPtr stream)
    {
        ELumps lump = LUMP_TEXTURES;
        const BSPLump& lmp = mHeader.mLumps[lump];
        uint32_t lumpOffset = getLumpOffset(lump);
        stream->seek(lumpOffset, IOStream::ORIGIN_SET);
        uint32_t numMipTextures = 0;
        stream->read(&numMipTextures, sizeof(uint32_t));
        std::vector<int32_t> offsets(numMipTextures);
        mMipTextures.resize(numMipTextures);
        if (numMipTextures > 0)
        {
            stream->read(&offsets[0], numMipTextures * sizeof(int32_t));
            for (uint32_t i = 0; i < numMipTextures; ++i)
            {
                stream->seek(lumpOffset + offsets[i], IOStream::ORIGIN_SET);
                stream->read(&mMipTextures[i], sizeof(BSPMipTex));
            }
        }
    }

    void BSPMap::readVertices(const Commons::IOStreamPtr stream)
    {
        readTypedVec(LUMP_VERTICES, stream, mVertices);
    }

    static void DecodePVS(const uint8_t* src, uint8_t* dst, uint32_t numClusters)
    {
        const uint8_t* curPos = src;
        for (uint32_t i = 0; i < numClusters;)
        {
            uint8_t curByte = *curPos++;
            if (curByte == 0)
            {
                uint32_t numSkip = *curPos++;
                for (uint32_t j = 0; j < numSkip; ++j)
                    *dst++ = 0;
                i += numSkip;
            }
            else
            {
                *dst++ = curByte;
                ++i;
            }
        }
    }

    void BSPMap::readVIS(const Commons::IOStreamPtr stream)
    {
        const uint32_t len = getLumpSize(LUMP_VISIBILITY);
        if (len > 0)
        {
            stream->seek(getLumpOffset(LUMP_VISIBILITY), IOStream::ORIGIN_SET);
            // TODO: decode lumps
            uint32_t numClusters;
            //stream->read(&numClusters, sizeof(uint32_t));
            uint32_t offsets[2]; // TODO: vec
            std::vector<uint8_t> compressedVIS(len);
            //readLump(LUMP_VISIBILITY, stream, &compressedVIS[0], len);
            // TODO: process
            // TODO: check method:
            //DecodePVS
            mDecodedVIS.resize(0);
        }
        else
        {
            mDecodedVIS.resize(0);
        }
    }

    void BSPMap::readNodes(const Commons::IOStreamPtr stream)
    {
        readTypedVec(LUMP_NODES, stream, mNodes);
    }

    void BSPMap::readTexInfo(const Commons::IOStreamPtr stream)
    {
        readTypedVec(LUMP_TEXINFO, stream, mTexInfo);
    }

    void BSPMap::readFaces(const Commons::IOStreamPtr stream)
    {
        readTypedVec(LUMP_FACES, stream, mFaces);
    }

    void BSPMap::readLightmaps(const Commons::IOStreamPtr stream)
    {
        // TODO
    }

    void BSPMap::readClipNodes(const Commons::IOStreamPtr stream)
    {
        readTypedVec(LUMP_CLIPNODES, stream, mClipNodes);
    }

    void BSPMap::readLeafs(const Commons::IOStreamPtr stream)
    {
        readTypedVec(LUMP_LEAVES, stream, mLeafs);
    }

    void BSPMap::readMarkSurfaces(const Commons::IOStreamPtr stream)
    {
        readTypedVec(LUMP_MARKSURFACES, stream, mMarkSurfaces);
    }

    void BSPMap::readEdges(const Commons::IOStreamPtr stream)
    {
        readTypedVec(LUMP_EDGES, stream, mEdges);
    }

    void BSPMap::readSurfEdges(const Commons::IOStreamPtr stream)
    {
        readTypedVec(LUMP_SURFEDGES, stream, mSurfEdges);
    }

    void BSPMap::readModels(const Commons::IOStreamPtr stream)
    {
        readTypedVec(LUMP_MODELS, stream, mModels);
    }

    void BSPMap::readLump(ELumps lump, const Commons::IOStreamPtr stream, void* data, uint32_t maxLength) const
    {
        uint32_t length = std::min(getLumpSize(lump), maxLength);
        if (length > 0)
        {
            stream->seek(getLumpOffset(lump), IOStream::ORIGIN_SET);
            stream->read(data, length);
        }
    }

    uint32_t BSPMap::getLumpOffset(ELumps lump) const
    {
        return mHeader.mLumps[lump].mOffset;
    }

    uint32_t BSPMap::getLumpSize(ELumps lump) const
    {
        return mHeader.mLumps[lump].mLength;
    }

    float BSPMap::getPlaneDist(const BSPPlane* plane, const glm::vec3& point) const
    {        
        return glm::dot(plane->mNormal, point) - plane->mDist;
    }

    int32_t BSPMap::getPointLeaf(const glm::vec3& point) const
    {
        int32_t headNode = mModels[0].mHeadnodes[0];// TODO: customize, assert
        const BSPNode* rootNode = &mNodes[headNode]; 
        const BSPNode* curNode = rootNode;
        while (true)
        {
            const float dist = getPlaneDist(&mPlanes[curNode->mPlaneIndex], point);
            int32_t nextNode = curNode->mChildren[dist > 0.F ? 0 : 1];
            if (nextNode < 0) 
            {
                return -nextNode - 1; // Found!                
            }
            curNode = &mNodes[nextNode];
        }

        return 0; 
    }

    bool BSPMap::isLeafVisible(int32_t fromLeafIndex, int32_t testLeafIndex) const
    {
        return true; // TODO: vis test
    }
}