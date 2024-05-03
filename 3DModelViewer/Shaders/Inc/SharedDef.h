#ifndef _SHARED_DEF_H_
#define _SHARED_DEF_H_

// C++ definitions
#if defined(__cplusplus)
#define _glsl_global_var static
#define _glsl_inline_proc inline

#include <glm/glm.hpp>

using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;
using mat4 = glm::mat4;

using uint = unsigned int;

#else
#define _glsl_global_var
#define _glsl_inline_proc

#endif // __cplusplus

#endif // _SHARED_DEF_H_
