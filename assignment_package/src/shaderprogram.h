#ifndef SHADERPROGRAM_H
#define SHADERPROGRAM_H

#include <openglcontext.h>
#include <glm_includes.h>
#include <glm/glm.hpp>

#include "drawable.h"


class ShaderProgram
{
public:
    GLuint vertShader; // A handle for the vertex shader stored in this shader program
    GLuint fragShader; // A handle for the fragment shader stored in this shader program
    GLuint prog;       // A handle for the linked shader program stored in this class

    int attrPos; // A handle for the "in" vec4 representing vertex position in the vertex shader
    int attrNor; // A handle for the "in" vec4 representing vertex normal in the vertex shader
    int attrCol; // A handle for the "in" vec4 representing vertex color in the vertex shader
    int attrPosOffset; // A handle for a vec3 used only in the instanced rendering shader
    int attrUV; // A handle for the "in" vec2 representing vertex uv in vertex shader

    int unifModel; // A handle for the "uniform" mat4 representing model matrix in the vertex shader
    int unifModelInvTr; // A handle for the "uniform" mat4 representing inverse transpose of the model matrix in the vertex shader
    int unifViewProj; // A handle for the "uniform" mat4 representing combined projection and view matrices in the vertex shader
    int unifDepthMVP; // A handle for the "uniform" mat4 representing combined model view projection matrices from the light's point of view in the vertex shader
    int unifTime; // A handle for the "uniform" time
    int unifSunSpeed; // A handle for the "uniform" sun moving speed

    int unifColor; // A handle for the "uniform" vec4 representing color of geometry in the vertex shader
    int unifSampler2D; // A handle for the "uniform" texture2d representing the texture
    int unifTrans; // A handle for the "uniform" int u_transparent
    int unifSampler2DShadow; // A handle for the "uniform" depth texture2d for shadow mapping

    int unifDimensions; // A handle to the "uniform" ivec2 that stores the width and height of the texture being rendered
    int unifEye; // A handle for the camera eye
    int unifLook; // A handle for the camera look vector

public:
    ShaderProgram(OpenGLContext* context);
    // Sets up the requisite GL data and shaders from the given .glsl files
    void create(const char *vertfile, const char *fragfile);
    // Tells our OpenGL context to use this shader to draw things
    void useMe();
    // Sets up shader-specific handles
    virtual void setupMemberVars();

    // Pass the given model matrix to this shader on the GPU
    void setModelMatrix(const glm::mat4 &model);
    // Pass the given Projection * View matrix to this shader on the GPU
    void setViewProjMatrix(const glm::mat4 &vp);
    // Pass the given Model View Projection matrix to this shader on the GPU
    void setDepthModelViewProj(const glm::mat4 &mvp);
    // Pass the given color to this shader on the GPU
    void setGeometryColor(glm::vec4 color);
    // Set time
    void setTime(float t);
    // Set sun moving speed
    void setSunSpeed(float v);
    // Pass the given screen dimensions to this shader on the GPU
    void setDimensions(glm::ivec2 dims);
    // Pass the given camera position to this shader on the GPU
    void setCameraEye(glm::vec3 pos);
    // Pass the given camera forward vector to this shader on the GPU
    void setCamLook(const glm::vec3 &look);

    // Draw the given object to our screen using this ShaderProgram's shaders
    virtual void draw(Drawable &d, int textureSlot = 0);
    // Draw the given object to our screen multiple times using instanced rendering
    void drawInstanced(InstancedDrawable &d);
    // Draw the given object with interleaved VBOS
    void drawWithItlvVBOs(Drawable &d);
    // Draw the transparent block
    void drawTransparent(Drawable &d);

    // Simplified draw for depth rendering, only using pos and index VBO data
    void drawPosOnly(Drawable &d);
    void drawTransPosOnly(Drawable &d);

    // Utility function used in create()
    char* textFileRead(const char*);
    // Utility function that prints any shader compilation errors to the console
    void printShaderInfoLog(int shader);
    // Utility function that prints any shader linking errors to the console
    void printLinkInfoLog(int prog);
    // Set time
    void setTime(int t);
    // Set transparency
    void setTransparent(bool trans);

    QString qTextFileRead(const char*);

protected:
    OpenGLContext* context;   // Since Qt's OpenGL support is done through classes like QOpenGLFunctions_3_2_Core,
                            // we need to pass our OpenGL context to the Drawable in order to call GL functions
                            // from within this class.
};


#endif // SHADERPROGRAM_H
