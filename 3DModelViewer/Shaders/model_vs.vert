#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_buffer_reference : require

#include "GlobalDef.h"

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vColor;
layout(location = 2) in vec2 vUV;
layout(location = 3) in vec3 vNormal;
layout(location = 4) in vec3 vTangent;
layout(location = 5) in vec3 vBitangent; 

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outColor;
layout(location = 3) out vec3 outWorldPosition;
layout(location = 4) out vec3 outEyePosition;
layout(location = 5) out mat3 outTBN;

layout(set = 0, binding = 0) uniform CameraSceneData {
    CameraMatrix camera;
}sceneCamera;

layout(push_constant) uniform GPUDrawData {
    mat4 modelMatrix;
    mat4 inverseTransposedModelMatrix;
} drawData;

void main()
{
    vec4 position = drawData.modelMatrix * vec4(vPosition, 1.0);

    gl_Position = sceneCamera.camera.projection * sceneCamera.camera.view * position;

    outUV = vUV;
    outNormal = normalize(mat3(drawData.inverseTransposedModelMatrix) * vNormal);
    outColor = vColor;

    outWorldPosition = position.xyz;

    outEyePosition = sceneCamera.camera.viewPosition.xyz;

    vec3 T = normalize(vec3(drawData.modelMatrix * vec4(vTangent,   0.0)));
    vec3 B = normalize(vec3(drawData.modelMatrix * vec4(vBitangent, 0.0)));
    vec3 N = normalize(vec3(drawData.modelMatrix * vec4(vNormal,    0.0)));
    outTBN = mat3(T, B, N);
}