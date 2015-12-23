#pragma once
#include <glm/glm.hpp>
#include <stdint.h>
#include <vector>
#include <Streams/IOStream.h>

namespace LambdaCore
{
    class BSPMap
    {
        friend class BSPRender;
    public:
        static const uint32_t HL_BSP_VERSION = 30;
        static const uint32_t HL_MAXTEXTURENAME = 16;
        static const uint32_t HL_MIPLEVELS = 4;
        static const uint32_t HL_MAX_MAP_HULLS = 4;

        enum ELumps
        {
            LUMP_ENTITIES = 0,
            LUMP_PLANES,
            LUMP_TEXTURES,
            LUMP_VERTICES,
            LUMP_VISIBILITY,
            LUMP_NODES,
            LUMP_TEXINFO,
            LUMP_FACES,
            LUMP_LIGHTING,
            LUMP_CLIPNODES,
            LUMP_LEAVES,
            LUMP_MARKSURFACES,
            LUMP_EDGES,
            LUMP_SURFEDGES,
            LUMP_MODELS,
            HEADER_LUMPS,
        };

        struct BSPLump
        {
            uint32_t mOffset;
            uint32_t mLength;
        };

        struct BSPHeader
        {
            uint32_t mVersion;
            BSPLump mLumps[HEADER_LUMPS];
        };

        enum EPlaneTypes
        {
            PLANE_X = 0,        // Plane is perpendicular to given axis
            PLANE_Y,
            PLANE_Z,
            PLANE_ANYX,         // Non-axial plane is snapped to the nearest
            PLANE_ANYY,
            PLANE_ANYZ,
        };

        struct BSPPlane
        {
            glm::vec3 mNormal;  // The planes normal vector
            float mDist;        // Plane equation is: vNormal * X = fDist
            uint32_t mType;     // Plane type, see EPlaneTypes
        };

        struct BSPMipTex
        {
            char mName[HL_MAXTEXTURENAME];
            uint32_t mWidth;
            uint32_t mHeight;
            uint32_t mOffsets[HL_MIPLEVELS]; // Offsets is relative to this struct! TODO: move?
        };

        struct BSPNode
        {
            uint32_t mPlaneIndex;               // Index into Planes lump
            int16_t mChildren[2];               // If > 0, then indices into Nodes // otherwise bitwise inverse indices into Leafs
            int16_t mMins[3], mMaxs[3];         // Defines bounding box
            uint16_t mFirstFace, mNumFaces;     // Index and count into Faces
        };

        struct BSPTextureInfo
        {
            glm::vec3 mS;
            float mSShift;
            glm::vec3 mT;
            float mTShift;
            uint32_t mMiptex;           // Index into textures array
            uint32_t mFlags;            // Texture flags, seem to always be 0
        };

        /* TODO: typelight
        value 0 is the normal value, to be used with a light map.
        value 0xFF is to be used when there is no light map.
        value 1 produces a fast pulsating light
        value 2 produces a slow pulsating light
        value 3 to 10 produce various other lighting effects, as defined in The code lump.*/

        struct BSPFace
        {
            uint16_t mPlane;          // Plane the face is parallel to
            uint16_t mPlaneSide;      // Set if different normals orientation
            uint32_t mFirstEdge;      // Index of the first surfedge
            uint16_t mNumEdges;       // Number of consecutive surfedges
            uint16_t mTextureInfo;    // Index of the texture info structure
            uint8_t mTypelight;       // The kind of lighting that should be applied to the face
            uint8_t mBaselight;       // from 0xFF (dark) to 0 (bright)
            uint8_t mLightModels[2];  // two additional light models  
            uint32_t mLightmapOffset; // Offsets into the raw lightmap data
        };
        
        struct BSPClipNode
        {
            int32_t mPlane;       // Index into planes
            int16_t mChildren[2]; // negative numbers are contents
        };

        enum ELeafContents
        {
            CONTENTS_EMPTY = -1,
            CONTENTS_SOLID = -2,
            CONTENTS_WATER = -3,
            CONTENTS_SLIME = -4,
            CONTENTS_LAVA = -5,
            CONTENTS_SKY = -6,
            CONTENTS_ORIGIN = -7,
            CONTENTS_CLIP = -8,
            CONTENTS_CURRENT_0 = -9,
            CONTENTS_CURRENT_90 = -10,
            CONTENTS_CURRENT_180 = -11,
            CONTENTS_CURRENT_270 = -12,
            CONTENTS_CURRENT_UP = -13,
            CONTENTS_CURRENT_DOWN = -14,
            CONTENTS_TRANSLUCENT = -15,
        };

        struct BSPLeaf
        {
            int32_t mContents;                              // Contents enumeration (ELeafContents)
            int32_t mVisOffset;                             // Offset into the visibility lump
            int16_t mMins[3], mMaxs[3];                     // Defines bounding box
            uint16_t mFirstMarkSurface, mNumMarkSurfaces;   // Index and count into marksurfaces array
            uint8_t mAmbientLevels[4];                      // Ambient sound levels
        };

        struct BSPEdge
        {
            uint16_t mVertex[2];                            // Indices into vertex array
        };

        struct BSPModel
        {
            float mMins[3], mMaxs[3];                   // Defines bounding box
            glm::vec3 mOrigin;                          // Coordinates to move the // coordinate system
            int32_t mHeadnodes[HL_MAX_MAP_HULLS];       // Index into nodes array
            int32_t mNumVisLeafs;                       // ???
            int32_t mFirstFace, mNumFaces;              // Index and count into faces
        };

    public:
        BSPMap(const Commons::IOStreamPtr stream);
        
        int32_t getPointLeaf(const glm::vec3& point) const;

        // Collision detection
        float getPlaneDist(const BSPPlane* plane, const glm::vec3& point) const;
        bool isLeafVisible(int32_t fromLeafIndex, int32_t testLeafIndex) const;

    private:
        // TODO: stream reader?
        void initFromStream(const Commons::IOStreamPtr stream);

        void checkVersion();
        template<typename T>
        void readTypedVec(ELumps lump, const Commons::IOStreamPtr stream, std::vector<T>& vec);

        void readEntities(const Commons::IOStreamPtr stream);
        void readPlanes(const Commons::IOStreamPtr stream);
        void readTextures(const Commons::IOStreamPtr stream);
        void readVertices(const Commons::IOStreamPtr stream);
        void readVIS(const Commons::IOStreamPtr stream);
        void readNodes(const Commons::IOStreamPtr stream);
        void readTexInfo(const Commons::IOStreamPtr stream);
        void readFaces(const Commons::IOStreamPtr stream);
        void readLightmaps(const Commons::IOStreamPtr stream);
        void readClipNodes(const Commons::IOStreamPtr stream);
        void readLeafs(const Commons::IOStreamPtr stream);
        void readMarkSurfaces(const Commons::IOStreamPtr stream);
        void readEdges(const Commons::IOStreamPtr stream);
        void readSurfEdges(const Commons::IOStreamPtr stream);
        void readModels(const Commons::IOStreamPtr stream);

        // Lump methods
        void readLump(ELumps lump, const Commons::IOStreamPtr stream, void* data, uint32_t maxLength) const;
        uint32_t getLumpSize(ELumps lump) const;
        uint32_t getLumpOffset(ELumps lump) const;

    private:
        BSPHeader mHeader;
        std::string mEntities;
        std::vector<BSPMipTex> mMipTextures;
        std::vector<BSPPlane> mPlanes;
        std::vector<glm::vec3> mVertices;
        std::vector<uint8_t> mDecodedVIS; // TODO
        std::vector<BSPNode> mNodes;
        std::vector<BSPTextureInfo> mTexInfo;
        std::vector<BSPFace> mFaces;
        std::vector<BSPClipNode> mClipNodes;
        std::vector<BSPLeaf> mLeafs;
        std::vector<uint16_t> mMarkSurfaces;
        std::vector<BSPEdge> mEdges;
        std::vector<int32_t> mSurfEdges;
        std::vector<BSPModel> mModels;
    };

    typedef std::shared_ptr<BSPMap> BSPMapPtr;
}