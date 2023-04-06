#pragma once
#include "drawable.h"
#include "smartpointerhelp.h"
#include "glm_includes.h"
#include <array>
#include <unordered_map>
#include <cstddef>
#include <Texture.h>


//using namespace std;

// C++ 11 allows us to define the size of an enum. This lets us use only one byte
// of memory to store our different block types. By default, the size of a C++ enum
// is that of an int (so, usually four bytes). This *does* limit us to only 256 different
// block types, but in the scope of this project we'll never get anywhere near that many.

enum BlockType : unsigned char
{
    // All blocks must be classified into correct lines

    // This First BlockType is only for EMPTY
    EMPTY,
    // Normal opaque BlockType
    GRASS, DIRT, STONE, SNOW, SAND, BEDROCK, CAVE, WOOD, LEAF, CACTUS, PURE_SNOW,

    // small asset
    PURE_GRASS, FLOWER_1, FLOWER_2, MUSHROOM_1, MUSHROOM_2,

    // Normal BlockType about fluid
    ICE,
    LAVA, WATER, // (Together with fluid, for speed up)
    // BlockType about fluid
    WATER_XP,WATER_XN,WATER_ZP,WATER_ZN,WATER_XPZP,WATER_XPZN,WATER_XNZP,WATER_XNZN,
    LAVA_XP,LAVA_XN,LAVA_ZP,LAVA_ZN,LAVA_XPZP,LAVA_XPZN,LAVA_XNZP,LAVA_XNZN
};

// The six cardinal directions in 3D space
enum Direction : unsigned char
{
    XPOS, XNEG, YPOS, YNEG, ZPOS, ZNEG
};

// Lets us use any enum class as the key of a
// std::unordered_map
struct EnumHash {
    template <typename T>
    size_t operator()(T t) const {
        return static_cast<size_t>(t);
    }
};

// One Chunk is a 16 x 256 x 16 section of the world,
// containing all the Minecraft blocks in that area.
// We divide the world into Chunks in order to make
// recomputing its VBO data faster by not having to
// render all the world at once, while also not having
// to render the world block by block.

class Chunk : public Drawable
{
private:
    // All of the blocks contained within this Chunk
    std::array<BlockType, 65536> m_blocks;
    // This Chunk's four neighbors to the north, south, east, and west
    // The third input to this map just lets us use a Direction as
    // a key for this map.
    // These allow us to properly determine
    std::unordered_map<Direction, Chunk*, EnumHash> m_neighbors;

    glm::vec3 m_pos;
    std::vector<glm::vec4> m_buffer;//used for opaque
    std::vector<glm::vec4> m_Transbuffer;//used for transparent
    std::vector<GLuint> m_idx;//used for opaque index
    std::vector<GLuint> m_Transidx;//used for transparent index

    //create two VBO data
    bool opaque = true;

    std::vector<glm::vec4> pos_buffer;
    std::vector<glm::vec4> col_buffer;
    std::vector<glm::vec4> nor_buffer;
    std::vector<glm::vec2> uv_buffer;

    void bufferVBOdata(std::vector<glm::vec4> &buffer, std::vector<GLuint> &idx);
    void bufferVBOTransdata(std::vector<glm::vec4> &buffer, std::vector<GLuint> &idx);
public:
    Chunk(OpenGLContext* context);
    virtual ~Chunk() override;
    void createVBOdata() override;

    static bool isOpaque(BlockType t);
    static bool isSmallAsset(BlockType t);

    BlockType getBlockAt(unsigned int x, unsigned int y, unsigned int z) const;
    BlockType getBlockAt(int x, int y, int z) const;
    void setBlockAt(unsigned int x, unsigned int y, unsigned int z, BlockType t);
    void linkNeighbor(uPtr<Chunk>& neighbor, Direction dir);

    void setPos(glm::vec3 pos);
    glm::vec3 getPos();

    void resetVBOCreated();

    //Texture binding and loading
    bool m_vboCreated;
    bool m_vboBuffered;
    bool m_vboTransBuffered;

    void bufferAlldata();

    glm::vec4 getUVOffset(enum Direction dir, enum BlockType curr);

    bool checkFluidAdjSkip(enum BlockType curr, enum Direction dir, enum BlockType adjacent);
    bool checkFluidPos(enum BlockType curr, float x, float z);
};
