#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_buffer_reference : require

#include "GlobalDef.h"

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec3 inWorldPosition;
layout(location = 4) in vec3 inEyePosition;
layout(location = 5) in mat3 inTBN;

layout(location = 0) out vec4 FragColor;


layout(buffer_reference) readonly buffer LightData {
    Light raw_data[];
};

layout(push_constant) uniform GPUDrawColorData {
    layout(offset = 128) Material material;
    LightData lights;
    uint samplerID;
    uint materialID;

    uint showDiffuse;
    uint showNormal;
    uint showSpecular;
} drawData;

layout(set = 0, binding = 1) uniform texture2D uDiffuseTexture[];
layout(set = 0, binding = 2) uniform texture2D uSpecularTexture[];
layout(set = 0, binding = 3) uniform texture2D uNormalTexture[];
layout(set = 0, binding = 4) uniform sampler globalSamplers[];

bool isTextureValid(in texture2D tex) {
    ivec2 size = textureSize(sampler2D(tex, globalSamplers[XGR_SAMPLER_NEAREST_BINDING]), 0);

    return size != vec2(1, 0);
}

float saturate(float x) {
    return clamp(x, 0.0, 1.0);
}

const vec4 GlobalAmbient = vec4(0.1f, 0.1f, 0.1f, 1.0f);

void main()
{
    if (drawData.showDiffuse == 1) {
        FragColor = texture(sampler2D(uDiffuseTexture[drawData.materialID], globalSamplers[drawData.samplerID]), inUV);
        return;
    } else if (drawData.showNormal == 1) {
        FragColor = texture(sampler2D(uNormalTexture[drawData.materialID], globalSamplers[drawData.samplerID]), inUV);
        return;
    } else if (drawData.showSpecular == 1) {
        FragColor = texture(sampler2D(uSpecularTexture[drawData.materialID], globalSamplers[drawData.samplerID]), inUV);
        return;
    }

    vec4 diffuse = vec4(0.0f);
    vec4 specular = vec4(0.0f);

    vec3 N = inNormal;
    if (isTextureValid(uNormalTexture[drawData.materialID])) {
        N = texture(sampler2D(uNormalTexture[drawData.materialID], globalSamplers[drawData.samplerID]), inUV).rgb * 2.0f - 1.0f;
        N = normalize(inTBN * N);
    }

    vec3 V = normalize(inEyePosition - inWorldPosition);

    // Light Calculation
    for (int i = 0; i < MAX_LIGHTS_COUNT; i++) {
        Light light = drawData.lights.raw_data[i];

        // If the type is NULL or is disabled, ignore it
        if (light.Enable == 0 && light.LightType == XGR_TYPE_NULL_LIGHT)
            continue;
        
        vec3 lightColor = light.Color.rgb * light.Color.w;

        float lightIntensity;
        float attenuation = 1.0f;
        vec3 lightDir;
        
        if (light.LightType == XGR_TYPE_DIRECTIONAL_LIGHT) {
            lightDir = normalize(vec3(light.Direction));

            lightIntensity = max(dot(N, lightDir), 0);
        }

        if (light.LightType == XGR_TYPE_POINT_LIGHT) {
            lightDir = light.Position.xyz - inWorldPosition;
            attenuation = 1.0f / dot(lightDir, lightDir);

            lightDir = normalize(lightDir);

            // Diffuse
            lightIntensity = max(dot(N, lightDir), 0);
        }

        
        // Specular: Blinn-Phong Model
        // specularLight = pow(dot(n, h), s)
        // n -> normal
        // h -> half vector -> norm(lightDir + viewDir)
        vec3 H = normalize(lightDir + V);
        float spec = pow(saturate(dot(N, H)), drawData.material.Specular.w);
        
        diffuse += vec4(lightColor, 1.0) * lightIntensity * attenuation;
        specular += vec4(lightColor, 1.0) * spec * attenuation;
    }
    
    // Final Result

    if (isTextureValid(uDiffuseTexture[drawData.materialID])) {
        diffuse *= texture(sampler2D(uDiffuseTexture[drawData.materialID], globalSamplers[drawData.samplerID]), inUV);
    }
    else {
        diffuse *= drawData.material.Diffuse;    
    }

    if (isTextureValid(uSpecularTexture[drawData.materialID])) {
        specular.rgb *= texture(sampler2D(uSpecularTexture[drawData.materialID], globalSamplers[drawData.samplerID]), inUV).rgb;
        specular.a = 1;
    }
    else {
        specular.rgb *= drawData.material.Specular.rgb;
        specular.a = 1;   
    }

    vec4 ambient = drawData.material.Ambient * GlobalAmbient;

    FragColor = ambient + diffuse + specular;
}