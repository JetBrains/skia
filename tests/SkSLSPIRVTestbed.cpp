/*
 * Copyright 2020 Google LLC
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "include/core/SkTypes.h"
#include "src/sksl/SkSLCompiler.h"
#include "src/sksl/SkSLProgramKind.h"
#include "src/sksl/SkSLProgramSettings.h"
#include "src/sksl/SkSLUtil.h"
#include "src/sksl/codegen/SkSLNativeShader.h"
#include "src/sksl/codegen/SkSLSPIRVCodeGenerator.h"
#include "src/sksl/ir/SkSLProgram.h"
#include "tests/Test.h"

#include <memory>
#include <string>

static constexpr uint32_t kOpImageSampleImplicitLod = 87;
static constexpr uint32_t kOpImageSampleExplicitLod = 88;

static void test(skiatest::Reporter* r,
                 const char* src,
                 SkSL::ProgramKind kind = SkSL::ProgramKind::kFragment) {
    SkSL::Compiler compiler;
    SkSL::ProgramSettings settings;
    std::unique_ptr<SkSL::Program> program = compiler.convertProgram(kind, std::string(src),
                                                                     settings);
    if (!program) {
        SkDebugf("Unexpected error compiling %s\n%s", src, compiler.errorText().c_str());
        REPORTER_ASSERT(r, program);
    } else {
        SkSL::NativeShader output;
        REPORTER_ASSERT(r, SkSL::ToSPIRV(*program, SkSL::ShaderCapsFactory::Default(), &output));
        REPORTER_ASSERT(r, !output.fBinary.empty());
    }
}

static bool contains_op(SkSpan<const uint32_t> binary, uint32_t op) {
    size_t offset = 5;  // Skip the SPIR-V header.
    while (offset < binary.size()) {
        uint32_t instruction = binary[offset];
        size_t length = instruction >> 16;
        if (length == 0 || offset + length > binary.size()) {
            return false;
        }
        if ((instruction & 0xFFFF) == op) {
            return true;
        }
        offset += length;
    }
    return false;
}

static void test_explicit_lod_for_runtime_effect(skiatest::Reporter* r) {
    SkSL::Compiler compiler;
    SkSL::ProgramSettings settings;
    settings.fForceExplicitLODZeroForUnmipmappedTextures = true;
    auto program = compiler.convertProgram(
            SkSL::ProgramKind::kRuntimeShader,
            R"__SkSL__(
                uniform shader child;
                half4 main(float2 coord) {
                    return child.eval(coord);
                }
            )__SkSL__",
            settings);
    if (!program) {
        ERRORF(r, "Unexpected error compiling runtime effect:\n%s",
               compiler.errorText().c_str());
        return;
    }

    SkSL::NativeShader output;
    REPORTER_ASSERT(r, SkSL::ToSPIRV(*program, SkSL::ShaderCapsFactory::Default(), &output));
    REPORTER_ASSERT(r, contains_op(output.fBinary, kOpImageSampleExplicitLod));
    REPORTER_ASSERT(r, !contains_op(output.fBinary, kOpImageSampleImplicitLod));
}

DEF_TEST(SkSLSPIRVTestbed, r) {
    // Add in your SkSL here.
    test(r,
         R"__SkSL__(
             void main() {
                 sk_FragColor = half4(0);
             }
         )__SkSL__");
    test_explicit_lod_for_runtime_effect(r);
}
