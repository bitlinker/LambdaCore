#include <BSPMap.h>
#include <Exception/Exception.h>
#include <Common/StringUtils.h>
#include <Logger/Log.h>
#include <algorithm>
#include <map>

using namespace Commons;

namespace LambdaCore
{  
    class Entity
    {
    public:
        void clear()
        {
            mValues.clear();
        }

        void put(const std::string& key, const std::string& value)
        {
            mValues.insert(TEntityValuesPair(key, value));
        }

        bool get(const std::string& key, std::string& value)
        {
            const auto it = mValues.find(key);
            if (it == mValues.end())
            {
                return false;
            }

            value = it->second;
            return true;
        }

        bool getClassName(std::string& value)
        {
            return get("classname", value);
        }

    private:
        typedef std::map<std::string, std::string> TEntityValuesMap;
        typedef std::pair<std::string, std::string> TEntityValuesPair;
        TEntityValuesMap mValues;
    };

    // TODO: move to parser file
    static const char CHAR_BRACE_OPEN = '{';
    static const char CHAR_BRACE_CLOSE = '}';
    static const char CHAR_QUOTE = '"';
    static const int32_t CHAR_EOF = -1;

    // Entities parser
    // Entities string is not owned
    class EntityParser
    {
    public:
        EntityParser(const char* str, uint32_t len)
            : mString(str)
            , mPos(0)
            , mLength(len)
        {
        }


    private:
        int32_t getChar()
        {
            if (mPos == mLength) {
                return CHAR_EOF;
            }
            return mString[mPos++];
        }

        bool isWhitespace(int32_t c)
        {
            return
                c == ' ' ||
                c == '\t' ||
                c == '\r' ||
                c == '\f' ||
                c == '\n';
        }

        bool parseEntity(Entity& entity)
        {
            std::string tmpStr;
            tmpStr.reserve(256); // TODO: const; common?

            std::string tmpKey;
            bool isKey = true;

            while (true)
            {
                int32_t c = getChar();
                if (c == CHAR_EOF)
                {
                    LOG_WARN("Error parsing entity - unexpected EOF, pos %d", mPos);
                    return false;
                }
                else if (isWhitespace(c))
                {
                    continue;
                }
                else if (c == CHAR_QUOTE)
                {
                    if (!parseQuotedString(tmpStr))
                    {
                        LOG_WARN("Error parsing entity string, skipping, pos %d", mPos);
                        return false;
                    }

                    if (isKey)
                    {
                        tmpKey = tmpStr;
                    }
                    else
                    {
                        // TODO: assert key is not found
                        entity.put(tmpKey, tmpStr);
                    }
                    isKey = !isKey;
                }
                else if (c == CHAR_BRACE_CLOSE)
                {
                    return isKey; // Should complete key-value pair at this moment
                }
            }          
        }
        
        bool parseQuotedString(std::string& str)
        {
            str.clear();
            while(true)
            {
                int32_t c = getChar();
                if (c == CHAR_EOF)
                {
                    LOG_WARN("Error parsing entity quoted string - unexpected EOF, pos %d", mPos);
                    return false;
                }
                else if (c == CHAR_QUOTE)
                {
                    return true;
                }
                else
                {
                    str.push_back(c);
                }
            };
        }

    public:
        bool parse(std::vector<Entity>& entities)
        {
            mPos = 0;
            entities.clear();
            while (true)
            {
                int32_t c = getChar();
                if (isWhitespace(c))
                {
                    continue;
                }
                else if (c == CHAR_BRACE_OPEN)
                {
                    Entity entity;
                    if (parseEntity(entity))
                    {
                        entities.push_back(entity);
                    }
                    else
                    {
                        LOG_WARN("Error parsing entity, skipping, pos %d", mPos);
                    }
                }
                else if (c == CHAR_EOF)
                {
                    break;
                }
            }
            return true;
        }

    private:
        const char* mString;
        uint32_t mPos;
        uint32_t mLength;
    };


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

        // TODO: not here
        EntityParser parser(mEntities.c_str(), mEntities.length());

        std::vector<Entity> entities;
        bool res = parser.parse(entities);

        std::string className;
        res = entities[0].getClassName(className);
        // TODO
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
        vec.resize(cnt);
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

    void BSPMap::readVIS(const Commons::IOStreamPtr stream)
    {
        readTypedVec(LUMP_VISIBILITY, stream, mCompressedVis);
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
        readTypedVec(LUMP_LIGHTING, stream, mLightmaps);
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

    // TODO: utils?
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
        return true; // TODO: vis test; deprecated?
    }

    void BSPMap::fillVisLeafs(int32_t fromLeafIndex, std::vector<bool>& visLeafs) const
    {        
        assert(visLeafs.size() == mLeafs.size());
        assert(fromLeafIndex >= 0);
        int32_t offset = mLeafs[fromLeafIndex].mVisOffset;

        if (mCompressedVis.empty() || offset < 0)
        {
            std::fill(visLeafs.begin(), visLeafs.end(), true);
        }
        else
        {
            std::fill(visLeafs.begin(), visLeafs.end(), false);
            auto dstIt = visLeafs.begin();
            const uint8_t* pCurVis = &mCompressedVis[offset];
            // TODO: optimize; check
            dstIt++;
            for (uint32_t i = 0; /*i < mLeafs.size()*/; )
            {
                uint8_t curByte = *pCurVis++;
                if (curByte == 0)
                {
                    uint32_t numSkip = *pCurVis++ * 8;
                    for (uint32_t j = 0; j < numSkip; ++j)
                    {
                        if (++dstIt == visLeafs.end())
                            return;
                    }
                    i += numSkip;
                }
                else
                {
                    for (uint32_t j = 0; j < 8; ++j)
                    {
                        *dstIt++ = ((curByte & (1 << j)) > 0);
                        if (dstIt == visLeafs.end())
                            return;
                    }
                    i += 8;
                }
            }
        }
    }
}