#include "framebuffer.h"
#include <iostream>

FrameBuffer::FrameBuffer(OpenGLContext *context,
                         unsigned int width, unsigned int height, unsigned int devicePixelRatio)
    : mp_context(context), m_frameBuffer(-1),
      m_outputTexture(-1), m_depthRenderBuffer(-1),
      m_width(width), m_height(height), m_devicePixelRatio(devicePixelRatio), m_created(false)
{}

void FrameBuffer::resize(unsigned int width, unsigned int height, unsigned int devicePixelRatio) {
    m_width = width;
    m_height = height;
    m_devicePixelRatio = devicePixelRatio;
}

void FrameBuffer::create(bool isShadowMap) {
    if (isShadowMap) {
        // The framebuffer
        mp_context->glGenFramebuffers(1, &m_frameBuffer);
        mp_context->glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);

        // Depth texture
        mp_context->glGenTextures(1, &m_outputTexture);
        mp_context->glBindTexture(GL_TEXTURE_2D, m_outputTexture);
        mp_context->glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_width * m_devicePixelRatio, m_height * m_devicePixelRatio, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);

        // Set the render settings for the texture we've just created.
        // Essentially zero filtering on the "texture" so it appears exactly as rendered
        mp_context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        mp_context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        // Clamp the colors at the edge of our texture
        mp_context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        mp_context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // texture comparison must be enabled to use sampler2DShadow
        mp_context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        mp_context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);

        // Set m_outputTexture as the depth output of our frame buffer
        mp_context->glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_outputTexture, 0);

        GLenum drawBuffers[1] = {GL_NONE};
        mp_context->glDrawBuffers(1, drawBuffers); // No color buffer is drawn to.

        m_created = true;
        if(mp_context->glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            m_created = false;
            std::cout << "Frame buffer for shadow mapping did not initialize correctly..." << std::endl;
            mp_context->printGLErrorLog("In FrameBuffer::create() shadow mapping,");
        }
    }
    else {
        // Initialize the frame buffers and render textures
        mp_context->glGenFramebuffers(1, &m_frameBuffer);
        mp_context->glGenTextures(1, &m_outputTexture);
        mp_context->glGenRenderbuffers(1, &m_depthRenderBuffer);

        mp_context->glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);
        // Bind our texture so that all functions that deal with textures will interact with this one
        mp_context->glBindTexture(GL_TEXTURE_2D, m_outputTexture);
        // Give an empty image to OpenGL ( the last "0" )
        mp_context->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_width * m_devicePixelRatio, m_height * m_devicePixelRatio, 0, GL_RGB, GL_UNSIGNED_BYTE, (void*)0);

        // Set the render settings for the texture we've just created.
        // Essentially zero filtering on the "texture" so it appears exactly as rendered
        mp_context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        mp_context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        // Clamp the colors at the edge of our texture
        mp_context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        mp_context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Initialize our depth buffer
        mp_context->glBindRenderbuffer(GL_RENDERBUFFER, m_depthRenderBuffer);
        mp_context->glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, m_width, m_height);
        mp_context->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthRenderBuffer);

        // Set m_outputTexture as the color output of our frame buffer
        mp_context->glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_outputTexture, 0);

        // Sets the color output of the fragment shader to be stored in GL_COLOR_ATTACHMENT0,
        // which we previously set to m_outputTexture
        GLenum drawBuffers[1] = {GL_COLOR_ATTACHMENT0};
        mp_context->glDrawBuffers(1, drawBuffers); // "1" is the size of drawBuffers

        m_created = true;
        if(mp_context->glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            m_created = false;
            std::cout << "Frame buffer did not initialize correctly..." << std::endl;
            mp_context->printGLErrorLog("In FrameBuffer::create(),");
        }
    }
}

void FrameBuffer::destroy() {
    if(m_created) {
        m_created = false;
        mp_context->glDeleteFramebuffers(1, &m_frameBuffer);
        mp_context->glDeleteTextures(1, &m_outputTexture);
        mp_context->glDeleteRenderbuffers(1, &m_depthRenderBuffer);
    }
}

void FrameBuffer::bindFrameBuffer(bool useDefault) {
    // Tell OpenGL to render to the viewport's frame buffer
    mp_context->glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);
    // Render on the whole framebuffer, complete from the lower left corner to the upper right
    mp_context->glViewport(0, 0, m_width * m_devicePixelRatio, m_height * m_devicePixelRatio);

    if (useDefault) {
        mp_context->glBindFramebuffer(GL_FRAMEBUFFER, mp_context->defaultFramebufferObject());
        mp_context->glViewport(0, 0, mp_context->width() * mp_context->devicePixelRatio(),
                               mp_context->height() * mp_context->devicePixelRatio());
    }

    // Clear the screen so that we only see newly drawn images
    mp_context->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void FrameBuffer::bindToTextureSlot(unsigned int slot) {
    m_textureSlot = slot;
    mp_context->glActiveTexture(GL_TEXTURE0 + slot);
    mp_context->glBindTexture(GL_TEXTURE_2D, m_outputTexture);
}

unsigned int FrameBuffer::getTextureSlot() const {
    return m_textureSlot;
}
