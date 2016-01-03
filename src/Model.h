#pragma once
#include <glm/glm.hpp>
#include <stdint.h>
#include <vector>
#include <Streams/IOStream.h>

namespace LambdaCore
{

#define	MAXVERTICES		2048
#define	MAXBONES		128

    // Lighting defines
#define LIGHT_FLATSHADE		0x0001
#define LIGHT_FULLBRIGHT	0x0004

    // Transition defines
#define	TRANSITION_X		0x0001
#define	TRANSITION_Y		0x0002	
#define	TRANSITION_Z		0x0004
#define	TRANSITION_XR		0x0008
#define	TRANSITION_YR		0x0010
#define	TRANSITION_ZR		0x0020
#define	TRANSITION_TYPES	0x7FFF
#define	TRANSITION_RLOOP	0x8000

    class Model
    {
    private:
        struct Header
        {
            char	        ID[4];						// File id
            uint32_t	    Version;				// File version
            char	        Name[64];				// Model name
            uint32_t	    Length;					// File length
            glm::vec3	    EyePosition;			// Eye position
            glm::vec3       Minimum;				// Ideal movement hull size
            glm::vec3	    Maximum;
            glm::vec3	    BoundingBoxMinimum;		// Bounding box
            glm::vec3	    BoundingBoxMaximum;
            uint32_t	    Flags;					// Flags
            uint32_t	    NumBones;				// Number of bones
            uint32_t	    BoneOffset;				// Offset to bone data
            uint32_t	    NumBoneControllers;		// Number of bone controllers
            uint32_t	    BoneControllerOffset;	// Offset to bone controller data
            uint32_t	    NumHitBoxes;			// Complex bounding boxes
            uint32_t	    HitBoxOffset;
            uint32_t	    NumSequences;			// Animation sequences
            uint32_t	    SequenceOffset;
            uint32_t	    NumSeqGroups;			// Demand loaded sequences
            uint32_t	    SeqGroupOffset;
            uint32_t	    NumTextures;			// Number of textures
            uint32_t	    TextureOffset;			// File texture offset
            uint32_t	    TextureData;			// Texture data
            uint32_t	    NumSkinReferences;		// Replaceable textures
            uint32_t	    NumSkinFamilies;
            uint32_t	    SkinOffset;
            uint32_t	    NumBodyParts;
            uint32_t	    BodyPartOffset;
            uint32_t	    NumAttachments;			// Queryable attachable points
            uint32_t	    AttachmentOffset;
            uint32_t	    SoundTable;
            uint32_t	    SoundOffset;
            uint32_t	    NumSoundGroups;
            uint32_t	    SoundGroupOffset;
            uint32_t	    NumTransitions;			// Animation node to animation node transition graph
            uint32_t	    TransitionOffset;
        };

        struct tagMDLCacheUser
        {
            void	*Data;
        };

        struct tagMDLSeqHeader
        {
            long	ID;
            long	Version;
            char	Name[64];
            long	Length;
        };

        struct tagMDLSeqGroup
        {
            char			Label[32];		// Textual name
            char			Name[64];		// Filename
            tagMDLCacheUser	Cache;			// Cache index pointer
            long			Data;			// Hack for group 0
        };

        struct tagMDLSeqDescription
        {
            char	Name[32];				// Sequence label
            float	Timing;					// Frame time
            long	Flags;					// Flags
            long	Activity;
            long	Actweight;
            long	NumEvents;
            long	EventIndex;
            long	NumFrames;				// Number of frames
            long	NumPivots;				// Number of foot pivots
            long	PivotOffset;
            long	MotionType;
            long	MotionBone;
            glm::vec3	LinearMovement;
            long	AutoMovePosIndex;
            long	AutoMoveAngleIndex;
            glm::vec3 BoundingBoxMinimum;		// Bounding box
            glm::vec3	BoundingBoxMaximum;
            long	NumBlends;
            long	AnimOffset;				// mstudioanim_t pointer relative to start of sequence group 
                                            // data[blend][bone][X, Y, Z, XR, YR, ZR]
            long	BlendType[2];			// X, Y, Z, XR, YR, ZR
            float	BlendStart[2];			// Starting value
            float	BlendEnd[2];			// Ending value
            long	BlendParent;
            long	SeqGroup;				// Sequence group for demand loading
            long	EntryNode;				// Transition node at entry
            long	ExitNode;				// Transition node at exit
            long	NodeFlags;				// Transition rules	
            long	NextSeq;				// Auto advancing sequences
        };

        struct tagMDLBone
        {
            char	Name[32];				// Bone name for symbolic links
            long	Parent;					// Parent bone
            long	Flags;					// ??
            long	BoneController[6];		// Bone controller index, -1 == none
            float	Value[6];				// Default DoF values
            float	Scale[6];				// Scale for delta DoF values
        };

        struct tagMDLBoneController
        {
            long	Bone;					// -1 == 0
            long	Type;					// X, Y, Z, XR, YR, ZR, M
            float	Start;
            float	End;
            long	Rest;					// Byte index value at rest
            long	Index;					// 0-3 user set controller, 4 mouth
        };

        struct tagAnimation
        {
            uint16_t	Offset[6];
        };

        union tagMDLAnimFrame
        {
            struct
            {
                uint8_t	Valid;
                uint8_t	Total;
            };

            short	Value;
        };

        struct tagMDLBodyPart
        {
            char	Name[64];
            long	NumModels;
            long	Base;
            long	ModelOffset;			// Index into models array
        };

        struct tagMDLTexture
        {
            char	Name[64];
            long	Flags;
            long	Width;
            long	Height;
            long	Index;
        };

        struct tagMDLModel
        {
            char	Name[64];
            long	Type;
            float	BoundingRadius;
            long	NumMesh;
            long	MeshOffset;
            long	NumVertices;			// Number of unique vertices
            long	VertexInfoOffset;		// Vertex bone info
            long	VertexOffset;			// Vertex offset
            long	NumNormals;				// number of unique surface normals
            long	NormalInfoOffset;		// Normal bone info
            long	NormalOffset;			// Normal offset
            long	NumGroups;				// Deformation groups
            long	GroupOffset;
        };

        struct tagMDLMesh
        {
            long	NumTriangles;
            long	TriangleOffset;
            long	SkinReference;
            long	NumNormals;				// Per mesh normals
            long	NormalOffset;			// Normal offset
        };

    public:
    };
}