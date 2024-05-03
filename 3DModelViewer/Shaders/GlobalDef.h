#ifndef _DEFINITION_HEADER_
#define _DEFINITION_HEADER_

#include "Inc/SharedDef.h"
#include "Inc/Lights.h"
#include "Inc/AppSettings.h"

struct CameraMatrix
{
    mat4 projection;
    mat4 invProjection;
    mat4 view;
    mat4 invView;
    vec4 viewPosition;
};

#endif