#include "player.h"
#include <QString>
#include <iostream>
#include <cmath>
#include "playerhelpers.h"

Player::Player(glm::vec3 pos, Terrain &terrain)
    : Entity(pos), m_velocity(0,0,0), m_acceleration(0,0,0),
      m_camera(pos + glm::vec3(0, 1.5f, 0)), mcr_terrain(terrain),
      isgrounded(false), isInLiquid(false), jump(false), left_button(false), right_button(false),
      mcr_camera(m_camera), camInWater(false), camInLava(false)
{

    wind_sound.setSource(QUrl::fromLocalFile(":/sound/wind.wav"));
    wind_sound.setLoopCount(QSoundEffect::Infinite);
    wind_sound.setVolume(0.15f);
    walk_sound.setSource(QUrl::fromLocalFile(":/sound/walk.wav"));
    walk_sound.setLoopCount(QSoundEffect::Infinite);
    walk_sound.setVolume(0.6f);
    river_sound.setSource(QUrl::fromLocalFile(":/sound/river.wav"));
    river_sound.setLoopCount(QSoundEffect::Infinite);
    river_sound.setVolume(0.25f);
    bird_sound.setSource(QUrl::fromLocalFile(":/sound/bird.wav"));
    bird_sound.setLoopCount(QSoundEffect::Infinite);
    bird_sound.setVolume(0.15f);
    lava_sound.setSource(QUrl::fromLocalFile(":/sound/lava.wav"));
    lava_sound.setLoopCount(QSoundEffect::Infinite);
    lava_sound.setVolume(0.8f);


}

Player::~Player()
{}

bool ifCollide(BlockType blockType)
{
    return (blockType != EMPTY && blockType != WATER && blockType != LAVA
            && !Chunk::isSmallAsset(blockType));
}

void Player::tick(float dT, InputBundle &input) {
    processInputs(input);
    computePhysics(dT, mcr_terrain);

    // check if camera is in lava/water, added by linda
    BlockType t = mcr_terrain.getBlockAt(m_camera.mcr_position);
    camInLava = false;
    camInWater = false;
    if (t == LAVA) {
        camInLava = true;
    }
    if (t == WATER) {
        camInWater = true;
    }
}

void Player::processInputs(InputBundle &inputs) {
    // TODO: Update the Player's velocity and acceleration based on the
    // state of the inputs.

    //wind sound

//#ifdef PLAY_SOUND
    if(!wind_sound.isPlaying()){
        wind_sound.play();
    }
    if(!bird_sound.isPlaying()){
        bird_sound.play();
    }
//#endif


    //mouse button clicked
    //left mouse button clicked
    if(inputs.leftButton){
        left_button = true;
        inputs.leftButton = false;
    }
    if(inputs.rightButton){
        right_button = true;
        inputs.rightButton = false;
    }

    if(inputs.mouseX > 0.9 || inputs.mouseX < -0.9){
        this->rotateOnUpGlobal(inputs.mouseX);
        inputs.mouseX = 0;
    }

    if(inputs.mouseY > 0.5 || inputs.mouseY < -0.5){
        float rad = glm::radians(inputs.mouseY);
        glm::mat4 rot(glm::rotate(glm::mat4(), rad, m_right));
        glm::vec3 temp_up = glm::vec3(rot * glm::vec4(m_up, 0.f));
        while(temp_up.y < 0){
            if(inputs.mouseY > 0){
                inputs.mouseY -= 1;
            }else{
                inputs.mouseY += 1;
            }

            rad = glm::radians(inputs.mouseY);
            glm::mat4 rot(glm::rotate(glm::mat4(), rad, m_right));
            temp_up = glm::vec3(rot * glm::vec4(m_up, 0.f));
        }
        this->rotateOnRightLocal(inputs.mouseY);
        inputs.mouseY = 0;
    }

    //move forward and backward

    if(fly_mode){
        if(inputs.wPressed){
            if(this->m_velocity.z < 10){
                this->m_acceleration.z = 0.5;
            }else{
                this->m_acceleration.z = 0;
            }
            inputs.wPressed = false;
        }else if(inputs.sPressed){
            if(this->m_velocity.z > -10){
                this->m_acceleration.z = -0.5;
            }else{
                this->m_acceleration.z = 0;
            }
            inputs.sPressed = false;
        }else{
            if(this->m_velocity.z < -0.2){
                this->m_acceleration.z = 0.25;
            }else if(this->m_velocity.z > 0.2){
                this->m_acceleration.z = -0.25;
            }else{
                m_acceleration.z = 0;
                m_velocity.z = 0;
            }
        }
        //move right and left
        if(inputs.dPressed){
            if(this->m_velocity.x < 10){
                this->m_acceleration.x = 0.5;
            }else{
                this->m_acceleration.x = 0;
            }
            inputs.dPressed = false;
        }else if(inputs.aPressed){
            if(this->m_velocity.x > -10){
                this->m_acceleration.x = -0.5;
            }else{
                this->m_acceleration.x = 0;
            }
            inputs.aPressed = false;
        }else{
            if(this->m_velocity.x < -0.2){
                this->m_acceleration.x = 0.25;
            }else if(this->m_velocity.x > 0.2){
                this->m_acceleration.x = -0.25;
            }else{
                m_acceleration.x = 0;
                m_velocity.x = 0;
            }
        }

        //move up and down
        if(inputs.ePressed){
            if(this->m_velocity.y < 10){
                this->m_acceleration.y = 0.5;
            }else{
                this->m_acceleration.y = 0;
            }
            inputs.ePressed = false;
        }else if(inputs.qPressed){
            if(this->m_velocity.y > -10){
                this->m_acceleration.y = -0.5;
            }else{
                this->m_acceleration.y = 0;
            }
            inputs.qPressed = false;
        }else{
            if(this->m_velocity.y < -0.2){
                this->m_acceleration.y = 0.25;
            }else if(this->m_velocity.y > 0.2){
                this->m_acceleration.y = -0.25;
            }else{
                m_acceleration.y = 0;
                m_velocity.y = 0;
            }
        }
//        std::cout << "velocity in normal is " << m_velocity.x << " " << m_velocity.y << " " << m_velocity.z << std::endl;
    }

    else{
        // not in flymode
        glm::vec3 z_dir = glm::normalize(glm::vec3(m_forward.x, 0, m_forward.z));
        glm::vec3 x_dir = glm::normalize(glm::cross(z_dir, glm::vec3(0,1,0)));
        float vel = glm::length(glm::vec2(m_velocity.x, m_velocity.z));
        glm::vec3 vel_xz = glm::vec3(m_velocity.x, 0.f, m_velocity.z);
        //move forward
        if(inputs.wPressed){
            if(vel < walkMaxSpeed){
                this->m_acceleration = z_dir * accRatio;
                this->m_velocity.x = this->m_acceleration.x + z_dir.x * (vel * 0.95f);
                this->m_velocity.z = this->m_acceleration.z + z_dir.z * (vel * 0.95f);
            }else{
                this->m_acceleration = z_dir;
            }
            lastInputCount = 0;
            inputs.wPressed = false;
        }else if(inputs.sPressed){
            if(vel < walkMaxSpeed){

                this->m_acceleration = -z_dir * accRatio;
                this->m_velocity.x = this->m_acceleration.x - z_dir.x * (vel * 0.95f);
                this->m_velocity.z = this->m_acceleration.z - z_dir.z * (vel * 0.95f);
            }
            lastInputCount = 0;
            inputs.sPressed = false;
        }else if(inputs.dPressed){
            if(vel < walkMaxSpeed){
                this->m_acceleration = x_dir * accRatio;
                this->m_velocity.x = this->m_acceleration.x + x_dir.x * (vel * 0.95f);
                this->m_velocity.z = this->m_acceleration.z + x_dir.z * (vel * 0.95f);
            }
            lastInputCount = 0;
            inputs.dPressed = false;
        }else if(inputs.aPressed){
            if(vel < walkMaxSpeed){
                this->m_acceleration = x_dir * -accRatio;
                this->m_velocity.x = this->m_acceleration.x - x_dir.x * (vel * 0.95f);
                this->m_velocity.z = this->m_acceleration.z - x_dir.z * (vel * 0.95f);
            }
            lastInputCount = 0;
            inputs.aPressed = false;
        }else if(inputs.spacePressed){
            if(isgrounded || isInLiquid){
                m_velocity.y = 3.f;
                if(isInLiquid)m_velocity.y += 2.f;
                if(isgrounded){
                    isgrounded = false;
                }
            }
            // if in lava/water, swim upwards at a constant rate
            lastInputCount = 0;
            inputs.spacePressed = false;
        }else{
            lastInputCount++;
            if(lastInputCount > 4)
            {
                if(vel > 0.8){
                    glm::vec3 dir = glm::normalize(glm::vec3(m_velocity.x, 0.f, m_velocity.z));
                    m_velocity -= 0.8f * dir;
                }else{
                    this->speed = 0;
                    m_acceleration = glm::vec3(0,0,0);
                    m_velocity.x = 0;
                    m_velocity.z = 0;
                }
            }

        }
//        std::cout << "velocity in f is " << m_velocity.x << " " << m_velocity.y << " " << m_velocity.z << std::endl;
    }
}

void Player::playerRemoveBlock(Terrain &terrain)
{
    for(float i = 0.f; i <= 3.f; i += 0.1f){
        glm::vec3 dir(i * m_forward);
        glm::vec3 pos(m_position);
        pos.y = pos.y + 1.5f;
        pos = pos + dir;
        BlockType type = terrain.getBlockAt(pos);
        if(movableBlocks.find(type) != movableBlocks.end()){
        // Jack: you could add other invalid type about removing blocks
            terrain.setBlockAt(pos.x,pos.y,pos.z,EMPTY);
            break;
        }
    }
}

void Player::playerAddBlock(Terrain &terrain)
{
    for(float i = 0; i <= 3; i += 0.01f){
        glm::vec3 dir(i*m_forward);
        glm::vec3 pos(m_position);
        pos.y = pos.y + 1.5f;
        pos = pos + dir;

//            if(terrain.checkBlockAt(pos)){
        BlockType type = terrain.getBlockAt(pos);
        if(movableBlocks.find(type) != movableBlocks.end()){
            pos = pos - 0.01f * m_forward;
            //judge if player is looking down, try to avoid place a block and lock player inside it
//                std::cout << "add block: " << pos.x << ", " << pos.y << ", " << pos.z << std::endl;
            if(!(m_forward.y >= -1 && m_forward.y <= -0.7)){
                if(i <= 1)break;
            }else{
                if(i <= 2)break;
            }
            std::cout << "add block: " << pos.x << ", " << pos.y << ", " << pos.z << std::endl;
#ifdef WATER_FLUID
            terrain.setBlockAt(pos.x, pos.y, pos.z, WATER);
#else
            terrain.setBlockAt(pos.x, pos.y, pos.z, LAVA);
#endif
            std::cout << "finished added" << std::endl;
            break;
        }
    }
}

//change terrain to nonconstant
void Player::computePhysics(float dT, Terrain &terrain) {
    // TODO: Update the Player's position based on its acceleration
    // and velocity, and also perform collision detection.


    //test if the water is around the player
    float water_dist = 676;
    float lava_dist = 676;
    bool contains_water = false;
    bool contains_lava = false;
    for(int i = -15; i <= 15; i++){
        for(int j = -15; j <= 15; j++){
            for(int k = -15; k <= 15; k++){
                glm::vec3 pos(m_position.x + i,m_position.y + j,m_position.z + k);
                if(terrain.getBlockAt(pos) == WATER){
                    if(i * i +j * j + k * k < water_dist){
                        water_dist = i * i +j * j + k * k;
                    }
                    contains_water = true;
                }
            }
        }
    }
    if(contains_water){
        if(!river_sound.isPlaying()){
            river_sound.setVolume((675 - water_dist) / 675.f * 0.5f);
            river_sound.play();
        }
    }else{
        river_sound.stop();
    }
    for(int i = -15; i <= 15; i++){
        for(int j = -15; j <= 15; j++){
            for(int k = -15; k <= 15; k++){
                glm::vec3 pos(m_position.x + i,m_position.y + j,m_position.z + k);
                if(terrain.getBlockAt(pos) == LAVA){
                    if(i * i +j * j + k * k < lava_dist){
                        lava_dist = i * i +j * j + k * k;
                    }
                    contains_lava = true;
                }
            }
        }
    }
    if(contains_water){
        if(!lava_sound.isPlaying()){
            //lava_sound.setVolume((675 - lava_dist) / 675.f * 0.5f);
            lava_sound.play();
        }
    }else{
        lava_sound.stop();
    }



    bool is_moved = false;
    if(m_velocity.x != 0 || m_velocity.z != 0)is_moved = true;
    //block remove and block add
    //block remove
    if(left_button){
        playerRemoveBlock(terrain);
        left_button = false;
    }

    //block add
    if(right_button){
        playerAddBlock(terrain);
        right_button = false;
    }

    //movement
    if(this->fly_mode){
        //friction
        this->m_velocity += this->m_acceleration - m_velocity * 0.02f;
        glm::vec3 speed = this->m_velocity.z * glm::normalize(m_forward) + this->m_velocity.x * glm::normalize(m_right) + this->m_velocity.y*glm::normalize(m_up);

        // std::cout << "speed:" << glm::length(speed) << std::endl;

        if(terrain.checkBlockAt(m_position)){
            isgrounded = true;
        }else{
            isgrounded = false;
        }

        if(dT > 100)dT = 100;
        this->moveAlongVector(speed * dT / 100.f);
    }else{
        //hashset stored water, empty, lava blocks
//        std::unordered_set<BlockType> fluid_collide{WATER_XP,WATER_XN,WATER_ZP,WATER_ZN,WATER_XNZN,WATER_XNZP,WATER_XPZN,WATER_XPZP,
//                                                   LAVA_XP,LAVA_XN,LAVA_ZP,LAVA_ZN,LAVA_XNZN,LAVA_XNZP,LAVA_XPZN,LAVA_XPZP};
        //if player is moved or not;
//        bool is_moved = false;
//        bool is_collided = true;

        //tell if y collides with the ground

        //test if is in liquid
        if(isInLiquid){
            m_velocity = m_velocity * 2.f / 3.f;
        }

        //tell if player is prepared to jump
        if(jump){
            jump = false;
            isgrounded = false;
        }
#ifdef Y_COLLISION
        if(m_velocity.y > -10){
            m_velocity.y -= 0.2;
        }
#endif

        //tell if player is already jumped when face unit one height block
        // std::cout << "velocity at begin " << m_velocity.x << " " << m_velocity.y << " " << m_velocity.z << std::endl;
        bool jumped = true;

        //calculate points

        glm::vec3 bot_zneg_xneg(m_position.x-0.5f,m_position.y,m_position.z-0.5f);
        glm::vec3 bot_zneg_xpos(m_position.x+0.5f,m_position.y,m_position.z-0.5f);
        glm::vec3 bot_zpos_xpos(m_position.x+0.5f,m_position.y,m_position.z+0.5f);
        glm::vec3 bot_zpos_xneg(m_position.x-0.5f,m_position.y,m_position.z+0.5f);
        glm::vec3 mid_zneg_xneg(m_position.x-0.5f,m_position.y+1,m_position.z-0.5f);
        glm::vec3 mid_zneg_xpos(m_position.x+0.5f,m_position.y+1,m_position.z-0.5f);
        glm::vec3 mid_zpos_xpos(m_position.x+0.5f,m_position.y+1,m_position.z+0.5f);
        glm::vec3 mid_zpos_xneg(m_position.x-0.5f,m_position.y+1,m_position.z+0.5f);
        glm::vec3 top_zneg_xneg(m_position.x-0.5f,m_position.y+2,m_position.z-0.5f);
        glm::vec3 top_zneg_xpos(m_position.x+0.5f,m_position.y+2,m_position.z-0.5f);
        glm::vec3 top_zpos_xpos(m_position.x+0.5f,m_position.y+2,m_position.z+0.5f);
        glm::vec3 top_zpos_xneg(m_position.x-0.5f,m_position.y+2,m_position.z+0.5f);
        //test if is in liquid
        BlockType znxn = terrain.getBlockAt(bot_zneg_xneg);
        BlockType znxp = terrain.getBlockAt(bot_zneg_xpos);
        BlockType zpxn = terrain.getBlockAt(bot_zpos_xneg);
        BlockType zpxp = terrain.getBlockAt(bot_zpos_xpos);

        if((znxn == LAVA && znxp == LAVA && zpxn == LAVA && zpxp == LAVA) ||
                (znxn == WATER && znxp == WATER && zpxn == WATER && zpxp == WATER)){
            isInLiquid = true;
        }else{
            isInLiquid = false;
        }

//        std::cout << "isliquid " << isInLiquid << std::endl;

        BlockType center = terrain.getBlockAt(m_position);
        BlockType mid_center = terrain.getBlockAt(m_position+glm::vec3(0,1.0,0));
        BlockType top_center = terrain.getBlockAt(m_position+glm::vec3(0,2.0,0));

//        if(fluid_collide.find(center) != fluid_collide.end() ||
//                fluid_collide.find(mid_center) != fluid_collide.end() ||
//                fluid_collide.find(top_center) != fluid_collide.end())
        /*if(center >= WATER_XP || mid_center >= WATER_XP || top_center >= WATER_XP)
        {*/
            if(center == WATER_XP || mid_center == WATER_XP || top_center == WATER_XP ||
                    center == LAVA_XP || mid_center == LAVA_XP || top_center == LAVA_XP){
                m_velocity.x += 1.8f;
            }else if(center == WATER_XN || mid_center == WATER_XN || top_center == WATER_XN ||
                     center == LAVA_XN || mid_center == LAVA_XN || top_center == LAVA_XN){
                m_velocity.x += -1.8f;
            }else if(center == WATER_ZP || mid_center == WATER_ZP || top_center == WATER_ZP ||
                     center == LAVA_ZP || mid_center == LAVA_ZP || top_center == LAVA_ZP){
                m_velocity.z += 1.8f;
            }else if(center == WATER_ZN || mid_center == WATER_ZN || top_center == WATER_ZN ||
                     center == LAVA_ZN || mid_center == LAVA_ZN || top_center == LAVA_ZN){
                m_velocity.z += -1.8f;
            }else if(center == WATER_XPZP || mid_center == WATER_XPZP || top_center == WATER_XPZP ||
                     center == LAVA_XPZP || mid_center == LAVA_XPZP || top_center == LAVA_XPZP){
                m_velocity.x += 1.8f;
                m_velocity.z += 1.8f;
            }else if(center == WATER_XPZN || mid_center == WATER_XPZN || top_center == WATER_XPZN ||
                     center == LAVA_XPZN || mid_center == LAVA_XPZN || top_center == LAVA_XPZN){
                m_velocity.x += 1.8f;
                m_velocity.z += -1.8f;
            }else if(center == WATER_XNZP || mid_center == WATER_XNZP || top_center == WATER_XNZP ||
                     center == LAVA_XNZP || mid_center == LAVA_XNZP || top_center == LAVA_XNZP){
                m_velocity.x += -1.8f;
                m_velocity.z += 1.8f;
            }else if(center == WATER_XNZN || mid_center == WATER_XNZN || top_center == WATER_XNZN ||
                     center == LAVA_XNZN || mid_center == LAVA_XNZN || top_center == LAVA_XNZN){
                m_velocity.x += -1.8f;
                m_velocity.z += -1.8f;
            }
        //}

#ifdef Y_COLLISION
        //tell if collide with wall
        //y axis colliding
        if(m_velocity.y > 0){
            for(float i = 0.05f; i <= m_velocity.y; i += 0.05f){
                std::array<glm::vec3, 4> ps = {
                    top_zneg_xneg+glm::vec3(0,i,0),
                    top_zneg_xpos+glm::vec3(0,i,0),
                    top_zpos_xneg+glm::vec3(0,i,0),
                    top_zpos_xpos+glm::vec3(0,i,0)
                };
                bool findWall = false;
                for(glm::vec3& p : ps){
                    BlockType t = terrain.getBlockAt(p);
                    if(t != EMPTY && t < LAVA)
                    {
                        findWall = true;
                        m_velocity.y = i - 0.05f;
                        break;
                    }
                }
                if(findWall){break;}
            }
        }else if(m_velocity.y < 0){
            for(float i = -0.05f; i >= m_velocity.y; i -= 0.05f){
                std::array<glm::vec3, 4> ps = {
                    bot_zneg_xneg+glm::vec3(0,i,0),
                    bot_zneg_xpos+glm::vec3(0,i,0),
                    bot_zpos_xneg+glm::vec3(0,i,0),
                    bot_zpos_xpos+glm::vec3(0,i,0)
                };
                bool findWall = false;
                for(glm::vec3& p : ps){
                    BlockType t = terrain.getBlockAt(p);
                    if(t != EMPTY && t < LAVA)
                    {
                        findWall = true;
                        m_velocity.y = i + 0.05f;
                        isgrounded = true;
                        break;
                    }
                }
                if(findWall){break;}
            }
        }
#endif



#ifdef XZ_COLLISION
        //tell if collide with wall
        //z axis colliding
        if(m_velocity.z < 0){
            for(float i = -0.05f; i >= m_velocity.z; i -= 0.05f){
                std::array<glm::vec3, 6> ps = {
                    top_zneg_xpos+glm::vec3(0,0,i),
                    top_zneg_xneg+glm::vec3(0,0,i),
                    mid_zneg_xneg+glm::vec3(0,0,i),
                    mid_zneg_xpos+glm::vec3(0,0,i),
                    bot_zneg_xneg+glm::vec3(0,0,i),
                    bot_zneg_xpos+glm::vec3(0,0,i)
                };
                bool findWall = false;
                for(glm::vec3& p : ps){
                    BlockType t = terrain.getBlockAt(p);
                    if(t != EMPTY && t < LAVA)
                    {
                        findWall = true;
                        m_velocity.z = i+0.05f;
                        break;
                    }
                }
                if(findWall){break;}
            }
        }else if(m_velocity.z > 0){
            for(float i = 0.05f; i <= m_velocity.z; i += 0.05f){
                std::array<glm::vec3, 6> ps = {
                    top_zpos_xpos+glm::vec3(0,0,i),
                    top_zpos_xneg+glm::vec3(0,0,i),
                    mid_zpos_xneg+glm::vec3(0,0,i),
                    mid_zpos_xpos+glm::vec3(0,0,i),
                    bot_zpos_xneg+glm::vec3(0,0,i),
                    bot_zpos_xpos+glm::vec3(0,0,i)
                };
                bool findWall = false;
                for(glm::vec3& p : ps){
                    BlockType t = terrain.getBlockAt(p);
                    if(t != EMPTY && t < LAVA)
                    {
                        findWall = true;
                        m_velocity.z = i-0.05f;
                        break;
                    }
                }
                if(findWall){break;}
            }
        }

        //tell if collide with wall
        //x axis colliding
        glm::vec3 del;
        if(m_velocity.x < 0){
            for(float i = -0.05f; i >= m_velocity.x; i -= 0.05f){
                std::array<glm::vec3, 6> ps = {
                    top_zneg_xneg+glm::vec3(i,0,0),
                    top_zpos_xneg+glm::vec3(i,0,0),
                    mid_zneg_xneg+glm::vec3(i,0,0),
                    mid_zpos_xneg+glm::vec3(i,0,0),
                    bot_zpos_xneg+glm::vec3(i,0,0),
                    bot_zneg_xneg+glm::vec3(i,0,0)
                };
                bool findWall = false;
                for(glm::vec3& p : ps){
                    BlockType t = terrain.getBlockAt(p);
                    if(t != EMPTY && t < LAVA)
                    {
                        findWall = true;
                        m_velocity.x = i+0.05f;
                        break;
                    }
                }
                if(findWall){break;}
            }
        }else if(m_velocity.x > 0){
            for(float i = 0.05f; i <= m_velocity.x; i += 0.05f){

                std::array<glm::vec3, 6> ps = {
                    top_zneg_xpos+glm::vec3(i,0,0),
                    top_zpos_xpos+glm::vec3(i,0,0),
                    mid_zneg_xpos+glm::vec3(i,0,0),
                    mid_zpos_xpos+glm::vec3(i,0,0),
                    bot_zpos_xpos+glm::vec3(i,0,0),
                    bot_zneg_xpos+glm::vec3(i,0,0)
                };
                bool findWall = false;
                for(glm::vec3& p : ps){
                    BlockType t = terrain.getBlockAt(p);
                    if(t != EMPTY && t < LAVA)
                    {
                        findWall = true;
                        m_velocity.x = i-0.05f;
                        break;
                    }
                }
                if(findWall){break;}
            }
        }
#endif

//        for(float i = 0.01f; i <= 1; i += 0.01f){
//            BlockType center_meet = terrain.getBlockAt(m_position+m_velocity * i);
//            if(center_meet != EMPTY && center_meet != LAVA && center_meet != WATER && fluid_collide.find(center_meet) == fluid_collide.end()){
//                std::cout << "entered" << std::endl;
//                m_velocity = glm::vec3(0,0,0);
//                break;
//            }
//        }
        // std::cout << "velocity at end " << m_velocity.x << " " << m_velocity.y << " " << m_velocity.z << std::endl;


#ifdef PLAY_SOUND
        if(isgrounded && is_moved){
            if(!walk_sound.isPlaying()){
                walk_sound.play();
            }
        }else{
            walk_sound.stop();
        }
#endif


        //friction
        if(dT > 100)dT = 100;
        this->moveAlongVector(this->m_velocity * dT / 100.f);
    }


}

void Player::setCameraWidthHeight(unsigned int w, unsigned int h) {
    m_camera.setWidthHeight(w, h);
}

void Player::moveAlongVector(glm::vec3 dir) {
    Entity::moveAlongVector(dir);
    m_camera.moveAlongVector(dir);
}
void Player::moveForwardLocal(float amount) {
    Entity::moveForwardLocal(amount);
    m_camera.moveForwardLocal(amount);
}
void Player::moveRightLocal(float amount) {
    Entity::moveRightLocal(amount);
    m_camera.moveRightLocal(amount);
}
void Player::moveUpLocal(float amount) {
    Entity::moveUpLocal(amount);
    m_camera.moveUpLocal(amount);
}
void Player::moveForwardGlobal(float amount) {
    Entity::moveForwardGlobal(amount);
    m_camera.moveForwardGlobal(amount);
}
void Player::moveRightGlobal(float amount) {
    Entity::moveRightGlobal(amount);
    m_camera.moveRightGlobal(amount);
}
void Player::moveUpGlobal(float amount) {
    Entity::moveUpGlobal(amount);
    m_camera.moveUpGlobal(amount);
}
void Player::rotateOnForwardLocal(float degrees) {
    Entity::rotateOnForwardLocal(degrees);
    m_camera.rotateOnForwardLocal(degrees);
}
void Player::rotateOnRightLocal(float degrees) {
    Entity::rotateOnRightLocal(degrees);
    m_camera.rotateOnRightLocal(degrees);
}
void Player::rotateOnUpLocal(float degrees) {
    Entity::rotateOnUpLocal(degrees);
    m_camera.rotateOnUpLocal(degrees);
}
void Player::rotateOnForwardGlobal(float degrees) {
    Entity::rotateOnForwardGlobal(degrees);
    m_camera.rotateOnForwardGlobal(degrees);
}
void Player::rotateOnRightGlobal(float degrees) {
    Entity::rotateOnRightGlobal(degrees);
    m_camera.rotateOnRightGlobal(degrees);
}
void Player::rotateOnUpGlobal(float degrees) {
    Entity::rotateOnUpGlobal(degrees);
    m_camera.rotateOnUpGlobal(degrees);
}

QString Player::posAsQString() const {
    std::string str("( " + std::to_string(m_position.x) + ", " + std::to_string(m_position.y) + ", " + std::to_string(m_position.z) + ")");
    return QString::fromStdString(str);
}
QString Player::velAsQString() const {
    std::string str("( " + std::to_string(m_velocity.x) + ", " + std::to_string(m_velocity.y) + ", " + std::to_string(m_velocity.z) + ")");
    return QString::fromStdString(str);
}
QString Player::accAsQString() const {
    std::string str("( " + std::to_string(m_acceleration.x) + ", " + std::to_string(m_acceleration.y) + ", " + std::to_string(m_acceleration.z) + ")");
    return QString::fromStdString(str);
}
QString Player::lookAsQString() const {
    std::string str("( " + std::to_string(m_forward.x) + ", " + std::to_string(m_forward.y) + ", " + std::to_string(m_forward.z) + ")");
    return QString::fromStdString(str);
}

glm::vec3 Player::getLookVec() const {
    return m_forward;
}
