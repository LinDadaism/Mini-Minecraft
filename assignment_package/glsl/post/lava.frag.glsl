#version 150

// A fragment shader used for post-processing that simply reads the
// image produced in the first render pass by the surface shader,
// applies a red overlay with 30% opacity, and outputs it to the frame buffer

in vec2 fs_UV;

out vec4 out_Col;

uniform sampler2D u_RenderedTexture;

uniform float u_Time;

void main()
{
    float time = u_Time;
    vec2 vs_UV = vec2(fs_UV.x,fs_UV.y+0.01f*sin(0.03141592f*time*2+fs_UV.x*5.f));
    vec3 baseColor = texture(u_RenderedTexture, vs_UV).rgb;
    vec3 overlay = vec3(1.f, 0, 0);
    out_Col = vec4(baseColor + overlay, 0.3f);
}
