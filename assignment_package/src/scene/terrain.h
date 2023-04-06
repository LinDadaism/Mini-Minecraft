#pragma once
#include "smartpointerhelp.h"
#include "glm_includes.h"
//#include "chunk.h"
#include <array>
#include <unordered_map>
#include <unordered_set>
#include "shaderprogram.h"
#include "chunkhelpers.h"
#include "noise.h"
#include <Qmutex>
#include <deque>

/** ALL macros are here, used for debugs */
#define LINK_REFRESH // update bufferData of neighbor chunks after linked
#define PLAY_SOUND

#define CAVE_GENERATION
#define WATER_GENERATION
#define RIVER_GENERATION
#define BIG_ASSET_GENERATION
#define XZ_COLLISION
#define Y_COLLISION

#define DRAW_SKY
#define DRAW_TRANSPARENT
#define DRAW_WOLRDAXES

//#define ENABLE_TEST
//#ifdef ENABLE_TEST

// #define ENABLE_TERRAIN_TEST
#ifdef ENABLE_TERRAIN_TEST
    // #define TEST_GROUND // only generate ground
    // #define TEST_MOUNT
#endif

/// Features only for test
// #define WATER_FLUID


// add big tree at (507, 36)


// Helper functions to convert (x, z) to and from hash map key
int64_t toKey(int x, int z);
glm::ivec2 toCoords(int64_t k);

// The container class for all of the Chunks in the game.
// Ultimately, while Terrain will always store all Chunks,
// not all Chunks will be drawn at any given time as the world
// expands.

enum BiomeType : unsigned char
{
    GRASSLAND, MOUNTAIN, DESERT, SNOWLAND
};

class Terrain {
private:
    // Stores every Chunk according to the location of its lower-left corner
    // in world space.
    // We combine the X and Z coordinates of the Chunk's corner into one 64-bit int
    // so that we can use them as a key for the map, as objects like std::pairs or
    // glm::ivec2s are not hashable by default, so they cannot be used as keys.
    std::unordered_map<int64_t, uPtr<Chunk>> m_chunks;

    // We will designate every 64 x 64 area of the world's x-z plane
    // as one "terrain generation zone". Every time the player moves
    // near a portion of the world that has not yet been generated
    // (i.e. its lower-left coordinates are not in this set), a new
    // 4 x 4 collection of Chunks is created to represent that area
    // of the world.
    // The world that exists when the base code is run consists of exactly
    // one 64 x 64 area with its lower-left corner at (0, 0).
    // When milestone 1 has been implemented, the Player can move around the
    // world to add more "terrain generation zone" IDs to this set.
    // While only the 3 x 3 collection of terrain generation zones
    // surrounding the Player should be rendered, the Chunks
    // in the Terrain will never be deleted until the program is terminated.
    std::unordered_set<int64_t> m_generatedTerrain;
    // std::unordered_set<int64_t> m_processingTerrain;

    std::vector<uPtr<Chunk>> m_sharedGeneratedChunks;
    std::vector<Chunk*> m_sharedCreatedChunks;
    std::unordered_set<Chunk*> m_newLinkedChunks;

    OpenGLContext* mp_context;

    Noise noise;

    QMutex generatedChunkMutex;
    QMutex createdChunkMutex;
    // std::mutex generatedTerrainMutex;
    // std::mutex debugMutex;

    bool allTerrainGenerated = false;
    int oriXFloor = INT_MAX;
    int oriZFloor = INT_MAX;
    bool firstDraw = true;

    const static std::unordered_set<BlockType> lava_fluid;
    const static std::unordered_set<BlockType> water_fluid;

    // std::vector<std::thread> threadVec;

public:
    Terrain(OpenGLContext *context);
    ~Terrain();

    // Instantiates a new Chunk and stores it in
    // our chunk map at the given coordinates.
    // Returns a pointer to the created Chunk.
    Chunk* instantiateChunkAt(int x, int z);
    void instantiateZoneAt(int xFloor, int zFloor);
    // Do these world-space coordinates lie within
    // a Chunk that exists?
    bool hasChunkAt(int x, int z) const;
    // Assuming a Chunk exists at these coords,
    // return a mutable reference to it
    uPtr<Chunk>& getChunkAt(int x, int z);
    // Assuming a Chunk exists at these coords,
    // return a const reference to it
    const uPtr<Chunk>& getChunkAt(int x, int z) const;
    // Given a world-space coordinate (which may have negative
    // values) return the block stored at that point in space.
    BlockType getBlockAt(int x, int y, int z) const;
    BlockType getBlockAt(glm::vec3 p) const;
    // Given a world-space coordinate (which may have negative
    // values) set the block at that point in space to the
    // given type.
    void setBlockAt(int x, int y, int z, BlockType t);
    bool checkBlockAt(glm::vec3 p) const;

    // Draws every Chunk that falls within the bounding box
    // described by the min and max coords, using the provided
    // ShaderProgram
    void draw(int minX, int maxX, int minZ, int maxZ, ShaderProgram *shaderProgram,
              bool isDepthRender = false);

    // Jack:
    // initialize the chunk at (x, z) with blocks
    void initializeChunkAt(int x, int z);
    // initialize all blocks at (x, y, z) for all y in [0, 255]
    void initializeBlocksAt(int x, int z, Chunk* cPtr);

    void addAsset(int xFloor, int zFloor, Chunk* cPtr);

    bool CheckZoneVboCreated(int xFloor, int zFloor);

    void BlockTypeWork(int xFloor, int zFloor);
    void VBOWork(Chunk* chunk);

    // Initializes the Chunks that store the 64 x 256 x 64 block scene you
    // see when the base code is run.
    // void CreateTestScene(glm::vec3 playerPos);
    // void createZone(int x, int z);

    // Checks whether a new Chunk should be added to the Terrain
    // based on the Player's proximity to the edge of a Chunk
    // without a neighbor in a particular direction.
    void ExpandDraw(glm::vec3 playerPos, ShaderProgram *shaderProgram,
                    bool isDepthRender);

    void CreateTestScene();

    // in multithread mode, to refresh a chunk after its block has changed
    void markChunkToUpdate(int x, int z);

    void linkChunk(int x, int z);

    void initialize();

    void fluid_sim(const std::unordered_set<BlockType> &fluidSet, int x, int y, int z,
                   BlockType t, std::deque<glm::vec3> &deq, std::vector<glm::vec3> &slant,
                   std::unordered_map<int64_t, std::pair<int, int>>& updateChunks);
};
