#version 450

#extension GL_GOOGLE_include_directive: require

#include "../common/commonLightFunctions.glsl"

precision highp float;

layout(location=0) in vec3 vPosition;
layout(location=1) in vec2 vTextCoords;



layout(location=0) out vec4 outputColor;

layout(set=1,binding=5) uniform sampler2D uTexture;

//Individual meshes
layout(std140, binding=4) uniform meshWideBlockPS {
    //ivec4 waterTypeV;
    vec4 color;
};

void main() {
//    int waterType = int(waterTypeV.x);
//    if (waterType == 13) { // LIQUID_WMO_Water
//        outputColor = vec4(0.0, 0, 0.3, 0.5);
//    } else if (waterType == 14) { //LIQUID_WMO_Ocean
//        outputColor = vec4(0, 0, 0.8, 0.8);
//    } else if (waterType == 19) { //LIQUID_WMO_Magma
//        outputColor = vec4(0.3, 0, 0, 0.5);
//    } else if (waterType == 20) { //LIQUID_WMO_Slime
//        outputColor = vec4(0.0, 0.5, 0, 0.5);
//    } else {
//        outputColor = vec4(0.5, 0.5, 0.5, 0.5);
//    }

    outputColor = vec4(color.rgb*texture(uTexture, vTextCoords).rgb, 0.7);
}
