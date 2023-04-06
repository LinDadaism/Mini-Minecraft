#pragma once
#include "chunk.h"
#include <unordered_set>

//EMPTY, GRASS, DIRT, STONE, WATER, SNOW, SAND,
//LAVA, ICE, BEDROCK, CAVE

const static std::unordered_set<BlockType, EnumHash> movableBlocks {
    GRASS, DIRT, STONE, SNOW, SAND, ICE, CAVE,
    WOOD, CACTUS, LEAF, PURE_SNOW, PURE_GRASS, FLOWER_1, FLOWER_2, MUSHROOM_1, MUSHROOM_2
    // WATER,WATER_XP,WATER_XN,WATER_ZP,WATER_ZN,
};

const static std::unordered_set<BlockType, EnumHash> collidableBlocks {
    GRASS, DIRT, STONE, SNOW, SAND, BEDROCK, ICE, CAVE, WOOD, CACTUS, LEAF, PURE_SNOW
};

// TODO: move +/- x & +/- z directions in collision detection here
