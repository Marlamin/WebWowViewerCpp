//
// Created by Deamon on 7/8/2018.
//

#ifndef AWEBWOWVIEWERCPP_GM2SHADERPERMUTATION_H
#define AWEBWOWVIEWERCPP_GM2SHADERPERMUTATION_H

#include "../GShaderPermutation.h"

class GM2ShaderPermutation : public GShaderPermutation{
    friend class GDeviceGL33;
protected:
    explicit GM2ShaderPermutation(std::string &shaderName, IDevice *device);

    virtual void compileShader() override;

};


#endif //AWEBWOWVIEWERCPP_GM2SHADERPERMUTATION_H
