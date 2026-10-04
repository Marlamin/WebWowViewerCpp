//
// Created by Deamon on 7/27/2018.
//

#include "GAdtShaderPermutationGL33.h"

GAdtShaderPermutationGL33::GAdtShaderPermutationGL33(std::string &shaderName,const HGDevice &device) : GShaderPermutationGL33(shaderName,
                                                                                                            device) {}

void GAdtShaderPermutationGL33::compileShader(const std::string &vertExtraDef, const std::string &fragExtraDef) {
    GShaderPermutationGL33::compileShader("", "");

    //Init newly created shader
    glUseProgram(this->m_programBuffer);

    glUniform1i(this->getUnf("uLayer0"), 0);
    glUniform1i(this->getUnf("uLayer1"), 1);
    glUniform1i(this->getUnf("uLayer2"), 2);
    glUniform1i(this->getUnf("uLayer3"), 3);
    glUniform1i(this->getUnf("uLayer4"), 4);
    glUniform1i(this->getUnf("uLayer5"), 5);
    glUniform1i(this->getUnf("uLayer6"), 6);
    glUniform1i(this->getUnf("uLayer7"), 7);
    glUniform1i(this->getUnf("uAlphaTexture"), 8);
    glUniform1i(this->getUnf("uAlphaTexture2"), 9);
    glUseProgram(0);
}
