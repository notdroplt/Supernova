#include <array>
#include <cstdint>
#include <functional>
#include <supernova.h>

#if __has_builtin(__builtin_expect)
#define expect(x) (__builtin_expect(x, 1))
#define unexpect(x) (__builtin_expect(x, 0))
#else
#define expect(x) x
#define unexpect(x) x
#endif

namespace {
using Thread = supernova::Thread;
using ProcessorCall = supernova::ProcessorCall;
using DestroyFor = supernova::ThreadDestruction;
using RInstr = supernova::RInstruction;
using SInstr = supernova::SInstruction;
using LInstr = supernova::LInstruction;
using Opcodes = supernova::inspx;
using thread_return = supernova::thread_return;

constexpr void dispatch_pcall(Thread &thread, ProcessorCall pcall) noexcept;

template <typename integer>
[[nodiscard]] constexpr auto fetch(Thread &t,
                                   uint64_t addr) noexcept -> integer {
  if expect (addr >= t.memsize()) {
    dispatch_pcall(t, ProcessorCall::MemoryLimit);
    return 0;
  }
  // NOLINTNEXTLINE: yeah it works
  return *reinterpret_cast<integer *>(t.memory().get() + addr);
}

template <typename integer>
constexpr auto place(Thread &t, uint64_t addr, integer v) noexcept -> void {
  if unexpect (addr >= t.memsize()) {
    dispatch_pcall(t, ProcessorCall::MemoryLimit);
    return;
  }
  // NOLINTNEXTLINE: looks good to me tho
  *reinterpret_cast<integer *>(t.memory().get() + addr) = v;
}

constexpr void pcall_minus_one(Thread &t) {
  switch (t.registers(Thread::pcall_reg)) {
  case 0x0000000000000000:
    t.registers(Thread::pcall_1stret) = 2;
    t.registers(Thread::pcall_2ndret) = t.model()->interrupt_count;
    break;
  case 0x0000000000000001:
    t.intvec() = t.registers(Thread::pcall_1stret);
    break;
  case 0x0000000100000000:
    t.registers(Thread::pcall_1stret) = 0;
    break;
  case 0x0000000300000000:
    t.registers(Thread::pcall_1stret) = 1;
    break;
  case 0x0000000300000001:
    t.signal() = DestroyFor::ProgramEnd;
    break;
  default:
    break;
  }
}

constexpr void dispatch_pcall(Thread &t, ProcessorCall pcall) noexcept {
  if expect (pcall == ProcessorCall::Functions) {
    return pcall_minus_one(t);
  }
  
  if unexpect (t.pcall() == ProcessorCall::DoubleFault) {
    t.pcall() = ProcessorCall::TripleFault;
    t.signal() = DestroyFor::InterruptCrashLoop;
  } else if unexpect (t.pcall() != ProcessorCall::NormalExecution) {
    t.pcall() = ProcessorCall::DoubleFault;
  } else {
    t.pcall() = pcall;
  }

  t.pcallret() = t.progc() + sizeof(uint64_t);

  t.progc() = fetch<uint64_t>(t, t.intvec() + pcall * sizeof(uint64_t));
}

using opcode_fun = void (*)(Thread &, uint64_t);
using dispatch_array_t = std::array<opcode_fun, Opcodes::instruction_count>;
namespace dispatchers {
[[using gnu: always_inline, hot]]
constexpr inline void invalid_instruction(Thread &t, uint64_t inst) noexcept {
  t.registers(supernova::Thread::pcall_invopc) = inst;
  dispatch_pcall(t, ProcessorCall::InvalidInstruction);
}

[[using gnu: always_inline, hot]]
constexpr inline void do_andr(Thread &t, uint64_t inst) noexcept {
  t.apply_instr(RInstr(inst), std::bit_and<>{});
}

[[using gnu: always_inline, hot]]
constexpr inline void do_andi(Thread &t, uint64_t inst) noexcept {
  t.apply_instr(SInstr(inst), std::bit_and<>{});
}

[[using gnu: always_inline, hot]]
constexpr inline void do_xorr(Thread &t, uint64_t inst) noexcept {
  t.apply_instr(RInstr(inst), std::bit_xor<>{});
}

[[using gnu: always_inline, hot]]
constexpr inline void do_xori(Thread &t, uint64_t inst) noexcept {
  t.apply_instr(SInstr(inst), std::bit_xor<>{});
}

[[using gnu: always_inline, hot]]
constexpr inline void do_orr(Thread &t, uint64_t inst) noexcept {
  t.apply_instr(RInstr(inst), std::bit_or<>{});
}

[[using gnu: always_inline, hot]]
constexpr inline void do_ori(Thread &t, uint64_t inst) noexcept {
  t.apply_instr(SInstr(inst), std::bit_or<>{});
}

[[using gnu: always_inline, hot]]
constexpr inline void do_not(Thread &t, uint64_t inst) noexcept {
  auto const &rinst = RInstr(inst);
  t.registers(rinst.rd()) = ~t.registers(rinst.r1());
}

[[using gnu: always_inline, hot]]
constexpr inline void do_cnt(Thread &t, uint64_t inst) noexcept {
  t.apply_instr(SInstr(inst), supernova::helpers::popcount);
}

[[using gnu: always_inline, hot]]
constexpr inline void do_llsr(Thread &t, uint64_t inst) noexcept {
  t.apply_instr(RInstr(inst), supernova::helpers::left_shift);
}

[[using gnu: always_inline, hot]]
constexpr inline void do_llsi(Thread &t, uint64_t inst) noexcept {
  t.apply_instr(SInstr(inst), supernova::helpers::left_shift);
}

[[using gnu: always_inline, hot]]
constexpr inline void do_lrsr(Thread &t, uint64_t inst) noexcept {
  t.apply_instr(RInstr(inst), supernova::helpers::right_shift);
}

[[using gnu: always_inline, hot]]
constexpr inline void do_lrsi(Thread &t, uint64_t inst) noexcept {
  t.apply_instr(SInstr(inst), supernova::helpers::right_shift);
}

[[using gnu: always_inline, hot]]
constexpr inline void do_addr(Thread &t, uint64_t inst) noexcept {
  t.apply_instr(RInstr(inst), std::plus<>{});
}

[[using gnu: always_inline, hot]]
constexpr inline void do_addi(Thread &t, uint64_t inst) noexcept {
  t.apply_instr(SInstr(inst), std::plus<>{});
}

[[using gnu: always_inline, hot]]
constexpr inline void do_subr(Thread &t, uint64_t inst) noexcept {
  t.apply_instr(RInstr(inst), std::minus<>{});
}

[[using gnu: always_inline, hot]]
constexpr inline void do_subi(Thread &t, uint64_t inst) noexcept {
  t.apply_instr(SInstr(inst), std::minus<>{});
}

[[using gnu: always_inline, hot]]
constexpr inline void do_umulr(Thread &t, uint64_t inst) noexcept {
  t.apply_instr(RInstr(inst), std::multiplies<uint64_t>{});
}

[[using gnu: always_inline, hot]]
constexpr inline void do_umuli(Thread &t, uint64_t inst) noexcept {
  t.apply_instr(SInstr(inst), std::multiplies<uint64_t>{});
}

[[using gnu: always_inline, hot]]
constexpr inline void do_smulr(Thread &t, uint64_t inst) noexcept {
  t.apply_instr(RInstr(inst), std::multiplies<int64_t>{});
}

[[using gnu: always_inline, hot]]
constexpr inline void do_smuli(Thread &t, uint64_t inst) noexcept {
  t.apply_instr(SInstr(inst), std::multiplies<int64_t>{});
}

[[using gnu: always_inline, hot]]
constexpr inline void do_udivr(Thread &t, uint64_t inst) noexcept {
  if unexpect (t.registers(RInstr(inst).r2()) == 0) {
    return dispatch_pcall(t, ProcessorCall::DivisionByZero);
  }
  t.apply_instr(RInstr(inst), std::multiplies<uint64_t>{});
}

[[using gnu: always_inline, hot]]
constexpr inline void do_udivi(Thread &t, uint64_t inst) noexcept {
  if unexpect (t.registers(SInstr(inst).imm()) == 0) {
    return dispatch_pcall(t, ProcessorCall::DivisionByZero);
  }
  t.apply_instr(SInstr(inst), std::multiplies<uint64_t>{});
}

[[using gnu: always_inline, hot]]
constexpr inline void do_sdivr(Thread &t, uint64_t inst) noexcept {
  if unexpect (t.registers(RInstr(inst).r2()) == 0) {
    dispatch_pcall(t, ProcessorCall::DivisionByZero);
    return;
  }
  t.apply_instr(RInstr(inst), std::multiplies<int64_t>{});
}

[[using gnu: always_inline, hot]]
constexpr inline void do_sdivi(Thread &t, uint64_t inst) noexcept {
  if unexpect (t.registers(SInstr(inst).imm()) == 0) {
    dispatch_pcall(t, ProcessorCall::DivisionByZero);
    return;
  }
  t.apply_instr(SInstr(inst), std::multiplies<int64_t>{});
}

[[using gnu: always_inline, hot]]
constexpr inline void do_call(Thread &t, uint64_t inst) noexcept {
  auto const &rinstr = RInstr(inst);
  auto const &stack_ptr = t.registers(rinstr.r1());
  auto const &base_ptr = t.registers(rinstr.r2());
  auto const &addr = t.registers(rinstr.rd());
  place<uint64_t>(t, stack_ptr + 0 * sizeof(uint64_t), base_ptr);
  place<uint64_t>(t, stack_ptr + 1 * sizeof(uint64_t),
                  t.progc() + sizeof(uint64_t));
  t.registers(rinstr.r1()) = stack_ptr + 2 * sizeof(uint64_t);
  t.progc() = addr;
}

[[using gnu: always_inline, hot]]
constexpr inline void do_push(Thread &t, uint64_t inst) noexcept {
  auto const &sinstr = SInstr(inst);
  auto const &retval = t.registers(sinstr.rd());
  auto const &stack_ptr = t.registers(sinstr.r1());
  auto const &imm = t.registers(sinstr.uimm());
  place<uint64_t>(t, stack_ptr, retval + imm);
  t.registers(sinstr.r1()) += sizeof(uint64_t);
}

[[using gnu: always_inline, hot]]
constexpr inline void do_retn(Thread &t, uint64_t inst) noexcept {
  auto const &rinstr = RInstr(inst);
  auto &stack_ptr = t.registers(rinstr.r1());
  auto &base_ptr = t.registers(rinstr.r2());
  auto &pcounter = t.progc();
  stack_ptr -= 2 * sizeof(uint64_t);
  base_ptr = fetch<uint64_t>(t, stack_ptr + 0 * sizeof(uint64_t));
  pcounter = fetch<uint64_t>(t, stack_ptr + 1 * sizeof(uint64_t));
}
[[using gnu: always_inline, hot]]
constexpr inline void do_pull(Thread &t, uint64_t inst) noexcept {
  auto const &sinstr = SInstr(inst);
  auto &retval = t.registers(sinstr.rd());
  auto &stack_ptr = t.registers(sinstr.r1());
  stack_ptr -= sizeof(uint64_t);
  retval = fetch<uint64_t>(t, stack_ptr);
}

[[using gnu: always_inline, hot]]
constexpr inline void do_ldb(Thread &t, uint64_t inst) noexcept {
  const SInstr &sinstr = SInstr(inst);
  t.registers(sinstr.rd()) =
      fetch<uint8_t>(t, t.registers(sinstr.r1()) + sinstr.imm());
}

[[using gnu: always_inline, hot]]
constexpr inline void do_ldh(Thread &t, uint64_t inst) noexcept {
  const SInstr &sinstr = SInstr(inst);
  t.registers(sinstr.rd()) =
      fetch<uint16_t>(t, t.registers(sinstr.r1()) + sinstr.imm());
}

[[using gnu: always_inline, hot]]
constexpr inline void do_ldw(Thread &t, uint64_t inst) noexcept {
  const SInstr &sinstr = SInstr(inst);
  t.registers(sinstr.rd()) =
      fetch<uint32_t>(t, t.registers(sinstr.r1()) + sinstr.imm());
}

[[using gnu: always_inline, hot]]
constexpr inline void do_ldd(Thread &t, uint64_t inst) noexcept {
  const SInstr &sinstr = SInstr(inst);
  t.registers(sinstr.rd()) =
      fetch<uint64_t>(t, t.registers(sinstr.r1()) + sinstr.imm());
}

[[using gnu: always_inline, hot]]
constexpr inline void do_stb(Thread &t, uint64_t inst) noexcept {
  const SInstr &sinstr = SInstr(inst);
  place<uint8_t>(t, t.registers(sinstr.rd()) + sinstr.imm(),
                 t.registers(sinstr.r1()));
}

[[using gnu: always_inline, hot]]
constexpr inline void do_sth(Thread &t, uint64_t inst) noexcept {
  const SInstr &sinstr = SInstr(inst);
  place<uint16_t>(t, t.registers(sinstr.rd()) + sinstr.imm(),
                  t.registers(sinstr.r1()));
}

[[using gnu: always_inline, hot]]
constexpr inline void do_stw(Thread &t, uint64_t inst) noexcept {
  const SInstr &sinstr = SInstr(inst);
  place<uint32_t>(t, t.registers(sinstr.rd()) + sinstr.imm(),
                  t.registers(sinstr.r1()));
}

[[using gnu: always_inline, hot]]
constexpr inline void do_std(Thread &t, uint64_t inst) noexcept {
  const SInstr &sinstr = SInstr(inst);
  place<uint64_t>(t, t.registers(sinstr.rd()) + sinstr.imm(),
                  t.registers(sinstr.r1()));
}

[[using gnu: always_inline, hot]]
constexpr inline void do_jal(Thread &t, uint64_t inst) {
  const LInstr &linstr = LInstr(inst);
  t.registers(linstr.r1()) = t.progc() + sizeof(uint64_t);
  t.progc() += linstr.imm() << 3;
}

[[using gnu: always_inline, hot]]
constexpr inline void do_jalr(Thread &t, uint64_t inst) {
  const SInstr &sinstr = SInstr(inst);
  t.registers(sinstr.rd()) = t.progc() + sizeof(uint64_t);
  t.progc() += t.registers(sinstr.r1()) + (sinstr.imm() << 3);
}

[[using gnu: always_inline, hot]]
constexpr inline void do_je(Thread &t, uint64_t inst) {
  const SInstr &sinstr = SInstr(inst);

  if (t.registers(sinstr.rd()) == t.registers(sinstr.r1())) {
    t.progc() += sinstr.imm() << 3;
  }
}

[[using gnu: always_inline, hot]]
constexpr inline void do_jne(Thread &t, uint64_t inst) {
  const SInstr &sinstr = SInstr(inst);
  if (t.registers(sinstr.rd()) != t.registers(sinstr.r1())) {
    t.progc() += sinstr.imm() << 3;
  }
}

[[using gnu: always_inline, hot]]
constexpr inline void do_jgu(Thread &t, uint64_t inst) {
  const SInstr &sinstr = SInstr(inst);
  if (t.registers(sinstr.rd()) > t.registers(sinstr.r1())) {
    t.progc() += sinstr.imm() << 3;
  }
}

[[using gnu: always_inline, hot]]
constexpr inline void do_jgs(Thread &t, uint64_t inst) {
  const SInstr &sinstr = SInstr(inst);
  if (static_cast<int64_t>(t.registers(sinstr.rd())) >
      static_cast<int64_t>(t.registers(sinstr.r1()))) {
    t.progc() += sinstr.imm() << 3;
  }
}

[[using gnu: always_inline, hot]]
constexpr inline void do_jleu(Thread &t, uint64_t inst) {
  const SInstr &sinstr = SInstr(inst);
  if (t.registers(sinstr.rd()) <= t.registers(sinstr.r1())) {
    t.progc() += sinstr.imm() << 3;
  }
}

[[using gnu: always_inline, hot]]
constexpr inline void do_jles(Thread &t, uint64_t inst) {
  const SInstr &sinstr = SInstr(inst);
  if (static_cast<int64_t>(t.registers(sinstr.rd())) <=
      static_cast<int64_t>(t.registers(sinstr.r1()))) {
    t.progc() += sinstr.imm() << 3;
  }
}

[[using gnu: always_inline, hot]]
constexpr inline void do_setgur(Thread &t, uint64_t inst) {
  const RInstr &rinstr = RInstr(inst);
  t.registers(rinstr.rd()) =
      t.registers(rinstr.r1()) > t.registers(rinstr.r2());
}

[[using gnu: always_inline, hot]]
constexpr inline void do_setgui(Thread &t, uint64_t inst) {
  const SInstr &sinstr = SInstr(inst);
  t.registers(sinstr.rd()) =
      t.registers(sinstr.r1()) > static_cast<uint64_t>(sinstr.uimm());
}

[[using gnu: always_inline, hot]]
constexpr inline void do_setgsr(Thread &t, uint64_t inst) {
  const RInstr &rinstr = RInstr(inst);
  t.registers(rinstr.rd()) = static_cast<int64_t>(t.registers(rinstr.r1())) >
                             static_cast<int64_t>(t.registers(rinstr.r2()));
}

[[using gnu: always_inline, hot]]
constexpr inline void do_setgsi(Thread &t, uint64_t inst) {
  const SInstr &sinstr = SInstr(inst);
  t.registers(sinstr.rd()) = static_cast<int64_t>(t.registers(sinstr.r1())) >
                             static_cast<int64_t>(sinstr.imm());
}

[[using gnu: always_inline, hot]]
constexpr inline void do_setleur(Thread &t, uint64_t inst) {
  const RInstr &rinstr = RInstr(inst);
  t.registers(rinstr.rd()) =
      t.registers(rinstr.r1()) <= t.registers(rinstr.r2());
}

[[using gnu: always_inline, hot]]
constexpr inline void do_setleui(Thread &t, uint64_t inst) {
  const SInstr &sinstr = SInstr(inst);
  t.registers(sinstr.rd()) = t.registers(sinstr.r1()) <= sinstr.uimm();
}

[[using gnu: always_inline, hot]]
constexpr inline void do_setlesr(Thread &t, uint64_t inst) {
  const RInstr &rinstr = RInstr(inst);
  t.registers(rinstr.rd()) = static_cast<int64_t>(t.registers(rinstr.r1())) <=
                             static_cast<int64_t>(t.registers(rinstr.r2()));
}

[[using gnu: always_inline, hot]]
constexpr inline void do_setlesi(Thread &t, uint64_t inst) {
  const SInstr &sinstr = SInstr(inst);
  t.registers(sinstr.rd()) = static_cast<int64_t>(t.registers(sinstr.r1())) <=
                             static_cast<int64_t>(sinstr.imm());
}

/**/
[[using gnu: always_inline, hot]]
constexpr inline void do_lui(Thread &t, uint64_t inst) {
  const LInstr &linstr = LInstr(inst);
  t.registers(linstr.r1()) |= linstr.imm() << supernova::LInstruction::off_imm;
}

[[using gnu: always_inline, hot]]
constexpr inline void do_auipc(Thread &t, uint64_t inst) {
  const LInstr &linstr = LInstr(inst);
  t.registers(linstr.r1()) =
      t.progc() + (linstr.imm() << supernova::LInstruction::off_imm);
}

[[using gnu: always_inline, hot]]
constexpr inline void do_pcall(Thread &t, uint64_t inst) {
  const LInstr &linstr = LInstr(inst);
  dispatch_pcall(t, static_cast<ProcessorCall>(linstr.imm()));
}

[[using gnu: always_inline, hot]]
constexpr inline void do_pret(Thread &t, uint64_t inst) {
  t.progc() = t.pcallret();
}

}; // namespace dispatchers

constexpr void init_dispatch(dispatch_array_t &array,
                             supernova::config_flags_1 flags) {
  using namespace dispatchers;
  array = {
      do_andr,
      do_andi,
      do_xorr,
      do_xori,
      do_orr,
      do_ori,
      do_not,
      do_cnt,
      do_llsr,
      do_llsi,
      do_lrsr,
      do_lrsi,
      invalid_instruction,
      invalid_instruction,
      invalid_instruction,
      invalid_instruction,

      do_addr,
      do_addi,
      do_subr,
      do_subi,
      do_umulr,
      do_umuli,
      do_smulr,
      do_smuli,
      flags & supernova::confflags_idiv ? do_udivr : invalid_instruction,
      flags & supernova::confflags_idiv ? do_udivi : invalid_instruction,
      flags & supernova::confflags_idiv ? do_sdivr : invalid_instruction,
      flags & supernova::confflags_idiv ? do_sdivi : invalid_instruction,
      flags & supernova::confflags_stack ? do_call : invalid_instruction,
      flags & supernova::confflags_stack ? do_push : invalid_instruction,
      flags & supernova::confflags_stack ? do_retn : invalid_instruction,
      flags & supernova::confflags_stack ? do_pull : invalid_instruction,

      do_ldb,
      do_ldh,
      do_ldw,
      do_ldd,
      do_stb,
      do_sth,
      do_stw,
      do_std,
      do_jal,
      do_jalr,
      do_je,
      do_jne,
      do_jgu,
      do_jgs,
      do_jleu,
      do_jles,

      flags & supernova::confflags_cset ? do_setgur : invalid_instruction,
      flags & supernova::confflags_cset ? do_setgui : invalid_instruction,
      flags & supernova::confflags_cset ? do_setgsr : invalid_instruction,
      flags & supernova::confflags_cset ? do_setgsi : invalid_instruction,
      flags & supernova::confflags_cset ? do_setleur : invalid_instruction,
      flags & supernova::confflags_cset ? do_setleui : invalid_instruction,
      flags & supernova::confflags_cset ? do_setlesr : invalid_instruction,
      flags & supernova::confflags_cset ? do_setlesi : invalid_instruction,
      do_lui,
      do_auipc,
      do_pcall,
      do_pret,
      invalid_instruction,
      invalid_instruction,
      invalid_instruction,
      invalid_instruction,
  };
}

constexpr void exec_instruction(Thread &thread,
                                dispatch_array_t &array) noexcept {
  uint64_t instruction{0UL};
  if unexpect (thread.signal() != DestroyFor::DoNotDestroy) {
    return;
  }

  instruction = fetch<uint64_t>(thread, thread.progc());

  if unexpect ((instruction & 0xFF) > supernova::inspx::instruction_count) {
    return dispatchers::invalid_instruction(thread, instruction);
  }

  array[instruction & 0xFF](thread, instruction);

  thread.progc() += sizeof(uint64_t);

  thread.registers(0) = 0;
}
} // namespace

namespace supernova {
void init(config_flags_1 flags, dispatch_array_t &array) {
  init_dispatch(array, flags);
}

auto run(int argc, char **argv, Thread &thread, bool step,
         config_flags_1 flags) -> thread_return {
  thread.registers(0) = 0;
  dispatch_array_t array;

  init(flags, array);

  if (step) {
    exec_instruction(thread, array);
    return {true, thread.registers(1)};
  }

  thread.registers(Thread::pcall_1stret) = argc;
  thread.registers(Thread::pcall_2ndret) = reinterpret_cast<uint64_t>(argv);

  while
    expect(thread.signal() != DestroyFor::DoNotDestroy) {
      exec_instruction(thread, array);
    }

  const int ret_val = thread.registers(1);

  if (thread.signal() == DestroyFor::ProgramEnd) {
    return {true, ret_val};
  }

  return {false, thread.signal()};
}
} // namespace supernova
