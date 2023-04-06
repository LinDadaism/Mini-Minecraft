//#include "chunk.h"
#include "chunkhelpers.h"
#include <iostream>
#include <unordered_set>

Chunk::Chunk(OpenGLContext* context)
    : Drawable(context),
      m_blocks(), m_neighbors{{XPOS, nullptr}, {XNEG, nullptr}, {ZPOS, nullptr}, {ZNEG, nullptr}},
      m_pos(glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX)), m_vboCreated(false), m_vboBuffered(false), m_vboTransBuffered(false), opaque(true)
{
    std::fill_n(m_blocks.begin(), 65536, EMPTY);
}

Chunk::~Chunk()
{
    m_idx.clear();
    m_buffer.clear();
//    pos_buffer.clear();
//    col_buffer.clear();
//    nor_buffer.clear();
    m_Transbuffer.clear();
    m_Transidx.clear();
}

bool Chunk::isOpaque(BlockType t) {
    return t != EMPTY && t != WATER && t != ICE && water_fluid.find(t) == water_fluid.end();
}

bool Chunk::isSmallAsset(BlockType t) {
    return t == PURE_GRASS || t == FLOWER_1 || t == FLOWER_2 || t == MUSHROOM_1 || t == MUSHROOM_2;
}

void Chunk::bufferVBOdata(std::vector<glm::vec4> &buffer, std::vector<GLuint> &idx) {
    m_count = idx.size();

    generateData();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufData);
    mp_context->glBufferData(GL_ARRAY_BUFFER, buffer.size() * sizeof(glm::vec4), buffer.data(), GL_STATIC_DRAW);

    generateIdx();
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdx);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_count * sizeof(GLuint), idx.data(), GL_STATIC_DRAW);

    m_vboBuffered = true;
}

void Chunk::bufferVBOTransdata(std::vector<glm::vec4> &buffer, std::vector<GLuint> &idx) {
    m_Transcount = idx.size();

    generateTranData();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufTransData);
    mp_context->glBufferData(GL_ARRAY_BUFFER, buffer.size() * sizeof(glm::vec4), buffer.data(), GL_STATIC_DRAW);

    generateTransIdx();
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufTransIdx);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_Transcount * sizeof(GLuint), idx.data(), GL_STATIC_DRAW);

    m_vboTransBuffered = true;
}

bool Chunk::checkFluidAdjSkip(enum BlockType curr, enum Direction dir, enum BlockType adjacent){
    if(curr == LAVA){
        if(dir == ZNEG && adjacent == LAVA_ZN) return true;
        if(dir == ZPOS && adjacent == LAVA_ZP) return true;
        if(dir == XNEG && adjacent == LAVA_XN) return true;
        if(dir == XPOS && adjacent == LAVA_XP) return true;
    }
    else if(curr == WATER || curr == ICE){
        if(dir == ZNEG && adjacent == WATER_ZN) return true;
        if(dir == ZPOS && adjacent == WATER_ZP) return true;
        if(dir == XNEG && adjacent == WATER_XN) return true;
        if(dir == XPOS && adjacent == WATER_XP) return true;
    }
    else if(curr == LAVA_XP){
        if(dir == ZPOS && adjacent == LAVA_XPZP) return true;
        if(dir == ZNEG && adjacent == LAVA_XPZN) return true;
    }else if(curr == LAVA_XN){
        if(dir == ZPOS && adjacent == LAVA_XNZP) return true;
        if(dir == ZNEG && adjacent == LAVA_XNZN) return true;
    }else if(curr == LAVA_ZN){
        if(dir == XPOS && adjacent == LAVA_XPZN) return true;
        if(dir == XNEG && adjacent == LAVA_XNZN) return true;
    }else if(curr == LAVA_ZP){
        if(dir == XPOS && adjacent == LAVA_XPZP) return true;
        if(dir == XNEG && adjacent == LAVA_XNZP) return true;
    }else if(curr == WATER_XP){
        if(dir == ZPOS && adjacent == WATER_XPZP) return true;
        if(dir == ZNEG && adjacent == WATER_XPZN) return true;
    }else if(curr == WATER_XN){
        if(dir == ZPOS && adjacent == WATER_XNZP) return true;
        if(dir == ZNEG && adjacent == WATER_XNZN) return true;
    }else if(curr == WATER_ZN){
        if(dir == XPOS && adjacent == WATER_XPZN) return true;
        if(dir == XNEG && adjacent == WATER_XNZN) return true;
    }else if(curr == WATER_ZP){
        if(dir == XPOS && adjacent == WATER_XPZP) return true;
        if(dir == XNEG && adjacent == WATER_XNZP) return true;
    }else{
        if(dir == YNEG) return true;
    }
    return false;
}

// This function return true when 1 of 4 conditions satifies:
// For lava with XP, X == 1; For lava with ZP, Z == 1
// For lava with XN, X == 0; For lava with ZN, Z == 0
bool Chunk::checkFluidPos(enum BlockType curr, float x, float z){
    if(curr == LAVA_XP || curr == LAVA_XPZN || curr == LAVA_XPZP
            || curr == WATER_XP || curr == WATER_XPZN || curr == WATER_XPZP){
        if(std::abs(x - 1.f) < 0.001) return true;
    }
    if(curr == LAVA_XN || curr == LAVA_XNZN || curr == LAVA_XNZP
            || curr == WATER_XN || curr == WATER_XNZN || curr == WATER_XNZP){
        if(std::abs(x) < 0.001) return true;
    }
    if(curr == LAVA_ZP || curr == LAVA_XPZP || curr == LAVA_XNZP
            || curr == WATER_ZP || curr == WATER_XPZP || curr == WATER_XNZP){
        if(std::abs(z - 1.f) < 0.001) return true;
    }
    if(curr == LAVA_ZN || curr == LAVA_XPZN || curr == LAVA_XNZN
            || curr == WATER_ZN || curr == WATER_XPZN || curr == WATER_XNZN){
        if(std::abs(z) < 0.001) return true;
    }
    return false;
}

void Chunk:: createVBOdata() {
    float unit_len = BLK_UV;

    if (!m_vboCreated) {
        m_buffer.clear();
        m_Transbuffer.clear();
//        col_buffer.clear();
//        pos_buffer.clear();
//        nor_buffer.clear();
        m_Transidx.clear();
        m_idx.clear();
        int totalVerts = 0;
        int totalTransVerts = 0;

        for (unsigned int x = 0; x < 16; x++) {
            for (unsigned int z = 0; z < 16; z++) {
                for (unsigned int y = 0; y < 256; y++) {
                    BlockType curr = getBlockAt(x, y, z);
                    bool smallAsset = isSmallAsset(curr);
                    bool inLavaFlow = lava_fluid.find(curr) != lava_fluid.end();


                    //1. fluid blocks
                    if(inLavaFlow || water_fluid.find(curr) != water_fluid.end())
                    {
                        // check relevant last blocks, then decide if shrink the height
                        int test_x;
                        int test_y;
                        int test_z;
                        float value = 0.f;
                        // test block
                        BlockType tb;
                        if(curr == LAVA_XP || curr == WATER_XP){
                            test_x = x + adjacentFaces[1].dirVec[0];
                            test_y = y + adjacentFaces[1].dirVec[1];
                            test_z = z + adjacentFaces[1].dirVec[2];
                            tb = getBlockAt(test_x, test_y, test_z);
                            if((inLavaFlow && (tb == LAVA_XP || tb == LAVA_XPZP || tb == LAVA_XPZN)) ||
                                (!inLavaFlow && (tb == WATER_XP || tb == WATER_XPZP || tb == WATER_XPZN)))
                                value -= 0.3f;
                        }else if(curr == LAVA_XN || curr == WATER_XN){
                            test_x = x + adjacentFaces[0].dirVec[0];
                            test_y = y + adjacentFaces[0].dirVec[1];
                            test_z = z + adjacentFaces[0].dirVec[2];
                            tb = getBlockAt(test_x, test_y, test_z);
                            if((inLavaFlow && (tb == LAVA_XN || tb == LAVA_XNZP || tb == LAVA_XNZN)) ||
                                (!inLavaFlow && (tb == WATER_XN || tb == WATER_XNZP || tb == WATER_XNZN)))
                                value -= 0.3f;
                        }else if(curr == LAVA_ZN || curr == WATER_ZN){
                            test_x = x + adjacentFaces[4].dirVec[0];
                            test_y = y + adjacentFaces[4].dirVec[1];
                            test_z = z + adjacentFaces[4].dirVec[2];
                            tb = getBlockAt(test_x, test_y, test_z);
                            if((inLavaFlow && (tb == LAVA_ZN || tb == LAVA_XPZN || tb == LAVA_XNZN)) ||
                                (!inLavaFlow && (tb == WATER_ZN || tb == WATER_XPZN || tb == WATER_XNZN)))
                                value -= 0.3f;
                        }else if(curr == LAVA_ZP || curr == WATER_ZP){
                            test_x = x + adjacentFaces[5].dirVec[0];
                            test_y = y + adjacentFaces[5].dirVec[1];
                            test_z = z + adjacentFaces[5].dirVec[2];
                            tb = getBlockAt(test_x, test_y, test_z);
                            if((inLavaFlow && (tb == LAVA_ZP || tb == LAVA_XPZP || tb == LAVA_XNZP)) ||
                                (!inLavaFlow && (tb == WATER_ZP || tb == WATER_XPZP || tb == WATER_XNZP)))
                                value -= 0.3f;
                        }else if(curr == LAVA_XPZP || curr == WATER_XPZP){
                            test_x = x - 1;
                            test_y = y;
                            test_z = z - 1;
                            tb = getBlockAt(test_x, test_y, test_z);
                            if((inLavaFlow && tb == LAVA_XPZP) ||
                                (!inLavaFlow && tb == WATER_XPZP))
                                value -= 0.3f;
                        }else if(curr == LAVA_XPZN || curr == WATER_XPZN){
                            test_x = x - 1;
                            test_y = y;
                            test_z = z + 1;
                            tb = getBlockAt(test_x, test_y, test_z);
                            if((inLavaFlow && tb == LAVA_XPZN) ||
                                (!inLavaFlow && tb == WATER_XPZN))
                                value -= 0.3f;
                        }else if(curr == LAVA_XNZP || curr == WATER_XNZP){
                            test_x = x + 1;
                            test_y = y;
                            test_z = z - 1;
                            tb = getBlockAt(test_x, test_y, test_z);
                            if((inLavaFlow && tb == LAVA_XNZP) ||
                                (!inLavaFlow && tb == WATER_XNZP))
                                value -= 0.3f;
                        }else { // curr == LAVA_XNZN || curr == WATER_XNZN
                            test_x = x + 1;
                            test_y = y;
                            test_z = z + 1;
                            tb = getBlockAt(test_x, test_y, test_z);
                            if((inLavaFlow && tb == LAVA_XNZN) ||
                                (!inLavaFlow && tb == WATER_XNZN))
                                value -= 0.3f;
                        }

                        for (auto &f : adjacentFaces) {
                            int faceVerts = 0;                            
                            int posX = x + f.dirVec[0];
                            int posY = y + f.dirVec[1];
                            int posZ = z + f.dirVec[2];
                            BlockType adjacent = getBlockAt(posX, posY, posZ);
                            if(adjacent == EMPTY
                                    || (inLavaFlow && lava_fluid.find(adjacent) != lava_fluid.end())
                                    || (!inLavaFlow && water_fluid.find(adjacent) != water_fluid.end()))

                            {
                                if(checkFluidAdjSkip(curr, f.dir, adjacent))
                                    continue;

                                for (auto &v : f.vertices) {
                                    glm::vec4 height_offset(0);
                                    glm::vec4 general_deduct(0);

                                    if(curr == LAVA_XNZP && adjacent == LAVA_ZP && f.dir == ZNEG && v.pos.y == 0){
                                        height_offset.y += 0.7f;
                                    }

                                    if((checkFluidPos(curr, v.pos.x, v.pos.z) && v.pos.y == 1)){
                                        height_offset.y = -0.3f;
                                    }
                                    if(std::abs(v.pos.y - 1) < 0.001f){
                                        general_deduct.y = value;
                                    }

                                    m_Transbuffer.push_back(glm::vec4(x, y, z, 0) + v.pos + glm::vec4(m_pos, 0) + height_offset + general_deduct); // position
                                    m_Transbuffer.push_back(glm::vec4(f.dirVec, 1)); // normal
                                    if(f.dir == YNEG || f.dir == YPOS){
                                        m_Transbuffer.push_back(fluidColorBufferVals.at(curr)); // color depricated, replaced by texture uv
                                    }else{
                                        m_Transbuffer.push_back(glm::vec4(0,-1,0,0));
                                    }

                                    glm::vec4 offset = getUVOffset(f.dir, inLavaFlow ? LAVA : WATER);
                                    if(f.dir != YPOS || curr == LAVA_ZN || curr == LAVA_ZP
                                            || curr == WATER_ZN || curr == WATER_ZP){
                                        if(v.uv.x == 0 && v.uv.y == 0){
                                            offset.y += BLK_UV;
                                        }else if(v.uv.x == 0 && v.uv.y > 0){
                                            offset.x += BLK_UV;
                                        }else if(v.uv.x > 0 && v.uv.y > 0){
                                            offset.y -= BLK_UV;
                                        }else if(v.uv.x > 0 && v.uv.y == 0){
                                            offset.x -= BLK_UV;
                                        }
                                    }
                                    else if( curr == LAVA_XPZN || curr == WATER_XPZN){
                                         offset.y -= unit_len;
                                    }else if(curr == LAVA_XNZP || curr == WATER_XNZP){
                                        offset.x += unit_len;
                                    }else if(curr == LAVA_XNZN || curr == WATER_XNZN){
                                        offset.x += unit_len;
                                        offset.y -= unit_len;
                                    }
                                    offset.x = offset.x + v.uv.x;
                                    offset.y = offset.y + v.uv.y;
                                    m_Transbuffer.push_back(offset);
                                    faceVerts++;
                                }
                            }
                            for (int i = 0; i < faceVerts - 2; i++) {
                                m_Transidx.push_back(totalTransVerts);
                                m_Transidx.push_back(i+1+totalTransVerts);
                                m_Transidx.push_back(i+2+totalTransVerts);
                            }
                            totalTransVerts += faceVerts;
                        }
                    }


                    //2. Lava, Water, Ice
                    else if(curr == LAVA || curr == WATER || curr == ICE){
                        for (auto &f : adjacentFaces) {
                            int faceVerts = 0;
                            int posX = x + f.dirVec[0];
                            int posY = y + f.dirVec[1];
                            int posZ = z + f.dirVec[2];
                            BlockType adjacent = getBlockAt(posX, posY, posZ);
                            if(adjacent == EMPTY
                                    || (curr == LAVA && lava_fluid.find(adjacent) != lava_fluid.end())
                                    || (curr == WATER && water_fluid.find(adjacent) != water_fluid.end())
                                ){

                                if((lava_fluid.find(adjacent) != lava_fluid.end() && inLavaFlow) ||
                                    (water_fluid.find(adjacent) != water_fluid.end() && !inLavaFlow)){
                                    if(f.dir == YPOS || f.dir == YNEG || checkFluidAdjSkip(curr, f.dir, adjacent)){
                                        continue;
                                    }
                                }

                                for (auto &v : f.vertices) {
                                    m_Transbuffer.push_back(glm::vec4(x, y, z, 0) + v.pos + glm::vec4(m_pos, 0)); // position
                                    m_Transbuffer.push_back(glm::vec4(f.dirVec, 1)); // normal
                                    m_Transbuffer.push_back(glm::vec4(0)); // color depricated, replaced by texture uv

                                    glm::vec4 offset = getUVOffset(f.dir, curr);
                                    offset.x = offset.x + v.uv.x;
                                    offset.y = offset.y + v.uv.y;
                                    if(curr == LAVA || curr == WATER){
                                        if(v.uv.x == 0 && v.uv.y == 0){
                                            offset.y += BLK_UV;
                                        }else if(v.uv.x == 0 && v.uv.y > 0){
                                            offset.x += BLK_UV;
                                        }else if(v.uv.x > 0 && v.uv.y > 0){
                                            offset.y -= BLK_UV;
                                        }else if(v.uv.x > 0 && v.uv.y == 0){
                                            offset.x -= BLK_UV;
                                        }
                                        if(f.dir != YPOS){
                                            m_Transbuffer.back().y = -1;
                                        }
                                    }
                                    m_Transbuffer.push_back(offset);
                                    faceVerts++;
                                }
                            }
                            for (int i = 0; i < faceVerts - 2; i++) {
                                m_Transidx.push_back(totalTransVerts);
                                m_Transidx.push_back(i+1+totalTransVerts);
                                m_Transidx.push_back(i+2+totalTransVerts);
                            }
                            totalTransVerts += faceVerts;
                        }
                    }


                    // 3. small asset, with large transparent area, only 2 faces
                    else if (smallAsset){
                        for (auto &f : adjacentFaces) {
                            int faceVerts = 0;
//                            int posX = x + f.dirVec[0];
//                            int posY = y + f.dirVec[1];
//                            int posZ = z + f.dirVec[2];
                            // BlockType adjacent = getBlockAt(posX, posY, posZ);

                            if(f.dir != YPOS || f.dir == YNEG){
                                for (auto &v : f.vertices) {
                                    glm::vec4 delPos = v.pos;
                                    if(f.dir == XPOS || f.dir == XNEG) {
                                        delPos.x = 0.5;
                                    }
                                    else{
                                        delPos.z = 0.5;
                                    }
                                    m_Transbuffer.push_back(glm::vec4(x, y, z, 0) + delPos + glm::vec4(m_pos, 0)); // position
                                    m_Transbuffer.push_back(glm::vec4(f.dirVec, 1)); // normal
                                    // m_Transbuffer.push_back(glm::vec4(blockColors.at(curr))); // color depricated, replaced by texture uv
                                    m_Transbuffer.push_back(glm::vec4(0));
                                    glm::vec4 offset = getUVOffset(f.dir, curr);

                                    offset.x = offset.x + v.uv.x;
                                    offset.y = offset.y + v.uv.y;
                                    m_Transbuffer.push_back(offset);
                                    faceVerts++;
                                }

                            }
                            for (int i = 0; i < faceVerts - 2; i++) {
                                m_Transidx.push_back(totalTransVerts);
                                m_Transidx.push_back(i+1+totalTransVerts);
                                m_Transidx.push_back(i+2+totalTransVerts);
                            }
                            totalTransVerts += faceVerts;
                        }
                    }


                    // 4. normal blocks
                    else if(curr != EMPTY){
                        if(!isOpaque(curr)){
                            std::cout << "ERROR: Transparent block in opaque render pass!!" << std::endl;
                        }
                        for (auto &f : adjacentFaces) {
                            int faceVerts = 0;
                            int posX = x + f.dirVec[0];
                            int posY = y + f.dirVec[1];
                            int posZ = z + f.dirVec[2];
                            BlockType adjacent = getBlockAt(posX, posY, posZ);
                            if (transparentBlocks.find(adjacent) != transparentBlocks.end()) {
                                for (auto &v : f.vertices) {
                                    m_buffer.push_back(glm::vec4(x, y, z, 0) + v.pos + glm::vec4(m_pos, 0)); // position
                                    m_buffer.push_back(glm::vec4(f.dirVec, 1)); // normal
                                    m_buffer.push_back(glm::vec4(0)); // color depricated, replaced by texture uv

                                    glm::vec4 offset = getUVOffset(f.dir, curr);

                                    offset.x = offset.x + v.uv.x;
                                    offset.y = offset.y + v.uv.y;
                                    //m_buffer.push_back(offset + v.uv);
                                    m_buffer.push_back(offset);
                                    faceVerts++;
                                }
                            }

                            for (int i = 0; i < faceVerts - 2; i++) {
                                m_idx.push_back(totalVerts);
                                m_idx.push_back(i+1+totalVerts);
                                m_idx.push_back(i+2+totalVerts);
                            }
                            totalVerts += faceVerts;
                        }
                    }
                }
            }
        }
        m_vboCreated = true;

        // Jack Comment:
        // This function is running on child-thread,
        // when communicating with OpenGLContext, need switch back to main thread.
        // So we divide those parts into bufferAllData(), and call it only in mainthread.
        // Details could be found in Terrain::ExpandDraw().
    }
}

// Does bounds checking with at()
BlockType Chunk::getBlockAt(unsigned int x, unsigned int y, unsigned int z) const {
    return m_blocks.at(x + 16 * y + 16 * 256 * z);
}

// Exists to get rid of compiler warnings about int -> unsigned int implicit conversion
BlockType Chunk::getBlockAt(int x, int y, int z) const {
    if (x > 15) {
        return m_neighbors.at(XPOS) == nullptr? EMPTY : m_neighbors.at(XPOS)->getBlockAt(0, y, z);
    }
    if (x < 0) {
        return m_neighbors.at(XNEG) == nullptr? EMPTY : m_neighbors.at(XNEG)->getBlockAt(15, y, z);
    }
    if (z > 15) {
        return m_neighbors.at(ZPOS) == nullptr? EMPTY : m_neighbors.at(ZPOS)->getBlockAt(x, y, 0);
    }
    if (z < 0) {
        return m_neighbors.at(ZNEG) == nullptr? EMPTY : m_neighbors.at(ZNEG)->getBlockAt(x, y, 15);
    }
    if (y < 0 || y > 255) {
        return EMPTY;
    }

    return getBlockAt(static_cast<unsigned int>(x), static_cast<unsigned int>(y), static_cast<unsigned int>(z));
}

// Does bounds checking with at()
void Chunk::setBlockAt(unsigned int x, unsigned int y, unsigned int z, BlockType t) {
    m_blocks.at(x + 16 * y + 16 * 256 * z) = t;
}


const static std::unordered_map<Direction, Direction, EnumHash> oppositeDirection {
    {XPOS, XNEG},
    {XNEG, XPOS},
    {YPOS, YNEG},
    {YNEG, YPOS},
    {ZPOS, ZNEG},
    {ZNEG, ZPOS}
};

glm::vec4 Chunk::getUVOffset(enum Direction dir, enum BlockType curr) {
    glm::vec4 offset(0.f);

    float unit_len = BLK_UV;
    //if block is a grass on

    switch(curr)
    {
    case GRASS:
        if(dir == YPOS){
            offset.x = 8 * unit_len;
            offset.y = 13 * unit_len;
        }else if(dir == YNEG){
            offset.x = 2 * unit_len;
            offset.y = 15 * unit_len;
        }else{
            offset.x = 3 * unit_len;
            offset.y = 15 * unit_len;
        }
        break;
    case DIRT:
        offset.x = 2 * unit_len;
        offset.y = 15 * unit_len;
        break;
    case STONE: //if block is a stone
        offset.x = unit_len;
        offset.y = 15 * unit_len;
        break;
    case SNOW://if block is a snow
        if(dir == YPOS){
            offset.x = 2 * unit_len;
            offset.y = 11 * unit_len;
        }else if(dir == YNEG){
            offset.x = 2 * unit_len;
            offset.y = 15 * unit_len;
        }else{
            offset.x = 4 * unit_len;
            offset.y = 11 * unit_len;
        }
        break;
    //if block is sand
    case SAND:
        offset.x = 2 * unit_len;
        offset.y = 14 * unit_len;
        break;
    case BEDROCK:// block is bedrock
        offset.x = unit_len;
        offset.y = 14 * unit_len;
        break;
    case CAVE:// block is cave
        offset.x = 8 * unit_len;
        offset.y = 9 * unit_len;
        break;
    //if block is a lava
    case LAVA:
        offset.x = 14 * unit_len;
        offset.y = unit_len;
        break;
    case WATER://block is water
        offset.x = 14 * unit_len;
        offset.y = 3 * unit_len;
        break;
    case ICE:
        offset.x = 3 * unit_len;
        offset.y = 11 * unit_len;
        break;
    case LEAF:
        offset.x = 5 * unit_len;
        offset.y = 12 * unit_len;
        break;
    case WOOD:
        if(dir == YPOS || dir == YNEG)
        {
            offset.x = 5 * unit_len;
            offset.y = 14 * unit_len;
        }
        else{
            offset.x = 4 * unit_len;
            offset.y = 14 * unit_len;
        }
        break;
    case CACTUS:
        if(dir == YPOS || dir == YNEG)
        {
            offset.x = 5 * unit_len;
            offset.y = 11 * unit_len;
        }
        else{
            offset.x = 6 * unit_len;
            offset.y = 11 * unit_len;
        }
        break;
    case PURE_SNOW:
        offset.x = 2 * unit_len;
        offset.y = 11 * unit_len;
        break;
    case PURE_GRASS:
        offset.x = 7 * unit_len;
        offset.y = 13 * unit_len;
        break;
    case FLOWER_1:
        offset.x = 12 * unit_len;
        offset.y = 15 * unit_len;
        break;
    case FLOWER_2:
        offset.x = 13 * unit_len;
        offset.y = 15 * unit_len;
        break;
    case MUSHROOM_1:
        offset.x = 12 * unit_len;
        offset.y = 14 * unit_len;
        break;
    case MUSHROOM_2:
        offset.x = 13 * unit_len;
        offset.y = 14 * unit_len;
        break;
    default:
        break;
    }

    return offset;
}

void Chunk::linkNeighbor(uPtr<Chunk> &neighbor, Direction dir) {
    if(neighbor != nullptr) {
        this->m_neighbors[dir] = neighbor.get();
        neighbor->m_neighbors[oppositeDirection.at(dir)] = this;
    }
}

void Chunk::setPos(glm::vec3 pos) {
    m_pos = pos;
}

glm::vec3 Chunk::getPos() {
    return m_pos;
}

void Chunk::resetVBOCreated() {
    m_vboCreated = false;
}

void Chunk::bufferAlldata()
{   
    bufferVBOdata(m_buffer, m_idx);
    bufferVBOTransdata(m_Transbuffer, m_Transidx);
}
