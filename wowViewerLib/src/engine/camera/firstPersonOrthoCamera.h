//
// Created by deamon on 18.05.17.
//

#ifndef WOWMAPVIEWERREVIVED_FIRSTPERSONORTHOCAMERA_H
#define WOWMAPVIEWERREVIVED_FIRSTPERSONORTHOCAMERA_H

#include <mathfu/vector.h>
#include <mathfu/glsl_mappings.h>
#include "CameraInterface.h"
#include "../../include/wowScene.h"


class FirstPersonOrthoCamera: public ICamera {
public:
    FirstPersonOrthoCamera(){};

private:
    mathfu::vec3 camera = {0, 0, 0};
    mathfu::vec3 lookAt = {0, 0, 0};
    mathfu::mat4 lookAtMat = {};

    float MDDepthPlus = 0;
    float MDDepthMinus = 0;
    float MDHorizontalPlus = 0;
    float MDHorizontalMinus = 0;

    float MDVerticalPlus = 0;
    float MDVerticalMinus = 0;

    float depthDiff = 0;

    bool staticCamera = false;

    float ah = 0;
    float av = 89;
public:
    //Implemented IControllable
    void addHorizontalViewDir(float val);
    void addVerticalViewDir(float val);
    void addForwardDiff(float val);
    void startMovingForward();
    void stopMovingForward();
    void startMovingBackwards();
    void stopMovingBackwards();
    void startStrafingLeft();
    void stopStrafingLeft();
    void startStrafingRight();
    void stopStrafingRight();
    void startMovingUp();
    void stopMovingUp();
    void startMovingDown();
    void stopMovingDown();

    void getCameraPosition(float *position) override {
        position[0] = camera.x;
        position[1] = camera.y;
        position[2] = camera.z;
    }

    mathfu::mat4 &getLookatMat() {
        return lookAtMat;
    }

public:
    //Implemented ICamera
    mathfu::vec3 getCameraPosition();
    mathfu::vec3 getCameraLookAt();

public:
    void tick(animTime_t timeDelta);
    void setCameraPos(float x, float y, float z);
};


#endif //WOWMAPVIEWERREVIVED_FIRSTPERSONORTHOCAMERA_H
