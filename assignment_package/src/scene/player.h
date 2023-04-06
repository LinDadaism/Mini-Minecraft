#pragma once
//#include "entity.h"
#include "camera.h"
#include "terrain.h"
#include <QSoundEffect>
#define PLAY_SOUND
class Player : public Entity {
private:
    glm::vec3 m_velocity, m_acceleration;
    Camera m_camera;
    //change from const to nonconst
    Terrain &mcr_terrain;

    void playerRemoveBlock(Terrain &terrain);
    void playerAddBlock(Terrain &terrain);

    //Adding sound
    QSoundEffect wind_sound;
    QSoundEffect walk_sound;
    QSoundEffect river_sound;
    QSoundEffect bird_sound;
    QSoundEffect lava_sound;


    void processInputs(InputBundle &inputs);
    //change from const to nonconst
    void computePhysics(float dT, Terrain &terrain);
    bool isgrounded;
    bool isInLiquid; // affects player speed
    bool jump;
    bool left_button;
    bool right_button;


public:
    // Readonly public reference to our camera
    // for easy access from MyGL
    float speed_setting = 0.4;
    const Camera& mcr_camera;
    bool fly_mode = true;
    bool camInWater; // for post-processing effect
    bool camInLava; // for post-processing effect
    double speed = 0;
    double acceleration = 0.2;
    double walkMaxSpeed = 8; // 8;
    int lastInputCount = 0;
    float accRatio = 0.05f;

    //change from const to nonconst
    Player(glm::vec3 pos, Terrain &terrain);
    virtual ~Player() override;

    void setCameraWidthHeight(unsigned int w, unsigned int h);

    void tick(float dT, InputBundle &input) override;

    // Player overrides all of Entity's movement
    // functions so that it transforms its camera
    // by the same amount as it transforms itself.
    void moveAlongVector(glm::vec3 dir) override;
    void moveForwardLocal(float amount) override;
    void moveRightLocal(float amount) override;
    void moveUpLocal(float amount) override;
    void moveForwardGlobal(float amount) override;
    void moveRightGlobal(float amount) override;
    void moveUpGlobal(float amount) override;
    void rotateOnForwardLocal(float degrees) override;
    void rotateOnRightLocal(float degrees) override;
    void rotateOnUpLocal(float degrees) override;
    void rotateOnForwardGlobal(float degrees) override;
    void rotateOnRightGlobal(float degrees) override;
    void rotateOnUpGlobal(float degrees) override;

    // Getter
    glm::vec3 getLookVec() const;

    // For sending the Player's data to the GUI
    // for display
    QString posAsQString() const;
    QString velAsQString() const;
    QString accAsQString() const;
    QString lookAsQString() const;
};
