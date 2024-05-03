#ifndef _APP_SETTINGS_H_
#define _APP_SETTINGS_H_

#include "SharedDef.h"

#ifdef __cplusplus
namespace AppSettings {
#endif

_glsl_global_var const uint MAX_BINDLESS = 1000u;

_glsl_global_var const uint MAX_LIGHTS_COUNT = 3u;

_glsl_global_var const uint MAX_SAMPLERS = 3u;

#ifdef __cplusplus
}
#endif

#endif // _APP_SETTINGS_H_