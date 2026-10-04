#version 450

#extension GL_GOOGLE_include_directive: require

precision highp float;
precision highp int;

#include "../common/commonLightFunctions.glsl"
#include "../common/commonFogFunctions.glsl"

layout(location = 0) in vec2 vChunkCoords;
layout(location = 1) in vec3 vPosition;
layout(location = 2) in vec4 vColor;
layout(location = 3) in vec3 vNormal;
layout(location = 4) in vec3 vVertexLighting;

layout(set=1, binding=5) uniform sampler2D uLayer0;
layout(set=1, binding=6) uniform sampler2D uLayer1;
layout(set=1, binding=7) uniform sampler2D uLayer2;
layout(set=1, binding=8) uniform sampler2D uLayer3;
layout(set=1, binding=9) uniform sampler2D uLayer4;
layout(set=1, binding=10) uniform sampler2D uLayer5;
layout(set=1, binding=11) uniform sampler2D uLayer6;
layout(set=1, binding=12) uniform sampler2D uLayer7;
layout(set=1, binding=13) uniform sampler2D uAlphaTexture;
layout(set=1, binding=14) uniform sampler2D uAlphaTexture2;

layout(std140, set=0, binding=0) uniform sceneWideBlockVSPS {
    SceneWideParams scene;
    PSFog fogData;
};

layout(std140, set=0, binding=3) uniform modelWideBlockPS {
    ivec4 uUseHeightMixFormula;

};

layout(std140, set=0, binding=4) uniform meshWideBlockPS {
    vec4 uHeightScale[2];
    vec4 uHeightOffset[2];
    mat4 animationMat[8];
};

layout(location = 0) out vec4 outColor;

const InteriorLightParam intLight = {
    vec4(0,0,0,0),
    vec4(0,0,0,1)
};

vec4 mixTextures(vec4 tex0, vec4 tex1, float alpha) {
    return  (alpha*(tex1-tex0)+tex0);
}

void main() {
    vec2 vTexCoord = vChunkCoords;
    const float threshold = 1.5;

    vec2 alphaCoord = vec2(vChunkCoords.x/8.0, vChunkCoords.y/8.0 );
    vec3 alphaBlend = texture(uAlphaTexture, alphaCoord).gba;
    vec4 alphaBlend2 = texture(uAlphaTexture2, alphaCoord).rgba;

    vec4 aTexCoord = vec4(vTexCoord, 0, 1);
    vec2 tcLayer[8];
    for (int i = 0; i < 8; i++) {
        tcLayer[i] = (animationMat[i] * aTexCoord).xy;
    }

    float layerAlpha[8];
    layerAlpha[0] = 0.0;
    layerAlpha[1] = alphaBlend.r;
    layerAlpha[2] = alphaBlend.g;
    layerAlpha[3] = alphaBlend.b;
    layerAlpha[4] = alphaBlend2.r;
    layerAlpha[5] = alphaBlend2.g;
    layerAlpha[6] = alphaBlend2.b;
    layerAlpha[7] = alphaBlend2.a;

    vec4 layerColor[8];
    layerColor[0] = texture(uLayer0, tcLayer[0]);
    layerColor[1] = texture(uLayer1, tcLayer[1]);
    layerColor[2] = texture(uLayer2, tcLayer[2]);
    layerColor[3] = texture(uLayer3, tcLayer[3]);
    layerColor[4] = texture(uLayer4, tcLayer[4]);
    layerColor[5] = texture(uLayer5, tcLayer[5]);
    layerColor[6] = texture(uLayer6, tcLayer[6]);
    layerColor[7] = texture(uLayer7, tcLayer[7]);

    vec4 final;
    if (uUseHeightMixFormula.r > 0) {
        float alphaSum = alphaBlend.r + alphaBlend.g + alphaBlend.b + alphaBlend2.r + alphaBlend2.g + alphaBlend2.b + alphaBlend2.a;
        layerAlpha[0] = 1.0 - clamp(alphaSum, 0.0, 1.0);

        // compared to old shader we moved to loops here like wow.export does
        float maxWeight = 0.0;
        float weights[8];
        for (int i = 0; i < 8; i++) {
            float heightScale = uHeightScale[i / 4][i % 4];
            float heightOffset = uHeightOffset[i / 4][i % 4];
            weights[i] = layerAlpha[i] * (layerColor[i].a * heightScale + heightOffset);
            maxWeight = max(maxWeight, weights[i]);
        }

        float weightSum = 0.0;
        for (int i = 0; i < 8; i++) {
            weights[i] *= 1.0 - clamp(maxWeight - weights[i], 0.0, 1.0);
            weightSum += weights[i];
        }

        // sidenote here compared to previous shader, we don't have specular channels in the alpha channel anymore because i decided to just bind height textures for now to stay under the webgl2 texture limit
        final = vec4(0.0);
        for (int i = 0; i < 8; i++) {
            final.rgb += layerColor[i].rgb * (weights[i] / weightSum);
        }
    } else {
        final = layerColor[0];
        for (int i = 1; i < 8; i++) {
            final = mixTextures(final, layerColor[i], layerAlpha[i]);
        }
    }

    vec3 matDiffuse = final.rgb * 2.0 * vColor.rgb;

    vec4 finalColor = vec4(
        calcLight(
            matDiffuse,
            vNormal,
            true,
            0.0,
            scene,
            intLight,
            vVertexLighting.rgb, /* accumLight */
            vec3(0.0), /*precomputedLight*/
            vec3(0.0), /* specular */
            vec3(0.0) /* emissive */
        ),
        1.0
    );

    //Spec part
    float specBlend = final.a;
    vec3 halfVec = -(normalize((scene.extLight.uExteriorDirectColorDir.xyz + normalize(vPosition))));
    vec3 lSpecular = ((scene.extLight.uExteriorDirectColor.xyz * pow(max(0.0, dot(halfVec, vNormal)), 20.0)));
    vec3 specTerm = (vec3(specBlend) * lSpecular) * scene.extLight.adtSpecMult.x;
    finalColor.rgb += specTerm;

    finalColor = makeFog(fogData, finalColor, vPosition.xyz, scene.extLight.uExteriorDirectColorDir.xyz, 0);

    finalColor.a = 1.0;
    outColor = finalColor;
}
