#include "noise.h"
#include <iostream>
#include <vector>

Noise::Noise()
{}

float Noise::lerp(float x1, float x2, float r)
{
    return x1 * r + x2 * (1.f - r);
}

float Noise::smoothStep(float x)
{
    return x * x * (3.f - 2.f * x);
}

float Noise::smoothStep(float edge0, float edge1, float x)
{
    x = (x - edge0) / (edge1 - edge0);
    x = std::min(std::max(0.f, x), 1.f);
    return smoothStep(x);
}

glm::vec2 Noise::random2(glm::vec2 p)
{
    glm::vec2 vec(glm::dot(p, glm::vec2(127.1f, 311.7f)),
                glm::dot(p, glm::vec2(269.5f, 183.3f)));
    vec = glm::vec2(std::sin(vec[0]), std::sin(vec[1])) * random2Seed;
    // return glm::fract(vec);
    return glm::vec2(std::modf(vec[0], nullptr), std::modf(vec[1], nullptr));
}

glm::vec2 Noise::random2Worley(glm::vec2 p)
{
    glm::vec2 vec(glm::dot(p, glm::vec2(127.1f, 311.7f)),
                glm::dot(p, glm::vec2(269.5f, 183.3f)));
    vec = glm::vec2(std::sin(vec[0]), std::sin(vec[1])) * 43758.5453f;
    return glm::fract(vec);
    // return glm::vec2(std::modf(vec[0], nullptr), std::modf(vec[1], nullptr));
}

glm::vec3 Noise::random3(glm::vec3 p)
{
    glm::vec3 vec((glm::dot(p, glm::vec3(127.1f, 311.7f, 191.999f))));
    return glm::fract(glm::sin(vec) * 43758.5453f);
}

float Noise::surflet2D(glm::vec2 P, glm::vec2 gridPoint)
{
    // Compute falloff function by converting linear distance to a polynomial
    float distX = std::abs(P.x - gridPoint.x);
    float distY = std::abs(P.y - gridPoint.y);
    float tX = 1 - 6 * std::pow(distX, 5.f) + 15 * std::pow(distX, 4.f) - 10 * std::pow(distX, 3.f);
    float tY = 1 - 6 * std::pow(distY, 5.f) + 15 * std::pow(distY, 4.f) - 10 * std::pow(distY, 3.f);
    // Get the random vector for the grid point
    glm::vec2 gradient = 2.f * random2(gridPoint) - glm::vec2(1.f);
    // Get the vector from the grid point to P
    glm::vec2 diff = P - gridPoint;
    // Get the value of our height field by dotting grid->P with our gradient
    float height = glm::dot(diff, gradient);
    // Scale our height field (i.e. reduce it) by our polynomial falloff function
    return height * tX * tY;
}

float Noise::perlinNoise2D(glm::vec2 uv)
{
    float surfletSum = 0.f;
    // Iterate over the four integer corners surrounding uv
    for(int dx = 0; dx <= 1; ++dx) {
        for(int dy = 0; dy <= 1; ++dy) {
        surfletSum += surflet2D(uv, glm::vec2(std::floor(uv[0]) + dx, std::floor(uv[1]) + dy));
        }
    }
    return surfletSum;
}

float Noise::expPerlinNoise2D(glm::vec2 uv, float exp, bool smooth)
{
    float noise = std::abs(perlinNoise2D(uv));
    if(smooth)
    {
        noise = smoothStep(noise);
    }
    return std::pow(noise, exp);
}

float Noise::surflet3D(glm::vec3 p, glm::vec3 gridPoint) {
    // Compute the distance between p and the grid point along each axis, and warp it with a
    // quintic function so we can smooth our cells
    glm::vec3 t2 = glm::abs(p - gridPoint);
    glm::vec3 v1 = 6.f * glm::vec3(glm::pow(t2[0], 5.f),
                                    glm::pow(t2[1], 5.f),
                                    glm::pow(t2[2], 5.f));
    glm::vec3 v2 = 15.f * glm::vec3(glm::pow(t2[0], 4.f),
                                    glm::pow(t2[1], 4.f),
                                    glm::pow(t2[2], 4.f));
    glm::vec3 v3 = 10.f * glm::vec3(glm::pow(t2[0], 3.f),
                                    glm::pow(t2[1], 3.f),
                                    glm::pow(t2[2], 3.f));
    glm::vec3 t = glm::vec3(1.f) - v1 + v2 - v3;
    // Get the random vector for the grid point (assume we wrote a function random2
    // that returns a vec2 in the range [0, 1])
    glm::vec3 gradient = glm::normalize(random3(gridPoint) * 2.f - glm::vec3(1.f));
    // Get the vector from the grid point to P
    glm::vec3 diff = p - gridPoint;
    // Get the value of our height field by dotting grid->P with our gradient
    float height = glm::dot(diff, gradient);
    // Scale our height field (i.e. reduce it) by our polynomial falloff function
    return height * t.x * t.y * t.z;
}

float Noise::perlinNoise3D(glm::vec3 p) {
    float surfletSum = 0.f;
    // Iterate over the eight integer corners surrounding a 3D grid cell
    for(int dx = 0; dx <= 1; ++dx) {
        for(int dy = 0; dy <= 1; ++dy) {
            for(int dz = 0; dz <= 1; ++dz) {
                surfletSum += surflet3D(p, glm::floor(p) + glm::vec3(dx, dy, dz));
            }
        }
    }
    return surfletSum;
}

// TODO: check if this is correct or necessary to use pow()
float Noise::expPerlinNoise3D(glm::vec3 p, float exp, bool smooth) {
    float noise = glm::fract(std::abs(perlinNoise3D(p)));
    if(smooth)
    {
        noise = smoothStep(noise);
    }
    return std::pow(noise, exp);
}

float Noise::noise2D(glm::vec2 p)
{
    float s = std::sin(p.x * 127.1f + p.y * 269.5f) * 43758.5453f;
    return s - std::floor(s);
    // return glm::fract(s * 43758.5453);
}

float Noise::interpNoise2D(float x, float y)
{
    int intX = int(std::floor(x));
    float fractX = x - std::floor(x);
    int intY = int(std::floor(y));
    float fractY = y - std::floor(y);
    float v1 = noise2D(glm::vec2(intX, intY));
    float v2 = noise2D(glm::vec2(intX + 1, intY));
    float v3 = noise2D(glm::vec2(intX, intY + 1));
    float v4 = noise2D(glm::vec2(intX + 1, intY + 1));

    float i1 = v1 * (1.f - fractX) + v2 * fractX;
    float i2 = v3 * (1.f - fractX) + v4 * fractX;
    return i1 * (1.f - fractY) + i2 * fractY;
}

float Noise::fbm(glm::vec2 v)
{
    float x = v[0] / fbmSize;
    float y = v[1] / fbmSize;
    float total = 0.f;
    float persistence = 0.5f;
    int octaves = 5;
    float freq = 2.f;
    float amp = 0.5f; // 1.f / 1.414f;
    // float max = 0.f;
    for(int i = 1; i <= octaves; i++) {
        total += interpNoise2D(x * freq, y * freq) * amp;
        freq *= 2.f;
        // max += amp;
        amp *= persistence;
    }
    // float t = total * total / max;
    // return smoothStep(total);
    return total;
}

float Noise::voronoiWorleyNoise(glm::vec2 uv)
{
    // vec2 targetPoint = uv;
    uv /= worleySize;
    glm::vec2 uvInt = glm::floor(uv); //glm::vec2(std::floor(uv[0]), std::floor(uv[1]));
    glm::vec2 uvFract = uv - uvInt;
    float minDist_1 = 1.0; // Minimum distance initialized to max.
    float minDist_2 = 1.0; // second minimum distance
    for(int y = -1; y <= 1; ++y) {
        for(int x = -1; x <= 1; ++x) {
            glm::vec2 neighbor = glm::vec2(float(x), float(y)); // Direction in which neighbor cell lies
            glm::vec2 point = random2Worley(uvInt + neighbor); // Get the Voronoi centerpoint for the neighboring cell
            glm::vec2 diff = neighbor + point - uvFract; // Distance between fragment coord and neighbor’s Voronoi point
            float dist = glm::length(diff);
            // minDist = min(minDist, dist);
            if(minDist_1 > dist)
            {
                minDist_2 = minDist_1;
                minDist_1 = dist;
                // targetPoint = uvInt + neighbor + point;
            }
            else if(minDist_2 > dist)
            {
                minDist_2 = dist;
            }
        }
    }
    return minDist_2 - minDist_1;
}

float Noise::fbmWorleyNoise(glm::vec2 p)
{
    float w_noise = voronoiWorleyNoise(p);
    // noise = smoothstep(0.f, 1.f, noise);
    float f_noise = fbm(p);
    return fbmRatio * f_noise + (1.f - fbmRatio) * w_noise;
    // return f_noise * fbmRatio;
}

float Noise::fbmWorleyNoisePattern(glm::vec2 p)
{
    float scale = 0.125f;
    glm::vec2 q = glm::vec2(fbm(p),  fbm(p + glm::vec2(5.3f, 3.8f)));
    return fbmWorleyNoise(p + q * scale);
}

float Noise::thermalNoise(glm::vec2 p)
{
    float max_noise = 0.f;
    for(int i = -1; i < 1; i++)
    {
        for(int j = -1; j < 1; j++)
        {
            max_noise = std::max(fbmWorleyNoisePattern(p + glm::vec2(i, j)), max_noise);
        }
    }
    return max_noise;
}

void Noise::setFbmWorleyConfig(float fbm_ratio, float worley_size, float fbm_size)
{
    fbmRatio = fbm_ratio;
    worleySize = worley_size;
    fbmSize = fbm_size;
}

void Noise::riverBuffer()
{
    glm::vec2 p = glm::vec2(-100, -100);
    routes[toRiverKey(p.x, p.y, 64.f)].push_back({p, glm::normalize(glm::vec2(1.1, 1))});
    routes[toRiverKey(p.x, p.y, 64.f)].push_back({p, glm::normalize(glm::vec2(-1.1, -1))});

    for(int iter = 0; iter < 4; iter++)
    {
        std::unordered_map<int64_t, std::vector<std::pair<glm::vec2, glm::vec2>>> newRoutes;

        for(auto it = routes.begin(); it != routes.end(); it++)
        {
            for(auto& route : it->second)
            {
                computeRiverRoute(route, newRoutes);
            }
        }
        routes.clear();
        routes = newRoutes;
    }
}

void Noise::updateRiverBuffer(int x, int z)
{
    int64_t key = toRiverKey(x, z, 64.f);
    while(!routes[key].empty())
    {
        auto route = routes[key][0];
        routes[key].erase(routes[key].begin());
        computeRiverRoute(route, routes);
    }
}

void Noise::computeRiverRoute(std::pair<glm::vec2, glm::vec2>& route, std::unordered_map<int64_t, std::vector<std::pair<glm::vec2, glm::vec2>>>& newRoutes)
{
    glm::vec2 pos = route.first;
    glm::vec2 dir = route.second;
    glm::vec2 endPos;

    if(!moveRiver(pos, endPos, dir, -1.f))
        return;
    pos = endPos;
    if(!moveRiver(pos, endPos, dir, 1.f))
        return;
    pos = endPos;
    if(!moveRiver(pos, endPos, dir, -1.f))
        return;
    pos = endPos;

    glm::vec2 savePos = pos;
    glm::vec2 saveDir = dir;
    if(moveRiver(pos, endPos, dir, -2.f, 1.5f))
    {
        pos = endPos;
        if(moveRiver(pos, endPos, dir, -1.f, 1.5f))
        {
            pos = endPos;
            if(moveRiver(pos, endPos, dir, 1.f, 1.5f))
            {
                // save right branch
                int64_t key = toRiverKey(endPos.x, endPos.y, 64.f);
                newRoutes[key].push_back({endPos, dir});
            }
        }
    }



    pos = savePos;
    dir = saveDir;
    if(moveRiver(pos, endPos, dir, 2.f, 1.5f))
    {
        pos = endPos;
        if(moveRiver(pos, endPos, dir, -1.f, 1.5f))
        {
            pos = endPos;
            if(moveRiver(pos, endPos, dir, 1.f, 1.5f))
            {
                // save left branch
                int64_t key = toRiverKey(endPos.x, endPos.y, 64.f);
                newRoutes[key].push_back({endPos, dir});
            }
        }
    }
}

bool Noise::moveRiver(glm::vec2& pos, glm::vec2& endPos, glm::vec2& dir, float dr, float lr)
{
    dir = rotate2d(dir, dr * riverAngle);
    endPos = pos + dir * riverLen * lr;
    //lines.push_back({pos, endPos});

    int64_t key1 = toRiverKey(pos.x, pos.y, riverMapGridSize);
    int64_t key2 = toRiverKey(endPos.x, endPos.y, riverMapGridSize);
    if(lineMap[key1].size() >= riverRouteLimit || lineMap[key2].size()>= riverRouteLimit)
    {
        return false;
    }
    lineMap[key1].push_back({pos, endPos});
    lineMap[key2].push_back({endPos, pos});
    return true;
}

// given height between 129-255
// water would fill [height+1, 138]
int Noise::getRiverHeight(int height, int x, int z)
{
    float dis0 = riverEffectRange + 1.f;
    // float dis1 = maxDis + 1.f;
    std::pair<glm::vec2, glm::vec2> closeLine;
    for(int xi = x - riverMapGridSize; xi <= x + riverMapGridSize; xi += riverMapGridSize)
    {
        for(int zi = z - riverMapGridSize; zi <= z + riverMapGridSize; zi += riverMapGridSize)
        {
            for(auto line : lineMap[toRiverKey(xi, zi, riverMapGridSize)])
            {
                float t = getSegmentDis(x, z, line);
                if(t < dis0)
                {
                    // dis1 = dis0;
                    dis0 = t;
                }
            }
        }
    }

    if(dis0 > riverEffectRange)
    {
        return height;
    }

    float r = dis0 / riverEffectRange;

    int targetHeight = 129 + (int)(r * 9.f);

    return lerp(height, targetHeight, r*r);
    // return targetHeight;
}

glm::vec2 Noise::rotate2d(glm::vec2 dir, float angle)
{
    float c = std::cos(angle / 180.f * 3.14159f);
    float s = std::sin(angle / 180.f * 3.14159f);
    return glm::normalize(glm::vec2(c * dir.x - s * dir.y, s * dir.x + c * dir.y));
}

float Noise::getSegmentDis(int x, int z, const std::pair<glm::vec2, glm::vec2>& line)
{
    glm::vec2 p = glm::vec2(x, z);
    glm::vec2 ap = p - line.first;
    glm::vec2 ab = line.second - line.first;
    float c = glm::dot(glm::normalize(ap), glm::normalize(ab));
    if(std::isnan(c))
    {
        c = 0.f;
    }
    glm::vec2 d = line.first + glm::normalize(ab) * glm::length(ap) * c;

    if((std::abs(line.first.x - d.x) + std::abs(line.second.x - d.x) - std::abs(line.first.x - line.second.x) > 3.f)
        || (std::abs(line.first.y - d.y) + std::abs(line.second.y - d.y) - std::abs(line.first.y - line.second.y) > 3.f))
    {
        float lpa = glm::length(p - line.first);
        float lpb = glm::length(p - line.second);
        return std::min(lpa, lpb);
    }
    else
    {
        return glm::length(p - d);
    }
}

float Noise::getDis(int x, int z, const std::pair<glm::vec2, glm::vec2>& line)
{
//    float b = (line.first.x - line.second.x) / (line.first.y * line.second.x - line.second.y * line.first.x);
//    float a = (- b * line.first.y - 1.f) / line.first.x;
//    float c = 1;

    // 0 = ax - y + c;
    float a = (line.first.y - line.second.y) / (line.first.x - line.second.x);
    if(!std::isfinite(a))
    {
        a = 0;
    }
    float b = -1.f;
    float c = line.first.y - a * line.first.x;

    float dis = std::abs(x * a + b * z + c) / std::sqrt(a * a + b * b);
    return dis;
}

bool Noise::checkLine(int x, int z, const std::pair<glm::vec2, glm::vec2>& line)
{
    if(std::abs(line.first.x - x) + std::abs(line.second.x - x) - std::abs(line.first.x - line.second.x) > riverEffectRange * 4.f)
    {
        return false;
    }

    if(std::abs(line.first.y - z) + std::abs(line.second.y - z) - std::abs(line.first.y - line.second.y) > riverEffectRange * 4.f)
    {
        return false;
    }

    return true;
}

int64_t Noise::toRiverKey(float x, float z, float size) {
    int64_t xz = 0xffffffffffffffff;
    int64_t x64 = (int)(x / size);
    int64_t z64 = (int)(z / size);

    // Set all lower 32 bits to 1 so we can & with Z later
    xz = (xz & (x64 << 32)) | 0x00000000ffffffff;

    // Set all upper 32 bits to 1 so we can & with XZ
    z64 = z64 | 0xffffffff00000000;

    // Combine
    xz = xz & z64;
    return xz;
}

void Noise::changeRandom2Seed(float newSeed)
{
    random2Seed = newSeed;
}


void Noise::treeBuffer()
{
    maxKey = tree_w * tree_l * tree_h;
    trees = std::vector<BlockType>(maxKey, EMPTY);

    for(int k = 0; k <= 8; k++){
        int s =
            (k <= 0) ? 4 :
                (k <= 2) ? 3 :
                    (k <= 5) ? 2 : 1;
        for(int i = -s; i <= s; i++){
            for(int j = -s; j <= s; j++){
                trees[treeKey(10 + i, k, 10 + j)] = WOOD;
            }
        }
    }

    std::vector<std::pair<unsigned int, glm::vec3>> pts;
    pts.push_back({treeKey(11, 7, 11), glm::normalize(glm::vec3(1.2, 2.0, 0.8))});
    pts.push_back({treeKey(9, 7, 9), glm::normalize(glm::vec3(-0.8, 2.0, -1.2))});
    pts.push_back({treeKey(11, 7, 9), glm::normalize(glm::vec3(1.2, 2.0, -0.8))});
    pts.push_back({treeKey(9, 7, 11), glm::normalize(glm::vec3(-0.8, 2.0, 1.2))});

    int count = 0;
    int tree_len = 10;
    for(int iter = 0; iter < 3; iter++)
    {
        count = pts.size();
        while(count > 0)
        {
            count--;
            auto kv = pts.front();
            pts.erase(pts.begin());
            int val = kv.first;
            int x = val / tree_h * tree_l;
            val -= (x * tree_h * tree_l);
            int y = val / tree_l;
            int z = val - y * tree_l;
            glm::vec3 pos = glm::vec3(x, y, z);
            glm::vec3 dir = kv.second;
            bool end = false;
            for(float t = 0.f; t < tree_len / 2.f; t += 0.25f)
            {
                unsigned int key = treeKey(pos + dir * t);
                if(key >= maxKey){
                    std::cout << "exceed tree range" << std::endl;
                    end = true;
                    break;
                }
                if(trees[key] == EMPTY){
                    trees[key] = WOOD;
                }
            }
            if(end){continue;}
            pts.push_back({treeKey(pos + dir * (tree_len / 2.f)), glm::vec3(-dir.z, dir.y, -dir.x)});

            for(float t = tree_len / 2.f; t <= tree_len; t += 0.25f)
            {
                unsigned int key = treeKey(pos + dir * t);
                if(key >= maxKey){
                    end = true;
                    std::cout << "exceed tree range" << std::endl;
                    break;
                }
                if(trees[key] == EMPTY)
                {
                    trees[key] = WOOD;
                }
            }
            if(end){continue;}
            pts.push_back({treeKey(pos + dir * (float)tree_len), glm::vec3(dir.z, dir.y, -dir.x)});
        }
        tree_len = std::max((int)(0.7f * tree_len), 4);
    }
}


unsigned int Noise::treeKey(int x, int y, int z)
{
    return x * tree_h * tree_l + y * tree_l + z;
}

unsigned int Noise::treeKey(glm::vec3 pos)
{
    return treeKey(std::round(pos.x), std::round(pos.y), std::round(pos.z));
}

BlockType Noise::getTree(int x, int y, int z)
{
    unsigned int key = treeKey(x, y, z);
    if(key >= maxKey){
        std::cout << "error! out of tree zone!" << std::endl;
        return EMPTY;
    }
    return trees[key];
}
