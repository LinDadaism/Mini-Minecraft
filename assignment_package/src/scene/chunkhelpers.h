#pragma once
#include "chunk.h"
#include <glm_includes.h>
#include <unordered_set>

// Helper data sets/ objects for the chunk class

struct VertexData {
    glm::vec4 pos;
    glm::vec2 uv; // TODO: for applying texture to terrain
                    // Relatvie UV coords, offsetted by BlockType

    VertexData(glm::vec4 p, glm::vec2 u) : pos(p), uv(u)
    {}
};

struct BlockFace {
    Direction dir;
    glm::vec3 dirVec;
    std::array<VertexData, 4> vertices;

    BlockFace(Direction direc, glm::vec3 v, const VertexData &a, const VertexData &b, const VertexData &c, const VertexData &d)
        : dir(direc), dirVec(v), vertices{a, b, c, d}
    {}
};

const static float BLK_UV = 1.f/16;

// iterate over this in Chunk::create to check each block
// adjacent to block[x][y][z] and get the relevant vertex info
const static std::array<BlockFace, 6> adjacentFaces {
    // +x
    BlockFace(XPOS, glm::vec3(1, 0, 0), VertexData(glm::vec4(1,0,1,1), glm::vec2(0,0)),
                                        VertexData(glm::vec4(1,0,0,1), glm::vec2(BLK_UV,0)),
                                        VertexData(glm::vec4(1,1,0,1), glm::vec2(BLK_UV,BLK_UV)),
                                        VertexData(glm::vec4(1,1,1,1), glm::vec2(0,BLK_UV))),
    // -x
    BlockFace(XNEG, glm::vec3(-1, 0, 0), VertexData(glm::vec4(0,0,0,1), glm::vec2(0,0)),
                                         VertexData(glm::vec4(0,0,1,1), glm::vec2(BLK_UV,0)),
                                         VertexData(glm::vec4(0,1,1,1), glm::vec2(BLK_UV,BLK_UV)),
                                         VertexData(glm::vec4(0,1,0,1), glm::vec2(0,BLK_UV))),
    // +y
    BlockFace(YPOS, glm::vec3(0, 1, 0), VertexData(glm::vec4(0,1,1,1), glm::vec2(0,0)),
                                        VertexData(glm::vec4(1,1,1,1), glm::vec2(BLK_UV,0)),
                                        VertexData(glm::vec4(1,1,0,1), glm::vec2(BLK_UV,BLK_UV)),
                                        VertexData(glm::vec4(0,1,0,1), glm::vec2(0,BLK_UV))),
    // -y
    BlockFace(YNEG, glm::vec3(0, -1, 0), VertexData(glm::vec4(0,0,0,1), glm::vec2(0,0)),
                                         VertexData(glm::vec4(1,0,0,1), glm::vec2(BLK_UV,0)),
                                         VertexData(glm::vec4(1,0,1,1), glm::vec2(BLK_UV,BLK_UV)),
                                         VertexData(glm::vec4(0,0,1,1), glm::vec2(0,BLK_UV))),
    // +z
    BlockFace(ZPOS, glm::vec3(0, 0, 1), VertexData(glm::vec4(0,0,1,1), glm::vec2(0,0)),
                                        VertexData(glm::vec4(1,0,1,1), glm::vec2(BLK_UV,0)),
                                        VertexData(glm::vec4(1,1,1,1), glm::vec2(BLK_UV,BLK_UV)),
                                        VertexData(glm::vec4(0,1,1,1), glm::vec2(0,BLK_UV))),
    // -z
    BlockFace(ZNEG, glm::vec3(0, 0, -1), VertexData(glm::vec4(1,0,0,1), glm::vec2(0,0)),
                                         VertexData(glm::vec4(0,0,0,1), glm::vec2(BLK_UV,0)),
                                         VertexData(glm::vec4(0,1,0,1), glm::vec2(BLK_UV,BLK_UV)),
                                         VertexData(glm::vec4(1,1,0,1), glm::vec2(0,BLK_UV)))
};

// naive BlockType color mapping
// WARNING: depricated after texturing is implemented,
// but still needs to populate if new blocktype added (unless not anymore passing vs_Col in m_buffer),
// otherwise WILL cause out_of_range errors!!!
const static std::unordered_map<BlockType, glm::vec4, EnumHash> blockColors {
    {GRASS, glm::vec4(glm::vec3(95.f, 159.f, 53.f) / 255.f, 1)},
    {DIRT,  glm::vec4(glm::vec3(121.f, 85.f, 58.f) / 255.f, 1)},
    {STONE, glm::vec4(glm::vec3(0.5f), 1)},
    {WATER, glm::vec4(glm::vec3(0.f, 0.f, 0.75f), 1)},
    {SNOW, glm::vec4(glm::vec3(1.f), 1)},
    {SAND, glm::vec4(glm::vec3(0.76f, 0.70f, 0.50f), 1)},
    {BEDROCK, glm::vec4(glm::vec3(0.25f), 1)},
    {CAVE, glm::vec4(glm::vec3(0.2f), 1)},
    {LAVA, glm::vec4(glm::vec3(1.f, 0.2f, 0.2f), 1)},
    {ICE, glm::vec4(glm::vec3(0.5, 0.5, 0.75f), 1)},
    {CACTUS, glm::vec4(glm::vec3(0.5f), 1)},
    {WOOD, glm::vec4(glm::vec3(0.5f), 1)},
    {LEAF, glm::vec4(glm::vec3(0.5f), 1)},
    {PURE_SNOW, glm::vec4(glm::vec3(0.5f), 1)},
    {PURE_GRASS, glm::vec4(glm::vec3(0.5f), 1)},
    {FLOWER_1, glm::vec4(glm::vec3(0.5f), 1)},
    {MUSHROOM_1, glm::vec4(glm::vec3(0.5f), 1)},
    {FLOWER_2, glm::vec4(glm::vec3(0.5f), 1)},
    {MUSHROOM_2, glm::vec4(glm::vec3(0.5f), 1)},
    {WATER_XP,glm::vec4(glm::vec3(1,1,1),1)}
};

const static std::array<glm::vec3, 8> expansionDirections {
    glm::vec3(16, 0, 0), glm::vec3(-16, 0, 0), // +x, -x
    glm::vec3(0, 0, 16), glm::vec3(0, 0, -16), // +z, -z
    glm::vec3(16, 0, 16), glm::vec3(-16, 0, -16), // diagonals
    glm::vec3(16, 0, -16), glm::vec3(-16, 0, 16)
};

const static std::unordered_set<BlockType> water_fluid{WATER_XP,WATER_XN,WATER_ZP,WATER_ZN,WATER_XPZP,WATER_XPZN,WATER_XNZP,WATER_XNZN};
const static std::unordered_set<BlockType> lava_fluid{LAVA_XP,LAVA_XN,LAVA_ZP,LAVA_ZN,LAVA_XPZP,LAVA_XPZN,LAVA_XNZP,LAVA_XNZN};
const static std::unordered_set<BlockType> transparentBlocks{
    EMPTY,WATER, // LAVA,ICE,
    PURE_GRASS,FLOWER_1,MUSHROOM_1,FLOWER_2,MUSHROOM_2,
    WATER_XP,WATER_XN,WATER_ZP,WATER_ZN,WATER_XPZP,WATER_XPZN,WATER_XNZP,WATER_XNZN,
    LAVA_XP,LAVA_XN,LAVA_ZP,LAVA_ZN,LAVA_XPZP,LAVA_XPZN,LAVA_XNZP,LAVA_XNZN
};

const static std::unordered_map<BlockType, glm::vec4> fluidColorBufferVals{
    {LAVA_XP, glm::vec4(1,0,0,0)},
    {LAVA_XN, glm::vec4(-1,0,0,0)},
    {LAVA_ZN, glm::vec4(1,0,0,0)},
    {LAVA_ZP, glm::vec4(-1,0,0,0)},
    {LAVA_XPZP, glm::vec4(1,0,1,0)},
    {LAVA_XPZN, glm::vec4(1,0,-1,2)},
    {LAVA_XNZP, glm::vec4(-1,0,1,2)},
    {LAVA_XNZN, glm::vec4(-1,0,-1,2)},
    {WATER_XP, glm::vec4(1,0,0,0)},
    {WATER_XN, glm::vec4(-1,0,0,0)},
    {WATER_ZN, glm::vec4(1,0,0,0)},
    {WATER_ZP, glm::vec4(-1,0,0,0)},
    {WATER_XPZP, glm::vec4(1,0,1,0)},
    {WATER_XPZN, glm::vec4(1,0,-1,1)},
    {WATER_XNZP, glm::vec4(-1,0,1,1)},
    {WATER_XNZN, glm::vec4(-1,0,-1,1)}
};
