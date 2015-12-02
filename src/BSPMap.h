#pragma once
#include <glm/glm.hpp>
#include <fstream> // TODO: self wrapper
#include <stdint.h>
#include <vector>

namespace LambdaCore
{
    class BSPMap
    {
    private:
        static const uint32_t HL_BSP_VERSION = 30;

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

    public:
        BSPMap(std::ifstream& stream);

    private:
        void initFromStream(std::ifstream& stream);

        void checkVersion();
        void readPlanes(std::ifstream& stream);
        void readTextures(std::ifstream& stream);
        void readVertices(std::ifstream& stream);

        // Lump methods
        void readLump(ELumps lump, std::ifstream& stream, uint8_t* data, uint32_t maxLength) const;
        uint32_t BSPMap::getLumpSize(ELumps lump) const;

    private:
        BSPHeader mHeader;
        std::vector<BSPPlane> mPlanes;
        std::vector<glm::vec3> mVertices;
    };
}