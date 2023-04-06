#version 150
// ^ Change this to version 130 if you have compatibility issues

// This is a fragment shader. If you've opened this file first, please
// open and read lambert.vert.glsl before reading on.
// Unlike the vertex shader, the fragment shader actually does compute
// the shading of geometry. For every pixel in your program's output
// screen, the fragment shader is run for every bit of geometry that
// particular pixel overlaps. By implicitly interpolating the position
// data passed into the fragment shader by the vertex shader, the fragment shader
// can compute what color to apply to its pixel based on things like vertex
// position, light position, and vertex color.

// uniform vec4 u_Color; // The color with which to render this instance of geometry.
uniform sampler2D u_Texture; // The texture
uniform sampler2DShadow u_ShadowMapTex; // The shadow map texture, 3D sampling
//uniform sampler2D u_ShadowMapTex; // The shadow map texture, used for 2D sampling
uniform vec3 u_Eye; // The camera position
uniform ivec2 u_Dimensions; // Screen dimensions

uniform int u_transparent;

// These are the interpolated values out of the rasterizer, so you can't know
// their specific values without knowing the vertices that contributed to them
in vec4 fs_Pos;
in vec4 fs_Nor;
in vec4 fs_LightVec;
in vec4 fs_Col;
in vec2 fs_UV;
//in float transparent;
in vec4 fs_ShadowPos;

out vec4 out_Col; // This is the final output color that you will see on your
                  // screen for the pixel that is currently being processed.

const vec3 dayColor = vec3(204, 235, 250) / 255.0; // Sync with dayColor in sky.frag.glsl
const vec3 duskColor = vec3(144, 96, 144) / 255.0; // Sync with duskColor in sky.frag.glsl

const float FogMax = 190.0; // NEED to adjust with respect to the scale of terrain being rendered
const float FogMin = 140.0; // The minimum and muximum distances within which the fog gradient exists.
                            // I.e., beyond the minimum distance the geometry will be totally visible,
                            // while beyond the maximum distance the geometry will not be visible anymore.

const vec3 fogColor = vec3(0.5); // Default fog color to blend with terrain using LERP

// For soft shadow
const float kernel[121] = float[121](
0.006849, 0.007239, 0.007559, 0.007795, 0.007941, 0.00799, 0.007941, 0.007795, 0.007559, 0.007239, 0.006849,
0.007239, 0.007653, 0.00799, 0.00824, 0.008394, 0.008446, 0.008394, 0.00824, 0.00799, 0.007653, 0.007239,
0.007559, 0.00799, 0.008342, 0.008604, 0.008764, 0.008819, 0.008764, 0.008604, 0.008342, 0.00799, 0.007559,
0.007795, 0.00824, 0.008604, 0.008873, 0.009039, 0.009095, 0.009039, 0.008873, 0.008604, 0.00824, 0.007795,
0.007941, 0.008394, 0.008764, 0.009039, 0.009208, 0.009265, 0.009208, 0.009039, 0.008764, 0.008394, 0.007941,
0.00799, 0.008446, 0.008819, 0.009095, 0.009265, 0.009322, 0.009265, 0.009095, 0.008819, 0.008446, 0.00799,
0.007941, 0.008394, 0.008764, 0.009039, 0.009208, 0.009265, 0.009208, 0.009039, 0.008764, 0.008394, 0.007941,
0.007795, 0.00824, 0.008604, 0.008873, 0.009039, 0.009095, 0.009039, 0.008873, 0.008604, 0.00824, 0.007795,
0.007559, 0.00799, 0.008342, 0.008604, 0.008764, 0.008819, 0.008764, 0.008604, 0.008342, 0.00799, 0.007559,
0.007239, 0.007653, 0.00799, 0.00824, 0.008394, 0.008446, 0.008394, 0.00824, 0.00799, 0.007653, 0.007239,
0.006849, 0.007239, 0.007559, 0.007795, 0.007941, 0.00799, 0.007941, 0.007795, 0.007559, 0.007239, 0.006849
);

const vec2 poissonDisk[16] = vec2[](
   vec2( -0.94201624, -0.39906216 ),
   vec2( 0.94558609, -0.76890725 ),
   vec2( -0.094184101, -0.92938870 ),
   vec2( 0.34495938, 0.29387760 ),
   vec2( -0.91588581, 0.45771432 ),
   vec2( -0.81544232, -0.87912464 ),
   vec2( -0.38277543, 0.27676845 ),
   vec2( 0.97484398, 0.75648379 ),
   vec2( 0.44323325, -0.97511554 ),
   vec2( 0.53742981, -0.47373420 ),
   vec2( -0.26496911, -0.41893023 ),
   vec2( 0.79197514, 0.19090188 ),
   vec2( -0.24188840, 0.99706507 ),
   vec2( -0.81409955, 0.91437590 ),
   vec2( 0.19984126, 0.78641367 ),
   vec2( 0.14383161, -0.14100790 )
);

// Returns a random number based on a vec3 and an int.
float random(vec3 seed, int i){
        vec4 seed4 = vec4(seed,i);
        float dot_product = dot(seed4, vec4(12.9898,78.233,45.164,94.673));
        return fract(sin(dot_product) * 43758.5453);
}

float random1(vec3 p) {
    return fract(sin(dot(p,vec3(127.1, 311.7, 191.999)))
                 *43758.5453);
}

float mySmoothStep(float a, float b, float t) {
    t = smoothstep(0, 1, t);
    return mix(a, b, t);
}

//float cubicTriMix(vec3 p) {
//    vec3 pFract = fract(p);
//    float llb = random1(floor(p) + vec3(0,0,0));
//    float lrb = random1(floor(p) + vec3(1,0,0));
//    float ulb = random1(floor(p) + vec3(0,1,0));
//    float urb = random1(floor(p) + vec3(1,1,0));

//    float llf = random1(floor(p) + vec3(0,0,1));
//    float lrf = random1(floor(p) + vec3(1,0,1));
//    float ulf = random1(floor(p) + vec3(0,1,1));
//    float urf = random1(floor(p) + vec3(1,1,1));

//    float mixLoBack = mySmoothStep(llb, lrb, pFract.x);
//    float mixHiBack = mySmoothStep(ulb, urb, pFract.x);
//    float mixLoFront = mySmoothStep(llf, lrf, pFract.x);
//    float mixHiFront = mySmoothStep(ulf, urf, pFract.x);

//    float mixLo = mySmoothStep(mixLoBack, mixLoFront, pFract.z);
//    float mixHi = mySmoothStep(mixHiBack, mixHiFront, pFract.z);

//    return mySmoothStep(mixLo, mixHi, pFract.y);
//}

//float fbm(vec3 p) {
//    float amp = 0.5;
//    float freq = 4.0;
//    float sum = 0.0;
//    for(int i = 0; i < 8; i++) {
//        sum += cubicTriMix(p * freq) * amp;
//        amp *= 0.5;
//        freq *= 2.0;
//    }
//    return sum;
//}

float fogFactor(float d) {
    if (d > FogMax) return 1.0;
    if (d < FogMin) return 0.0;

    return 1.0 - (FogMax - d) / (FogMax - FogMin);
}

//float gaussianBlur()
//{
//    vec3 blurredColor = vec3(0);
//    float stepX = 1.f / u_Dimensions.x;
//    float stepY = 1.f/ u_Dimensions.y;

//    for (int i = 0; i < 11; i++) // row
//    {
//        for (int j = 0; j < 11; j++) // col
//        {
//            vec2 pixelUV = vec2(fs_ShadowPos.x + (-5+j) * stepX, fs_ShadowPos.y - (-5+i) * stepY);
//            vec4 pixelColor = texture(u_ShadowMapTex, pixelUV);
//            blurredColor += kernel[11*i+j] * pixelColor.rgb;
//        }
//    }

//    return blurredColor.r;
//}


#define FOG
#define SHADOW
void main()
{
        // Material base color (before shading)
        vec4 diffuseColor = texture(u_Texture, fs_UV);

        // Calculate the diffuse term for Lambert shading
        vec3 diffuseTerm = vec3(dot(normalize(fs_Nor), normalize(fs_LightVec)));
        // Avoid negative lighting values
        diffuseTerm = clamp(diffuseTerm, 0, 1) * dayColor; // blend in daylight color

        vec3 ambientTerm = vec3(0.5) * duskColor; // blend in night dusk

        vec3 lightIntensity = diffuseTerm + ambientTerm;   //Add a small float value to the color multiplier
                                                            //to simulate ambient lighting. This ensures that faces that are not
                                                            //lit by our point light are not completely black.
        vec3 texColor = diffuseColor.rgb * lightIntensity;

        // Distance fog
        float alpha = 0;
        vec4 fog = vec4(texColor, 1);
#ifdef FOG
        float d = distance(u_Eye, fs_Pos.xyz);
        alpha = fogFactor(d);
        fog = vec4(fogColor, 1-alpha);
#endif

        // Shadow mapping
#define BIAS 0.005
        float visibility = 1.0;
#ifdef SHADOW
        vec3 N = normalize(fs_Nor.xyz);
        vec3 L = normalize(fs_LightVec.xyz);
        float cosTheta = clamp(dot(N, L), 0, 1);
        float bias = BIAS * tan(acos(cosTheta));
        bias = clamp(bias, 0, 0.01);

        // Sample the shadow map 4 times
        for (int i = 0; i < 4; i++){
                // use either :
                //  - Always the same samples.
                //    Gives a fixed pattern in the shadow, but no noise
                int index = i;
                //  - A random sample, based on the pixel's screen location.
                //    No banding, but the shadow moves with the camera, which looks weird.
                // int index = int(16.0*random(gl_FragCoord.xyy, i))%16;
                //  - A random sample, based on the pixel's position in world space.
                //    The position is rounded to the millimeter to avoid too much aliasing
                //int index = int(16.0*random(floor(fs_Pos.xyz*1000.0), i))%16;

                visibility -= 0.1*(1-texture(u_ShadowMapTex, vec3(fs_ShadowPos.xy + poissonDisk[index]/700.0, (fs_ShadowPos.z-bias)/fs_ShadowPos.w)));
        }

        // Below requires shadow map as a Sampler2D
//        float lightDepth = texture(u_ShadowMapTex, fs_ShadowPos.xy).r;

//        float lightDepth = gaussianBlur();
//        if (lightDepth < fs_ShadowPos.z - bias) {
//            visibility -= 0.2*(1-lightDepth/100);
//        }
#endif

        // Compute final shaded color
        vec4 c = vec4(1.0);
        if(u_transparent > 0){
            if(diffuseColor.w < 0.01){
                discard;
            }
            c = mix(vec4(texColor, diffuseColor.w), fog, alpha); // mix = texColor*(1-alpha)+fog*alpha
        }else{
            c = mix(vec4(texColor, 1), fog, alpha);

        }
        out_Col = vec4(visibility * c.rgb, c.a);
}
