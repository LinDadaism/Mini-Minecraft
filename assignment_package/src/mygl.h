#ifndef MYGL_H
#define MYGL_H

#include "openglcontext.h"
#include "postprocessshader.h"
#include "scene/quad.h"
#include "scene/worldaxes.h"
#include "scene/camera.h"
#include "scene/terrain.h"
#include "scene/player.h"
#include "framebuffer.h"

#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <smartpointerhelp.h>
#include <QDateTime>


class MyGL : public OpenGLContext
{
    Q_OBJECT
private:
    Quad m_geomQuad; // The screen-space quadrangle used to draw
                     // the scene with the post-process shaders.
    WorldAxes m_worldAxes; // A wireframe representation of the world axes. It is hard-coded to sit centered at (32, 128, 32).
    ShaderProgram m_progLambert;// A shader program that uses lambertian reflection
    ShaderProgram m_progFlat;// A shader program that uses "flat" reflection (no shadowing at all)
    ShaderProgram m_progInstanced;// A shader program that is designed to be compatible with instanced rendering
    ShaderProgram m_progSky; // A screen-space shader for creating the sky background
    ShaderProgram m_progShadow; // A shader program that creates a shadow map

    // The collection of post-process shaders available to the user. This vector is only
    // ever modified once, in initializeGL().
    std::vector<sPtr<PostProcessShader>> m_postprocessShaders;
    // A pointer to the post-process shader currently being used to render the scene.
    // Used by paintGL to determine which post-process shader to apply to the scene.
    PostProcessShader* mp_progPostprocessCurrent;
    // A pointer to our no-operation post-process shader, used to draw the scene's background
//    PostProcessShader* m_progPostprocessNoOp;

    FrameBuffer m_frameBuffer; // A frame buffer to render terrain texture in the OpenGL pipeline
    FrameBuffer m_depthTexFrameBuffer; // A frame buffer to render the depth texture for shadow mapping in the OpenGL pipeline

    GLuint vao; // A handle for our vertex array object. This will store the VBOs created in our geometry classes.
                // Don't worry too much about this. Just know it is necessary in order to render geometry.

    Terrain m_terrain; // All of the Chunks that currently comprise the world.
    Player m_player; // The entity controlled by the user. Contains a camera to display what it sees as well.
    InputBundle m_inputs; // A collection of variables to be updated in keyPressEvent, mouseMoveEvent, mousePressEvent, etc.
    double previoustime = 0;
    QTimer m_timer; // Timer linked to tick(). Fires approximately 60 times per second.
    float m_time;
    //static member variable to get mouse position
    static glm::vec2 mouse_position;
    static double x_rotate;
    static double y_rotate;

    void moveMouseToCenter(); // Forces the mouse position to the screen's center. You should call this
                              // from within a mouse move event after reading the mouse movement so that
                              // your mouse stays within the screen bounds and is always read.

    void sendPlayerDataToGUI() const;

    Texture m_texture; // Texture for all the blocktypes

public:
    explicit MyGL(QWidget *parent = nullptr);
    ~MyGL();
    double original_y = 90;

    // Called once when MyGL is initialized.
    // Once this is called, all OpenGL function
    // invocations are valid (before this, they
    // will cause segfaults)
    void initializeGL() override;
    // Called whenever MyGL is resized.
    void resizeGL(int w, int h) override;
    // Called whenever MyGL::update() is called.
    // In the base code, update() is called from tick().
    void paintGL() override;

    // Called from paintGL().
    // Calls Terrain::draw().
    void renderTerrain(bool isDepthRender = false);

    void createShaders(); // link vert & frag shaders into shader programs, including postprocessing

    // A helper function that iterates through
    // each of the render passes required by the
    // currently bound post-process shader and
    // invokes them.
    void performPostprocessRenderPass();

    void renderSky(); // Called from paintGL to render the sky box.

protected:
    // Automatically invoked when the user
    // presses a key on the keyboard
    void keyPressEvent(QKeyEvent *e);
    // Automatically invoked when the user
    // moves the mouse
    void mouseMoveEvent(QMouseEvent *e);
    // Automatically invoked when the user
    // presses a mouse button
    void mousePressEvent(QMouseEvent *e);

private slots:
    void tick(); // Slot that gets called ~60 times per second by m_timer firing.

signals:
    void sig_sendPlayerPos(QString) const;
    void sig_sendPlayerVel(QString) const;
    void sig_sendPlayerAcc(QString) const;
    void sig_sendPlayerLook(QString) const;
    void sig_sendPlayerChunk(QString) const;
    void sig_sendPlayerTerrainZone(QString) const;
};


#endif // MYGL_H
