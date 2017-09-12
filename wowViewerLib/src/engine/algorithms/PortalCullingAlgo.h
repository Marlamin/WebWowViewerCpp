//
// Created by deamon on 11.09.17.
//

#ifndef WEBWOWVIEWERCPP_PORTALCULLINGALGO_H
#define WEBWOWVIEWERCPP_PORTALCULLINGALGO_H


#include "../objects/wmoObject.h"

class PortalCullingAlgo {
    bool startTraversingFromInteriorWMO (
        WmoObject &wmoObject,
        WmoGroupObject &wmoGroupsResult,
        mathfu::vec4 &cameraVec4,
        mathfu::mat4 &lookat,
        std::vector<mathfu::vec4> &frustumPlanes,
        std::set<M2Object*> &m2RenderedThisFrame);

    bool startTraversingFromExterior (
            WmoObject &wmoObject,
            mathfu::vec4 &cameraVec4,
            mathfu::mat4 &lookat,
            std::vector<mathfu::vec4> &frustumPlanes,
            std::set<M2Object*> &m2RenderedThisFrame);

    void checkGroupDoodads(
            WmoObject &wmoObject,
            int groupId,
            mathfu::vec4 &cameraVec4,
            std::vector<mathfu::vec4> &frustumPlanes,
            int level,
            std::set<M2Object*> &m2RenderedThisFrame);

    void transverseGroupWMO (
            WmoObject &wmoObject,
            int groupId,
            bool fromInterior,
            mathfu::vec4 &cameraVec4,
            mathfu::vec4 &cameraLocal,
            mathfu::mat4 &lookat,
            std::vector<mathfu::vec4> &frustumPlanes,
            int level,
            std::set<M2Object*> &m2RenderedThisFrame);


};


#endif //WEBWOWVIEWERCPP_PORTALCULLINGALGO_H