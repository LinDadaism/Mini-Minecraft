#ifndef NOISE_H
#define NOISE_H

#include "scene/chunk.h"
#include <glm_includes.h>
#include <vector>
#include <unordered_map>

class Noise
{
private:
    float riverLen = 15.f;
    float riverAngle = 15.f;
    float riverEffectRange = 10.f;
    float riverMapGridSize = 25.f;
    int riverRouteLimit = 6;

    float fbmRatio = 0.67f;
    float worleySize = 64.f;
    float fbmSize = 64.f;
    float random2Seed = 43758.5453f;

    std::unordered_map<int64_t, std::vector<std::pair<glm::vec2, glm::vec2>>> routes;
    std::unordered_map<int64_t, std::vector<std::pair<glm::vec2, glm::vec2>>> lineMap;
    // std::vector<std::pair<glm::vec2, glm::vec2>> lines;

public:
    Noise();

    float lerp(float x1, float x2, float r);
    float smoothStep(float val);
    float smoothStep(float edge0, float edge1, float x);
    glm::vec2 random2(glm::vec2 p);
    glm::vec2 random2Worley(glm::vec2 p);
    glm::vec3 random3(glm::vec3 p);

    // Perlin noise
    float surflet2D(glm::vec2 P, glm::vec2 gridPoint);
    float perlinNoise2D(glm::vec2 uv);
    float expPerlinNoise2D(glm::vec2 uv, float exp, bool smooth);

    float surflet3D(glm::vec3 p, glm::vec3 gridPoint);
    float perlinNoise3D(glm::vec3 p);
    float expPerlinNoise3D(glm::vec3 p, float exp, bool smooth);

    // FBM
    float noise2D(glm::vec2 p);
    float interpNoise2D(float x, float y);
    float fbm(glm::vec2 v);

    // Worley noise
    float voronoiWorleyNoise(glm::vec2 uv);

    // Summed
    float fbmWorleyNoise(glm::vec2 p);
    float fbmWorleyNoisePattern(glm::vec2 p);
    float thermalNoise(glm::vec2 p);
    void setFbmWorleyConfig(float fbm_ratio, float worley_size, float fbm_size);

    // calculate new height, considering original height
    int getRiverHeight(int height, int x, int z);
    void riverBuffer();
    void updateRiverBuffer(int x, int z);
    void computeRiverRoute(std::pair<glm::vec2, glm::vec2>& route, std::unordered_map<int64_t, std::vector<std::pair<glm::vec2, glm::vec2>>>& newRoutes);
    bool moveRiver(glm::vec2& pos, glm::vec2& endPos, glm::vec2& dir, float dr, float lr = 1.f);

    int tree_h = 40;
    int tree_w = 21;
    int tree_l = 21;
    unsigned int maxKey;
    std::vector<BlockType> trees;
    void treeBuffer();
    unsigned int treeKey(int x, int y, int z);
    unsigned int treeKey(glm::vec3 pos);
    BlockType getTree(int x, int y, int z);

    glm::vec2 rotate2d(glm::vec2 dir, float angle);
    float getSegmentDis(int x, int z, const std::pair<glm::vec2, glm::vec2>& line);
    float getDis(int x, int z, const std::pair<glm::vec2, glm::vec2>& line);
    bool checkLine(int x, int z, const std::pair<glm::vec2, glm::vec2>& line);

    int64_t toRiverKey(float x, float z, float size);

    void changeRandom2Seed(float newSeed);    
};

#endif // NOISE_H
