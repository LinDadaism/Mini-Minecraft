#version 150

// A fragment shader used for post-processing that simply reads the
// image produced in the first render pass by the surface shader
// and outputs it to the frame buffer

/** Warning:
 * This noOp shader is currently NOT IN USE unless
 * you want to use m_progPostprocessNoOp in mygl.
 **/
in vec2 fs_UV;

out vec4 out_Col;

uniform sampler2D u_RenderedTexture;
uniform int u_Time;

void main()
{
    vec3 diffuseColor = texture(u_RenderedTexture, fs_UV).rgb;
    out_Col = vec4(diffuseColor, 1);
}
