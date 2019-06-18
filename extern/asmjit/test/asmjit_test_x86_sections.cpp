// [AsmJit]
// Machine Code Generation for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// This is a working example that demonstrates how multiple sections can be
// used in a JIT-based code generator. It shows also the necessary tooling
// that is expected to be done by the user when the feature is used. It's
// important to handle the following cases:
//
//   - Assign offsets to sections when the code generation is finished.
//   - Tell the CodeHolder to resolve unresolved links and check whether
//     all links were resolved.
//   - Relocate the code
//   - Copy the code to the location you want.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./asmjit.h"

using namespace asmjit;

// The generated function is very simple, it only accesses the built-in data
// (from .data section) at the index as provided by its first argument. This
// data is inlined into the resulting function so we can use it this array
// for verification that the function returns correct values.
static const uint8_t dataArray[] = { 2, 9, 4, 7, 1, 3, 8, 5, 6, 0 };

static void fail(const char* message, Error err) {
  printf("%s: %s\n", message, DebugUtils::errorAsString(err));
  exit(1);
}

int main(int argc, char* argv[]) {
  ASMJIT_UNUSED(argc);
  ASMJIT_UNUSED(argv);

  CodeInfo codeInfo(ArchInfo::kIdHost);
  JitAllocator allocator;

  FileLogger logger(stdout);
  logger.setIndentation(FormatOptions::kIndentationCode, 2);

  CodeHolder code;
  code.init(codeInfo);
  code.setLogger(&logger);

  Section* section;
  Error err = code.newSection(&section, ".data", SIZE_MAX, 0, 8);

  if (err) {
    fail("Failed to create a .data section", err);
  }
  else {
    printf("Generating code:\n");
    x86::Assembler a(&code);
    x86::Gp idx = a.zax();
    x86::Gp addr = a.zcx();

    Label data = a.newLabel();

    FuncDetail func;
    func.init(FuncSignatureT<size_t, size_t>(CallConv::kIdHost));

    FuncFrame frame;
    frame.init(func);
    frame.addDirtyRegs(idx, addr);

    FuncArgsAssignment args(&func);
    args.assignAll(idx);
    args.updateFuncFrame(frame);
    frame.finalize();

    a.emitProlog(frame);
    a.emitArgsAssignment(frame, args);

    a.lea(addr, x86::ptr(data));
    a.movzx(idx, x86::byte_ptr(addr, idx));

    a.emitEpilog(frame);

    a.section(section);
    a.bind(data);

    a.embed(dataArray, sizeof(dataArray));
  }

  // Manually change he offsets of each section, start at 0. This code is very
  // similar to what `CodeHolder::flatten()` does, however, it's shown here
  // how to do it explicitly.
  printf("\nCalculating section offsets:\n");
  uint64_t offset = 0;
  for (Section* section : code.sections()) {
    offset = Support::alignUp(offset, section->alignment());
    section->setOffset(offset);
    offset += section->realSize();

    printf("  [0x%08X %s] {Id=%u Size=%u}\n",
           uint32_t(section->offset()),
           section->name(),
           section->id(),
           uint32_t(section->realSize()));
  }
  size_t codeSize = size_t(offset);
  printf("  Final code size: %zu\n", codeSize);

  // Resolve cross-section links (if any). On 32-bit X86 this is not necessary
  // as this is handled through relocations as the addressing is different.
  if (code.hasUnresolvedLinks()) {
    printf("\nResolving cross-section links:\n");
    printf("  Before 'resolveUnresolvedLinks()': %zu\n", code.unresolvedLinkCount());

    err = code.resolveUnresolvedLinks();
    if (err)
      fail("Failed to resolve cross-section links", err);
    printf("  After 'resolveUnresolvedLinks()': %zu\n", code.unresolvedLinkCount());
  }

  // Allocate memory for the function and relocate it there.
  void* roPtr;
  void* rwPtr;
  err = allocator.alloc(&roPtr, &rwPtr, codeSize);
  if (err)
    fail("Failed to allocate executable memory", err);

  // Relocate to the base-address of the allocated memory.
  code.relocateToBase(uint64_t(uintptr_t(roPtr)));

  // Copy the flattened code into `mem.rw`. There are two ways. You can either copy
  // everything manually by iterating over all sections or use `copyFlattenedData`.
  // This code is similar to what `copyFlattenedData(p, codeSize, 0)` would do:
  for (Section* section : code.sections())
    memcpy(static_cast<uint8_t*>(rwPtr) + size_t(section->offset()), section->data(), section->bufferSize());

  // Execute the function and test whether it works.
  typedef size_t (*Func)(size_t idx);
  Func fn = (Func)roPtr;

  printf("\nTesting the generated function:\n");
  if (fn(0) != dataArray[0] ||
      fn(3) != dataArray[3] ||
      fn(6) != dataArray[6] ||
      fn(9) != dataArray[9] ) {
    printf("  [FAILED] The generated function returned incorrect result(s)\n");
    return 1;
  }
  else {
    printf("  [PASSED] The generated function returned expected results\n");
  }

  allocator.release((void*)fn);
  return 0;
}
