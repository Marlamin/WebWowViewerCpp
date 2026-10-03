//
// Created by Deamon on 9/4/2018.
//

#include "IDeviceFactory.h"
#include "ogl3.3/GDeviceGL33.h"

HGDevice IDeviceFactory::createDevice() {
    auto device = std::make_shared<GDeviceGL33>();
    device->initialize();
    return device;
}

