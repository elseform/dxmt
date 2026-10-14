// Detector for the converter half of the shader interface relaxations
// A vertex input missing from the input layout is zeroed in
// the prologue, and a pixel input no upstream stage writes is zeroed instead of
// becoming a fragment input. Runs without a Metal device: conversion only.
#include "airconv_public.h"
#include "DXBCParser/DXBCUtils.h"
#include "shader_interface_fixtures.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

static int failures = 0;

#define CHECK(cond, ...)                                                       \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::fprintf(stderr, "FAIL %s:%d: ", __FILE__, __LINE__);                \
      std::fprintf(stderr, __VA_ARGS__);                                       \
      std::fprintf(stderr, "\n");                                              \
      failures++;                                                              \
    }                                                                          \
  } while (0)

// MTLAttributeFormat values used by the fixtures.
static const uint32_t kFloat2 = 29, kFloat3 = 30, kFloat4 = 31;

static std::string compile(
    sm50_shader_t shader, SM50_SHADER_COMPILATION_ARGUMENT_DATA *args,
    const char *name, bool *ok) {
  sm50_bitcode_t bitcode = {};
  sm50_error_t error = {};
  *ok = SM50Compile(shader, args, name, &bitcode, &error) == 0;
  if (!*ok) {
    char message[512] = {};
    SM50GetErrorMessage(error, message, sizeof(message));
    std::fprintf(stderr, "SM50Compile(%s): %s\n", name, message);
    SM50FreeError(error);
    return {};
  }
  SM50_COMPILED_BITCODE data = {};
  SM50GetCompiledBitcode(bitcode, &data);
  std::string out((const char *)(uintptr_t)data.Data, (size_t)data.Size);
  SM50DestroyBitcode(bitcode);
  return out;
}

static bool contains(const std::string &haystack, const char *needle) {
  return haystack.find(needle) != std::string::npos;
}

int main() {
  using namespace microsoft;

  // Register numbers of the fixture's inputs.
  //
  // Known gap: vkd3d-shader's HLSL path marks every declared input component
  // as always read (`element->used_mask = element->mask; /* FIXME */` in
  // hlsl_codegen.c), so NORMAL below has a full always-read mask although it is
  // never read. CreateInputLayout's read-mask relaxation therefore does not
  // apply to vkd3d-compiled vertex shaders yet. Add the signature check here
  // once the compiler reports the real mask.
  CSignatureParser parser;
  CHECK(DXBCGetInputSignature(kVsDxbc, &parser) == S_OK, "input signature");
  const D3D11_SIGNATURE_PARAMETER *params = nullptr;
  UINT count = parser.GetParameters(&params);
  uint32_t reg_position = ~0u, reg_normal = ~0u, reg_texcoord = ~0u;
  for (UINT i = 0; i < count; i++) {
    if (!strcasecmp(params[i].SemanticName, "POSITION"))
      reg_position = params[i].Register;
    else if (!strcasecmp(params[i].SemanticName, "NORMAL"))
      reg_normal = params[i].Register;
    else if (!strcasecmp(params[i].SemanticName, "TEXCOORD"))
      reg_texcoord = params[i].Register;
  }
  CHECK(reg_position != ~0u && reg_normal != ~0u && reg_texcoord != ~0u,
        "fixture signature lost an input");

  sm50_shader_t vs = {}, ps = {};
  MTL_SHADER_REFLECTION vs_refl = {}, ps_refl = {};
  sm50_error_t error = {};
  CHECK(SM50Initialize(kVsDxbc, sizeof(kVsDxbc), &vs, &vs_refl, &error) == 0,
        "initialize vs");
  CHECK(SM50Initialize(kPsDxbc, sizeof(kPsDxbc), &ps, &ps_refl, &error) == 0,
        "initialize ps");
  if (failures)
    return 1;

  // Reflection: SV_Position takes register 0 in both stages. The vertex shader
  // writes TEXCOORD0 (register 1) only; the pixel shader reads TEXCOORD0 and
  // TEXCOORD1 (registers 1 and 2).
  CHECK(vs_refl.OutputRegisterMask == 0x2, "vs output mask %x",
        vs_refl.OutputRegisterMask);
  CHECK(ps_refl.InputRegisterMask == 0x6, "ps input mask %x",
        ps_refl.InputRegisterMask);
  const uint32_t unwritten = ps_refl.InputRegisterMask & ~vs_refl.OutputRegisterMask;
  CHECK(unwritten == 0x4, "unwritten mask %x", unwritten);

  // Vertex shader: a layout that omits NORMAL converts, and the omitted input
  // is zeroed instead of pulled, so the code differs from the full layout.
  SM50_IA_INPUT_ELEMENT full[3] = {
      {reg_position, 0, 0, kFloat4, 0, 1},
      {reg_normal, 0, 16, kFloat3, 0, 1},
      {reg_texcoord, 0, 28, kFloat2, 0, 1},
  };
  SM50_IA_INPUT_ELEMENT partial[2] = {full[0], full[2]};
  SM50_SHADER_IA_INPUT_LAYOUT_DATA layout = {};
  layout.type = SM50_SHADER_IA_INPUT_LAYOUT;
  layout.slot_mask = 0x1;
  layout.elements = full;
  layout.num_elements = 3;
  bool ok_full = false, ok_partial = false;
  std::string vs_full = compile(
      vs, (SM50_SHADER_COMPILATION_ARGUMENT_DATA *)&layout, "vs_full", &ok_full);
  layout.elements = partial;
  layout.num_elements = 2;
  std::string vs_partial = compile(
      vs, (SM50_SHADER_COMPILATION_ARGUMENT_DATA *)&layout, "vs_partial",
      &ok_partial);
  CHECK(ok_full, "vs with the full layout converts");
  CHECK(ok_partial, "vs with NORMAL missing from the layout converts");
  CHECK(vs_full != vs_partial, "the omitted input is zeroed, not pulled");

  // Pixel shader: register 2 is a fragment input unless the upstream stage
  // does not write it.
  SM50_SHADER_PSO_PIXEL_SHADER_DATA pso = {};
  pso.type = SM50_SHADER_PSO_PIXEL_SHADER;
  pso.sample_mask = 0xffffffff;
  bool ok_linked = false, ok_unwritten = false;
  std::string ps_linked = compile(
      ps, (SM50_SHADER_COMPILATION_ARGUMENT_DATA *)&pso, "ps_linked", &ok_linked);
  pso.unwritten_input_reg_mask = unwritten;
  std::string ps_unwritten = compile(
      ps, (SM50_SHADER_COMPILATION_ARGUMENT_DATA *)&pso, "ps_unwritten",
      &ok_unwritten);
  CHECK(ok_linked && ok_unwritten, "ps converts with and without the mask");
  CHECK(contains(ps_linked, "reg2_0"), "linked ps has the reg2 fragment input");
  CHECK(!contains(ps_unwritten, "reg2_0"),
        "unwritten reg2 is not a fragment input");
  CHECK(contains(ps_unwritten, "reg1_0"), "written reg1 stays a fragment input");

  SM50Destroy(vs);
  SM50Destroy(ps);
  if (failures) {
    std::fprintf(stderr, "%d check(s) failed\n", failures);
    return 1;
  }
  std::printf("shader interface relaxations: ok\n");
  return 0;
}
