#include "include/gpu/ganesh/gl/GrGLAssembleInterface.h"
#include "include/gpu/ganesh/gl/GrGLFunctions.h"
#include "include/gpu/ganesh/gl/GrGLInterface.h"

#include <cstring>
#include "include/core/SkTypes.h"

// Each declaration remains undefined when Skia is linked. Because Skiko links
// with --allow-undefined, these become functions imported by the final Wasm
// module.
//
// GrGLWasmProcs.inc contains entries such as:
//
//     GL_PROC(GetString)
//     GL_PROC(BindBuffer)
//     GL_PROC(DrawArrays)
//
extern "C" {

#define GL_PROC(F) GrGL##F##Fn gl##F;
#include "src/gpu/ganesh/gl/wasm/GrGLWasmProcs.inc"
#undef GL_PROC

GrGLTextureBarrierFn glTextureBarrierNV;
GrGLInsertEventMarkerFn glInsertEventMarkerEXT;
GrGLPopGroupMarkerFn glPopGroupMarkerEXT;
GrGLPushGroupMarkerFn glPushGroupMarkerEXT;

}  // extern "C"

static GrGLFuncPtr wasm_host_get_gl_proc(void*, const char* name) {
#define GL_PROC(F)                                                        \
    if (std::strcmp(name, "gl" #F) == 0) {                               \
        return reinterpret_cast<GrGLFuncPtr>(&gl##F);                    \
    }

#include "src/gpu/ganesh/gl/wasm/GrGLWasmProcs.inc"
#undef GL_PROC

#define GL_ALIAS(F, S)                                                     \
    if (std::strcmp(name, "gl" #F #S) == 0) {                             \
        return reinterpret_cast<GrGLFuncPtr>(&gl##F##S);                  \
    }

GL_ALIAS(TextureBarrier, NV)
GL_ALIAS(InsertEventMarker, EXT)
GL_ALIAS(PopGroupMarker, EXT)
GL_ALIAS(PushGroupMarker, EXT)

#undef GL_ALIAS

    // Returning nullptr is correct for an unavailable function or extension.
    SkDebugf("[Skiko Wasm GL] unresolved function: %s\n", name);
    return nullptr;
}

namespace GrGLInterfaces {

sk_sp<const GrGLInterface> MakeWasmHostGL() {
    return GrGLMakeAssembledGLInterface(
        nullptr,
        wasm_host_get_gl_proc
    );
}

}  // namespace GrGLInterfaces

#if !defined(SK_DISABLE_LEGACY_GL_MAKE_NATIVE_INTERFACE)

sk_sp<const GrGLInterface> GrGLMakeNativeInterface() {
    return GrGLInterfaces::MakeWasmHostGL();
}

#endif