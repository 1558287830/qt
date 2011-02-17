attribute highp vec2 vPos;
attribute highp vec2 vTex;                              
attribute highp vec3 vData; //  x = time,  y = size,  z = endSize
attribute highp vec4 vVec; // x,y = constant speed,  z,w = acceleration
attribute highp vec4 vAnimData;// idx, duration, frameCount (this anim), timestamp (this anim)

uniform highp mat4 matrix;                              
uniform highp float timestamp;                          
uniform highp float timelength;
uniform lowp float opacity;
uniform highp float framecount; //maximum of all anims
uniform highp float animcount;

varying highp vec2 fTex;                                
varying lowp vec4 fColor;

void main() {                                           
    highp float size = vData.y;
    highp float endSize = vData.z;

    highp float t = (timestamp - vData.x) / timelength;

    //Calculate frame location in texture
    highp float frameIndex = fract((((timestamp - vAnimData.w)*1000.)/vAnimData.y)/vAnimData.z) * vAnimData.z;
    //fract(x/z)*z used to avoid uints and % (GLSL chokes on them?)

    frameIndex = floor(frameIndex);
    highp vec2 frameTex = vTex;
    if(vTex.x == 0.)
        frameTex.x = (frameIndex/framecount);
    else
        frameTex.x = 1. * ((frameIndex + 1.)/framecount);

    if(vTex.y == 0.)
        frameTex.y = (vAnimData.x/animcount);
    else
        frameTex.y = 1. * ((vAnimData.x + 1.)/animcount);

    fTex = frameTex;


    highp float currentSize = mix(size, endSize, t * t);

    if (t < 0. || t > 1.)
        currentSize = 0.;

    //If affector is mananging pos, they don't set speed?
    highp vec2 pos = vPos
                   - currentSize / 2. + currentSize * vTex          // adjust size
                   + vVec.xy * t * timelength         // apply speed vector..
                   + vVec.zw * pow(t * timelength, 2.);

    gl_Position = matrix * vec4(pos.x, pos.y, 0, 1);

    //gl_Position = matrix * vec4(vPos.x, vPos.y, 0, 1);

    // calculate opacity
    highp float fadeIn = min(t * 10., 1.);
    highp float fadeOut = 1. - max(0., min((t - 0.75) * 4., 1.));

    lowp vec4 white = vec4(1.);
    fColor = white * fadeIn * fadeOut * opacity;
}
