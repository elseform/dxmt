// Detector for the converter half of the shader interface relaxations
// A vertex input missing from the input layout is zeroed in
// the prologue, and a pixel input no upstream stage writes is zeroed instead of
// becoming a fragment input. Runs without a Metal device: conversion only.
#include "airconv_public.h"
#include "DXBCParser/DXBCUtils.h"
#include "shader_interface_fixtures.h"
#include "shader_interface_fxc.h"
#include "shader_interface_vkd3d.h"
#include "airconv_context.hpp"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/Error.h"

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

namespace dxmt::dxbc {
llvm::Error convertDXBC(sm50_shader_t, const char *, llvm::LLVMContext &,
                        llvm::Module &, SM50_SHADER_COMPILATION_ARGUMENT_DATA *);
}

// Inspect zero stores into the declared input register before optimization.
// A full-layout float3 may already have a zero-filled fourth component, which
// must not be mistaken for the missing NORMAL.xy components.
static uint32_t zero_components(sm50_shader_t shader,
    SM50_SHADER_COMPILATION_ARGUMENT_DATA *args, uint32_t reg) {
  llvm::LLVMContext context;
  context.setOpaquePointers(false);
  llvm::Module module("detector", context);
  dxmt::initializeModule(module);
  auto error = dxmt::dxbc::convertDXBC(shader, "detector", context, module, args);
  if (error) {
    CHECK(false, "%s", llvm::toString(std::move(error)).c_str());
    return 0;
  }
  uint32_t zeros = 0;
  for (auto &function : module) {
    llvm::AllocaInst *inputs = nullptr;
    for (auto &block : function)
      for (auto &instruction : block) {
        if (!inputs)
          if (auto *allocation = llvm::dyn_cast<llvm::AllocaInst>(&instruction))
            if (auto *array = llvm::dyn_cast<llvm::ArrayType>(allocation->getAllocatedType()))
              if (array->getElementType()->isVectorTy()) inputs = allocation;
        auto *store = llvm::dyn_cast<llvm::StoreInst>(&instruction);
        if (!store) continue;
        auto *value = llvm::dyn_cast<llvm::Constant>(store->getValueOperand());
        auto *pointer = llvm::dyn_cast<llvm::GetElementPtrInst>(store->getPointerOperand());
        if (!value || !value->isNullValue() || !pointer ||
            pointer->getPointerOperand() != inputs || pointer->getNumIndices() < 2) continue;
        auto *index = llvm::dyn_cast<llvm::ConstantInt>(pointer->getOperand(2));
        if (!index || index->getZExtValue() != reg) continue;
        if (pointer->getNumIndices() == 2) zeros |= 15;
        else if (auto *component = llvm::dyn_cast<llvm::ConstantInt>(pointer->getOperand(3)))
          zeros |= 1u << component->getZExtValue();
      }
  }
  return zeros;
}

static void run_unpacked(const unsigned char *kVsDxbc, size_t vs_size,
                         const unsigned char *kPsDxbc, size_t ps_size, bool read_normal) {

  using namespace microsoft;

  // Register numbers of the fixture's inputs. CreateInputLayout accepts a
  // layout that omits any input of the shader (ExtractMTLInputLayoutElements
  // warns and continues); the converter below supplies the zero. The always-read
  // mask is not consulted, because compilers disagree on it.
  CSignatureParser parser;
  CHECK(DXBCGetInputSignature(kVsDxbc, &parser) == S_OK, "input signature");
  const D3D11_SIGNATURE_PARAMETER *params = nullptr;
  UINT count = parser.GetParameters(&params);
  uint32_t reg_position = ~0u, reg_normal = ~0u, reg_texcoord = ~0u;
  for (UINT i = 0; i < count; i++) {
    if (!strcasecmp(params[i].SemanticName, "POSITION"))
      reg_position = params[i].Register;
    else if (!strcasecmp(params[i].SemanticName, "NORMAL")) {
      reg_normal = params[i].Register;
    }
    else if (!strcasecmp(params[i].SemanticName, "TEXCOORD"))
      reg_texcoord = params[i].Register;
  }
  CHECK(reg_position != ~0u && reg_normal != ~0u && reg_texcoord != ~0u,
        "fixture signature lost an input");

  sm50_shader_t vs = {}, ps = {};
  MTL_SHADER_REFLECTION vs_refl = {}, ps_refl = {};
  sm50_error_t error = {};
  CHECK(SM50Initialize(kVsDxbc, vs_size, &vs, &vs_refl, &error) == 0,
        "initialize vs");
  CHECK(SM50Initialize(kPsDxbc, ps_size, &ps, &ps_refl, &error) == 0,
        "initialize ps");
  if (failures)
    return;

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
  // is zeroed instead of pulled when read; an eliminated unread input is unchanged.
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
      vs, (SM50_SHADER_COMPILATION_ARGUMENT_DATA *)&layout, "vs", &ok_full);
  unsigned full_zeros = zero_components(vs, (SM50_SHADER_COMPILATION_ARGUMENT_DATA *)&layout, reg_normal);
  layout.elements = partial;
  layout.num_elements = 2;
  std::string vs_partial = compile(
      vs, (SM50_SHADER_COMPILATION_ARGUMENT_DATA *)&layout, "vs",
      &ok_partial);
  CHECK(ok_full, "vs with the full layout converts");
  CHECK(ok_partial, "vs with NORMAL missing from the layout converts");
  if (read_normal)
    CHECK((zero_components(vs, (SM50_SHADER_COMPILATION_ARGUMENT_DATA *)&layout, reg_normal) & 3) == 3 && (full_zeros & 3) == 0,
          "missing NORMAL.xy has explicit zero stores");
  else
    CHECK(vs_full == vs_partial, "unread NORMAL omission preserves converted output");

  // Pixel shader: register 2 is a fragment input unless the upstream stage
  // does not write it.
  SM50_SHADER_PSO_PIXEL_SHADER_DATA pso = {};
  pso.type = SM50_SHADER_PSO_PIXEL_SHADER;
  pso.sample_mask = 0xffffffff;
  bool ok_linked = false, ok_unwritten = false;
  std::string ps_linked = compile(
      ps, (SM50_SHADER_COMPILATION_ARGUMENT_DATA *)&pso, "ps", &ok_linked);
  unsigned linked_zeros = zero_components(ps, (SM50_SHADER_COMPILATION_ARGUMENT_DATA *)&pso, 2);
  pso.unwritten_input_reg_mask = unwritten;
  std::string ps_unwritten = compile(
      ps, (SM50_SHADER_COMPILATION_ARGUMENT_DATA *)&pso, "ps",
      &ok_unwritten);
  CHECK(zero_components(ps, (SM50_SHADER_COMPILATION_ARGUMENT_DATA *)&pso, 2) > linked_zeros,
        "unwritten pixel input has explicit zero stores");
  CHECK(ok_linked && ok_unwritten, "ps converts with and without the mask");
  CHECK(contains(ps_linked, "reg2_0"), "linked ps has the reg2 fragment input");
  CHECK(!contains(ps_unwritten, "reg2_0"),
        "unwritten reg2 is not a fragment input");
  CHECK(contains(ps_unwritten, "reg1_0"), "written reg1 stays a fragment input");

  SM50Destroy(vs);
  SM50Destroy(ps);
}

static void run_packed() {
  sm50_shader_t vs = {}, ps = {};
  MTL_SHADER_REFLECTION vr = {}, pr = {};
  sm50_error_t error = {};
  CHECK(SM50Initialize(kFxc_vs_packed, sizeof(kFxc_vs_packed), &vs, &vr, &error) == 0, "packed VS");
  CHECK(SM50Initialize(kFxc_ps_packed, sizeof(kFxc_ps_packed), &ps, &pr, &error) == 0, "packed PS");
  if (!vs || !ps) return;
  microsoft::CSignatureParser parser;
  CHECK(microsoft::DXBCGetInputSignature(kFxc_ps_packed, &parser) == S_OK, "packed signature");
  const microsoft::D3D11_SIGNATURE_PARAMETER *parameters;
  unsigned count = parser.GetParameters(&parameters), packed = 0;
  for (unsigned i = 0; i < count; ++i)
    if (parameters[i].SystemValue == microsoft::D3D10_SB_NAME_UNDEFINED && parameters[i].Register == 1)
      packed |= parameters[i].Mask;
  CHECK(packed == 15, "two float2 varyings occupy all four components");
  CHECK((pr.InputRegisterMask & ~vr.OutputRegisterMask) == 0, "supplied packed inputs stay linked");
  SM50_SHADER_PSO_PIXEL_SHADER_DATA pso = {};
  pso.type = SM50_SHADER_PSO_PIXEL_SHADER;
  pso.sample_mask = 0xffffffff;
  bool ok;
  std::string linked = compile(ps, (SM50_SHADER_COMPILATION_ARGUMENT_DATA *)&pso, "packed", &ok);
  CHECK(ok && contains(linked, "reg1_0") && contains(linked, "reg1_2"), "both packed varyings survive");
  unsigned zeros = zero_components(ps, (SM50_SHADER_COMPILATION_ARGUMENT_DATA *)&pso, 1);
  pso.unwritten_input_reg_mask = 2;
  std::string omitted = compile(ps, (SM50_SHADER_COMPILATION_ARGUMENT_DATA *)&pso, "packed", &ok);
  CHECK(ok && !contains(omitted, "reg1_0") && !contains(omitted, "reg1_2"), "missing packed register removes both varyings");
  CHECK(zero_components(ps, (SM50_SHADER_COMPILATION_ARGUMENT_DATA *)&pso, 1) == 15 && zeros == 0, "missing packed register zeros all four components");
  SM50Destroy(vs); SM50Destroy(ps);
}

int main() {
  run_unpacked(kVsDxbc, sizeof(kVsDxbc), kPsDxbc, sizeof(kPsDxbc), false);
  run_unpacked(kVkd_vs_read, sizeof(kVkd_vs_read), kVkd_ps, sizeof(kVkd_ps), true);
  run_unpacked(kFxc_vs_read, sizeof(kFxc_vs_read), kFxc_ps, sizeof(kFxc_ps), true);
  run_unpacked(kFxc_vs_unread, sizeof(kFxc_vs_unread), kFxc_ps, sizeof(kFxc_ps), false);
  run_packed();
  if (failures) {
    std::fprintf(stderr, "%d check(s) failed\n", failures);
    return 1;
  }
  std::puts("shader interface relaxations: vkd3d and Microsoft fxc passed");
  return 0;
}
