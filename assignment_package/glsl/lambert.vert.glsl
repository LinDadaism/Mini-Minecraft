#version 150
// ^ Change this to version 130 if you have compatibility issues

//This is a vertex shader. While it is called a "shader" due to outdated conventions, this file
//is used to apply matrix transformations to the arrays of vertex data passed to it.
//Since this code is run on your GPU, each vertex is transformed simultaneously.
//If it were run on your CPU, each vertex would have to be processed in a FOR loop, one at a time.
//This simultaneous transformation allows your program to run much faster, especially when rendering
//geometry with millions of vertices.

uniform mat4 u_Model;       // The matrix that defines the transformation of the
                            // object we're rendering. In this assignment,
                            // this will be the result of traversing your scene graph.

uniform mat4 u_ModelInvTr;  // The inverse transpose of the model matrix.
                            // This allows us to transform the object's normals properly
                            // if the object has been non-uniformly scaled.

uniform mat4 u_ViewProj;    // The matrix that defines the camera's transformation.
                            // We've written a static matrix for you to use for HW2,
                            // but in HW3 you'll have to generate one yourself

uniform mat4 u_LightCamMVP;    // The matrix that defines the transformation of the light source
                            // that we used when creating the shadow map.

uniform vec4 u_Color;       // TODO: DELETE When drawing the cube instance, we'll set our uniform color to represent different block types.
uniform float u_Time;       // add uniform time here;
uniform float u_SunSpeed;   // The sun's moving speed

// uniform int u_Trans;

in vec4 vs_Pos;             // The array of vertex positions passed to the shader

in vec4 vs_Nor;             // The array of vertex normals passed to the shader

in vec4 vs_Col;             // The array of vertex colors passed to the shader.

in vec4 vs_UV;              // The array of vertex uv passed to the shader.

out vec4 fs_Pos;
out vec4 fs_Nor;            // The array of normals that has been transformed by u_ModelInvTr. This is implicitly passed to the fragment shader.
out vec4 fs_LightVec;       // The direction in which our virtual light lies, relative to each vertex. This is implicitly passed to the fragment shader.
out vec4 fs_Col;            // The color of each vertex. This is implicitly passed to the fragment shader.
out vec2 fs_UV;
//out float transparent;
out vec4 fs_ShadowPos;      // The array of vertex position as seen from the light source camera

const float PI = 3.14159265359;

void main()
{
    fs_Pos = vs_Pos;
    fs_Col = vs_Col;                         // Pass the vertex colors to the fragment shader for interpolation
    fs_UV = vs_UV.xy;
    fs_ShadowPos = u_LightCamMVP * vs_Pos;

    //check the UV position to tell if it is animated or not.
    if((fs_UV.x == 0.875f || fs_UV.x == 0.9375f) && (fs_UV.y == 0.1875 || fs_UV.y == 0.25) || vs_Col.w == 1){


        float time = mod(u_Time, 90) / 180.f * 3.14159265;
        float value = cos(time);

        //water waves effects
        highp int posx = int(fs_Pos.x);
        if(posx % 4 == 0){
            float value1 = cos(u_Time / 180.f * 3.14159265);
            fs_Pos.y += value1 * 0.2;
        }else if(posx % 4 == 1){
            float value1 = cos(u_Time / 180.f * 3.14159265 + 3.14159265 / 3.0);
            fs_Pos.y += value1 * 0.2;
        }else if(posx % 4 == 2){
            float value1 = cos(u_Time / 180.f * 3.14159265 + 3.14159265 / 3.0 * 2.0);
            fs_Pos.y += value1 * 0.2;
        }else if(posx % 4 == 3){
            float value1 = cos(u_Time / 180.f * 3.14159265 + 3.14159265);
            fs_Pos.y += value1 * 0.2;
        }

        if(vs_Col.y != -1){
            if(vs_Col.x == 1){
                fs_UV.x = value * 0.0625f + fs_UV.x;
            }else if(vs_Col.x == -1){
                fs_UV.x = -value * 0.0625f + fs_UV.x;
            }else{
                float value1 = cos(u_Time / 180.f * 3.14159265);
                fs_UV.x = value1 * 0.0625 + fs_UV.x;
            }
        }else{
            fs_UV.x = -value * 0.0625f + fs_UV.x;
        }

        if(vs_Col.z == 1){
            fs_UV.y = -value * 0.0625f + fs_UV.y;
        }else if(vs_Col.z == -1){
            fs_UV.y = value * 0.0625f + fs_UV.y;
        }
        fs_UV.x = clamp(fs_UV.x, 0.8125001, 1.0);
        fs_UV.y = clamp(fs_UV.y, 0.1250001, 0.3125001);
        // transparent = 1.0f;
    }

    //check if it is lava
    if((fs_UV.x == 0.875f || fs_UV.x == 0.9375f) && (fs_UV.y == 0.0625 || fs_UV.y == 0.125) || vs_Col.w == 2){
//    if((fs_UV.x - 0.875f > epsilon ||
//        fs_UV.x - 0.9375f > epsilon) &&
//            (fs_UV.y - 0.0625f > epsilon ||
//             fs_UV.y - 0.125f > epsilon)){
        //float value = cos(u_Time % 90 / 30.f);
        float time = mod(u_Time, 90) / 180.f * 3.14159265;
        float value = cos(time);
        if(vs_Col.y != -1){
            if(vs_Col.x == 1){
                fs_UV.x = value * 0.0625f + fs_UV.x;
            }else if(vs_Col.x == -1){
                fs_UV.x = -value * 0.0625f + fs_UV.x;
            }else{
                float value1 = cos(u_Time / 180.f * 3.14159265);
                fs_UV.x = value1 * 0.0625 + fs_UV.x;
            }
        }else{
            fs_UV.x = -value * 0.0625f + fs_UV.x;
        }
        if(vs_Col.z == 1){
            fs_UV.y = -value * 0.0625f + fs_UV.y;
        }else if(vs_Col.z == -1){
            fs_UV.y = value * 0.0625f + fs_UV.y;
        }
        //fs_UV.x = value * 0.0625f + fs_UV.x;
    }

    //check if it is ice
    if((fs_UV.x == 0.1875f || fs_UV.x == 0.25f) && (fs_UV.y == 0.6875f || fs_UV.y == 0.75f)){
//    if((fs_UV.x - 0.1875f > epsilon ||
//        fs_UV.x - 0.25f > epsilon) &&
//            (fs_UV.y - 0.6875f > epsilon ||
//             fs_UV.y - 0.75f > epsilon)){
        // transparent = 1.0f;
    }
//    used for testing
//    if(value > 0.5 || value < -0.5){
//        fs_UV.x = fs_UV.x + 0.0625f;
//    }
    mat3 invTranspose = mat3(u_ModelInvTr);
    fs_Nor = vec4(invTranspose * vec3(vs_Nor), 0);          // Pass the vertex normals to the fragment shader for interpolation.
                                                            // Transform the geometry's normals by the inverse transpose of the
                                                            // model matrix. This is necessary to ensure the normals remain
                                                            // perpendicular to the surface after the surface is transformed by
                                                            // the model matrix.

    //modified vs_Pos to fs_Pos
    vec4 modelposition = u_Model * fs_Pos;   // Temporarily store the transformed vertex positions for use below

    // The direction of our sun, which is used to compute the shading of
    // the geometry in the fragment shader.
    float slowerTime = u_Time == 1.6f ? u_Time-1.6f+1.f : u_Time/16.f+1.f;
    vec4 lightDir = normalize(vec4(0, cos(slowerTime * u_SunSpeed - PI/2), cos(slowerTime * u_SunSpeed), 0)); //normalize(vec4(0.5f, 1, 0.75f, 0));
    fs_LightVec = (lightDir);  // Compute the direction in which the light source lies

    gl_Position = u_ViewProj * modelposition;// gl_Position is a built-in variable of OpenGL which is
                                             // used to render the final positions of the geometry's vertices
}
