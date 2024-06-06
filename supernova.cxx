#include "supernova.h"
#include <functional>

namespace
{
    using Thread = zenith::supernova::Thread;
    using ProcessorCall = zenith::supernova::ProcessorCall;
    using DestroyFor = zenith::supernova::ThreadDestruction;
    using Instruction = zenith::supernova::Instruction;
    using RInstr = zenith::supernova::RInstruction;
    using SInstr = zenith::supernova::SInstruction;
    using LInstr = zenith::supernova::LInstruction;
    using Opcodes = zenith::supernova::instruction_prefixes;
    using thread_return = zenith::supernova::thread_return;

    constexpr void dispatch_pcall(Thread &thread, ProcessorCall pcall) noexcept;

    template <typename integer>
    [[nodiscard]] constexpr integer fetch(Thread &thread, uint64_t address) noexcept
    {
        if (address >= thread.memsize())
        {
            dispatch_pcall(thread, ProcessorCall::MemoryLimit);
            return 0;
        }
        // NOLINTNEXTLINE: yeah it works
        return *reinterpret_cast<integer *>(thread.memory().get() + address);
    }

    template <typename integer>
    constexpr void place(Thread &thread, uint64_t address, integer value) noexcept
    {
        if (address >= thread.memsize())
        {
            dispatch_pcall(thread, ProcessorCall::MemoryLimit);
            return;
        }
        // NOLINTNEXTLINE: looks good to me tho
        *reinterpret_cast<integer *>(thread.memory().get() + address) = value;
    }

    void hwpush64(Thread &thread, uint64_t value) noexcept
    {
        place<uint64_t>(thread, thread.registers(1), value);
        thread.registers(1) -= sizeof(uint64_t);
    }

    // static uint64_t hwpop64(register Thread *thread)
    // {
    //     uint64_t val = fetch(64, thread, thread->registers[1]);
    //     thread->registers[1] += 8;
    //     return val;
    // }

    [[nodiscard, gnu::const]] constexpr int64_t lsextend(uint64_t number) noexcept
    {
        constexpr auto mask = 0xfff8000000000000LU;
        constexpr auto index = 51U;
        return number >> index != 0 ? static_cast<int64_t>(number) | mask : static_cast<int64_t>(number);
    }

    [[nodiscard, gnu::const]] constexpr int64_t ssextend(uint64_t number) noexcept
    {
        constexpr auto mask = 0xffffc00000000000LU;
        constexpr auto index = 46U;
        return number >> index != 0 ? static_cast<int64_t>(number) | mask : static_cast<int64_t>(number);
    }

    constexpr void pcall_minus_one(Thread &thread)
    {
        auto const &interrupt_space = thread.registers(Thread::pcall_intspace);
        auto const &function_switch = thread.registers(Thread::pcall_fswitch);
        switch (interrupt_space)
        {
        case 0:
            if (function_switch == 0)
            {
                thread.registers(Thread::pcall_1stret) = 2;
                thread.registers(Thread::pcall_2ndnret) = thread.model()->interrupt_count;
            }
            else if (function_switch == 1)
            {
                thread.intvec() = thread.registers(Thread::pcall_1stret);
            }
            break;
        case 1:
            thread.registers(Thread::pcall_1stret) = 0;
            break;
        case 2:

        default:
            break;
        }
    }

    constexpr void dispatch_pcall(Thread &thread, ProcessorCall pcall) noexcept
    {
        if (pcall == ProcessorCall::Functions)
        {
            pcall_minus_one(thread);
            return;
        }

        if (thread.pcall() == ProcessorCall::DoubleFault)
        {
            thread.pcall() = ProcessorCall::TripleFault;
            thread.signal() = DestroyFor::InterruptCrashLoop;
        }
        else if (thread.pcall() != ProcessorCall::NormalExecution)
        {
            thread.pcall() = ProcessorCall::DoubleFault;
        }
        else
        {
            thread.pcall() = pcall;
        }

        for (const uint64_t reg : thread.allregs())
        {
            hwpush64(thread, reg);
        }

        hwpush64(thread, thread.progc());

        thread.progc() = fetch<uint64_t>(thread, thread.intvec() + pcall * sizeof(uint64_t));
    }

    void exec_instruction(Thread &thread)
    {
        uint64_t instruction{0UL};
        if (thread.signal() != DestroyFor::DoNotDestroy)
        {
            return;
        }

        instruction = fetch<uint64_t>(thread, thread.progc());
        thread.progc() += sizeof(uint64_t);

        const auto rinstr = RInstr(instruction);
        const auto sinstr = SInstr(instruction);
        const auto linstr = LInstr(instruction);

        /// how many bits can shift actually shift
#define max_shift_size 64

        /// the amount of bits not accounted by an linstruction immediate
        constexpr auto low_bit_count = 13;
        switch (rinstr.opcode())
        {
        case Opcodes::andr_instrc:
            thread.apply_instr(rinstr, std::bit_and<>{}); 
            break;
        case Opcodes::andi_instrc:
            thread.apply_instr(sinstr, std::bit_and<>{});
            break;
        case Opcodes::xorr_instrc:
            thread.apply_instr(rinstr, std::bit_xor<>{});
            break;
        case Opcodes::xori_instrc:
            thread.apply_instr(sinstr, std::bit_xor<>{});
            break;
        case Opcodes::orr_instrc:
            thread.apply_instr(rinstr, std::bit_or<>{});
            break;
        case Opcodes::ori_instrc:
            thread.apply_instr(sinstr, std::bit_or<>{});
            break;
        case Opcodes::not_instrc:
            thread.registers(rinstr.rd()) = ~thread.registers(rinstr.r1());
            break;
        case Opcodes::cnt_instrc:
            thread.apply_instr(sinstr, [](auto left, [[maybe_unused]] auto imm) {
                (void)imm;
#if __has_builtin(__builtin_popcountl)
                return __builtin_popcountl(left);
#else
                int count = 0;
                uint64_t val = left;
                for (; val != 0; val &= val - 1)
                    count++;
                return count;
#endif
            });
            break;
        case Opcodes::llsr_instrc:
            thread.apply_instr(rinstr, [](auto left, auto right) -> uint64_t {
                if (right >= max_shift_size){
                    return 0;
                }
                return left << right; 
            });
            break;
        case Opcodes::llsi_instrc:
            thread.apply_instr(sinstr, [](auto left, auto imm) -> uint64_t {
                if (imm >= max_shift_size){
                    return 0;
                }
                return left << imm;
            });
            break;
        case Opcodes::lrsr_instrc:
            thread.apply_instr(sinstr, [](auto left, auto right) -> uint64_t {
                if (right >= max_shift_size){
                    return 0;
                }
                return left >> right;
            });
            break;
        case Opcodes::lrsi_instrc:
            thread.apply_instr(sinstr, [](auto left, auto imm) -> uint64_t {
                if (imm >= max_shift_size){
                    return 0;
                }
                return left >> imm;
            });
            break;
        /**/

        /**/
        case Opcodes::addr_instrc:
            thread.apply_instr(rinstr, std::plus<>{});
            break;
        case Opcodes::addi_instrc:
            thread.apply_instr(sinstr, std::plus<>{});
            break;
        case Opcodes::subr_instrc:
            thread.apply_instr(rinstr, std::minus<>{});
            break;
        case Opcodes::subi_instrc:
            thread.apply_instr(sinstr, std::minus<>{});
            break;
        /**/
        case Opcodes::umulr_instrc:
            thread.apply_instr(rinstr, std::multiplies<uint64_t>{});
            break;
        case Opcodes::umuli_instrc:
            thread.apply_instr(sinstr, std::multiplies<uint64_t>{});
            break;
        case Opcodes::smulr_instrc:
            thread.apply_instr(rinstr, std::multiplies<int64_t>{});
            break;
        case Opcodes::smuli_instrc:
            thread.apply_instr(sinstr, std::multiplies<int64_t>{});
            break;
        case Opcodes::udivr_instrc:
            if (thread.registers(rinstr.r2()) == 0)
            {
                dispatch_pcall(thread, ProcessorCall::DivisionByZero);
                return;
            }
            thread.apply_instr(rinstr, std::divides<uint64_t>{});
            break;
        case Opcodes::udivi_instrc:
            if (thread.registers(sinstr.imm()) == 0)
            {
                dispatch_pcall(thread, ProcessorCall::DivisionByZero);
                return;
            }
            thread.apply_instr(sinstr, std::divides<uint64_t>{});
            break;
        case Opcodes::sdivr_instrc:
            if (thread.registers(rinstr.r2()) == 0)
            {
                dispatch_pcall(thread, ProcessorCall::DivisionByZero);
                return;
            }
            thread.apply_instr(rinstr, std::divides<int64_t>{});
            break;
        case Opcodes::sdivi_instrc:
            if (thread.registers(sinstr.imm()) == 0)
            {
                dispatch_pcall(thread, ProcessorCall::DivisionByZero);
                return;
            }
            thread.apply_instr(sinstr, std::divides<int64_t>{});
            break;
        /**/
        case Opcodes::call_instrc:
        {
            auto &stack_ptr = thread.registers(rinstr.r1());
            auto &base_ptr = thread.registers(rinstr.r2());
            auto const &addr = thread.registers(rinstr.rd());
            place<uint64_t>(thread, stack_ptr + 0 * sizeof(uint64_t), base_ptr);
            place<uint64_t>(thread, stack_ptr + 1 * sizeof(uint64_t), thread.progc() + sizeof(uint64_t));
            stack_ptr += 2 * sizeof(uint64_t);
            base_ptr = stack_ptr;
            thread.progc() = addr;
            break;
        }
        case Opcodes::push_instrc:
        {
            auto &retval = thread.registers(sinstr.rd());
            auto &stack_ptr = thread.registers(sinstr.r1());
            auto &imm = thread.registers(sinstr.imm());
            place<uint64_t>(thread, stack_ptr, retval + imm);
            stack_ptr += sizeof(uint64_t);
            break;
        }
        case Opcodes::retn_instrc:
        {
            auto &stack_ptr = thread.registers(rinstr.r1());
            auto &base_ptr = thread.registers(rinstr.r2());
            auto &pcounter = thread.progc();
            stack_ptr -= 2 * sizeof(uint64_t);
            base_ptr = fetch<uint64_t>(thread, stack_ptr + 0 * sizeof(uint64_t));
            pcounter = fetch<uint64_t>(thread, stack_ptr + 1 * sizeof(uint64_t));
            break;
        }
        case Opcodes::pull_instrc:
        {
            auto &retval = thread.registers(sinstr.rd());
            auto &stack_ptr = thread.registers(sinstr.r1());
            stack_ptr -= sizeof(uint64_t);
            retval = fetch<uint64_t>(thread, stack_ptr);
            break;
        }
        /**/
        case Opcodes::ld_byte_instrc:
            thread.registers(sinstr.rd()) = fetch<uint8_t>(thread, thread.registers(sinstr.r1()) + sinstr.imm());
            break;
        case Opcodes::ld_half_instrc:
            thread.registers(sinstr.rd()) = fetch<uint16_t>(thread, thread.registers(sinstr.r1()) + sinstr.imm());
            break;
        case Opcodes::ld_word_instrc:
            thread.registers(sinstr.rd()) = fetch<uint32_t>(thread, thread.registers(sinstr.r1()) + sinstr.imm());
            break;
        case Opcodes::ld_dwrd_instrc:
            thread.registers(sinstr.rd()) = fetch<uint64_t>(thread, thread.registers(sinstr.r1()) + sinstr.imm());
            break;
        /**/
        case Opcodes::st_byte_instrc:
            place<uint8_t>(thread, thread.registers(sinstr.rd()) + sinstr.imm(), thread.registers(sinstr.r1()));
            break;
        case Opcodes::st_half_instrc:
            place<uint16_t>(thread, thread.registers(sinstr.rd()) + sinstr.imm(), thread.registers(sinstr.r1()));
            break;
        case Opcodes::st_word_instrc:
            place<uint32_t>(thread, thread.registers(sinstr.rd()) + sinstr.imm(), thread.registers(sinstr.r1()));
            break;
        case Opcodes::st_dwrd_instrc:
            place<uint64_t>(thread, thread.registers(sinstr.rd()) + sinstr.imm(), thread.registers(sinstr.r1()));
            break;
        /**/
        case Opcodes::jal_instrc:
            thread.registers(linstr.r1()) = thread.progc() + sizeof(uint64_t);
            thread.progc() += ssextend(linstr.imm());
            break;
        case Opcodes::jalr_instrc:
            thread.registers(sinstr.rd()) = thread.progc() + sizeof(uint64_t);
            thread.progc() += thread.registers(sinstr.r1()) + sinstr.imm();
            break;
        case Opcodes::je_instrc:
            if (thread.registers(sinstr.rd()) == thread.registers(sinstr.r1()))
            {
                thread.progc() += ssextend(sinstr.imm());
            }
            break;
        case Opcodes::jne_instrc:
            if (thread.registers(sinstr.rd()) != thread.registers(sinstr.r1()))
            {
                thread.progc() += ssextend(sinstr.imm());
            }
            break;
        /**/
        case Opcodes::jgu_instrc:
            if (thread.registers(sinstr.rd()) > thread.registers(sinstr.r1()))
            {
                thread.progc() += ssextend(sinstr.imm());
            }
            break;
        case Opcodes::jgs_instrc:
            if (static_cast<int64_t>(thread.registers(sinstr.rd())) > static_cast<int64_t>(thread.registers(sinstr.r1())))
            {
                thread.progc() += ssextend(sinstr.imm());
            }
            break;
        case Opcodes::jleu_instrc:
            if (thread.registers(sinstr.rd()) <= thread.registers(sinstr.r1()))
            {
                thread.progc() += ssextend(sinstr.imm());
            }
            break;
        case Opcodes::jles_instrc:
            if (static_cast<int64_t>(thread.registers(sinstr.rd())) <= static_cast<int64_t>(thread.registers(sinstr.r1())))
            {
                thread.progc() += ssextend(sinstr.imm());
            }
            break;
        /**/
        case Opcodes::setgur_instrc:
            thread.registers(rinstr.rd()) = thread.registers(rinstr.r1()) > thread.registers(rinstr.r2());
            break;
        case Opcodes::setgui_instrc:
            thread.registers(sinstr.rd()) = thread.registers(sinstr.r1()) > sinstr.imm();
            break;
        case Opcodes::setgsr_instrc:
            thread.registers(rinstr.rd()) = static_cast<int64_t>(thread.registers(rinstr.r1())) > static_cast<int64_t>(thread.registers(rinstr.r2()));
            break;
        case Opcodes::setgsi_instrc:
            thread.registers(sinstr.rd()) = static_cast<int64_t>(thread.registers(sinstr.r1())) > static_cast<int64_t>(sinstr.imm());
            break;
        /**/
        case Opcodes::setleur_instrc:
            thread.registers(rinstr.rd()) = thread.registers(rinstr.r1()) <= thread.registers(rinstr.r2());
            break;
        case Opcodes::setleui_instrc:
            thread.registers(sinstr.rd()) = thread.registers(sinstr.r1()) <= sinstr.imm();
            break;
        case Opcodes::setlesr_instrc:
            thread.registers(rinstr.rd()) = static_cast<int64_t>(thread.registers(rinstr.r1())) <= static_cast<int64_t>(thread.registers(rinstr.r2()));
            break;
        case Opcodes::setlesi_instrc:
            thread.registers(sinstr.rd()) = static_cast<int64_t>(thread.registers(sinstr.r1())) <= static_cast<int64_t>(sinstr.imm());
            break;
        /**/
        case Opcodes::lui_instrc:
            thread.registers(linstr.r1()) |= linstr.imm() << low_bit_count;
            break;
        case Opcodes::auipc_instrc:
            thread.registers(linstr.r1()) = thread.progc() + (linstr.imm() << low_bit_count);
            break;
        case Opcodes::pcall_instrc:
            dispatch_pcall(thread, static_cast<ProcessorCall>(linstr.imm()));
            break;
        default:
            thread.registers(31) = rinstr.opcode();
            dispatch_pcall(thread, ProcessorCall::InvalidInstruction);
            break;
        }
        thread.registers(0) = 0;
    }
} // namespace

namespace zenith::supernova
{
    thread_return run(int argc, char **argv, Thread &thread)
    {
        thread.registers(0) = 0;
        thread.registers(31) = argc;
        thread.registers(30) = reinterpret_cast<uint64_t>(argv);

        while (thread.signal() != DestroyFor::DoNotDestroy)
        {
            exec_instruction(thread);
        }

        const int ret_val = thread.registers(1);

        if (thread.signal() == DestroyFor::ProgramEnd)
        {
            return thread_return{true, ret_val};
        }

        return {false, thread.signal()};
    }
} // namespace zenith::supernova
