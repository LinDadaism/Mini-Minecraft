#version 150

uniform mat4 u_ViewProj;    // We're actually passing the inverse of the viewproj
                            // from our CPU, but it's named u_ViewProj so we don't
                            // have to bother rewriting our ShaderProgram class

uniform ivec2 u_Dimensions; // Screen dimensions

uniform vec3 u_Eye; // Camera position

uniform float u_Time;
uniform float u_SunSpeed;

out vec4 outColor;

const float PI = 3.14159265359;
const float TWO_PI = 6.28318530718;

// Sunset palette
//const vec3 sunset[5] = vec3[](vec3(255, 229, 119) / 255.0,
//                               vec3(254, 192, 81) / 255.0,
//                               vec3(255, 137, 103) / 255.0,
//                               vec3(253, 96, 81) / 255.0,
//                               vec3(57, 32, 51) / 255.0);
const vec3 sunset[5] = vec3[](vec3(254, 243, 210) / 255.0,
                               vec3(237, 207, 192) / 255.0,
                               vec3(195, 211, 214) / 255.0,
                               vec3(213, 228, 217) / 255.0,
                               vec3(228, 213, 227) / 255.0);
// Dusk palette
const vec3 dusk[5] = vec3[](vec3(144, 96, 144) / 255.0,
                            vec3(96, 72, 120) / 255.0,
                            vec3(72, 48, 120) / 255.0,
                            vec3(48, 24, 96) / 255.0,
                            vec3(0, 24, 72) / 255.0);

const vec3 sunColor = vec3(255, 255, 210) / 255.0;
const vec3 cloudColor = vec3(0.85);
// Daylight sky
const vec3 dayColor = vec3(133, 189, 222) / 255.0;
const float starRotSpeed = 1/10000.0;
vec3 sunDir = normalize(vec3(0, cos(u_Time * u_SunSpeed - PI/2), cos(u_Time * u_SunSpeed))); // use cos to animate the sun position, the smaller scalar sunSpeed is, the slower the sun moves


vec2 sphereToUV(vec3 p) {
    float phi = atan(p.z, p.x); // angle between the positive x-axis and p on the x-z plane
    if(phi < 0) {
        phi += TWO_PI; // convert glsl atan->[-PI, PI] to [0, 2PI]
    }
    float theta = acos(p.y); // map point's y coord to north pole ~> south pole
    return vec2(1 - phi / TWO_PI, 1 - theta / PI); // polar parameterization of our 3d point
}

vec3 uvToSunset(vec2 uv) {
    if(uv.y < 0.4) { // when the y coord is less than 0.5, return the first color
        return sunset[0];
    }
    else if(uv.y < 0.45) { // band with y in range [0.5, 0.55], sample color as a linear combination of 1st and 2nd color
        return mix(sunset[0], sunset[1], (uv.y - 0.4) / 0.05); // t = a remapping of [0.5, 0.55] to [0,1]
    }
    else if(uv.y < 0.5) {
        return mix(sunset[1], sunset[2], (uv.y - 0.45) / 0.05);
    }
    else if(uv.y < 0.55) {
        return mix(sunset[2], sunset[3], (uv.y - 0.5) / 0.05);
    }
    else if(uv.y < 0.65) {
        return mix(sunset[3], sunset[4], (uv.y - 0.55) / 0.1);
    }
    return sunset[4];
}

vec3 uvToDusk(vec2 uv) {
    if(uv.y < 0.4) {
        return dusk[0];
    }
    else if(uv.y < 0.45) {
        return mix(dusk[0], dusk[1], (uv.y - 0.4) / 0.05);
    }
    else if(uv.y < 0.5) {
        return mix(dusk[1], dusk[2], (uv.y - 0.45) / 0.05);
    }
    else if(uv.y < 0.55) {
        return mix(dusk[2], dusk[3], (uv.y - 0.5) / 0.05);
    }
    else if(uv.y < 0.65) {
        return mix(dusk[3], dusk[4], (uv.y - 0.55) / 0.1);
    }
    return dusk[4];
}

vec2 random2( vec2 p ) {
    return fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))))*43758.5453);
}

vec3 random3( vec3 p ) {
    return fract(sin(vec3(dot(p,vec3(127.1, 311.7, 191.999)),
                          dot(p,vec3(269.5, 183.3, 765.54)),
                          dot(p, vec3(420.69, 631.2,109.21))))
                 *43758.5453);
}

float WorleyNoise3D(vec3 p)
{
    // Tile the space
    vec3 pointInt = floor(p);
    vec3 pointFract = fract(p);

    float minDist = 1.0; // Minimum distance initialized to max.

    // Search all neighboring cells and this cell for their point
    for(int z = -1; z <= 1; z++)
    {
        for(int y = -1; y <= 1; y++)
        {
            for(int x = -1; x <= 1; x++)
            {
                vec3 neighbor = vec3(float(x), float(y), float(z));

                // Random point inside current neighboring cell
                vec3 point = random3(pointInt + neighbor);

                // Animate the point
                point = 0.5 + 0.5 * sin(u_Time * 0.01 + 6.2831 * point); // 0 to 1 range

                // Compute the distance b/t the point and the fragment
                // Store the min dist thus far
                vec3 diff = neighbor + point - pointFract;
                float dist = length(diff);
                minDist = min(minDist, dist);
            }
        }
    }
    return minDist;
}

float WorleyNoise(vec2 uv)
{
    // Tile the space
    vec2 uvInt = floor(uv);
    vec2 uvFract = fract(uv);

    float minDist = 1.0; // Minimum distance initialized to max.

    // Search all neighboring cells and this cell for their point
    for(int y = -1; y <= 1; y++)
    {
        for(int x = -1; x <= 1; x++)
        {
            vec2 neighbor = vec2(float(x), float(y));

            // Random point inside current neighboring cell
            vec2 point = random2(uvInt + neighbor);

            // Animate the point
            point = 0.5 + 0.5 * sin(u_Time * 0.01 + 6.2831 * point); // 0 to 1 range

            // Compute the distance b/t the point and the fragment
            // Store the min dist thus far
            vec2 diff = neighbor + point - uvFract;
            float dist = length(diff);
            minDist = min(minDist, dist);
        }
    }
    return minDist;
}

float worleyFBM(vec3 uv) {
    float sum = 0;
    float freq = 4;
    float amp = 0.5;
    for(int i = 0; i < 8; i++) {
        sum += WorleyNoise3D(uv * freq) * amp;
        freq *= 2;
        amp *= 0.5;
    }
    return sum;
}

float noise1D(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) *
                 43758.5453);
}


///-------------stars----------------///
// Controls how many layers of stars
#define LAYERS            4.0

#define ROT(a)            mat2(cos(a), sin(a), -sin(a), cos(a))
#define PCOS(x)           (0.5 + 0.5*cos(x))
#define TTIME             (TWO_PI*u_Time)

// https://stackoverflow.com/questions/15095909/from-rgb-to-hsv-in-opengl-glsl
vec3 hsv2rgb(vec3 c) {
  const vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
  vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
  return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

// http://mercury.sexy/hg_sdf/
vec2 mod2(inout vec2 p, vec2 size) {
  vec2 c = floor((p + size*0.5)/size);
  p = mod(p + size*0.5,size) - size*0.5;
  return c;
}

vec2 hash2(vec2 p) {
  p = vec2(dot (p, vec2 (127.1, 311.7)), dot (p, vec2 (269.5, 183.3)));
  return fract(sin(p)*43758.5453123);
}

vec3 toSpherical(vec3 p) {
  float r   = length(p);
  float t   = acos(p.z/r);
  float ph  = atan(p.y, p.x);
  return vec3(r, t, ph);
}

vec3 postProcess(vec3 col, vec2 q)  {
  col=pow(clamp(col,0.0,1.0),vec3(1.0/2.2));
  col=col*0.6+0.4*col*col*(3.0-2.0*col);  // contrast
  col=mix(col, vec3(dot(col, vec3(0.33))), -0.4);  // satuation
  col*=0.5+0.5*pow(19.0*q.x*q.y*(1.0-q.x)*(1.0-q.y),0.7);  // vigneting
  return col;
}

vec3 stars(vec3 ro, vec3 rd) {
  vec3 col = vec3(0.0);
  vec3 srd = toSpherical(rd.xzy);

  for (float i = 0.0; i < LAYERS; ++i) {
    vec2 pp = srd.yz+0.5*i;
    float s = i/(LAYERS-1.0);
    vec2 dim  = vec2(mix(0.05, 0.003, s)*PI);
    vec2 np = mod2(pp, dim);
    vec2 h = hash2(np+127.0+i);
    vec2 o = -1.0+2.0*h;
    float y = sin(srd.y);
    pp += o*dim*0.5;
    pp.y *= y;
    float l = length(pp);

    float h1 = fract(h.x*109.0);
    float h2 = fract(h.x*113.0);
    float h3 = fract(h.x*127.0);

    vec3 hsv = vec3(fract(0.025-0.4*h1*h1), mix(0.5, 0.125, s), 1.0);
    vec3 scol = mix(8.0*h2, 0.25*h2*h2, s)*hsv2rgb(hsv);

    vec3 ccol = col+ exp(-(2000.0/mix(2.0, 0.25, s))*max(l-0.001, 0.0))*scol;
    col = h3 < y ? ccol : col;
  }

  return col;
}

vec3 starField() {
    vec2 q = gl_FragCoord.xy / vec2(u_Dimensions);
    vec2 p = -1.0 + 2.0*q;
    p.x *= u_Dimensions.x/u_Dimensions.y;

      vec3 ro = mix(0.5, 0.25, TTIME)*vec3(2.0, 0, 0.2)+vec3(0.0, -0.125, 0.0);
//      ro.yx *= ROT(TTIME/120.0*sqrt(0.5));
      ro.xz *= ROT(TTIME * starRotSpeed);

      vec3 ww = normalize(vec3(0.0)-ro);
      vec3 uu = normalize(cross( vec3(0.0,1.0,0.0), ww));
      vec3 vv = normalize(cross(ww,uu));
      const float rdd = 1.0;//2.0;
      vec3 rd = normalize(p.x*uu + p.y*vv + rdd*ww);

      vec3 col = stars(ro, rd);

      return postProcess(col, q);
}
///--------------stars----------------///

#define DARK_THRESHOLD 73.61678
//http://alienryderflex.com/hsp.html
bool isDarkColor(vec3 col) {
    col *= 255.0;
    float hsp = sqrt(0.299 * col.r * col.r + 0.587 * col.g * col.g + 0.114 * col.b * col.b);
    return hsp < DARK_THRESHOLD;
}


///----------------shooting star----------------///
//besselham line function
//creates an oriented distance field from o to b and then applies a curve with smoothstep to sharpen it into a line
//p = point field, o = origin, b = bound, sw = StartingWidth, ew = EndingWidth,
float shootingStarLine(vec2 p, vec2 o, vec2 b, float sw, float ew){
        float d = distance(o, b);
        vec2  n = normalize(b - o);
        vec2 l = vec2(max(abs(dot(p - o, n.yx * vec2(-1.0, 1.0))), 0.0),
                      max(abs(dot(p - o, n) - d * 0.5) - d * 0.5, 0.0));
        return smoothstep(mix(sw, ew, 1.-distance(b,p)/d) , 0., l.x+l.y);
}

vec3 comet(vec2 p)
{
    const float modu = 4.;        // Period: 4 | 8 | 16
    const float endPointY = -.1; // Hide point / Punto disparizione Y
    vec2 cmtVel = mod(u_Time/modu+modu*.5, 2.) > 1. ? vec2(2., 1.4)*.3 : vec2(-2., 1.4)*.2;  // Speed component X,Y
    vec2 cmtLen = cmtVel; //cmt length

    vec2 cmtPt = 1. - mod(u_Time*cmtVel, modu);
    cmtPt.x += 1.;

    vec2 cmtStartPt, cmtEndPt;

    if(cmtPt.y < endPointY) {
        cmtEndPt = cmtPt + cmtLen*2;
        if(cmtEndPt.y > endPointY) {
            cmtStartPt = vec2(cmtPt.x + cmtLen.x*((endPointY - cmtPt.y)/cmtLen.y), endPointY);
        }
        else {
            return vec3(.0);
        }
    }
    else {
        cmtStartPt = cmtPt;
        cmtEndPt = cmtStartPt+cmtLen*2;
    }

    float bright = clamp(smoothstep(-.2,.65,distance(cmtStartPt, cmtEndPt)),0.,1.);

    vec2 dlt = vec2(.003) * cmtVel;

    float q = clamp((p.y+.2)*2., 0., 1.);

    return  (bright * .75 * (smoothstep(0.993, 0.999, 1. - length(p - cmtStartPt)) +
             shootingStarLine(p, cmtStartPt, cmtStartPt+vec2(.06)*cmtVel, 0.009, 0.003)) +
             vec3(1., .7, .2) * .33 * shootingStarLine(p, cmtStartPt,         cmtEndPt,        0.006, .0006) +
             vec3(1., .5, .1) * .33 * shootingStarLine(p, cmtStartPt+dlt,     cmtEndPt+dlt*2., 0.004, .0004) +
             vec3(1., .3, .0) * .33 * shootingStarLine(p, cmtStartPt+dlt+dlt, cmtEndPt+dlt*4., 0.002, .0002)) * bright * q;
}
///----------------shooting star----------------///

///-----------------cloud-----------------------///
float noise(vec2 p) {
  return fract(sin(p.x * 83.876 + p.y * 76.123) * 3853.875);
}

float perlin(vec2 uv, float iters) {
  float c = 1.0;
  for (float i = 0.0; i < iters; i++) {
    float p = pow(2.0, i + 1.0);
    vec2 luv = uv * vec2(p) + u_Time * 0.05;
    vec2 gv = smoothstep(0.0, 1.0, fract(luv));
    vec2 id = floor(luv);
    float lb = noise(id + vec2(0.0, 0.0));
    float rb = noise(id + vec2(1.0, 0.0));
    float lt = noise(id + vec2(0.0, 1.0));
    float rt = noise(id + vec2(1.0, 1.0));
    float b = mix(lb, rb, gv.x);
    float t = mix(lt, rt, gv.x);
    c += 1.0 / p * mix(b, t, gv.y);
  }
  return c / 2.0;
}
///-----------------cloud-----------------------///


//#define RAY_AS_COLOR
//#define SPHERE_UV_AS_COLOR
#define WORLEY_OFFSET

#define COMET
#define STARS
#define CLOUDS

void main()
{
    // Raytracing
    vec2 ndc = (gl_FragCoord.xy / vec2(u_Dimensions)) * 2.0 - 1.0; // -1 to 1 NDC
    vec4 p = vec4(ndc.xy, 1, 1); // Pixel at the far clip plane
    p *= 1000.0; // Times far clip plane value
    p = /*Inverse of*/ u_ViewProj * p; // Convert from unhomogenized screen to world

    vec3 rayDir = normalize(p.xyz - u_Eye);

#ifdef RAY_AS_COLOR
    outColor = vec4(0.5 * (rayDir + vec3(1,1,1)), 1);
    return;
#endif

    vec2 uv = sphereToUV(rayDir);
#ifdef SPHERE_UV_AS_COLOR
    outColor = vec4(uv, 0, 1);
    return;
#endif


    vec2 offset = vec2(0.0);
#ifdef WORLEY_OFFSET
    // Get a noise value in the range [-1, 1]
    // by using Worley noise as the noise basis of FBM
    offset = vec2(worleyFBM(rayDir));
    offset *= 2.0;
    offset -= vec2(1.0);
#endif

    // Compute a gradient from the bottom of the sky-sphere to the top
    vec3 sunsetColor = uvToSunset(uv + offset * 0.1);
    vec3 duskColor = uvToDusk(uv + offset * 0.1);

    // Add a glowing sun in the sky
    float sunSize = 20; // including the halo
    float raySunDot = dot(rayDir, sunDir);
    float angle = acos(raySunDot) * 360.0 / PI; // angle in degrees between the sun direction and every ray from the camera

    // Our ray is looking into just the sky
    vec3 skyCol = dayColor;

#define DAYLIGHT_THRESHOLD 0.2
#define SUNSET_THRESHOLD 0.1
#define DUSK_THRESHOLD -0.3
#define SUNRISE_THRESHOLD -0.1
    if(sunDir.y > DAYLIGHT_THRESHOLD) {
        // Draw sky in day light time
#ifdef CLOUDS
        vec2 uv = (gl_FragCoord.xy - 0.5 * vec2(u_Dimensions)) / vec2(u_Dimensions).y;
        uv *= 5.0;
        uv += 33.0;

        float j = sin(2.0) * 0.1 + 0.7;
        float distCloud = smoothstep(j, j + 0.1, perlin(uv, 8.0));
        skyCol = mix(dayColor, cloudColor, distCloud);
#endif
        outColor = vec4(skyCol, 1);
    }
    else {
        // Generate star field
        vec3 starCol = vec3(0.0);
#ifdef STARS
        starCol = starField();
#endif

        // Shooting star
        vec3 cometA = vec3(0.0);
#ifdef COMET
        float limit = noise1D(vec2(u_Time)); // limited chance of appearance
        if (limit < 0.1) {
            cometA = comet(ndc * (u_Dimensions / min(u_Dimensions.x, u_Dimensions.y)));
        }
#endif

        // Any y coord between 0.2 and -0.3 is a LERP b/t sunset and dusk color
        if(sunDir.y > DUSK_THRESHOLD) {
            float t = (sunDir.y - DUSK_THRESHOLD) / (DAYLIGHT_THRESHOLD - DUSK_THRESHOLD);
            skyCol = mix(duskColor, skyCol, t);
        }
        // Any y coord <= -0.3 are pure dusk color
        else {
            skyCol = duskColor;
        }

        skyCol = isDarkColor(skyCol)? skyCol + starCol + cometA : skyCol;
        outColor = vec4(skyCol, 1);
    }

    // if sun is near the horizon (+-0.1 along y, assuming horizon has y=0) -> sunset/sunrise blend
    // sunset
    if (sunDir.y < SUNSET_THRESHOLD && sunDir.y > 0.001f && sunDir.z < 0.001f) {
        float t1 = (sunDir.y - SUNSET_THRESHOLD) / (-SUNSET_THRESHOLD);
        skyCol = mix(skyCol, mix(sunsetColor, duskColor, t1), t1);
        outColor = vec4(skyCol, 1);
    }
    // sunrise
    if (sunDir.y > SUNRISE_THRESHOLD && sunDir.y < 0.001f && sunDir.z > 0.001f) {
        float t1 = (sunDir.y - SUNRISE_THRESHOLD) / (-SUNRISE_THRESHOLD);
        skyCol = mix(mix(duskColor, sunsetColor, t1), skyCol, t1);
        outColor = vec4(skyCol, 1);
    }

//    if (abs(sunDir.y) < SUNRISE_THRESHOLD) {
//        float t1 = (abs(sunDir.y) - SUNRISE_THRESHOLD) / (-SUNRISE_THRESHOLD);
//        //loat t2 = (raySunDot - SUNSET_THRESHOLD) / (DUSK_THRESHOLD - SUNSET_THRESHOLD);
//        // sunrise
//        if (sunDir.y < 0 && sunDir.z < 0) { // assume horizon has y = 0
//            skyCol = mix(mix(duskColor, sunsetColor, t1), dayColor, t1);
//        }
//        // sunset
//        else if (sunDir.y > 0 && sunDir.z > 0){
//            skyCol = mix(skyCol, mix(sunsetColor, duskColor, t1), t1);
//        }
//        outColor = vec4(skyCol, 1);
//    }

    // If the angle between our ray dir and vector to center of sun
    // is less than the threshold, then we're looking at the sun
    if(angle < sunSize) {
        // Full center of sun
        if(angle < 10) {
            outColor = vec4(sunColor, 1);
        }
        // Corona of sun, mix with sky color
        else {
            outColor = vec4(mix(sunColor, skyCol, (angle - 10) / 10), 1); // remap t to [0,1]
        }
    }
}
