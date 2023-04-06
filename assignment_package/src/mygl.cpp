#include "mygl.h"
#include <glm_includes.h>

#include <iostream>
#include <QApplication>
#include <QKeyEvent>

//Initialize mouse position
glm::vec2 MyGL::mouse_position(960,480);
double MyGL::x_rotate = 0;
double MyGL::y_rotate = 90;
static const glm::vec3 INIT_PLAYER_POS = glm::vec3(48.f, 129.f+65.0f, 48.f);
static const float TIME_DIFF = 16.f;
static const float RATE = 10.f;
static const float SUN_SPEED = 0.001f;
static const float PI = 3.14159265359;
static const glm::mat4 BIAS_MAT(
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 0.5, 0.0,
    0.5, 0.5, 0.5, 1.0
);

MyGL::MyGL(QWidget *parent)
    : OpenGLContext(parent), m_geomQuad(this), m_worldAxes(this),
      m_progLambert(this), m_progFlat(this), m_progInstanced(this), m_progSky(this),
      m_progShadow(this), m_postprocessShaders(), mp_progPostprocessCurrent(nullptr), //m_progPostprocessNoOp(nullptr),
      m_frameBuffer(this, this->width(), this->height(), this->devicePixelRatio()),
      m_depthTexFrameBuffer(this, this->width(), this->height(), this->devicePixelRatio()),
      m_terrain(this), m_player(INIT_PLAYER_POS, m_terrain), m_time(0), m_texture(this)
{
    // Connect the timer to a function so that when the timer ticks the function is executed
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(tick()));
    // Tell the timer to redraw 60 times per second
    m_timer.start(16);
    setFocusPolicy(Qt::ClickFocus);

    setMouseTracking(true); // MyGL will track the mouse's movements even if a mouse button is not pressed
    setCursor(Qt::BlankCursor); // Make the cursor invisible
}

MyGL::~MyGL() {
    makeCurrent();
    glDeleteVertexArrays(1, &vao);
    m_geomQuad.destroyVBOdata();
    m_frameBuffer.destroy();
    m_depthTexFrameBuffer.destroy();
}


void MyGL::moveMouseToCenter() {
    QCursor::setPos(this->mapToGlobal(QPoint(width() / 2, height() / 2)));
}

void MyGL::initializeGL()
{
    // Create an OpenGL context using Qt's QOpenGLFunctions_3_2_Core class
    // If you were programming in a non-Qt context you might use GLEW (GL Extension Wrangler)instead
    initializeOpenGLFunctions();
    // Print out some information about the current OpenGL context
    debugContextVersion();

    // Set a few settings/modes in OpenGL rendering
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);

    // Enable transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    // Set the color with which the screen is filled at the start of each render call.
    glClearColor(0.37f, 0.74f, 1.0f, 1);

    printGLErrorLog("In MyGL::initalizeGL(), ");

    // Create a Vertex Attribute Object
    glGenVertexArrays(1, &vao);

    // Create a render buffer for terrain texture
    m_frameBuffer.create();
    // Create a render buffer for depth texture
    m_depthTexFrameBuffer.create(true);

    // Create the instance of a quad for window render passes
    m_geomQuad.createVBOdata();

    //Create the instance of the world axes
    m_worldAxes.createVBOdata();

    createShaders();

    // We have to have a VAO bound in OpenGL 3.2 Core. But if we're not
    // using multiple VAOs, we can just bind one once.
    glBindVertexArray(vao);

    // Load texture
    m_texture.create(":/textures/minecraft_textures_all.png");
    m_texture.load(0);


//    m_texture.bind(0);
    m_terrain.initialize();
}

void MyGL::resizeGL(int w, int h) {
    //This code sets the concatenated view and perspective projection matrices used for
    //our scene's camera view.
    m_player.setCameraWidthHeight(static_cast<unsigned int>(w), static_cast<unsigned int>(h));
    glm::mat4 viewproj = m_player.mcr_camera.getViewProj();

    // Upload the view-projection matrix to our shaders (i.e. onto the graphics card)
    m_progLambert.setViewProjMatrix(viewproj);
    m_progFlat.setViewProjMatrix(viewproj);
    m_progSky.setViewProjMatrix(glm::inverse(viewproj)); // inverse for ray-tracing matrix computation in frag shader

    // Update camera position for distance fog in terrain rendering -> lambert shader
    m_progLambert.setCameraEye(m_player.mcr_camera.mcr_position);
    m_progLambert.setDimensions(glm::ivec2(w * this->devicePixelRatio(), h * this->devicePixelRatio()));

    // Update the screen dimensions and camera position for the sky box shader
    m_progSky.setDimensions(glm::ivec2(w * this->devicePixelRatio(), h * this->devicePixelRatio()));
    m_progSky.setCameraEye(m_player.mcr_camera.mcr_position);

    // Update postprocess shaders' screen dimensions
    for(sPtr<PostProcessShader> p : m_postprocessShaders)
    {
        p->setDimensions(glm::ivec2(w * this->devicePixelRatio(), h * this->devicePixelRatio()));
    }

    // Update FrameBuffer if window is resized
    m_frameBuffer.resize(w, h, this->devicePixelRatio());
    m_frameBuffer.destroy();
    m_frameBuffer.create();

    m_depthTexFrameBuffer.resize(w, h, this->devicePixelRatio());
    m_depthTexFrameBuffer.destroy();
    m_depthTexFrameBuffer.create(true);

    printGLErrorLog("In MyGL::resizeGL(), ");
}


// MyGL's constructor links tick() to a timer that fires 60 times per second.
// We're treating MyGL as our game engine class, so we're going to perform
// all per-frame actions here, such as performing physics updates on all
// entities in the scene.
void MyGL::tick() {
    update(); // Calls paintGL() as part of a larger QOpenGLWidget pipeline
    sendPlayerDataToGUI(); // Updates the info in the secondary window displaying player data
    m_time += TIME_DIFF / RATE;
    m_player.tick(TIME_DIFF, this->m_inputs);
}

void MyGL::sendPlayerDataToGUI() const {
    emit sig_sendPlayerPos(m_player.posAsQString());
    emit sig_sendPlayerVel(m_player.velAsQString());
    emit sig_sendPlayerAcc(m_player.accAsQString());
    emit sig_sendPlayerLook(m_player.lookAsQString());
    glm::vec2 pPos(m_player.mcr_position.x, m_player.mcr_position.z);
    glm::ivec2 chunk(16 * glm::ivec2(glm::floor(pPos / 16.f)));
    glm::ivec2 zone(64 * glm::ivec2(glm::floor(pPos / 64.f)));
    emit sig_sendPlayerChunk(QString::fromStdString("( " + std::to_string(chunk.x) + ", " + std::to_string(chunk.y) + " )"));
    emit sig_sendPlayerTerrainZone(QString::fromStdString("( " + std::to_string(zone.x) + ", " + std::to_string(zone.y) + " )"));
}

// This function is called whenever update() is called.
// MyGL's constructor links update() to a timer that fires 60 times per second,
// so paintGL() called at a rate of 60 frames per second.
void MyGL::paintGL() {
    glm::mat4 viewproj = m_player.mcr_camera.getViewProj();

    m_progFlat.setViewProjMatrix(viewproj);
    m_progLambert.setViewProjMatrix(viewproj);
    //m_progInstanced.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    m_progSky.setViewProjMatrix(glm::inverse(viewproj));
    m_progFlat.setModelMatrix(glm::mat4());
    m_progLambert.setModelMatrix(glm::mat4());

    // Sunlight dir sync with shader
    // Compute the MVP matrix from the light's point of view
    float slowerTime = m_time == TIME_DIFF/RATE ? m_time - TIME_DIFF/RATE + 1.f : m_time/TIME_DIFF + 1.f;
    glm::vec3 lightDir = INIT_PLAYER_POS + glm::vec3(100.f, cos(slowerTime * SUN_SPEED - PI/2) * 200.f, cos(slowerTime * SUN_SPEED) * 200.f);
    glm::mat4 depthProj = glm::ortho<float>(-200, 200, -100, 100, 1., 400.);
    glm::mat4 depthView = glm::lookAt(lightDir, INIT_PLAYER_POS, glm::vec3(0,1,0));
    glm::mat4 depthMVP = depthProj * depthView;
    m_progShadow.setDepthModelViewProj(depthMVP);

    // We can convert homogeneous coords to texture UVs directly in the frag shader,
    // but it's more efficient to multiply the homogeneous coords by BIAS_MAT here.
    m_progLambert.setDepthModelViewProj(BIAS_MAT * depthMVP);

    m_progLambert.setSunSpeed(SUN_SPEED);
    m_progSky.setSunSpeed(SUN_SPEED);

    // First render terrain scene to a depth texture
    m_depthTexFrameBuffer.bindFrameBuffer();
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK); // Cull back-facing triangles -> draw only front-facing triangles
    renderTerrain(true); // only need position VBO data for this render pass

    // Render terrain scene to our framebuffer rather than the viewport
    m_frameBuffer.bindFrameBuffer();
    m_frameBuffer.bindToTextureSlot(0);
    m_texture.bind(0);
    // Bind depth texture in Texture Unit 2
    m_depthTexFrameBuffer.bindToTextureSlot(2);

#ifdef DRAW_SKY
    renderSky();
#endif
    renderTerrain();
    performPostprocessRenderPass();

    //set time
    m_progLambert.setTime(m_time);
    //mp_progPostprocessCurrent->setTime(m_time);
//    m_progPostprocessNoOp->setTime(m_time);
    mp_progPostprocessCurrent->setTime(slowerTime);
//    m_progPostprocessNoOp->setTime(slowerTime);
    m_progSky.setTime(slowerTime);

    // qint64 allSecs = QDateTime::currentDateTime().toMSecsSinceEpoch();
    // std::cout << allSecs << std::endl;
    // m_time += (allSecs - previoustime) / 2;
    // previoustime = allSecs;

    glDisable(GL_DEPTH_TEST);
#ifdef DRAW_WOLRDAXES
    m_progFlat.setCamLook(m_player.mcr_camera.mcr_position + m_player.getLookVec() * 50.f);
    m_progFlat.draw(m_worldAxes);
#endif
    glEnable(GL_DEPTH_TEST);
}

void MyGL::renderSky() {
    m_progSky.setCameraEye(m_player.mcr_camera.mcr_position);
    m_progSky.draw(m_geomQuad);
}

// Renders the nine zones of generated terrain that surround the player
// (refer to Terrain::m_generatedTerrain for more info)
void MyGL::renderTerrain(bool isDepthRender) {
    m_progLambert.setCameraEye(m_player.mcr_camera.mcr_position); // for distance fog
    glm::vec3 playerPos = glm::floor(m_player.mcr_position);
    if (isDepthRender) {
        m_terrain.ExpandDraw(playerPos, &m_progShadow, isDepthRender);
    } else {
        m_terrain.ExpandDraw(playerPos, &m_progLambert, isDepthRender);
    }
}

void MyGL::keyPressEvent(QKeyEvent *e) {
    float amount = 2.0f;
    if(e->modifiers() & Qt::ShiftModifier){
        amount = 10.0f;
    }
    // http://doc.qt.io/qt-5/qt.html#Key-enum
    // This could all be much more efficient if a switch
    // statement were used, but I really dislike their
    // syntax so I chose to be lazy and use a long
    // chain of if statements instead
    /*if (e->key() == Qt::Key_Escape) {
        QApplication::quit();
    } else if (e->key() == Qt::Key_Right) {
        m_player.rotateOnUpGlobal(-amount);
    } else if (e->key() == Qt::Key_Left) {
        m_player.rotateOnUpGlobal(amount);
    } else if (e->key() == Qt::Key_Up) {
        m_player.rotateOnRightLocal(-amount);
    } else if (e->key() == Qt::Key_Down) {
        m_player.rotateOnRightLocal(amount);
    } else*/
    /*if (e->key() == Qt::Key_W) {
        m_inputs.wPressed = true;
        //m_player.moveForwardLocal(amount);
    } else if (e->key() == Qt::Key_S) {
        m_player.moveForwardLocal(-amount);
    } else if (e->key() == Qt::Key_D) {
        m_player.moveRightLocal(amount);
    } else if (e->key() == Qt::Key_A) {
        m_player.moveRightLocal(-amount);
    } else if (e->key() == Qt::Key_Q) {
        m_player.moveUpGlobal(-amount);
    } else if (e->key() == Qt::Key_E) {
        m_player.moveUpGlobal(amount);
    }else if(e->key() == Qt::Key_F){
        m_player.fly_mode = !m_player.fly_mode;
    }else{
        m_inputs.wPressed = false;
    }*/
    if (e->key() == Qt::Key_Escape) {
        QApplication::quit();
    }
    if(e->key() == Qt::Key_W){
        m_inputs.wPressed = true;
    }else{
        m_inputs.wPressed = false;
    }
    if(e->key() == Qt::Key_A){
        m_inputs.aPressed = true;
    }else{
        m_inputs.aPressed = false;
    }
    if(e->key() == Qt::Key_S){
        m_inputs.sPressed = true;
    }else{
        m_inputs.sPressed = false;
    }
    if(e->key() == Qt::Key_D){
        m_inputs.dPressed = true;
    }else{
        m_inputs.dPressed = false;
    }
    if(e->key() == Qt::Key_Q){
        m_inputs.qPressed = true;
    }else{
        m_inputs.qPressed = false;
    }
    if(e->key() == Qt::Key_E){
        m_inputs.ePressed = true;
    }else{
        m_inputs.ePressed = false;
    }
    if(e->key() == Qt::Key_Space){
        m_inputs.spacePressed = true;
    }else{
        m_inputs.spacePressed = false;
    }
    if(e->key() == Qt::Key_F){
        m_player.fly_mode = !m_player.fly_mode;
    }
    if(e->key() == Qt::Key_Z){
//        std::cout << "z pressed" << std::endl;
        if(m_player.speed_setting > 0)m_player.speed_setting += 0.2f;
    }
    if(e->key() == Qt::Key_C){
        if(m_player.speed_setting > 0)m_player.speed_setting -= 0.2f;
    }
}

void MyGL::mouseMoveEvent(QMouseEvent *e) {
    // TODO
    const QPoint p = e->pos();
    QCursor c = cursor();
    c.setPos(mapToGlobal(QPoint(width() / 2, height() / 2)));
    //c.setShape(Qt::BlankCursor);
    //setCursor(c);
    double rotatex = (p.x() - m_player.mcr_camera.m_width / 2.f) * 0.5;
    double rotatey = (p.y() - m_player.mcr_camera.m_height / 2.f) * 0.5;
    if((rotatey > 0.5 || rotatey < -0.5)){
        original_y += rotatey;
        m_inputs.mouseY = -rotatey;
    }
    if(rotatex > 0.9 || rotatex < -0.9){
        m_inputs.mouseX = -rotatex;
    }
}

void MyGL::mousePressEvent(QMouseEvent *e) {
    // TODO
    if(e->button() == Qt::LeftButton){
        m_inputs.leftButton = true;
    }else{
        m_inputs.leftButton = false;
    }
    if(e->button() == Qt::RightButton){
        m_inputs.rightButton = true;
    }else{
        m_inputs.rightButton = false;
    }
}

void MyGL:: createShaders() {
    /*------------- Surface shaders ---------------*/
    // diffuse shader
    m_progLambert.create(":/glsl/lambert.vert.glsl", ":/glsl/lambert.frag.glsl");

    // flat lighting shader
    m_progFlat.create(":/glsl/flat.vert.glsl", ":/glsl/flat.frag.glsl");

    //m_progInstanced.create(":/glsl/instanced.vert.glsl", ":/glsl/lambert.frag.glsl");

    // sky box shader
    m_progSky.create(":/glsl/sky.vert.glsl", ":/glsl/sky.frag.glsl");

    // shadow map shader
    m_progShadow.create(":glsl/shadow.vert.glsl", ":/glsl/shadow.frag.glsl");

    /*------------- Post-process shaders ------------*/
    // no-operation shader
    sPtr<PostProcessShader> noOp = mkS<PostProcessShader>(this);
    noOp->create(":/glsl/post/passthrough.vert.glsl", ":/glsl/post/noOp.frag.glsl");
    m_postprocessShaders.push_back(noOp);

    // water tint
    sPtr<PostProcessShader> water = mkS<PostProcessShader>(this);
    water->create(":/glsl/post/passthrough.vert.glsl", ":/glsl/post/water.frag.glsl");
    m_postprocessShaders.push_back(water);

    // lava tint
    sPtr<PostProcessShader> lava = mkS<PostProcessShader>(this);
    lava->create(":/glsl/post/passthrough.vert.glsl", ":/glsl/post/lava.frag.glsl");
    m_postprocessShaders.push_back(lava);

    mp_progPostprocessCurrent = m_postprocessShaders[0].get();
//    m_progPostprocessNoOp = m_postprocessShaders[0].get();
}

void MyGL::performPostprocessRenderPass()
{
    // Render the frame buffer as a texture on a screen-size quad
    m_frameBuffer.bindFrameBuffer(true);
    m_frameBuffer.bindToTextureSlot(1);

    // Decide which post-processing shader to use, default is no-operation
    mp_progPostprocessCurrent = m_postprocessShaders[0].get();
    if (m_player.camInWater) {
        mp_progPostprocessCurrent = m_postprocessShaders[1].get();
    }
    if (m_player.camInLava) {
        mp_progPostprocessCurrent = m_postprocessShaders[2].get();
    }
//    m_progPostprocessNoOp->draw(m_geomQuad, 1);
    mp_progPostprocessCurrent->setTime(m_time);
    mp_progPostprocessCurrent->draw(m_geomQuad, 1);
}
