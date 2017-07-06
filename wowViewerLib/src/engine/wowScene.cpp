#include "opengl/header.h"
#include "wowScene.h"
#include "shaders.h"
#include "shader/ShaderRuntimeData.h"
#include <mathfu/glsl_mappings.h>
#include <iostream>

WoWSceneImpl::WoWSceneImpl(Config *config, IFileRequest * requestProcessor, int canvWidth, int canvHeight)
        : wmoMainCache(requestProcessor), wmoGeomCache(requestProcessor), m2GeomCache(requestProcessor), skinGeomCache(requestProcessor), textureCache(requestProcessor)  {
    constexpr const shaderDefinition *definition = getShaderDef("adtShader");
//    constexpr const int attributeIndex = getShaderAttribute("m2Shader", "aNormal");
//    constexpr const int attributeIndex = +m2Shader::Attribute::aNormal;
//    constexpr const shaderDefinition *definition = getShaderDef("readDepthBuffer");
//    std::cout << "aHeight = " << definition->shaderString<< std::flush;
    std::cout << "aNormal = " << +m2Shader::Attribute::aNormal << std::flush;
    this->m_config = config;

    this->canvWidth = canvWidth;
    this->canvHeight = canvHeight;
    this->canvAspect = canvWidth / canvHeight;

    /* Allocate and assign a Vertex Array Object to our handle */
    GLuint vao;
    glGenVertexArrays(1, &vao);

    /* Bind our Vertex Array Object as the current used object */
    glBindVertexArray(vao);

//    self.initGlContext(canvas);
    this->initArrayInstancedExt();
    this->initDepthTextureExt();
//    if (this->enableDeferred) {
//        this->initDeferredRendering();
//    }
    this->initRenderBuffers();
    this->initAnisotropicExt();
    this->initVertexArrayObjectExt();
    this->initCompressedTextureS3tcExt();

    this->initShaders();

    this->initSceneApi();
    this->initSceneGraph();
    this->createBlackPixelTexture();

    this->initBoxVBO();
    this->initTextureCompVBO();
    this->initCaches();
    this->initCamera();

    //Init caches

    m2Object = new M2Object(this);
    m2Object->setLoadParams(
//            "WORLD\\AZEROTH\\KARAZAHN\\PASSIVEDOODADS\\CHANDELIERS\\KARAZANCHANDELIER_02.m2",
            "WORLD\\EXPANSION01\\DOODADS\\GENERIC\\BLOODELF\\PLANETARIUM\\BE_PLANETARIUM.m2",
//            "WORLD\\EXPANSION01\\DOODADS\\HELLFIREPENINSULA\\LAMPPOST\\ANCIENT_DRAINEI_LAMPPOST.m2",
            0,
            std::vector<uint8_t>(),
            std::vector<std::string>()
    );
}

ShaderRuntimeData * WoWSceneImpl::compileShader(std::string shaderName,
        std::string vertShaderString,
        std::string fragmentShaderString,
        std::string *vertExtraDefStringsExtern, std::string *fragExtraDefStringsExtern) {

    std::string vertExtraDefStrings = (vertExtraDefStringsExtern != nullptr) ? *vertExtraDefStringsExtern : "";
    std::string fragExtraDefStrings = (fragExtraDefStringsExtern != nullptr) ? *fragExtraDefStringsExtern : "";


    if (fragExtraDefStringsExtern == nullptr) {
        fragExtraDefStrings = "";
    }

    if (m_enable) {
        vertExtraDefStrings = "#define ENABLE_DEFERRED 1\r\n"
                "#define precision\n"
                "#define lowp\n"
                "#define mediump\n"
                "#define highp\n";
        fragExtraDefStrings = "#define ENABLE_DEFERRED 1\r\n"
                "#define precision\n"
                "#define lowp\n"
                "#define mediump\n"
                "#define highp\n";
    }

    GLint maxVertexUniforms;
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &maxVertexUniforms);
    int maxMatrixUniforms = (maxVertexUniforms / 4) - 6;

    vertExtraDefStrings = vertExtraDefStrings + "#define MAX_MATRIX_NUM "+std::to_string(maxMatrixUniforms)+"\r\n"+"#define COMPILING_VS 1\r\n ";
    fragExtraDefStrings = fragExtraDefStrings + "#define COMPILING_FS 1\r\n";

    vertShaderString = trimmed(vertShaderString.insert(
        vertShaderString.find("\n",vertShaderString.find("#version", 0)+1)+1,
        vertExtraDefStrings));

    fragmentShaderString = trimmed(fragmentShaderString.insert(
            fragmentShaderString.find("\n",fragmentShaderString.find("#version", 0)+1)+1,
            fragExtraDefStrings));

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const GLchar *vertexShaderConst = (const GLchar *)vertShaderString.c_str();
    glShaderSource(vertexShader, 1, &vertexShaderConst, 0);
    glCompileShader(vertexShader);

    GLint success = 0;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (success != GL_TRUE) {
        // Something went wrong during compilation; get the error
        GLint maxLength = 0;
        glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &maxLength);

        //The maxLength includes the NULL character
        std::vector<GLchar> infoLog(maxLength);
        glGetShaderInfoLog(vertexShader, maxLength, &maxLength, &infoLog[0]);
        std::cout << "\ncould not compile vertex shader "<<shaderName<<":" << std::endl
                  << vertexShaderConst << std::endl << std::endl
                  << "error: "<<std::string(infoLog.begin(),infoLog.end())<< std::endl <<std::flush;

        throw "" ;
    }

    /* 1.2 Compile fragment shader */
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const GLchar *fragmentShaderConst = (const GLchar *) fragmentShaderString.c_str();
    glShaderSource(fragmentShader, 1, &fragmentShaderConst, 0);
    glCompileShader(fragmentShader);

    // Check if it compiled
    success = 0;
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        // Something went wrong during compilation; get the error
        GLint maxLength = 0;
        glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &maxLength);

        //The maxLength includes the NULL character
        std::vector<GLchar> infoLog(maxLength);
        glGetShaderInfoLog(fragmentShader, maxLength, &maxLength, &infoLog[0]);
        std::cout << "\ncould not compile fragment shader "<<shaderName<<":" << std::endl
            << fragmentShaderConst << std::endl << std::endl
            << "error: "<<std::string(infoLog.begin(),infoLog.end())<< std::endl <<std::flush;

        throw "" ;
    }

    /* 1.3 Link the program */
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    const shaderDefinition *shaderDefinition1 = getShaderDef(shaderName.c_str());
    for (int i = 0; i < shaderDefinition1->attributesNum; i++) {
        glBindAttribLocation(program, shaderDefinition1->attributes[i].number, shaderDefinition1->attributes[i].variableName);
    }

    // link the program.
    glLinkProgram(program);

    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (!status) {
        throw "could not compile shader:" ;
    }

    //Get Uniforms
    ShaderRuntimeData *data = new ShaderRuntimeData();
    data->setProgram(program);

    GLint count;
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &count);
//    printf("Active Uniforms: %d\n", count);
    for (GLint i = 0; i < count; i++)
    {
        const GLsizei bufSize = 32; // maximum name length
        GLchar name[bufSize]; // variable name in GLSL
        GLsizei length; // name length
        GLint size; // size of the variable
        GLenum type; // type of the variable (float, vec3 or mat4, etc)

        glGetActiveUniform(program, (GLuint)i, bufSize, &length, &size, &type, name);
        GLint location = glGetUniformLocation(program, name);

        data->setUnf(std::string(name), location);
//        printf("Uniform #%d Type: %u Name: %s Location: %d\n", i, type, name, location);
    }

    return data;
}
void WoWSceneImpl::initShaders() {
    const std::string textureCompositionShader = getShaderDef("textureCompositionShader")->shaderString;
    this->textureCompositionShader = this->compileShader("textureCompositionShader", textureCompositionShader, textureCompositionShader);

    const  std::string renderFrameShader = getShaderDef("renderFrameBufferShader")->shaderString;
    this->renderFrameShader        = this->compileShader("renderFrameBufferShader", renderFrameShader, renderFrameShader);

    const  std::string drawDepthBuffer = getShaderDef("drawDepthShader")->shaderString;
    this->drawDepthBuffer          = this->compileShader("drawDepthShader", drawDepthBuffer, drawDepthBuffer);

    const  std::string readDepthBuffer = getShaderDef("readDepthBufferShader")->shaderString;
    this->readDepthBuffer          = this->compileShader("readDepthBufferShader", readDepthBuffer, readDepthBuffer);

    const  std::string wmoShader = getShaderDef("wmoShader")->shaderString;
    this->wmoShader                = this->compileShader("wmoShader", wmoShader, wmoShader);

    const  std::string m2Shader = getShaderDef("m2Shader")->shaderString;
    this->m2Shader                 = this->compileShader("m2Shader", m2Shader, m2Shader);
    this->m2InstancingShader       = this->compileShader("m2Shader", m2Shader, m2Shader,
                                                         new std::string("#define INSTANCED 1\r\n "),
                                                         new std::string("#define INSTANCED 1\r\n "));

    const  std::string bbShader = getShaderDef("m2Shader")->shaderString;
    this->bbShader                 = this->compileShader("drawBBShader", bbShader, bbShader);

    const  std::string adtShader = getShaderDef("adtShader")->shaderString;
    this->adtShader                = this->compileShader("adtShader", adtShader, adtShader);

    const  std::string drawPortalShader = getShaderDef("drawPortalShader")->shaderString;
    this->drawPortalShader         = this->compileShader("drawPortalShader", drawPortalShader, drawPortalShader);

    const  std::string drawFrustumShader = getShaderDef("drawFrustumShader")->shaderString;
    this->drawFrustumShader        = this->compileShader("drawFrustumShader", drawFrustumShader, drawFrustumShader);

    const  std::string drawLinesShader = getShaderDef("drawLinesShader")->shaderString;
    this->drawLinesShader          = this->compileShader("drawLinesShader", drawLinesShader, drawLinesShader);
}

void WoWSceneImpl::initGlContext() {

}
void WoWSceneImpl::createBlackPixelTexture() {

}
void WoWSceneImpl::initAnisotropicExt() {

}
void WoWSceneImpl::initArrayInstancedExt() {

}
void WoWSceneImpl::initBoxVBO() {

}
void WoWSceneImpl::initCaches() {

}
void WoWSceneImpl::initCamera() {

}
void WoWSceneImpl::initCompressedTextureS3tcExt() {

}
void WoWSceneImpl::initDeferredRendering() {

}
void WoWSceneImpl::initDepthTextureExt() {

}
void WoWSceneImpl::initDrawBuffers(int frameBuffer) {

}

void WoWSceneImpl::initVertexArrayObjectExt() {

}

void WoWSceneImpl::initRenderBuffers() {
    GLuint framebuffer = 0;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    GLuint colorTexture = 0;
    glGenTextures(1, &colorTexture);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, this->canvWidth, this->canvHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
//    // Create the depth texture
        GLuint depthTexture = 0;
//    if (this.depth_texture_ext) {
        glGenTextures(1, &depthTexture);
        glBindTexture(GL_TEXTURE_2D, depthTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, this->canvWidth, this->canvHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT,
                     nullptr);
//    }
//
//    if (!this.enableDeferred) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
//    }
//    if (this.depth_texture_ext) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
//    }
    this->frameBuffer = framebuffer;
    this->frameBufferColorTexture = colorTexture;
    this->frameBufferDepthTexture = depthTexture;
//
    std::cout << "passed" << std::endl;
    static const float verts[] = {
        1,  1,
       -1,  1,
       -1, -1,
        1,  1,
       -1, -1,
        1,  -1,
    };
    const int vertsLength = sizeof(verts) / sizeof(verts[0]);
    std::cout << "vertsLength = " << vertsLength << std::endl;
    GLuint vertBuffer = 0;
    glGenBuffers(1, &vertBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertBuffer);
    glBufferData(GL_ARRAY_BUFFER, vertsLength, verts, GL_STATIC_DRAW);

    this->vertBuffer = vertBuffer;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void WoWSceneImpl::initSceneApi() {

}

void WoWSceneImpl::initSceneGraph() {

}

void WoWSceneImpl::initTextureCompVBO() {

}

/* Shaders stuff */

void WoWSceneImpl::activateRenderFrameShader () {
    glUseProgram(this->renderFrameShader->getProgram());
    glActiveTexture(GL_TEXTURE0);

    glDisableVertexAttribArray(1);

    float uResolution[2] = {this->canvWidth, this->canvHeight };
    glUniform2fv(this->renderFrameShader->getUnf("uResolution"), 2, uResolution);

    glUniform1i(this->renderFrameShader->getUnf("u_sampler"), 0);
    if (this->renderFrameShader->hasUnf("u_depth")) {
        glUniform1i(this->renderFrameShader->getUnf("u_depth"), 1);
    }
}
/*
void WoWSceneImpl::activateTextureCompositionShader(GLuint texture) {
    glUseProgram(this->textureCompositionShader->getProgram());

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    if (this.textureCompVars.depthTexture) {
        GL_framebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, this.textureCompVars.depthTexture, 0);
    }

    glBindBuffer(GL_ARRAY_BUFFER, this.textureCompVars.textureCoords);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this.textureCompVars.elements);


    glVertexAttribPointer(this.currentShaderProgram.shaderAttributes.aTextCoord, 2, GL_FLOAT, false, 0, 0);  // position

    glActiveTexture(GL_TEXTURE0);
    glUniform1i(this.currentShaderProgram.shaderUniforms.uTexture, 0);

    glDepthMask(true);
    glDisable(GL_CULL_FACE);

    glClearColor(0,0,1,1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(false);
    glViewport(0,0,1024,1024);

}*/
void WoWSceneImpl::activateRenderDepthShader () {
    glUseProgram(this->drawDepthBuffer->getProgram());
    glActiveTexture(GL_TEXTURE0);
}/*
void WoWSceneImpl::activateReadDepthBuffer () {
    this.currentShaderProgram = this.readDepthBuffer;
    if (this.currentShaderProgram) {
        var gl = this.gl;
        GL_useProgram(this.currentShaderProgram.program);
        var shaderAttributes = this.sceneApi.shaders.getShaderAttributes();

        GL_activeTexture(GL_TEXTURE0);

        GL_enableVertexAttribArray(this.currentShaderProgram.shaderAttributes.position);
        GL_enableVertexAttribArray(this.currentShaderProgram.shaderAttributes.texture);

    }
}
void WoWSceneImpl::activateAdtShader (){
    this.currentShaderProgram = this.adtShader;
    if (this.currentShaderProgram) {
        var gl = this.gl;
        var instExt = this.sceneApi.extensions.getInstancingExt();
        var shaderAttributes = this.sceneApi.shaders.getShaderAttributes();

        GL_useProgram(this.currentShaderProgram.program);

        GL_enableVertexAttribArray(shaderAttributes.aHeight);
        GL_enableVertexAttribArray(shaderAttributes.aIndex);

        GL_uniformMatrix4fv(this.currentShaderProgram.shaderUniforms.uLookAtMat, false, this.lookAtMat4);
        GL_uniformMatrix4fv(this.currentShaderProgram.shaderUniforms.uPMatrix, false, this.perspectiveMatrix);

        if (this.currentWdt && ((this.currentWdt.flags & 0x04) > 0)) {
            GL_uniform1i(this.currentShaderProgram.shaderUniforms.uNewFormula, 1);
        } else {
            GL_uniform1i(this.currentShaderProgram.shaderUniforms.uNewFormula, 0);
        }

        GL_uniform1i(this.currentShaderProgram.shaderUniforms.uLayer0, 0);
        GL_uniform1i(this.currentShaderProgram.shaderUniforms.uAlphaTexture, 1);
        GL_uniform1i(this.currentShaderProgram.shaderUniforms.uLayer1, 2);
        GL_uniform1i(this.currentShaderProgram.shaderUniforms.uLayer2, 3);
        GL_uniform1i(this.currentShaderProgram.shaderUniforms.uLayer3, 4);

        GL_uniform1f(this.currentShaderProgram.shaderUniforms.uFogStart, this.uFogStart);
        GL_uniform1f(this.currentShaderProgram.shaderUniforms.uFogEnd, this.uFogEnd);

        GL_uniform3fv(this.currentShaderProgram.shaderUniforms.uFogColor, this.fogColor);
    }
}
void WoWSceneImpl::activateWMOShader () {
    this.currentShaderProgram = this.wmoShader;
    if (this.currentShaderProgram) {
        var gl = this.gl;
        GL_useProgram(this.currentShaderProgram.program);
        var shaderAttributes = this.sceneApi.shaders.getShaderAttributes();

        GL_enableVertexAttribArray(shaderAttributes.aPosition);
        if (shaderAttributes.aNormal) {
            GL_enableVertexAttribArray(shaderAttributes.aNormal);
        }
        GL_enableVertexAttribArray(shaderAttributes.aTexCoord);
        GL_enableVertexAttribArray(shaderAttributes.aTexCoord2);
        GL_enableVertexAttribArray(shaderAttributes.aColor);
        GL_enableVertexAttribArray(shaderAttributes.aColor2);

        GL_uniformMatrix4fv(this.currentShaderProgram.shaderUniforms.uLookAtMat, false, this.lookAtMat4);
        GL_uniformMatrix4fv(this.currentShaderProgram.shaderUniforms.uPMatrix, false, this.perspectiveMatrix);

        GL_uniform1i(this.currentShaderProgram.shaderUniforms.uTexture, 0);
        GL_uniform1i(this.currentShaderProgram.shaderUniforms.uTexture2, 1);

        GL_uniform1f(this.currentShaderProgram.shaderUniforms.uFogStart, this.uFogStart);
        GL_uniform1f(this.currentShaderProgram.shaderUniforms.uFogEnd, this.uFogEnd);

        GL_uniform3fv(this.currentShaderProgram.shaderUniforms.uFogColor, this.fogColor);

        GL_activeTexture(GL_TEXTURE0);
    }
}
void WoWSceneImpl::deactivateWMOShader () {
    var gl = this.gl;
    var instExt = this.sceneApi.extensions.getInstancingExt();
    var shaderAttributes = this.sceneApi.shaders.getShaderAttributes();

    //GL_disableVertexAttribArray(shaderAttributes.aPosition);
    if (shaderAttributes.aNormal) {
        GL_disableVertexAttribArray(shaderAttributes.aNormal);
    }

    GL_disableVertexAttribArray(shaderAttributes.aTexCoord);
    GL_disableVertexAttribArray(shaderAttributes.aTexCoord2);

    GL_disableVertexAttribArray(shaderAttributes.aColor);
    GL_disableVertexAttribArray(shaderAttributes.aColor2);
}
void WoWSceneImpl::deactivateTextureCompositionShader() {
    var gl = this.gl;
    GL_useProgram(this.currentShaderProgram.program);
    var shaderAttributes = this.sceneApi.shaders.getShaderAttributes();

    GL_bindFramebuffer(GL_FRAMEBUFFER, null);

    GL_bindBuffer(GL_ELEMENT_ARRAY_BUFFER, null);
    GL_bindBuffer(GL_ARRAY_BUFFER, null);

    GL_enable(GL_DEPTH_TEST);
    GL_depthMask(true);
    GL_disable(GL_BLEND);
}

void WoWSceneImpl::activateM2ShaderAttribs() {
    var gl = this.gl;
    var shaderAttributes = this.sceneApi.shaders.getShaderAttributes();
    GL_enableVertexAttribArray(shaderAttributes.aPosition);
    if (shaderAttributes.aNormal) {
        GL_enableVertexAttribArray(shaderAttributes.aNormal);
    }
    GL_enableVertexAttribArray(shaderAttributes.boneWeights);
    GL_enableVertexAttribArray(shaderAttributes.bones);
    GL_enableVertexAttribArray(shaderAttributes.aTexCoord);
    GL_enableVertexAttribArray(shaderAttributes.aTexCoord2);
}
void WoWSceneImpl::deactivateM2ShaderAttribs() {
    var gl = this.gl;
    var shaderAttributes = this.sceneApi.shaders.getShaderAttributes();

    GL_disableVertexAttribArray(shaderAttributes.aPosition);

    if (shaderAttributes.aNormal) {
        GL_disableVertexAttribArray(shaderAttributes.aNormal);
    }
    GL_disableVertexAttribArray(shaderAttributes.boneWeights);
    GL_disableVertexAttribArray(shaderAttributes.bones);

    GL_disableVertexAttribArray(shaderAttributes.aTexCoord);
    GL_disableVertexAttribArray(shaderAttributes.aTexCoord2);
    GL_enableVertexAttribArray(0);
}
void WoWSceneImpl::activateM2Shader () {
    this.currentShaderProgram = this.m2Shader;
    if (this.currentShaderProgram) {
        var gl = this.gl;
        GL_useProgram(this.currentShaderProgram.program);
        GL_enableVertexAttribArray(0);
        if (!this.vao_ext) {
            this.activateM2ShaderAttribs()
        }

        GL_uniformMatrix4fv(this.currentShaderProgram.shaderUniforms.uLookAtMat, false, this.lookAtMat4);
        GL_uniformMatrix4fv(this.currentShaderProgram.shaderUniforms.uPMatrix, false, this.perspectiveMatrix);
        if (this.currentShaderProgram.shaderUniforms.isBillboard) {
            GL_uniform1i(this.currentShaderProgram.shaderUniforms.isBillboard, 0);
        }

        GL_uniform1i(this.currentShaderProgram.shaderUniforms.uTexture, 0);
        GL_uniform1i(this.currentShaderProgram.shaderUniforms.uTexture2, 1);

        GL_uniform1f(this.currentShaderProgram.shaderUniforms.uFogStart, this.uFogStart);
        GL_uniform1f(this.currentShaderProgram.shaderUniforms.uFogEnd, this.uFogEnd);

        GL_uniform3fv(this.currentShaderProgram.shaderUniforms.uFogColor, this.fogColor);


        GL_activeTexture(GL_TEXTURE0);
    }
}
void WoWSceneImpl::deactivateM2Shader () {
    var gl = this.gl;
    var instExt = this.sceneApi.extensions.getInstancingExt();

    if (!this.vao_ext) {
        this.deactivateM2ShaderAttribs()
    }
}
void WoWSceneImpl::activateM2InstancingShader () {
    this.currentShaderProgram = this.m2InstancingShader;
    if (this.currentShaderProgram) {
        var gl = this.gl;
        var instExt = this.sceneApi.extensions.getInstancingExt();
        var shaderAttributes = this.sceneApi.shaders.getShaderAttributes();

        GL_useProgram(this.currentShaderProgram.program);

        GL_uniformMatrix4fv(this.currentShaderProgram.shaderUniforms.uLookAtMat, false, this.lookAtMat4);
        GL_uniformMatrix4fv(this.currentShaderProgram.shaderUniforms.uPMatrix, false, this.perspectiveMatrix);

        GL_uniform1i(this.currentShaderProgram.shaderUniforms.uTexture, 0);
        GL_uniform1i(this.currentShaderProgram.shaderUniforms.uTexture2, 1);

        GL_uniform1f(this.currentShaderProgram.shaderUniforms.uFogStart, this.uFogStart);
        GL_uniform1f(this.currentShaderProgram.shaderUniforms.uFogEnd, this.uFogEnd);

        GL_activeTexture(GL_TEXTURE0);
        GL_enableVertexAttribArray(0);
        GL_enableVertexAttribArray(shaderAttributes.aPosition);
        if (shaderAttributes.aNormal) {
            GL_enableVertexAttribArray(shaderAttributes.aNormal);
        }
        GL_enableVertexAttribArray(shaderAttributes.boneWeights);
        GL_enableVertexAttribArray(shaderAttributes.bones);
        GL_enableVertexAttribArray(shaderAttributes.aTexCoord);
        GL_enableVertexAttribArray(shaderAttributes.aTexCoord2);

        GL_enableVertexAttribArray(shaderAttributes.aPlacementMat + 0);
        GL_enableVertexAttribArray(shaderAttributes.aPlacementMat + 1);
        GL_enableVertexAttribArray(shaderAttributes.aPlacementMat + 2);
        GL_enableVertexAttribArray(shaderAttributes.aPlacementMat + 3);
        GL_enableVertexAttribArray(shaderAttributes.aDiffuseColor);
        if (instExt != null) {
            instExt.vertexAttribDivisorANGLE(shaderAttributes.aPlacementMat + 0, 1);
            instExt.vertexAttribDivisorANGLE(shaderAttributes.aPlacementMat + 1, 1);
            instExt.vertexAttribDivisorANGLE(shaderAttributes.aPlacementMat + 2, 1);
            instExt.vertexAttribDivisorANGLE(shaderAttributes.aPlacementMat + 3, 1);
            instExt.vertexAttribDivisorANGLE(shaderAttributes.aDiffuseColor, 1);
        }

        GL_uniform3fv(this.currentShaderProgram.shaderUniforms.uFogColor, this.fogColor);
    }

}
void WoWSceneImpl::deactivateM2InstancingShader () {
    var gl = this.gl;
    var instExt = this.sceneApi.extensions.getInstancingExt();
    var shaderAttributes = this.sceneApi.shaders.getShaderAttributes();

    GL_disableVertexAttribArray(shaderAttributes.aPosition);
    if (shaderAttributes.aNormal) {
        GL_disableVertexAttribArray(shaderAttributes.aNormal);
    }
    GL_disableVertexAttribArray(shaderAttributes.boneWeights);
    GL_disableVertexAttribArray(shaderAttributes.bones);
    GL_disableVertexAttribArray(shaderAttributes.aTexCoord);
    GL_disableVertexAttribArray(shaderAttributes.aTexCoord2);

    if (instExt) {
        instExt.vertexAttribDivisorANGLE(shaderAttributes.aPlacementMat + 0, 0);
        instExt.vertexAttribDivisorANGLE(shaderAttributes.aPlacementMat + 1, 0);
        instExt.vertexAttribDivisorANGLE(shaderAttributes.aPlacementMat + 2, 0);
        instExt.vertexAttribDivisorANGLE(shaderAttributes.aPlacementMat + 3, 0);
    }
    GL_disableVertexAttribArray(shaderAttributes.aPlacementMat + 0);
    GL_disableVertexAttribArray(shaderAttributes.aPlacementMat + 1);
    GL_disableVertexAttribArray(shaderAttributes.aPlacementMat + 2);
    GL_disableVertexAttribArray(shaderAttributes.aPlacementMat + 3);

    GL_enableVertexAttribArray(0);
}
void WoWSceneImpl::activateBoundingBoxShader () {
    this.currentShaderProgram = this.bbShader;
    if (this.currentShaderProgram) {
        var gl = this.gl;
        GL_useProgram(this.currentShaderProgram.program);

        GL_bindBuffer(GL_ELEMENT_ARRAY_BUFFER, this.bbBoxVars.ibo_elements);
        GL_bindBuffer(GL_ARRAY_BUFFER, this.bbBoxVars.vbo_vertices);

        //GL_enableVertexAttribArray(this.currentShaderProgram.shaderAttributes.aPosition);
        GL_vertexAttribPointer(this.currentShaderProgram.shaderAttributes.aPosition, 3, GL_FLOAT, false, 0, 0);  // position

        GL_uniformMatrix4fv(this.currentShaderProgram.shaderUniforms.uLookAtMat, false, this.lookAtMat4);
        GL_uniformMatrix4fv(this.currentShaderProgram.shaderUniforms.uPMatrix, false, this.perspectiveMatrix);
    }
}
void WoWSceneImpl::activateFrustumBoxShader () {
    this.currentShaderProgram = this.drawFrustumShader;
    if (this.currentShaderProgram) {
        var gl = this.gl;
        GL_useProgram(this.currentShaderProgram.program);

        GL_bindBuffer(GL_ELEMENT_ARRAY_BUFFER, this.bbBoxVars.ibo_elements);
        GL_bindBuffer(GL_ARRAY_BUFFER, this.bbBoxVars.vbo_vertices);

        GL_enableVertexAttribArray(this.currentShaderProgram.shaderAttributes.aPosition);
        GL_vertexAttribPointer(this.currentShaderProgram.shaderAttributes.aPosition, 3, GL_FLOAT, false, 0, 0);  // position

        GL_uniformMatrix4fv(this.currentShaderProgram.shaderUniforms.uLookAtMat, false, this.lookAtMat4);
        GL_uniformMatrix4fv(this.currentShaderProgram.shaderUniforms.uPMatrix, false, this.perspectiveMatrix);
    }
}
void WoWSceneImpl::activateDrawLinesShader () {
    this.currentShaderProgram = this.drawLinesShader;
    if (this.currentShaderProgram) {
        var gl = this.gl;
        GL_useProgram(this.currentShaderProgram.program);

        GL_uniformMatrix4fv(this.currentShaderProgram.shaderUniforms.uLookAtMat, false, this.lookAtMat4);
        GL_uniformMatrix4fv(this.currentShaderProgram.shaderUniforms.uPMatrix, false, this.perspectiveMatrix);
    }
}
void WoWSceneImpl::activateDrawPortalShader () {
    this.currentShaderProgram = this.drawPortalShader;
    if (this.currentShaderProgram) {
        var gl = this.gl;
        GL_useProgram(this.currentShaderProgram.program);

        GL_uniformMatrix4fv(this.currentShaderProgram.shaderUniforms.uLookAtMat, false, this.lookAtMat4);
        GL_uniformMatrix4fv(this.currentShaderProgram.shaderUniforms.uPMatrix, false, this.perspectiveMatrix);
    }
} */
/****************/



void glClearScreen() {
    glClearDepth(1.0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glDisable(GL_BLEND);
    glClearColor(0.6, 0.95, 1.0, 1);
    //glClearColor(0.117647, 0.207843, 0.392157, 1);
    //glClearColor(fogColor[0], fogColor[1], fogColor[2], 1);
//    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_CULL_FACE);
}
bool testLoad = false;
void WoWSceneImpl::draw(int deltaTime) {
    glClearScreen();
    mathfu::vec3 *cameraVector;

    static const mathfu::vec3 upVector(0,0,1);

    int farPlane = 250;
    int nearPlane = 1;
    float fov = 45.0;
    if (!testLoad) {
        wmoMainCache.get("WORLD\\WMO\\OUTLAND\\TEROKKAR\\SHATTRATHCITY.WMO");
        wmoGeomCache.get("WORLD\\WMO\\OUTLAND\\TEROKKAR\\SHATTRATHCITY_002.wmo");
    }

    //If use camera settings
    //Figure out way to assign the object with camera
    //config.setCameraM2(this.graphManager.m2Object[0]);
//    var m2Object = config.getCameraM2();
//    if (m2Object && m2Object.loaded) {
//        m2Object.updateCameras(deltaTime);
//
//        var cameraSettings = m2Object.cameras[0];
//        farPlane = cameraSettings.farClip;
//        nearPlane = cameraSettings.nearClip;
//        fov = cameraSettings.fov * 32 * Math.PI / 180;
//
//        this.mainCamera = cameraSettings.currentPosition;
//        vec4.transformMat4(this.mainCamera, this.mainCamera, m2Object.placementMatrix);
//        this.mainCameraLookAt = cameraSettings.currentTarget;
//        vec4.transformMat4(this.mainCameraLookAt, this.mainCameraLookAt, m2Object.placementMatrix);
//        cameraVecs = {
//                lookAtVec3: this.mainCameraLookAt,
//                cameraVec3: this.mainCamera,
//                staticCamera: true
//        }
//    }
//
    if (this->uFogStart == -1) {
        this->uFogStart = farPlane - 10;
    }
    if (this->uFogEnd == -1) {
        this->uFogEnd = farPlane;
    }

    if (!m_config->getUseSecondCamera()){
        this->m_firstCamera.tick(deltaTime);
    } else {
        this->m_secondCamera.tick(deltaTime);
    }
//
//    var adt_x = Math.floor((32 - (this.mainCamera[1] / 533.33333)));
//    var adt_y = Math.floor((32 - (this.mainCamera[0] / 533.33333)));
//
//    //TODO: HACK!!
//    for (var x = adt_x-1; x <= adt_x+1; x++) {
//        for (var y = adt_y-1; y <= adt_y+1; y++) {
//            this.addAdtChunkToCurrentMap(x, y);
//        }
//    }
//
    mathfu::mat4 lookAtMat4 =
        mathfu::mat4::LookAt(
                this->m_firstCamera.getCameraPosition(),
                this->m_firstCamera.getCameraLookAt(),
                upVector);

    m_lookAtMat4 = lookAtMat4;


// Second camera for debug
    mathfu::mat4 secondLookAtMat =
            mathfu::mat4::LookAt(
                    this->m_secondCamera.getCameraPosition(),
                    this->m_secondCamera.getCameraLookAt(),
                    upVector);


    mathfu::mat4 perspectiveMatrix =
        mathfu::mat4::Perspective(
                fov,
                this->canvAspect,
                nearPlane,
                farPlane);

    m_perspectiveMatrix = perspectiveMatrix;

    //var o_height = (this.canvas.height * (533.333/256/* zoom 7 in Alram viewer */))/ 8 ;
    //var o_width = o_height * this.canvas.width / this.canvas.height;
    //mat4.ortho(perspectiveMatrix, -o_width, o_width, -o_height, o_height, 1, 1000);


    mathfu::mat4 perspectiveMatrixForCulling =
            mathfu::mat4::Perspective(
                    fov,
                    this->canvAspect,
                    nearPlane,
                    farPlane);
    //Camera for rendering
    mathfu::mat4 perspectiveMatrixForCameraRender =
            mathfu::mat4::Perspective(fov,
                                        this->canvAspect,
                                        nearPlane,
                                        farPlane);
    mathfu::mat4 viewCameraForRender =
            perspectiveMatrixForCameraRender * lookAtMat4;

//    this.perspectiveMatrix = perspectiveMatrix;
//    this.viewCameraForRender = viewCameraForRender;
//    if (!this.isShadersLoaded) return;
//
//    var cameraPos = vec4.fromValues(
//            this.mainCamera[0],
//            this.mainCamera[1],
//            this.mainCamera[2],
//            1
//    );
//    this.graphManager.setCameraPos(cameraPos);
//    this.graphManager.setLookAtMat(lookAtMat4);
//
//    // Update objects
//    this.adtGeomCache.processCacheQueue(10);
    this->wmoGeomCache.processCacheQueue(10);
    this->wmoMainCache.processCacheQueue(10);
    this->m2GeomCache.processCacheQueue(10);
    this->skinGeomCache.processCacheQueue(10);
    this->textureCache.processCacheQueue(10);
//
//
//    var updateRes = this.graphManager.update(deltaTime);
    m2Object->update(deltaTime, this->m_firstCamera.getCameraPosition(), lookAtMat4);
//    this.worldObjectManager.update(deltaTime, cameraPos, lookAtMat4);
//
//    this.graphManager.checkCulling(perspectiveMatrixForCulling, lookAtMat4);
//    this.graphManager.sortGeometry(perspectiveMatrixForCulling, lookAtMat4);
//
//

    glViewport(0,0,this->canvWidth, this->canvHeight);

    activateM2Shader();
    m2Object->draw(false, mathfu::mat4::Identity(), mathfu::vec4(1,1,1,1));
    m2Object->draw(true, mathfu::mat4::Identity(), mathfu::vec4(1,1,1,1));
    deactivateM2Shader();

    /*
    if (this->m_config->getDoubleCameraDebug()) {
        //Draw static camera
//        this.isDebugCamera = true;
//        this.lookAtMat4 = secondLookAtMat;
        glBindFramebuffer(GL_FRAMEBUFFER, this->frameBuffer);
//
        //glClearScreen(this->fogColor);
//
        glActiveTexture(GL_TEXTURE0);
        glDepthMask(GL_TRUE);
        glEnableVertexAttribArray(0);
//        this.graphManager.draw();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
//
        //Draw debug camera from framebuffer into screen
//        this.glClearScreen(gl, this.fogColor);
        this->activateRenderFrameShader();
        glViewport(0,0,this->canvWidth, this->canvHeight);
        glEnableVertexAttribArray(0);
//        this.drawFrameBuffer();
//
//        this.isDebugCamera = false;
    }
//
//    //Render real camera
//    this.lookAtMat4 = lookAtMat4;
//    gl.bindFramebuffer(gl.FRAMEBUFFER, this.frameBuffer);
//    this.glClearScreen(gl, this.fogColor);
//
    glActiveTexture(GL_TEXTURE0);
    glDepthMask(true);
    glEnableVertexAttribArray(0);
//    this.graphManager.draw();

    glBindFramebuffer(GL_FRAMEBUFFER, GL_ZERO);

    if (!this->m_config->getDoubleCameraDebug()) {
        //Draw real camera into screen
//
        glClearScreen();
        glEnableVertexAttribArray(0);
        this->activateRenderFrameShader();
        glViewport(0,0,this->canvWidth, this->canvHeight);
//        this.drawFrameBuffer();
    } else {
//        //Draw real camera into square at bottom of screen
//
        this->activateRenderDepthShader();
        glEnableVertexAttribArray(0);
//        this.drawTexturedQuad(gl, this.frameBufferColorTexture,
//                              this.canvas.width * 0.60,
//                              0,//this.canvas.height * 0.75,
//                              this.canvas.width * 0.40,
//                              this.canvas.height * 0.40,
//                              this.canvas.width, this.canvas.height);
    }
    if (this->m_config->getDrawDepthBuffer() /*&& this.depth_texture_ext*//*) {
        this->activateRenderDepthShader();
        glEnableVertexAttribArray(0);
        glUniform1f(drawDepthBuffer->getUnf("uFarPlane"), farPlane);
        glUniform1f(drawDepthBuffer->getUnf("uNearPlane"), nearPlane);

//        this->drawTexturedQuad(gl, this.frameBufferDepthTexture,
//                              this.canvas.width * 0.60,
//                              0,//this.canvas.height * 0.75,
//                              this.canvas.width * 0.40,
//                              this.canvas.height * 0.40,
//                              this.canvas.width, this.canvas.height,
//                              true);
    }*/
}

void WoWSceneImpl::activateM2Shader() {
    glUseProgram(this->m2Shader->getProgram());
//    glEnableVertexAttribArray(0);
//    if (!this.vao_ext) {
        this->activateM2ShaderAttribs();
//    }

    glUniformMatrix4fv(this->m2Shader->getUnf("uLookAtMat"), 1, GL_FALSE, &this->m_lookAtMat4[0]);
    glUniformMatrix4fv(this->m2Shader->getUnf("uPMatrix"), 1, GL_FALSE, &this->m_perspectiveMatrix[0]);
//    if (this.currentShaderProgram.shaderUniforms.isBillboard) {
//        gl.uniform1i(this.currentShaderProgram.shaderUniforms.isBillboard, 0);
//    }

    glUniform1i(this->m2Shader->getUnf("uTexture"), 0);
    glUniform1i(this->m2Shader->getUnf("uTexture2"), 1);

    glUniform1f(this->m2Shader->getUnf("uFogStart"), this->uFogStart);
    glUniform1f(this->m2Shader->getUnf("uFogEnd"), this->uFogEnd);

//    glUniform3fv(this->m2Shader->getUnf("uFogColor"), this->fogColor);


    glActiveTexture(GL_TEXTURE0);
}

void WoWSceneImpl::deactivateM2Shader() {

}

void WoWSceneImpl::activateM2ShaderAttribs() {
    glEnableVertexAttribArray(+m2Shader::Attribute::aPosition);
//    if (shaderAttributes.aNormal) {
//        glEnableVertexAttribArray(shaderAttributes.aNormal);
//    }
    glEnableVertexAttribArray(+m2Shader::Attribute::boneWeights);
    glEnableVertexAttribArray(+m2Shader::Attribute::bones);
    glEnableVertexAttribArray(+m2Shader::Attribute::aTexCoord);
    glEnableVertexAttribArray(+m2Shader::Attribute::aTexCoord2);
}


WoWScene * createWoWScene(Config *config, IFileRequest * requestProcessor, int canvWidth, int canvHeight){
//	glewExperimental = true; // Needed in core profile
//	if (glewInit() != GLEW_OK) {
//		fprintf(stderr, "Failed to initialize GLEW\n");
//		return nullptr;
//	}
    return new WoWSceneImpl(config, requestProcessor, canvWidth, canvHeight);
}