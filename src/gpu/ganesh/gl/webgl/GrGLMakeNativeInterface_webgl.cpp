/*
 * Copyright 2020 Google LLC
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "include/gpu/ganesh/gl/GrGLAssembleInterface.h"
#include "include/gpu/ganesh/gl/GrGLInterface.h"
#include "include/gpu/ganesh/gl/GrGLMakeWebGLInterface.h"

#include <GLES3/gl32.h>
#include <cstring>

static const GrGLubyte* GR_GL_FUNCTION_TYPE webgl_get_string_i(GrGLenum name, GrGLuint index) {
    const GrGLubyte* s = glGetStringi(name, index);
    if (name == 0x1F03 /* GL_EXTENSIONS */ && s) {
        const char* str = (const char*) s;
        // Filter out timer query extensions because we don't provide the required functions
        // for them (like glQueryCounterEXT) and they are not in the standard GLES 3.0 headers.
        if (strstr(str, "disjoint_timer_query")) {
            return (const GrGLubyte*) "";
        }
    }
    return s;
}

static const GrGLubyte* GR_GL_FUNCTION_TYPE webgl_get_string(GrGLenum name) {
    return glGetString(name);
}

static GrGLFuncPtr webgl_get_gl_proc(void* ctx, const char name[]) {
    if (0 == strcmp("glGetString", name)) {
        return (GrGLFuncPtr) webgl_get_string;
    }
    if (0 == strcmp("glGetStringi", name)) {
        return (GrGLFuncPtr) webgl_get_string_i;
    }

    #define M(X) if (0 == strcmp(#X, name)) { return (GrGLFuncPtr) X; }
    M(glActiveTexture)
    M(glAttachShader)
    M(glBeginQuery)
    M(glBindAttribLocation)
    M(glBindBuffer)
    M(glBindFramebuffer)
    M(glBindRenderbuffer)
    M(glBindSampler)
    M(glBindTexture)
    M(glBindVertexArray)
    M(glBlendColor)
    M(glBlendEquation)
    M(glBlendFunc)
    M(glBlitFramebuffer)
    M(glBufferData)
    M(glBufferSubData)
    M(glCheckFramebufferStatus)
    M(glClear)
    M(glClearColor)
    M(glClearStencil)
    M(glClientWaitSync)
    M(glColorMask)
    M(glCompileShader)
    M(glCompressedTexImage2D)
    M(glCompressedTexSubImage2D)
    M(glCopyBufferSubData)
    M(glCopyTexSubImage2D)
    M(glCreateProgram)
    M(glCreateShader)
    M(glCullFace)
    M(glDeleteBuffers)
    M(glDeleteFramebuffers)
    M(glDeleteProgram)
    M(glDeleteQueries)
    M(glDeleteRenderbuffers)
    M(glDeleteSamplers)
    M(glDeleteShader)
    M(glDeleteSync)
    M(glDeleteTextures)
    M(glDeleteVertexArrays)
    M(glDepthMask)
    M(glDisable)
    M(glDisableVertexAttribArray)
    M(glDrawArrays)
    M(glDrawArraysInstanced)
    M(glDrawBuffers)
    M(glDrawElements)
    M(glDrawElementsInstanced)
    M(glDrawRangeElements)
    M(glEnable)
    M(glEnableVertexAttribArray)
    M(glEndQuery)
    M(glFenceSync)
    M(glFinish)
    M(glFlush)
    M(glFramebufferRenderbuffer)
    M(glFramebufferTexture2D)
    M(glFrontFace)
    M(glGenBuffers)
    M(glGenFramebuffers)
    M(glGenQueries)
    M(glGenRenderbuffers)
    M(glGenSamplers)
    M(glGenTextures)
    M(glGenVertexArrays)
    M(glGenerateMipmap)
    M(glGetBufferParameteriv)
    M(glGetError)
    M(glGetFloatv)
    M(glGetFramebufferAttachmentParameteriv)
    M(glGetIntegerv)
    M(glGetInternalformativ)
    M(glGetProgramInfoLog)
    M(glGetProgramiv)
    M(glGetQueryObjectuiv)
    M(glGetQueryiv)
    M(glGetRenderbufferParameteriv)
    M(glGetShaderInfoLog)
    M(glGetShaderiv)
    M(glGetShaderPrecisionFormat)
    M(glGetUniformLocation)
    M(glInvalidateFramebuffer)
    M(glInvalidateSubFramebuffer)
    M(glIsSync)
    M(glIsTexture)
    M(glLineWidth)
    M(glLinkProgram)
    M(glPixelStorei)
    M(glReadBuffer)
    M(glReadPixels)
    M(glRenderbufferStorage)
    M(glRenderbufferStorageMultisample)
    M(glSamplerParameterf)
    M(glSamplerParameteri)
    M(glSamplerParameteriv)
    M(glScissor)
    M(glShaderSource)
    M(glStencilFunc)
    M(glStencilFuncSeparate)
    M(glStencilMask)
    M(glStencilMaskSeparate)
    M(glStencilOp)
    M(glStencilOpSeparate)
    M(glTexImage2D)
    M(glTexParameterf)
    M(glTexParameterfv)
    M(glTexParameteri)
    M(glTexParameteriv)
    M(glTexStorage2D)
    M(glTexSubImage2D)
    M(glUniform1f)
    M(glUniform1fv)
    M(glUniform1i)
    M(glUniform1iv)
    M(glUniform2f)
    M(glUniform2fv)
    M(glUniform2i)
    M(glUniform2iv)
    M(glUniform3f)
    M(glUniform3fv)
    M(glUniform3i)
    M(glUniform3iv)
    M(glUniform4f)
    M(glUniform4fv)
    M(glUniform4i)
    M(glUniform4iv)
    M(glUniformMatrix2fv)
    M(glUniformMatrix3fv)
    M(glUniformMatrix4fv)
    M(glUseProgram)
    M(glVertexAttrib1f)
    M(glVertexAttrib2fv)
    M(glVertexAttrib3fv)
    M(glVertexAttrib4fv)
    M(glVertexAttribDivisor)
    M(glVertexAttribIPointer)
    M(glVertexAttribPointer)
    M(glViewport)
    M(glWaitSync)
    #undef M

    // We explicitly do not use GetProcAddress or something similar because
    // its code size is quite large.
    return nullptr;
}

namespace GrGLInterfaces {
sk_sp<const GrGLInterface> MakeWebGL() {
    return GrGLMakeAssembledWebGLInterface(nullptr, webgl_get_gl_proc);
}
}  // namespace GrGLInterfaces

#if !defined(SK_DISABLE_LEGACY_GL_MAKE_NATIVE_INTERFACE)
sk_sp<const GrGLInterface> GrGLMakeNativeInterface() {
    return GrGLInterfaces::MakeWebGL();
}
#endif
