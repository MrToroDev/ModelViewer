#ifndef _LIGHTS_H_
#define _LIGHTS_H_

#include "SharedDef.h"
#include "AppSettings.h"

_glsl_global_var const uint XGR_TYPE_DIRECTIONAL_LIGHT = 0;
_glsl_global_var const uint XGR_TYPE_POINT_LIGHT = 1;
_glsl_global_var const uint XGR_TYPE_SPOT_LIGHT = 2;
_glsl_global_var const uint XGR_TYPE_NULL_LIGHT = 0xFF;

_glsl_global_var const uint XGR_SAMPLER_LINEAR_BINDING = 0;
_glsl_global_var const uint XGR_SAMPLER_NEAREST_BINDING = 1;

struct Material
{
    vec4 Ambient;         // 16 bytes
    vec4 Diffuse;         // 16 bytes
    vec4 Specular;        // 16 bytes
};


_glsl_inline_proc Material fxCreateTextureMaterial(vec4 ambient) {
    Material mat;
    mat.Diffuse = vec4(1.0f);
    mat.Specular = vec4(1.0f);
    mat.Ambient = ambient;

    return mat;
}

struct Light
{
    vec4 Color;             // 16 bytes
    vec4 Direction;         // 16 bytes
    vec4 Position;          // 16 bytes
    float spotCutOff;       // 4 bytes
    float spotOutCutOff;    // 4 bytes
    uint LightType;         // 4 bytes
    uint Enable;            // 4 bytes
};

_glsl_inline_proc Light fxCreateNullLight() {
    Light l;
    l.LightType = XGR_TYPE_NULL_LIGHT;
    l.Enable = 0;
    return l;
}

_glsl_inline_proc Light fxCreateDirectionalLight(vec4 Color, vec3 Direction) {
    Light l;
    l.Color = Color;
    l.Direction = vec4(Direction, 0.0f);
    l.Enable = 1;
    l.LightType = XGR_TYPE_DIRECTIONAL_LIGHT;

    return l;
}

_glsl_inline_proc Light fxCreatePointLight(vec4 Color, vec3 Position) {
    Light l;
    l.Color = Color;
    l.Position = vec4(Position, 0.0f);
    l.Enable = 1;
    l.LightType = XGR_TYPE_POINT_LIGHT;

    return l;
}

#endif // _LIGHTS_H_
