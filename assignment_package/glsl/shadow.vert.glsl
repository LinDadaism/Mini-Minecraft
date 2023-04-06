#version 150

uniform mat4 u_LightCamMVP; // The matrix that defines the vertex transformation from the light's point of view.

in vec4 vs_Pos;

void main()
{
    gl_Position = u_LightCamMVP * vs_Pos;
}
