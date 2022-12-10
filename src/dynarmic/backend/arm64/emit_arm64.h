/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <vector>

#include <mcl/stdint.hpp>
#include <tsl/robin_map.h>

#include "dynarmic/interface/A32/coprocessor.h"
#include "dynarmic/interface/optimization_flags.h"
#include "dynarmic/ir/location_descriptor.h"

namespace oaknut {
struct PointerCodeGeneratorPolicy;
template<typename>
class BasicCodeGenerator;
using CodeGenerator = BasicCodeGenerator<PointerCodeGeneratorPolicy>;
struct Label;
}  // namespace oaknut

namespace Dynarmic::FP {
class FPCR;
}  // namespace Dynarmic::FP

namespace Dynarmic::IR {
class Block;
class Inst;
enum class Cond;
enum class Opcode;
}  // namespace Dynarmic::IR

namespace Dynarmic::Backend::Arm64 {

struct EmitContext;

using CodePtr = std::byte*;

enum class LinkTarget {
    ReturnToDispatcher,
    ReturnFromRunCode,
    ReadMemory8,
    ReadMemory16,
    ReadMemory32,
    ReadMemory64,
    ReadMemory128,
    ExclusiveReadMemory8,
    ExclusiveReadMemory16,
    ExclusiveReadMemory32,
    ExclusiveReadMemory64,
    ExclusiveReadMemory128,
    WriteMemory8,
    WriteMemory16,
    WriteMemory32,
    WriteMemory64,
    WriteMemory128,
    ExclusiveWriteMemory8,
    ExclusiveWriteMemory16,
    ExclusiveWriteMemory32,
    ExclusiveWriteMemory64,
    ExclusiveWriteMemory128,
    CallSVC,
    ExceptionRaised,
    InstructionSynchronizationBarrierRaised,
    InstructionCacheOperationRaised,
    DataCacheOperationRaised,
    GetCNTPCT,
    AddTicks,
    GetTicksRemaining,
};

struct Relocation {
    std::ptrdiff_t code_offset;
    LinkTarget target;
};

struct BlockRelocation {
    std::ptrdiff_t code_offset;
};

struct EmittedBlockInfo {
    CodePtr entry_point;
    size_t size;
    std::vector<Relocation> relocations;
    tsl::robin_map<IR::LocationDescriptor, std::vector<BlockRelocation>> block_relocations;
};

struct EmitConfig {
    OptimizationFlag optimizations;
    bool HasOptimization(OptimizationFlag f) const { return (f & optimizations) != no_optimizations; }

    bool hook_isb;

    // System registers
    u64 cntfreq_el0;
    u32 ctr_el0;
    u32 dczid_el0;
    const u64* tpidrro_el0;
    u64* tpidr_el0;

    // Memory
    bool check_halt_on_memory_access;

    // Page table
    u64 page_table_pointer;
    size_t page_table_address_space_bits;
    int page_table_pointer_mask_bits;
    bool silently_mirror_page_table;
    bool absolute_offset_page_table;
    u8 detect_misaligned_access_via_page_table;
    bool only_detect_misalignment_via_page_table_on_page_boundary;

    // Timing
    bool wall_clock_cntpct;
    bool enable_cycle_counting;

    // Endianness
    bool always_little_endian;

    // Frontend specific callbacks
    FP::FPCR (*descriptor_to_fpcr)(const IR::LocationDescriptor& descriptor);
    oaknut::Label (*emit_cond)(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Cond cond);
    void (*emit_condition_failed_terminal)(oaknut::CodeGenerator& code, EmitContext& ctx);
    void (*emit_terminal)(oaknut::CodeGenerator& code, EmitContext& ctx);
    void (*emit_check_memory_abort)(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst, oaknut::Label& end);

    // State offsets
    size_t state_nzcv_offset;
    size_t state_fpsr_offset;
    size_t state_exclusive_state_offset;

    // A32 specific
    std::array<std::shared_ptr<A32::Coprocessor>, 16> coprocessors{};
};

EmittedBlockInfo EmitArm64(oaknut::CodeGenerator& code, IR::Block block, const EmitConfig& emit_conf);

template<IR::Opcode op>
void EmitIR(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst);
void EmitRelocation(oaknut::CodeGenerator& code, EmitContext& ctx, LinkTarget link_target);
void EmitBlockLinkRelocation(oaknut::CodeGenerator& code, EmitContext& ctx, const IR::LocationDescriptor& descriptor);
oaknut::Label EmitA32Cond(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Cond cond);
oaknut::Label EmitA64Cond(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Cond cond);
void EmitA32Terminal(oaknut::CodeGenerator& code, EmitContext& ctx);
void EmitA64Terminal(oaknut::CodeGenerator& code, EmitContext& ctx);
void EmitA32ConditionFailedTerminal(oaknut::CodeGenerator& code, EmitContext& ctx);
void EmitA64ConditionFailedTerminal(oaknut::CodeGenerator& code, EmitContext& ctx);
void EmitA32CheckMemoryAbort(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst, oaknut::Label& end);
void EmitA64CheckMemoryAbort(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst, oaknut::Label& end);

}  // namespace Dynarmic::Backend::Arm64
