#include "terrain.h"
#include <stdexcept>
#include <iostream>
#include "blocktypeworker.h"
#include "vboworker.h"
#include <QThreadPool>
#include <QMutex>
#include <deque>

const std::unordered_set<BlockType> Terrain::lava_fluid
    {LAVA_XP, LAVA_XN, LAVA_ZP, LAVA_ZN, LAVA_XPZP, LAVA_XPZN, LAVA_XNZP, LAVA_XNZN};
const std::unordered_set<BlockType> Terrain::water_fluid
    {WATER_XP, WATER_XN, WATER_ZP, WATER_ZN, WATER_XPZP, WATER_XPZN, WATER_XNZP, WATER_XNZN};

Terrain::Terrain(OpenGLContext *context)
    : m_chunks(), m_generatedTerrain(), mp_context(context), noise()
{}

Terrain::~Terrain()
{}

// Combine two 32-bit ints into one 64-bit int
// where the upper 32 bits are X and the lower 32 bits are Z
int64_t toKey(int x, int z) {
    int64_t xz = 0xffffffffffffffff;
    int64_t x64 = x;
    int64_t z64 = z;

    // Set all lower 32 bits to 1 so we can & with Z later
    xz = (xz & (x64 << 32)) | 0x00000000ffffffff;

    // Set all upper 32 bits to 1 so we can & with XZ
    z64 = z64 | 0xffffffff00000000;

    // Combine
    xz = xz & z64;
    return xz;
}

glm::ivec2 toCoords(int64_t k) {
    // Z is lower 32 bits
    int64_t z = k & 0x00000000ffffffff;
    // If the most significant bit of Z is 1, then it's a negative number
    // so we have to set all the upper 32 bits to 1.
    // Note the 8    V
    if(z & 0x0000000080000000) {
        z = z | 0xffffffff00000000;
    }
    int64_t x = (k >> 32);

    return glm::ivec2(x, z);
}

// Surround calls to this with try-catch if you don't know whether
// the coordinates at x, y, z have a corresponding Chunk
BlockType Terrain::getBlockAt(int x, int y, int z) const
{
    if(hasChunkAt(x, z)) {
        // Just disallow action below or above min/max height,
        // but don't crash the game over it.
        if(y < 0 || y >= 256) {
            return EMPTY;
        }
        const uPtr<Chunk> &c = getChunkAt(x, z);
        glm::vec2 chunkOrigin = glm::vec2(floor(x / 16.f) * 16, floor(z / 16.f) * 16);
        return c->getBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                             static_cast<unsigned int>(y),
                             static_cast<unsigned int>(z - chunkOrigin.y));
    }
    else {
        throw std::out_of_range("getBlockAt: Coordinates " + std::to_string(x) +
                                " " + std::to_string(y) + " " +
                                std::to_string(z) + " have no Chunk!");
    }
}

BlockType Terrain::getBlockAt(glm::vec3 p) const {
    if(!hasChunkAt(p.x, p.z))
        return EMPTY;
    return getBlockAt(p.x, p.y, p.z);
}

bool Terrain::checkBlockAt(glm::vec3 p) const {
    if(!hasChunkAt(p.x, p.z))
    {
        return false;
    }

    BlockType bType = getBlockAt(p);
    return (bType == EMPTY || bType == WATER);
}

bool Terrain::hasChunkAt(int x, int z) const {
    // Map x and z to their nearest Chunk corner
    // By flooring x and z, then multiplying by 16,
    // we clamp (x, z) to its nearest Chunk-space corner,
    // then scale back to a world-space location.
    // Note that floor() lets us handle negative numbers
    // correctly, as floor(-1 / 16.f) gives us -1, as
    // opposed to (int)(-1 / 16.f) giving us 0 (incorrect!).
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));

    auto search = m_chunks.find(toKey(16 * xFloor, 16 * zFloor));
    return (search != m_chunks.end() && search->second != nullptr );
}


uPtr<Chunk>& Terrain::getChunkAt(int x, int z) {
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks[toKey(16 * xFloor, 16 * zFloor)];
}


const uPtr<Chunk>& Terrain::getChunkAt(int x, int z) const {
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks.at(toKey(16 * xFloor, 16 * zFloor));
}

/**
 * IMPORTANT MESSAGE:
 *      For anyone who might call this function:
 *      1. It should only be called outside Terrain.
 *      2. The fluid of water and lava is called from here.
*/
void Terrain::setBlockAt(int x, int y, int z, BlockType t)
{
    std::vector<glm::vec3> dir = {glm::vec3(1,0,0),glm::vec3(-1,0,0),glm::vec3(0,0,-1),glm::vec3(0,0,1),glm::vec3(1,0,1),glm::vec3(1,0,-1),glm::vec3(-1,0,1),glm::vec3(-1,0,-1),glm::vec3(0,1,0)};
    // chunkMutex.lock();
//     std::cout << "Terrain::setBlockAt " << x << ", " << z << std::endl;
    if(hasChunkAt(x, z)) {
        uPtr<Chunk> &c = getChunkAt(x, z);
        glm::vec2 chunkOrigin = glm::vec2(floor(x / 16.f) * 16, floor(z / 16.f) * 16);
        // c->resetVBOCreated();
        c->setBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                      static_cast<unsigned int>(y),
                      static_cast<unsigned int>(z - chunkOrigin.y),
                      t);

        std::unordered_map<int64_t, std::pair<int, int>> updateChunks;

        if(t == WATER){
            std::vector<glm::vec3> slant;
            std::deque<glm::vec3> new_water;
            new_water.push_back(glm::vec3(x,y,z));
            while(new_water.size() > 0){


                // TODO: update it inside fluid_sim
                /*int xFloor = static_cast<int>(glm::floor(new_water.front().x / 16.f)) * 16;
                int zFloor = static_cast<int>(glm::floor(new_water.front().z / 16.f)) * 16;
                int64_t key = toKey(xFloor, zFloor);
                if(chunkSet.find(key) == chunkSet.end())
                {
                    chunkSet.insert(key);
                    chunkToUpdate.push_back({xFloor, zFloor});
                }*/
                fluid_sim(water_fluid, new_water.front().x, new_water.front().y, new_water.front().z,
                          t, new_water, slant, updateChunks);
                new_water.pop_front();
            }
        }else if(t == LAVA){
            std::vector<glm::vec3> slant;
            std::deque<glm::vec3> new_lava;
            new_lava.push_back(glm::vec3(x,y,z));
            while(new_lava.size() > 0){
//                int xFloor = static_cast<int>(glm::floor(new_lava.front().x / 16.f)) * 16;
//                int zFloor = static_cast<int>(glm::floor(new_lava.front().z / 16.f)) * 16;
//                int64_t key = toKey(xFloor, zFloor);
//                if(chunkSet.find(key) == chunkSet.end())
//                {
//                    chunkSet.insert(key);
//                    chunkToUpdate.push_back({xFloor, zFloor});
//                }
                fluid_sim(lava_fluid, new_lava.front().x, new_lava.front().y, new_lava.front().z,
                          t, new_lava, slant, updateChunks);
                new_lava.pop_front();
            }
        }else{
            markChunkToUpdate(x, z);
        }

        if(t == WATER || t == LAVA)
        {
            for(auto kv: updateChunks)
            {
                markChunkToUpdate(kv.second.first, kv.second.second);
            }
        }
    }
    else {
        throw std::out_of_range("setBlockAT: Coordinates " + std::to_string(x) +
                                " " + std::to_string(y) + " " +
                                std::to_string(z) + " have no Chunk!");
    }
    // chunkMutex.unlock();
}

// not used now
Chunk* Terrain::instantiateChunkAt(int x, int z) {
    uPtr<Chunk> chunk = mkU<Chunk>(mp_context);
    Chunk *cPtr = chunk.get();

    m_chunks[toKey(x, z)] = move(chunk);
    // Set the neighbor pointers of itself and its neighbors
    if(hasChunkAt(x, z + 16)) {
        auto &chunkNorth = m_chunks[toKey(x, z + 16)];
        cPtr->linkNeighbor(chunkNorth, ZPOS);
    }
    if(hasChunkAt(x, z - 16)) {
        auto &chunkSouth = m_chunks[toKey(x, z - 16)];
        cPtr->linkNeighbor(chunkSouth, ZNEG);
    }
    if(hasChunkAt(x + 16, z)) {
        auto &chunkEast = m_chunks[toKey(x + 16, z)];
        cPtr->linkNeighbor(chunkEast, XPOS);
    }
    if(hasChunkAt(x - 16, z)) {
        auto &chunkWest = m_chunks[toKey(x - 16, z)];
        cPtr->linkNeighbor(chunkWest, XNEG);
    }
    return cPtr;
}

// used now in multithread
void Terrain::linkChunk(int x, int z) {
    uPtr<Chunk>& cPtr = m_chunks[toKey(x, z)];
    // Set the neighbor pointers of itself and its neighbors
    std::array<int, 4> posX = {x,    x,    x+16, x-16};
    std::array<int, 4> posZ = {z+16, z-16, z,    z};
    std::array<Direction, 4> dir = {ZPOS, ZNEG, XPOS, XNEG};

    for(int i = 0; i < 4; i++)
    {
        if(hasChunkAt(posX[i], posZ[i])) {
            auto &chunkNorth = m_chunks[toKey(posX[i], posZ[i])];
            cPtr->linkNeighbor(chunkNorth, dir[i]);
#ifdef LINK_REFRESH
            if(chunkNorth != nullptr && chunkNorth->m_vboCreated)
            {
                //std::cout << "re-create VBO when linked" << std::endl;
                Chunk* cp = chunkNorth.get();
                m_newLinkedChunks.insert(cp);
            }
#endif
        }
    }
}

void Terrain::instantiateZoneAt(int xFloor, int zFloor)
{
    for(int x = xFloor; x < xFloor + 64; x += 16)
    {
        for(int z = zFloor; z < zFloor + 64; z += 16)
        {
            instantiateChunkAt(x, z);
        }
    }
}

// Jack: initialize the chunk at (x, z) with blocks
void Terrain::initializeChunkAt(int x, int z)
{
    int xFloor = static_cast<int>(glm::floor(x / 16.f)) * 16;
    int zFloor = static_cast<int>(glm::floor(z / 16.f)) * 16;

    // std::cout << "initializeChunkAt:" << x << ", " << z << std::endl;

    uPtr<Chunk> chunk = mkU<Chunk>(mp_context);
    Chunk *cPtr = chunk.get();
    cPtr->setPos(glm::vec3(xFloor, 0, zFloor));

    for(int xi = xFloor; xi < xFloor + 16; xi++)
    {
        for(int zi = zFloor; zi < zFloor + 16; zi++)
        {
            initializeBlocksAt(xi, zi, cPtr);
        }
    }

    addAsset(xFloor, zFloor, cPtr);

    generatedChunkMutex.lock();
    m_sharedGeneratedChunks.push_back(move(chunk));
    generatedChunkMutex.unlock();
}

// initialize all blocks at (x, y, z) for all y in [0, 255]
void Terrain::initializeBlocksAt(int x, int z, Chunk* cPtr)
{
    glm::vec2 chunkOrigin = glm::vec2(floor(x / 16.f) * 16, floor(z / 16.f) * 16);

#ifdef TEST_GROUND
    for (int y = 100; y < 130; y++)
    {
        cPtr->setBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                      static_cast<unsigned int>(y),
                      static_cast<unsigned int>(z - chunkOrigin.y),
                      GRASS);
    }
    return;
#endif

#ifdef TEST_MOUNT
    float mn =
            noise.expPerlinNoise2D(glm::vec2(x, z) / 64.f, 1.f, false);
    mn = std::clamp(mn, 0.f, 1.f);
    for(int y = 129; y <= 129 + (int)(mn * 65.f); y++)
    {
        cPtr->setBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                      static_cast<unsigned int>(y),
                      static_cast<unsigned int>(z - chunkOrigin.y),
                      GRASS);
        //setBlockAt(x, y, z, GRASS);
    }
    return;
#endif

    float moistureExp = 1.f;
    float moistureGridSize = 600.0f;
    float moisture =
            noise.expPerlinNoise2D(glm::vec2(x, z) / moistureGridSize, moistureExp, true);
    moisture = noise.smoothStep(0.15f, 0.65f, moisture);

    float temperatureExp = 1.f;
    float temperatureGridSize = 600.0f;
    noise.changeRandom2Seed(2594.7126f);
    float temperature =
            noise.expPerlinNoise2D(glm::vec2(x + 563, z - 850) / temperatureGridSize, temperatureExp, true);
    temperature = noise.smoothStep(0.35f, 0.65f, temperature);


    BiomeType biome;
    if(moisture > 0.5f) // for convenience, rules for biome might not match reality
    {
        if(temperature < 0.5f)
        {
            biome = MOUNTAIN;
        }
        else
        {
            biome = DESERT;
        }
    }
    else
    {
        if(temperature < 0.5f)
        {
            biome = SNOWLAND;
        }
        else
        {
            biome = GRASSLAND;
        }
    }


    float mountHeight;
    float grassHeight;
    int height;
    if(moisture > 0.02f)
    {
        float mountExp = 2.f;
        float mountainGridSize = 128.0f;
        float mountNoise =
                noise.expPerlinNoise2D(glm::vec2(x, z) / mountainGridSize, mountExp, true);

        mountHeight = (255.f-139.f) * mountNoise + 139.f;
        mountHeight = std::clamp(mountHeight, 139.f, 255.f);
    }
    if(moisture < 0.98f || temperature > 0.5f)
    {
        noise.setFbmWorleyConfig(0.67f, 64.f, 128.f);
        // float grassNoise = noise.thermalNoise(glm::vec2(x, z));
        float grassNoise = noise.fbmWorleyNoisePattern(glm::vec2(x, z));
        // float grassNoise = noise.interpNoise2D(x / 64.f, z / 64.f);

        //grassNoise = noise.smoothStep(noise.smoothStep(grassNoise));
        float grassBottom =  128.f;
        float grassDuration = 40.f;
        grassHeight = grassDuration * grassNoise + grassBottom;
        grassHeight = std::clamp(grassHeight, grassBottom, 255.f);
#ifdef RIVER_GENERATION
        // river
        grassHeight = noise.getRiverHeight(grassHeight, x, z);
#endif
    }

    if(temperature > 0.5f)
    {
        float tr = 1 - noise.smoothStep(2.f * (temperature - 0.5f));
        mountHeight = noise.lerp(mountHeight, grassHeight, tr);
    }


    if(moisture <= 0.02f)
    {
        height = grassHeight;
    }
    else if(moisture >= 0.98f)
    {
        height = mountHeight;
    }
    else
    {
        height = noise.lerp(mountHeight, grassHeight, moisture);
    }


    if(biome == GRASSLAND)
    {
        for(int y = 129; y < height; y++)
        {
            cPtr->setBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                          static_cast<unsigned int>(y),
                          static_cast<unsigned int>(z - chunkOrigin.y),
                          DIRT);
            //setBlockAt(x, y, z, GRASS);
        }
        cPtr->setBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                      static_cast<unsigned int>(height),
                      static_cast<unsigned int>(z - chunkOrigin.y),
                      GRASS);
    }
    else if(biome == MOUNTAIN)
    {

        for(int y = 129; y <= height; y++)
        {
            cPtr->setBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                          static_cast<unsigned int>(y),
                          static_cast<unsigned int>(z - chunkOrigin.y),
                          STONE);
            // setBlockAt(x, y, z, STONE);
        }
        if(height >= 200)
        {
            cPtr->setBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                          static_cast<unsigned int>(height),
                          static_cast<unsigned int>(z - chunkOrigin.y),
                          SNOW);
            // setBlockAt(x, height, z, SNOW);
        }
    }
    else if(biome == DESERT)
    {
        for(int y = 129; y <= height; y++)
        {
            cPtr->setBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                          static_cast<unsigned int>(y),
                          static_cast<unsigned int>(z - chunkOrigin.y),
                          SAND);
            //setBlockAt(x, y, z, GRASS);
        }
    }
    else if(biome == SNOWLAND)
    {
        int heightLimit = height > 140 ? 140 : height - 1;
        for(int y = 129; y <= heightLimit; y++)
        {
            cPtr->setBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                          static_cast<unsigned int>(y),
                          static_cast<unsigned int>(z - chunkOrigin.y),
                          STONE);
            // setBlockAt(x, y, z, STONE);
        }
        for(int y = heightLimit + 1; y <= height; y++)
        {
            cPtr->setBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                          static_cast<unsigned int>(y),
                          static_cast<unsigned int>(z - chunkOrigin.y),
                          SNOW);
            // setBlockAt(x, y, z, STONE);
        }
    }

#ifdef WATER_GENERATION
    if (height < 138)
    {
        for(int y = height + 1; y <= 138; y++)
        {
            cPtr->setBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                          static_cast<unsigned int>(y),
                          static_cast<unsigned int>(z - chunkOrigin.y),
                          WATER);
            // setBlockAt(x, y, z, WATER);
        }
    }
#endif

#ifdef CAVE_GENERATION
    // caves
    float caveGridSize = 32.f;
    for (int y = 1; y < 129; y++)
    {
        float caveNoise = noise.perlinNoise3D(glm::vec3(x, y, z) / caveGridSize);
        if (caveNoise < 0) {
            if (y < 25) {
                cPtr->setBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                              static_cast<unsigned int>(y),
                              static_cast<unsigned int>(z - chunkOrigin.y),
                              LAVA);
            } else {
                cPtr->setBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                              static_cast<unsigned int>(y),
                              static_cast<unsigned int>(z - chunkOrigin.y),
                              EMPTY);
            }
        } else {
            cPtr->setBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                          static_cast<unsigned int>(y),
                          static_cast<unsigned int>(z - chunkOrigin.y),
                          CAVE);
        }
    }
    cPtr->setBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                  static_cast<unsigned int>(0),
                  static_cast<unsigned int>(z - chunkOrigin.y),
                  BEDROCK);
#endif


#ifdef BIG_BUILDING_GENERATION
//
#endif

}

void Terrain::addAsset(int xf, int zf, Chunk* cPtr)
{

#ifdef BIG_ASSET_GENERATION
    int fixHeight = 153;
    // glm::vec2 tree_center(-24, 24);
    glm::vec2 tree_center(500, 20);
    if(std::abs(xf + 8 - tree_center[0]) < noise.tree_l + 16
        && std::abs(zf + 8 - tree_center[1]) < noise.tree_w + 16)
    {
        for(int xi = 0; xi < 16; xi++)
        {
            int nx = xi + xf - tree_center[0] + noise.tree_l / 2;
            if(nx < 0 || nx >= noise.tree_l){
                continue;
            }
            for(int zi = 0; zi < 16; zi++)
            {
                int nz = zi + zf - tree_center[1] + noise.tree_w / 2;
                if(nz < 0 || nz >= noise.tree_w){
                    continue;
                }
                for(int y = fixHeight; y < fixHeight + noise.tree_h; y++)
                {
                    BlockType bt = noise.getTree(
                                nx, y-fixHeight, nz);
                    if(bt != EMPTY)
                    {
                        cPtr->setBlockAt(
                                    static_cast<unsigned int>(xi),
                                    static_cast<unsigned int>(y),
                                    static_cast<unsigned int>(zi),
                                    bt);
                    }

                }

            }
        }

        return;
    }
#endif

    int assetRange = std::rand() % 100;
    // int assetRange = 50;

    bool smallAssetMode;
    int rx, rz;
    if(assetRange < 25)
    {
        return;
    }
    else if(assetRange < 75) // add small asset
    {
        rx = std::rand() % 16;
        rz = std::rand() % 16;
        smallAssetMode = true;
    }
    else{ // add big asset
        rx = std::rand() % 12 + 2;
        rz = std::rand() % 12 + 2;
        smallAssetMode = false;
    }


    if(cPtr->getBlockAt(rx, 251, rz)!= EMPTY){return;}

    for(int i = 250; i > 128; i--){
        BlockType type = cPtr->getBlockAt(rx, i, rz);
        if(type != EMPTY)
        {
            if(type == SAND){
                // add cactus
                // std::cout << "add sand asset" << std::endl;
                cPtr->setBlockAt(static_cast<unsigned int>(rx), static_cast<unsigned int>(i+1), static_cast<unsigned int>(rz), CACTUS);
                cPtr->setBlockAt(static_cast<unsigned int>(rx), static_cast<unsigned int>(i+2), static_cast<unsigned int>(rz), CACTUS);
                cPtr->setBlockAt(static_cast<unsigned int>(rx), static_cast<unsigned int>(i+3), static_cast<unsigned int>(rz), CACTUS);
                // std::cout << "end sand asset" << std::endl;
            }
            else if(type == GRASS || type == SNOW)
            {
                if(smallAssetMode)
                {
                    // add flower / grass

                    BlockType smallAssetType = std::rand() % 100 < 50 ? FLOWER_1 : FLOWER_2;

                    cPtr->setBlockAt(static_cast<unsigned int>(rx), static_cast<unsigned int>(i+1), static_cast<unsigned int>(rz), smallAssetType);
                }
                else{ // add tree
                    cPtr->setBlockAt(static_cast<unsigned int>(rx), static_cast<unsigned int>(i+1), static_cast<unsigned int>(rz), WOOD);
                    cPtr->setBlockAt(static_cast<unsigned int>(rx), static_cast<unsigned int>(i+2), static_cast<unsigned int>(rz), WOOD);
                    cPtr->setBlockAt(static_cast<unsigned int>(rx), static_cast<unsigned int>(i+3), static_cast<unsigned int>(rz), WOOD);

                    for(int dx = -2; dx <= 2; dx++){
                        for(int dz = -2; dz <= 2; dz++){
                            if(cPtr->getBlockAt(rx+dx, i+4, rz+dz) == EMPTY){
                                cPtr->setBlockAt(static_cast<unsigned int>(rx+dx), static_cast<unsigned int>(i+4), static_cast<unsigned int>(rz+dz), LEAF);
                            }
                        }
                    }
                    for(int dx = -1; dx <= 1; dx++){
                        for(int dz = -1; dz <= 1; dz++){
                            if(cPtr->getBlockAt(rx+dx, i+5, rz+dz) == EMPTY){
                                cPtr->setBlockAt(static_cast<unsigned int>(rx+dx), static_cast<unsigned int>(i+5), static_cast<unsigned int>(rz+dz), LEAF);
                            }
                        }
                    }
                    cPtr->setBlockAt(static_cast<unsigned int>(rx), static_cast<unsigned int>(i+4), static_cast<unsigned int>(rz), WOOD);
                    cPtr->setBlockAt(static_cast<unsigned int>(rx), static_cast<unsigned int>(i+5), static_cast<unsigned int>(rz), WOOD);
                    cPtr->setBlockAt(static_cast<unsigned int>(rx), static_cast<unsigned int>(i+6), static_cast<unsigned int>(rz), LEAF);
                }
            }
            else if(type == STONE)
            {
                BlockType smallAssetType = std::rand() % 100 < 75  ? MUSHROOM_1 : MUSHROOM_2;

                cPtr->setBlockAt(static_cast<unsigned int>(rx), static_cast<unsigned int>(i+1), static_cast<unsigned int>(rz), smallAssetType);
            }

            return;
        }
    }

}

bool Terrain::CheckZoneVboCreated(int xFloor, int zFloor)
{
    for(int x = xFloor; x < xFloor + 16 * 4; x += 16)
    {
        for(int z = zFloor; z < zFloor + 16 * 4; z += 16)
        {
            if(!getChunkAt(x, z)->m_vboCreated)
            {
                return false;
            }
        }
    }
    return true;
}

void Terrain::markChunkToUpdate(int x, int z)
{
//    std::cout << "mark chunk:" << x << ", " << z << std::endl;
    auto& chunk = getChunkAt(x, z);
    chunk->resetVBOCreated();
    chunk->createVBOdata();
//    std::cout << "after createBO" << std::endl;
    chunk->bufferAlldata();
}

void Terrain::BlockTypeWork(int xFloor, int zFloor)
{
    // std::cout << "BlockTypeWorker:" << xFloor << ", " << zFloor << std::endl;

    for(int x = xFloor; x < xFloor + 16 * 4; x += 16)
    {
        for(int z = zFloor; z < zFloor + 16 * 4; z += 16)
        {
            initializeChunkAt(x, z);
        }
    }
    return;
}

void Terrain::VBOWork(Chunk* chunk)
{
    // std::cout << "VBOWorker" << chunkUPtr->getPos().x << ", " << chunkUPtr->getPos().z << std::endl;

    chunk->createVBOdata();

    createdChunkMutex.lock();
    m_sharedCreatedChunks.push_back(chunk);
    createdChunkMutex.unlock();
}

void Terrain::ExpandDraw(glm::vec3 playerPos, ShaderProgram *shaderProgram, bool isDepthRender)
{
    int zoneLen = 5;

    int xFloor = static_cast<int>(glm::floor(playerPos.x / 64.f)) * 64;
    int zFloor = static_cast<int>(glm::floor(playerPos.z / 64.f)) * 64;

    // the corner of drawing zone
    xFloor -= (zoneLen - 1) * 32;
    zFloor -= (zoneLen - 1) * 32;

    if(xFloor != oriXFloor || zFloor != oriZFloor)
    {
        if(!firstDraw) {
            for(int iter = 0; iter < 5; iter++)
            {
                for(int xi = xFloor; xi < xFloor + 64 * zoneLen; xi += 64)
                {
                    for(int zi = zFloor; zi < zFloor + 64 * zoneLen; zi += 64)
                    {
                        if(m_generatedTerrain.find(toKey(xi, zi)) == m_generatedTerrain.end())
                        {
                            noise.updateRiverBuffer(xi, zi);
                        }
                    }
                }
            }

        }
        else{
            firstDraw = false;
        }

        oriXFloor = xFloor;
        oriZFloor = zFloor;
        allTerrainGenerated = false;
    }

    // generate chunks with random noise, using multithread
    if(!allTerrainGenerated)
    {
        bool checkTerrain = true;
        for(int xi = xFloor; xi < xFloor + 64 * zoneLen; xi += 64)
        {
            for(int zi = zFloor; zi < zFloor + 64 * zoneLen; zi += 64)
            {
                if(m_generatedTerrain.find(toKey(xi, zi)) == m_generatedTerrain.end())
                {
                    m_generatedTerrain.insert(toKey(xi, zi));

                    BlockTypeWorker *w = new BlockTypeWorker(xi, zi, this);
                    QThreadPool::globalInstance()->start(w);

                    checkTerrain = false;
                }
            }
        }
        if(checkTerrain)
        {
            allTerrainGenerated = true;
        }
    }

    if(m_sharedGeneratedChunks.size() > 0)
    {
        generatedChunkMutex.lock();
        for(uPtr<Chunk>& chunkUPtr : m_sharedGeneratedChunks)
        {
            Chunk* cp = chunkUPtr.get();
            glm::vec3 p = chunkUPtr->getPos();
            m_chunks[toKey(p.x, p.z)] = move(chunkUPtr);
            linkChunk(p.x, p.z);
            VBOWorker *w = new VBOWorker(cp, this);
            QThreadPool::globalInstance()->start(w);
        }
        m_sharedGeneratedChunks.clear();
        generatedChunkMutex.unlock();
    }

    if(m_sharedCreatedChunks.size() > 0)
    {
        createdChunkMutex.lock();
        for(Chunk* chunk : m_sharedCreatedChunks)
        {
            chunk->bufferAlldata();
        }
        m_sharedCreatedChunks.clear();
        createdChunkMutex.unlock();
    }
    else if(m_newLinkedChunks.size() > 0 && allTerrainGenerated)
    {
        for(Chunk* cp : m_newLinkedChunks)
        {
            cp->resetVBOCreated();
            VBOWorker *w = new VBOWorker(cp, this);
            QThreadPool::globalInstance()->start(w);
        }
        m_newLinkedChunks.clear();
    }

    draw(xFloor, xFloor + 64 * zoneLen, zFloor, zFloor + 64 * zoneLen, shaderProgram, isDepthRender);
}

// DONE: When you make Chunk inherit from Drawable, change this code so
// it draws each Chunk with the given ShaderProgram, remembering to set the
// model matrix to the proper X and Z translation!
void Terrain::draw(int minX, int maxX, int minZ, int maxZ, ShaderProgram *shaderProgram, bool isDepthRender) {
    // std::cout << "draw:" << std::endl << minX << ", " << maxX << ", " << minZ << ", " << maxZ << std::endl;
    for(int x = minX; x < maxX; x += 16) {
        for(int z = minZ; z < maxZ; z += 16) {
            const uPtr<Chunk> &chunk = getChunkAt(x, z);
            if (chunk != nullptr && chunk->m_vboBuffered) {
                if (isDepthRender) {
                    shaderProgram->drawPosOnly(*chunk);
                } else {
                    shaderProgram->drawWithItlvVBOs(*chunk);
                }
            }
        }
    }

#ifdef DRAW_TRANSPARENT
    for(int x = minX; x < maxX; x += 16) {
        for(int z = minZ; z < maxZ; z += 16) {
            const uPtr<Chunk> &chunk = getChunkAt(x, z);
            if (chunk != nullptr && chunk->m_vboTransBuffered) {
                if (isDepthRender) {
                    shaderProgram->drawTransPosOnly(*chunk);
                } else {
                    shaderProgram->drawTransparent(*chunk);
                }
            }
        }
    }
#endif
}

void Terrain::initialize()
{
    QThreadPool::globalInstance()->setMaxThreadCount(8);
#ifdef RIVER_GENERATION
    noise.riverBuffer();
#endif

#ifdef BIG_ASSET_GENERATION
    noise.treeBuffer();
#endif
}


void Terrain::fluid_sim(const std::unordered_set<BlockType> &fluidSet, int x, int y, int z,
                        BlockType t, std::deque<glm::vec3> &deq, std::vector<glm::vec3> &slant,
                        std::unordered_map<int64_t, std::pair<int, int>>& updateChunks)
{
    uPtr<Chunk> &c = getChunkAt(x, z);
    glm::vec2 chunkOrigin = glm::vec2(floor(x / 16.f) * 16, floor(z / 16.f) * 16);
    int j = -1;

    // set all available blocks below the beginning position, to lava
    while(getBlockAt(x,y+j,z) == EMPTY || fluidSet.find(getBlockAt(x,y+j,z)) != fluidSet.end()){
        c->setBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                      static_cast<unsigned int>(y+j),
                      static_cast<unsigned int>(z - chunkOrigin.y),
                      t);
        j -= 1;
    }
    if(j != -1){
        deq.push_back(glm::vec3(x,y+j+1,z));
    }

    for(BlockType bt: fluidSet)
    {
        int maxi = 2;
        if((bt >= WATER_XPZP && bt <= WATER_XNZN) || (bt >= LAVA_XPZP && bt <= LAVA_XNZN))
        {
            maxi = 1;
        }
        for(int i = 0; i < maxi; i++){
            int nx, nz;
            switch(bt)
            {
            case LAVA_XP:
            case WATER_XP:
                nx = x + i + 1;
                nz = z;
                break;
            case LAVA_XN:
            case WATER_XN:
                nx = x - i - 1;
                nz = z;
                break;
            case LAVA_ZP:
            case WATER_ZP:
                nx = x;
                nz = z + i + 1;
                break;
            case LAVA_ZN:
            case WATER_ZN:
                nx = x;
                nz = z - i - 1;
                break;
            case LAVA_XPZP:
            case WATER_XPZP:
                nx = x + i + 1;
                nz = z + i + 1;
                break;
            case LAVA_XPZN:
            case WATER_XPZN:
                nx = x + i + 1;
                nz = z - i - 1;
                break;
            case LAVA_XNZP:
            case WATER_XNZP:
                nx = x - i - 1;
                nz = z + i + 1;;
                break;
            case LAVA_XNZN:
            case WATER_XNZN:
                nx = x - i - 1;
                nz = z - i - 1;
                break;
            default:
                nx = x; nz = z;
                std::cout << "ERROR: wrong block in fluid_sim" << std::endl;
                break;
            }

            if(getBlockAt(nx, y, nz) == EMPTY){
                // BlockType bt = LAVA_XP;
                uPtr<Chunk> &c = getChunkAt(nx, nz);
                glm::vec2 chunkOrigin = glm::vec2(floor((nx) / 16.f) * 16, floor(nz / 16.f) * 16);
                c->setBlockAt(static_cast<unsigned int>(nx - chunkOrigin.x),
                              static_cast<unsigned int>(y),
                              static_cast<unsigned int>(nz - chunkOrigin.y),
                              bt);
                //add slant blocks into vector
                slant.push_back(glm::vec3(nx, y, nz));
                int j = -1;
                while(getBlockAt(nx, y+j, nz) == EMPTY || fluidSet.find(getBlockAt(nx, y+j, nz)) != fluidSet.end()){
                    c->setBlockAt(static_cast<unsigned int>(nx - chunkOrigin.x),
                                  static_cast<unsigned int>(y+j),
                                  static_cast<unsigned int>(nz - chunkOrigin.y),
                                  t);
                    j -= 1;
                }
                if(j != -1){
                    deq.push_back(glm::vec3(nx, y+j+1, nz));
                }

                int nxf = static_cast<int>(glm::floor(nx / 16.f)) * 16;
                int nzf = static_cast<int>(glm::floor(nz / 16.f)) * 16;
                int64_t key = toKey(nxf, nzf);
                if(updateChunks.find(key) == updateChunks.end())
                {
                    updateChunks.insert({key, {nxf, nzf}});
                }
            }else{
                // std::cout << "xp not empty" << std::endl;
                break;
            }
        }
    }
}
