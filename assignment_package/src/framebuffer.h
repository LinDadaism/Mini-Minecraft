#pragma once
#include "openglcontext.h"
#include "glm_includes.h"

// A class representing a frame buffer in the OpenGL pipeline.
// Stores three GPU handles: one to a frame buffer object, one to
// a texture object that will store the frame buffer's contents,
// and one to a depth buffer needed to properly render to the frame
// buffer.
// Redirect your render output to a FrameBuffer by invoking
// bindFrameBuffer() before ShaderProgram::draw, and read
// from the frame buffer's output texture by invoking
// bindToTextureSlot() and then associating a ShaderProgram's
// sampler2d with the appropriate texture slot.
class FrameBuffer {
private:
    OpenGLContext *mp_context;
    GLuint m_frameBuffer; // A collection of handles to the frame buffers we've given
                            // ourselves to perform render passes. The 0th frame buffer is always
                            // written to by the render pass that uses the currently bound surface shader.
    GLuint m_outputTexture; // A collection of handles to the textures used by the frame buffers.
                            // m_frameBuffers[i] writes to m_outputTextures[i].
    GLuint m_depthRenderBuffer; // A collection of handles to the depth buffers used by our frame buffers.
                                // m_frameBuffers[i] writes to m_depthRenderBuffers[i].

    unsigned int m_width, m_height, m_devicePixelRatio;
    bool m_created;

    unsigned int m_textureSlot;

public:
    FrameBuffer(OpenGLContext *context, unsigned int width, unsigned int height, unsigned int devicePixelRatio);
    // Make sure to call resize from MyGL::resizeGL to keep your frame buffer up to date with
    // your screen dimensions
    void resize(unsigned int width, unsigned int height, unsigned int devicePixelRatio);
    // Initialize all GPU-side data required
    void create(bool isShadowMap = false);
    // Deallocate all GPU-side data
    void destroy();
    void bindFrameBuffer(bool useDefault = false);
    // Binds the relevant output textures on the GPU
    void bindToTextureSlot(unsigned int slot);
    unsigned int getTextureSlot() const;
};
