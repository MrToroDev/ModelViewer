#version 460
layout(set = 0, binding = 0) uniform sampler2DMS renderTarget_texture;

layout(location = 0) in vec2 inTextcoord;
layout(location = 1) in vec4 inPos;

layout(location = 0) out vec4 FragColor;

void main()
{
    vec4 pixel = vec4(0.0f);
    
    const int samples = textureSamples(renderTarget_texture);
    const ivec2 texSize = textureSize(renderTarget_texture);
    const ivec2 texCoord = ivec2(inTextcoord * texSize);

    for (int i = 0; i < samples; i++) {
        pixel += texelFetch(renderTarget_texture, texCoord, i);
    }
    pixel /= float(samples);

    float gamma = 2.1;
    pixel.rgb = pow(pixel.rgb, vec3(1.0f / gamma));
    
    FragColor = pixel;
}