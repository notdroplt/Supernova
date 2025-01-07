/**
 * @file supernova.h
 * @author notdroplt (117052412+notdroplt@users.noreply.github.com)
 * @brief custom ISA designed for Zenith
 * @version 0.0.1
 * @date 2023-01-14
 *
 * @copyright Copyright (c) 2023-2024
 *
 */

#pragma once
#include <array>
#include <cstdint>
#include <memory>

#ifndef SUPERNOVA_VERSION_MAJOR
/** should be set if compiling with cmake, this is just a failback for lsp servers */
#define SUPERNOVA_VERSION_MAJOR 0LLU
#endif
#ifndef SUPERNOVA_VERSION_MINOR
/** should be set if compiling with cmake, this is just a failback for lsp servers */
#define SUPERNOVA_VERSION_MINOR 0LLU
#endif
#ifndef SUPERNOVA_VERSION_PATCH
/** should be set if compiling with cmake, this is just a failback for lsp servers */
#define SUPERNOVA_VERSION_PATCH 0LLU
#endif

#ifndef SUPERNOVA_HEADER_DEFINED
#define SUPERNOVA_HEADER_DEFINED 1
#endif

namespace supernova
{
    namespace helpers
    {
        /**
         * @brief shifts a number to the left
         *
         * @param left left side
         * @param right right side
         *
         * @return variable shifted, also capped to not go over if shift size is greater than the size of the variable
         */
        constexpr auto left_shift(uint64_t left, uint64_t right) -> uint64_t
        {
            return left << (right & 0xFF);
        };

        /**
         * @brief shifts a number to the right
         *
         * @param left left side
         * @param right right side
         *
         * @return variable shifter, capped to not go over UB
         */
        constexpr auto right_shift(uint64_t left, uint64_t right) -> uint64_t
        {

            return left >> (right & 0xFF);
        };

        constexpr auto popcount(uint64_t left, uint64_t right [[maybe_unused]]) -> uint64_t
        {
            if constexpr (__has_builtin(__builtin_popcountl))
                return __builtin_popcountl(left);
            else
            {
                auto count = 0U;
                for (auto val = left; val != 0; val &= val - 1)
                    count++;
                return count;
            }
        };

        /**
     * @brief Sign extend immediates on small integer instructions
     *
     * @param number number to sign extend
     *
     * @returns int64_t sign extended number
     */
    [[nodiscard, gnu::const]] constexpr auto ssextend(uint64_t number) noexcept -> int64_t
    {
        constexpr auto neg_mask = 0xFFFF000000000000LU;
        constexpr auto sign_bit = 0x0000800000000000LU;
        return (number & sign_bit) != 0 ? static_cast<int64_t>(number | neg_mask) : static_cast<int64_t>(number);
    }

    /**
     * @brief Sign extend immediates on long integer instructions
     *
     * @param number number to sign extend
     *
     * @returns int64_t sign extended number
     */
    [[nodiscard, gnu::const]] constexpr auto lsextend(uint64_t number) noexcept -> int64_t
    {
        constexpr auto neg_mask = 0xFFF0000000000000LU;
        constexpr auto sign_bit = 0x0008000000000000LU;
        return (number & sign_bit) != 0 ? static_cast<int64_t>(number | neg_mask) : static_cast<int64_t>(number);
    }

    }; // namespace helpers

    /**
     * @brief supernova opcodes (64 bit version)
     *
     * instruction types:
     *
     * R type (registers only) => register-register-register
     *
     * S type ("small" immediate) => register-register-immediate
     *
     * L type ("long" immediate) => register-immediate
     *
     * base instructions are split into 8 groups (named after values at bits [6,4]):
     *
     * groups 0-3 make part of a "base" group, on which all thread models need to implement
     * most of the operations, the exception being division and stack instructions on group 2.
     *
     * groups 4-7 make part of an "extension" group, the cpu will have flags that can be checked
     * to see if those operations are hardware-implemented
     *
     * - group 0: bitwise instructions, span opcodes `0x00` to `0x0F`,
     * - group 1: math / control flow instructions, span opcodes `0x10` to `0x1F`
     * - group 2: memory / control flow instructions, span opcodes `0x20` to `0x2F`
     * - group 3: conditional set / interrupts / IO, span opcodes `0x30` to `0x3F`
     */
    enum inspx : uint8_t
    {
        /**
         * @defgroup InP instruction prefixes
         *
         * @brief those groups define instructions that have similar behaviors
         * between themselves
         *
         * @{
         */

        /**
         * @defgroup InPG0 instruction prefixes, group 0
         * @ingroup InP
         * @brief Group 0 defines bitwise instructions to operate on registers or
         * immediates
         *
         * @{
         */
        andr_instrc = 0x00, /**< `and r#, r#, r#`  : R type */
        andi_instrc = 0x01, /**< `and r#, r#, imm` : S type */
        xorr_instrc = 0x02, /**< `xor r#, r#, r#`  : R type */
        xori_instrc = 0x03, /**< `xor r#, r#, imm` : S type */
        orr_instrc = 0x04,  /**< `or r#, r#, r#`   : R type */
        ori_instrc = 0x05,  /**< `or r#, r#, imm`  : S type */
        not_instrc = 0x06,  /**< `not r#, r#, r#`  : R type */
        cnt_instrc = 0x07,  /**< `cnt r#, r#, imm` : S type */
        llsr_instrc = 0x08, /**< `lls r#, r#, r#`  : R type */
        llsi_instrc = 0x09, /**< `lls r#, r#, imm` : S type */
        lrsr_instrc = 0x0A, /**< `lrs r#, r#, r#`  : R type */
        lrsi_instrc = 0x0B, /**< `lrs r#, r#, imm` : S type */
        /* reserved : R type */
        /* reserved : S type */
        /* reserved : R type */
        /* reserved : S type */
        /** @} */ /* InPG0 */

        /**
         * @defgroup InPG1 Instruction prefixes, group 1
         * @ingroup InP
         * @brief define prefixes for arithimetic and stack instructions
         *
         * @note all div instructions in this group and the signed multiplication
         * ones do not need to be implemented in a processor if flagged
         *
         * @{
         */
        addr_instrc = 0x10,  /**< `add r#, r#, r#`   : R type */
        addi_instrc = 0x11,  /**< `add r#, r#, imm`  : S type */
        subr_instrc = 0x12,  /**< `sub r#, r#, r#`   : R type */
        subi_instrc = 0x13,  /**< `sub r#, r#, imm`  : S type */
        umulr_instrc = 0x14, /**< `umul r#, r#, r#`  : R type */
        umuli_instrc = 0x15, /**< `umul r#, r#, imm` : S type */
        smulr_instrc = 0x16, /**< `smul r#, r#, r#`  : R type */
        smuli_instrc = 0x17, /**< `smul r#, r#, imm` : S type */
        udivr_instrc = 0x18, /**< `udiv r#, r#, r#`  : R type */
        udivi_instrc = 0x19, /**< `udiv r#, r#, imm` : S type */
        sdivr_instrc = 0x1A, /**< `sdiv r#, r#, r#`  : R type */
        sdivi_instrc = 0x1B, /**< `sdiv r#, r#, imm` : S type */
        call_instrc = 0x1C,  /**< `call r#, r#, r#`  : R type */ /* reserved, only for cross compiling */
        push_instrc = 0x1D,  /**< `push r#, r#, imm` : S type */ /* reserved, only for cross compiling */
        retn_instrc = 0x1E,  /**< `retn r#, r#, r#`  : R type */ /* reserved, only for cross compiling */
        pull_instrc = 0x1F,  /**< `pull r#, r#, imm` : S type */ /* reserved, only for cross compiling */
        /** @} */                           /* InPG1 */

        /**
         * @defgroup InPG2 Instruction prefixes, group 2
         *
         * @brief define prefixes for load/store instructions, and
         * (un)conditional jumps
         *
         * @{
         */
        ld_byte_instrc = 0x20, /**< `ldb r#, r#, imm` : S type */
        ld_half_instrc = 0x21, /**< `ldh r#, r#, imm` : S type */
        ld_word_instrc = 0x22, /**< `ldw r#, r#, imm` : S type */
        ld_dwrd_instrc = 0x23, /**< `ldd r#, r#, imm` : S type */
        st_byte_instrc = 0x24, /**< `stb r#, r#, imm` : S type */
        st_half_instrc = 0x25, /**< `sth r#, r#, imm` : S type */
        st_word_instrc = 0x26, /**< `stw r#, r#, imm` : S type */
        st_dwrd_instrc = 0x27, /**< `std r#, r#, imm` : S type */
        jal_instrc = 0x28,     /**< `jal r#, imm`      : L type */
        jalr_instrc = 0x29,    /**< `jalr r#, r#, imm` : S type */
        je_instrc = 0x2A,      /**< `je r#, r#, imm`   : S type */
        jne_instrc = 0x2B,     /**< `jne r#, r#, imm`  : S type */
        jgu_instrc = 0x2C,     /**< `jgu r#, r#, imm`  : S type */
        jgs_instrc = 0x2D,     /**< `jgs r#, r#, imm`  : S type */
        jleu_instrc = 0x2E,    /**< `jleu r#, r#, imm` : S type */
        jles_instrc = 0x2F,    /**< `jles r#, r#, imm` : S type */
        /** @} */              /* InPG2 */

        /**
         * @defgroup InPG3 Instruction prefixes, group 3
         * @brief define prefixes for some miscellaneous instructions, mainly
         * condset isntructions, upper immediate handling, processor calls and
         * input / output
         *
         * @note set* instructions do not need to be implemented if the necessary
         * flag is set on the model information register
         *
         * @note this group also marks the end of the required instruction groups
         *
         * @{
         */
        setgur_instrc = 0x30,  /**< `setgu r#, r#, r#`   : R type */
        setgui_instrc = 0x31,  /**< `setgu r#, r#, imm`  : S type */
        setgsr_instrc = 0x32,  /**< `setgs r#, r#, r#`   : R type */
        setgsi_instrc = 0x33,  /**< `setgs r#, r#, imm`  : S type */
        setleur_instrc = 0x34, /**< `setleu r#, r#, r#`  : R type */
        setleui_instrc = 0x35, /**< `setleu r#, r#, imm` : S type */
        setlesr_instrc = 0x36, /**< `setles r#, r#, r#`  : R type */
        setlesi_instrc = 0x37, /**< `setles r#, r#, imm` : S type */
        lui_instrc = 0x38,     /**< `lui r#, imm`        : L type */
        auipc_instrc = 0x39,   /**< `auipc r#, imm`      : L type */
        pcall_instrc = 0x3A,   /**< `pcall r#, imm`      : L type */
        pret_instrc = 0x3B,    /**< `pret r#, imm`       : L type */
        bout_instrc = 0x3C, /**< `outb r#, r#, 0`    : R type */
        out_instrc = 0x3D,  /**< `outw r#, r#, 0`    : S type */
        bin_instrc = 0x3E,  /**< `inb r#, r#, 0`     : R type */
        in_instrc = 0x3F,   /**< `inw r#, r#, 0`     : S type */
        /** @} */           /** InPG3*/

        /** group 4, floating point operation instructions */

        /**
         * @defgroup InPG4 Instruction prefixes, group 4
         *
         * @brief first of the default extension group, instructions are only
         * implemented if the required flag is present in the model information
         * register
         *
         * @{
         */
        flt_ldu_instrc = 0x40, /**< `fldu fr#, r#, 0` : R type */
        flt_lds_instrc = 0x41, /**< `flds fr#, r#, 0` : R type */
        flt_stu_instrc = 0x42, /**< `fstu r#, fr#, 0` : R type */
        flt_sts_instrc = 0x43, /**< `fsts r#, fr#, 0` : R type */
        flt_add_instrc = 0x44, /**< `fadd fr#, fr#, fr#` : R type */
        flt_sub_instrc = 0x45, /**< `fsub fr#, fr#, fr#` : R type */
        flt_mul_instrc = 0x46, /**< `fmul fr#, fr#, fr#` : R type */
        flt_div_instrc = 0x47, /**< `fdiv fr#, fr#, fr#` : R type */
        flt_ceq_instrc = 0x48, /**< `fcmpeq r#, fr#, fr#` : R type */
        flt_cne_instrc = 0x49, /**< `fcmpne r#, fr#, fr#` : R type */
        flt_cgt_instrc = 0x4A, /**< `fcmpgt r#, fr#, fr#` : R type */
        flt_cle_instrc = 0x4B, /**< `fcmple r#, fr#, fr#` : R type */
        flt_rou_instrc = 0x4C, /**< `fround fr#, fr#, imm` : S type */
        flt_flr_instrc = 0x4D, /**< `ffloor fr#, fr#, imm` : S type */
        flt_cei_instrc = 0x4E, /**< `fceil fr#, fr#, imm` : S type */
        flt_trn_instrc = 0x4F, /**< `ftrnc fr#, fr#, imm` : S type */
        /** @}*/
        /** @}*/
        /** TODO: group 5, conditional move instructions */
        /** TODO: group 6, memory fences */


        __last_plus_one,
        instruction_count = __last_plus_one - 1
    };

    /**
     * @brief define processor calls and their offsets
     *
     * @note the offsets do not apply to `pcall -1`
     */
    enum ProcessorCall : int8_t
    {
        Functions = -1,     /**< processor defined functions */
        DivisionByZero = 0, /**< tried to divide by zero */
        Halt,               /**< required to halt */
        GeneralFault,       /**< undiagnosticated faults */
        DoubleFault,        /**< processor faulted trying to recover */
        TripleFault,        /**< processor faulted when recovering a double fault*/
        InvalidInstruction, /** unknown instruction would be executed */
        PageFault,          /**< invalid page used/accessed */
        MemoryLimit,        /**< invalid memory range selected */
        UnalignedAccess,    /**< processor tried to read/write unaligned memory*/
        NormalExecution,    /**< nothing happened, just continue */
    };

    /**
     * @brief reasons to why the thread had to be destroyed
     */
    enum ThreadDestruction : uint8_t
    {
        DoNotDestroy = 0,   /**< thread should continue running */
        ProgramEnd,         /**< program requested to end execution */
        CorruptedMemory,    /**< memory access was not permitted */
        InterruptCrashLoop, /**< program got into an irrecoverable triple fault */
    };

    

    /**
     * @brief R type instruction layout
     *
     * this class is a proxy for an `uint64_t` be interpreted as an Register
     * instruction
     *
     * Size : 8 bytes
     */
    struct RInstruction
    {
    public:
        /** mask for the opcode on a raw uint64_t */
        static const constexpr auto mask_op = 0x00000000000000FFU;

        /** mask for the r1 index on a raw uint64_t */
        static const constexpr auto mask_r1 = 0x0000000000000F00U;

        /** mask for the r2 index on a raw uint64_t */
        static const constexpr auto mask_r2 = 0x000000000000F000U;

        /** mask for the rd index on a raw uint64_t */
        static const constexpr auto mask_rd = 0x00000000000F0000U;

        /** offset for the opcode on a raw uint64_t */
        static const constexpr auto off_op = 0U;

        /** offset for the r1 index on a raw uint64_t */
        static const constexpr auto off_r1 = off_op + 8U;

        /** offset for the r2 index on a raw uint64_t */
        static const constexpr auto off_r2 = off_r1 + 4U;

        /** offset for the rd index on a raw uint64_t */
        static const constexpr auto off_rd = off_r2 + 4U;

        /**
         * @brief Construct an R instruction from different values
         * @param opcode opcode for this instruction
         * @param reg1 first register
         * @param reg2 second register
         * @param regd destination register
         *
         * @note the constructor does not check if the instruction is a valid
         * R instruction
         */
        constexpr RInstruction(inspx opcode, uint64_t reg1, uint64_t reg2, uint64_t regd)
        {
            m_instruction |= opcode & mask_op;
            m_instruction |= (reg1 & (mask_r1 >> off_r1)) << off_r1;
            m_instruction |= (reg2 & (mask_r2 >> off_r2)) << off_r2;
            m_instruction |= (regd & (mask_rd >> off_rd)) << off_rd;
        }

        /**
         * @brief Construct an R isntruction from a
         * @param raw
         */
        constexpr explicit RInstruction(uint64_t raw) : m_instruction{raw} {}

        /**
         * @brief get the opcode in this instruction
         * @return opcode
         */
        [[nodiscard]] constexpr auto opcode() const noexcept -> inspx { return static_cast<inspx>(this->m_instruction & mask_op); }

        /**
         * @brief get the first register in this instruction
         * @return first register index
         */
        [[nodiscard]] constexpr auto r1() const noexcept -> uint8_t { return (this->m_instruction & mask_r1) >> off_r1; }

        /**
         * @brief get the second register in this instruction
         * @return second register index
         */
        [[nodiscard]] constexpr auto r2() const noexcept -> uint8_t { return (this->m_instruction & mask_r2) >> off_r2; }

        /**
         * @brief get the destination register in this instruction
         * @return destination register index
         */
        [[nodiscard]] constexpr auto rd() const noexcept -> uint8_t { return (this->m_instruction & mask_rd) >> off_rd; }

        /**
         * @brief cast this instruction to an `uint64_t`
         */
        [[nodiscard]] constexpr explicit operator uint64_t() const noexcept { return this->m_instruction; }

    private:
        /** raw instruction value */
        uint64_t m_instruction{0};
    };

    /**
     * @brief S instruction layout
     *
     * this class is a proxy for an integer to be interpreted as an Small
     * immediate instruction
     *
     * Size : 8 bytes
     */
    class SInstruction
    {

    public:
        /** mask for the opcode on a raw `uint64_t` */
        static const constexpr auto mask_op = 0x00000000000000FFU;

        /** mask for the r1 index on a raw `uint64_t` */
        static const constexpr auto mask_r1 = 0x0000000000000F00U;

        /** mask for the r2 index on a raw `uint64_t` */
        static const constexpr auto mask_rd = 0x000000000000F000U;

        /** mask for the immediate on a raw `uint64_t` */
        static const constexpr auto mask_imm = 0xFFFFFFFFFFFF0000U;

        /** mask used for signed comparisions */
        static const constexpr auto big_uimm = 0x00007FFFFFFFFFFFU;

        /** offset for the opcode on a raw `uint64_t` */
        static const constexpr auto off_op = 0U;

        /** offset for the r1 index on a raw `uint64_t` */
        static const constexpr auto off_r1 = off_op + 8U;

        /** offset for the r2 index on a raw `uint64_t` */
        static const constexpr auto off_rd = off_r1 + 4U;

        /** offset for the immediate on a raw `uint64_t` */
        static const constexpr auto off_imm = off_rd + 4U;

        /**
         * @brief construct an S instruction from different values
         * @param opcode instruction opcode
         * @param reg1 first register index
         * @param regd destination register index
         * @param imm immediate value
         *
         * @note the constructor does not check if the opcode corresponds to a
         * valid S instruction
         */
        constexpr SInstruction(inspx opcode, uint64_t reg1, uint64_t regd, uint64_t imm)
        {
            m_instruction |= opcode & mask_op;
            m_instruction |= (reg1 & (mask_r1 >> off_r1)) << off_r1;
            m_instruction |= (regd & (mask_rd >> off_rd)) << off_rd;
            m_instruction |= (imm & (mask_imm >> off_imm)) << off_imm;
        }

        /**
         * @brief construct an S instruction from a raw `uint64_t`
         * @param raw raw integer value
         */
        constexpr explicit SInstruction(uint64_t raw) : m_instruction{raw} {}

        /**
         * @brief get the opcode in this instruction
         * @return opcode
         */
        [[nodiscard]] constexpr auto opcode() const noexcept -> inspx { return static_cast<inspx>(this->m_instruction & mask_op); }

        /**
         * @brief get the first register in this instruction
         * @return first register index
         */
        [[nodiscard]] constexpr auto r1() const noexcept -> uint8_t { return (this->m_instruction & mask_r1) >> off_r1; }

        /**
         * @brief get the destination register in this instruction
         * @return destination register index
         */
        [[nodiscard]] constexpr auto rd() const noexcept -> uint8_t { return (this->m_instruction & mask_rd) >> off_rd; }

        /**
         * @brief get the second register in this instruction, sign extended
         * @return second register index
         */
        [[nodiscard]] constexpr auto imm() const noexcept -> int64_t { return helpers::ssextend((this->m_instruction & mask_imm) >> off_imm); }

        /**
         * @brief get the second register in this instruction, do not extend sign
         * @return second register index
         */
        [[nodiscard]] constexpr auto uimm() const noexcept -> uint64_t { return (this->m_instruction & mask_imm) >> off_imm; }

        /**
         * @brief cast this instruction as an `uint64_t`
         */
        [[nodiscard]] constexpr explicit operator uint64_t() const noexcept { return this->m_instruction; }

    private:
        /** raw instruction value */
        uint64_t m_instruction{0};
    };

    /**
     * @brief L instruction layout
     *
     * Size : 8 bytes
     */
    struct LInstruction
    {

    public:
        /** mask for the opcode on a raw `uint64_t` */
        static const constexpr auto mask_op = 0x00000000000000FFU;

        /** mask for the r1 index on a raw `uint64_t` */
        static const constexpr auto mask_r1 = 0x0000000000000F00U;

        /** mask for the immediate index on a raw `uint64_t` */
        static const constexpr auto mask_imm = 0xFFFFFFFFFFFFF000U;

        static const constexpr auto big_uimm = 0x0007FFFFFFFFFFFFU;

        /** offset for the opcode on a raw `uint64_t` */
        static const constexpr auto off_op = 0U;

        /** offset for the r1 index on a raw `uint64_t` */
        static const constexpr auto off_r1 = off_op + 8U;

        /** offset for the r2 index on a raw `uint64_t` */
        static const constexpr auto off_imm = off_r1 + 4U;

        /**
         * @brief Generate an L instruction from multiple values
         * @param opcode L instruction opcode
         * @param reg1 first register
         * @param imm immediate value
         *
         * @note the constructor does not check if the opcode is a valid L instruction opcode
         */
        constexpr LInstruction(inspx opcode, uint64_t reg1, uint64_t imm)
        {
            m_instruction |= opcode & mask_op;
            m_instruction |= (reg1 & (mask_r1 >> off_r1)) << off_r1;
            m_instruction |= (imm & (mask_imm >> off_imm)) << off_imm;
        }

        /**
         * @brief Construct an L instruction from a raw `uint64_t`
         * @param raw raw value to use
         */
        constexpr explicit LInstruction(uint64_t raw) : m_instruction{raw} {}

        /**
         * @brief get the opcode in this instruction
         * @return opcode
         */
        [[nodiscard]] constexpr auto opcode() const noexcept -> inspx { return static_cast<inspx>(this->m_instruction & mask_op); }

        /**
         * @brief get the first register in this instruction
         * @return first register index
         */
        [[nodiscard]] constexpr auto r1() const noexcept -> uint8_t { return (this->m_instruction & mask_r1) >> off_r1; }

        /**
         * @brief get the immediate in this instruction, sign extended
         * @return immediate value
         */
        [[nodiscard]] constexpr auto imm() const noexcept -> int64_t { return helpers::lsextend(this->m_instruction & mask_imm) >> off_imm; }

        /**
         * @brief get the second register in this instruction
         * @return immediate value
         */
        [[nodiscard]] constexpr auto uimm() const noexcept -> uint64_t { return (this->m_instruction & mask_imm) >> off_imm; }

        /**
         * @brief cast this instruction as an `uint64_t`
         */
        [[nodiscard]] constexpr explicit operator uint64_t() const noexcept { return this->m_instruction; }

    private:
        /** raw instruction value */
        uint64_t m_instruction{0};
    };

    /**
     * @brief first configuration register, readonly
     */
    enum config_flags_1 : uint16_t
    {
        confflags_page  = 0x0001, /**< support for memory paging */
        confflags_stack = 0x0002, /**< support for stack instructions */
        confflags_idiv  = 0x0004, /**< support for integer division instructions */
        confflags_int   = 0x0008, /**< support for software interrupts */
        confflags_flt   = 0x0010, /**< support for hardware floating point */
        confflags_fence = 0x0020, /**< support for memory fences */
        confflags_cset  = 0x0040, /**< support for conditional get/set */
        confflags_cmove = 0x0080, /**< support for conditional move */
        confflags_m64   = 0x0100, /**< multiple execution instructions, 64 bit */
        confflags_m128  = 0x0200, /**< multiple execution instructions, 128 bit */
        confflags_m256  = 0x0400, /**< multiple execution instructions, 256 bit */
        confflags_m512  = 0x0800, /**< multiple execution instructions, 512 bit */
        confflags_ioint = 0x1000, /**< @b programmable hardware interrupts */
        confflags_host  = 0x2000  /**< supports hosted environment functions */
    };

    constexpr uint64_t config_value = confflags_stack | confflags_idiv | confflags_host | confflags_ioint;
    constexpr uint64_t int_count = 0xFFFFFFFFFFFFEU; // 2^52 - 2

    struct thread_model_t
    {
        /** processor functionality flags */
        uint64_t flags;

        /** how many interrupts the processor is able to handle*/
        uint64_t interrupt_count;

        /** how deep can pages go */
        uint64_t page_level;

        /** how big can pages be*/
        uint64_t page_size;

        /** string with model name */
        std::array<uint64_t, 4> model_name;

        /** define the last known i/o address */
        uint64_t io_address_space;

        /** define the last instruction opcode */
        uint64_t last_instruction_index;
    };

    /**
     * @brief defines a thread that will run vm code
     *
     * threads have 16 registers, but 15 are actually usable (r0 is reset to zero
     * every cycle). besides that, the architecture design only worries about stack
     * registers to save state on interrupts,
     * but the compiler does set some conventions, being:
     *
     * r1: 1st return register
     * r2: stack pointer
     * r3: base pointer
     *
     * r13: second processor call argument (function switch)
     * r14: first processor call argument (interrupt space)
     *
     * r15 going upwards: function arguments
     *
     */
    class Thread
    {
    public:
        /** register index for pcall arguments */
        static const constexpr auto pcall_reg = 15;

        /** register index for the first return value on processor calls */
        static const constexpr auto pcall_1stret = 14;

        /** register index for the second return value on processor calls*/
        static const constexpr auto pcall_2ndret = 13;

        /** register to push invalid opcode into before calling the required invalid opcode */
        static const constexpr auto pcall_invopc = 14;

        /** count all registers inside the processor */
        static const constexpr auto register_count = 16;

        /** count for all special registers */
        static const constexpr auto sreg_count = 6;

        /** instruction pointer register index */
        static const constexpr auto instrptr_idx = 0;

        /** interrupt vector register index */
        static const constexpr auto intvec_idx = 1;

        /** high level page pointer index */
        static const constexpr auto pagptr_idx = 2;

        /** current called pcall*/
        static const constexpr auto curpcall_idx = 3;

        /** return address for current pcall */
        static const constexpr auto pcallret_idx = 4;

        /**
         * index for special flags register
        */
        static const constexpr auto flags_reg = 5;

        /**
         * @brief initalize a thread
         *
         * @param memory pointer to memory region
         * @param memory_size size of memory in bytes
         * @param model thread information
         */
        Thread(std::unique_ptr<uint8_t[]> memory, uint64_t memory_size, struct thread_model_t *model, uint64_t entry_point = 0)
            : m_special{{entry_point, 0, 0, 0}}, m_memory{std::move(memory)}, m_memory_size{memory_size}, m_model{model}
        {
        }

        /**
         * @brief get a register by index
         * @param index index of register to get value from
         * @return reference of the register on given index
         */
        [[nodiscard]] constexpr auto registers(std::size_t index) noexcept -> auto &
        {
            return this->m_registers[index];
        }

        /**
         * @brief get all registers in an array
         * @return reference to the array of registers
         */
        [[nodiscard]] constexpr auto allregs() noexcept -> auto & { return this->m_registers; }

        /**
         * @brief get the program counter register
         * @return program counter reference
         */
        [[nodiscard]] constexpr auto progc() noexcept -> auto & { return this->m_special[instrptr_idx]; }

        /**
         * @brief get the interrupt vector register
         * @return interrupt vector reference
         */
        [[nodiscard]] constexpr auto intvec() noexcept -> auto & { return this->m_special[intvec_idx]; }

        /**
         * @brief get the size of the memory in bytes
         * @return memory byte size
         */
        [[nodiscard]] constexpr auto memsize() const noexcept -> auto { return this->m_memory_size; }

        /**
         * @brief get the pointer to the working memory region
         * @return memory pointer
         *
         * @note `fetch` is not implemented as a member function as it requires processor calls
         */
        [[nodiscard]] constexpr auto memory() noexcept -> auto & { return this->m_memory; }

        /**
         * @brief get the model information register
         * @return model information register value
         */
        [[nodiscard]] constexpr auto model() const noexcept -> auto { return this->m_model; }

        /**
         * @brief get the current processor call status
         * @return processor call reference
         */
        [[nodiscard]] constexpr auto pcall() noexcept -> auto & { return this->m_pcall; }

        /**
         * @brief get the current processor signal
         * @return processor signal reference
         */
        [[nodiscard]] constexpr auto signal() noexcept -> auto & { return this->m_signal; }

        /**
         * @brief get current pcall return address
         * @return processor pcall return reference
         */
        [[nodiscard]] constexpr auto pcallret() noexcept -> auto& { return this->m_special[pcallret_idx]; }

        /**
         * @brief apply a function from Rinstruction values
         * @tparam T type of function (template deducted)
         * @param instr R instruction to get register indexes from
         * @param func function to use
         *
         * does: `rd <- func r1 r2`
         *
         */
        template <typename T>
        constexpr void apply_instr(RInstruction instr, T func) noexcept
        {
            this->m_registers[instr.rd()] =
                func(this->m_registers[instr.r1()], this->m_registers[instr.r2()]);
        }

        /**
         * @brief apply a function from Sinstruction values
         * @tparam T type of function (template deducted)
         * @param instr S instruction to get register indexes and immediates from
         * @param func function to use
         *
         * does: 'rd <- func r1 imm`
         */
        template <typename T>
        constexpr void apply_instr(SInstruction instr, T func) noexcept
        {
            this->m_registers[instr.rd()] =
                func(this->m_registers[instr.r1()], instr.uimm());
        }

    private:
        std::array<uint64_t, register_count> m_registers{{0}}; /**< thread registers */
        /**
         * @brief status registers, not directly affected by instructions but there
         * are ways to use them,
         */
        std::array<uint64_t, sreg_count> m_special{{
            0, /* instruction pointer, >= 61 bits */
            0, /* interrupt vector, >= 60 bits */
            0, /* page pointer, >= 60 bits */
            0, /* pcall index, >=52 bits*/
            0, /* interrupt return pointer, >= 60bits */
            0, /* processor flags */
        }};
        std::unique_ptr<uint8_t[]> m_memory{};    /**< thread memory pointer */
        uint64_t m_memory_size;                   /**< thread memory size */
        struct thread_model_t *m_model;           /**< thread model pointer */
        ProcessorCall m_pcall{NormalExecution};   /**< which execution state the cpu is in */
        ThreadDestruction m_signal{DoNotDestroy}; /**< thread destruction signal */
    };

    /**
     * @brief define a buffer which translates recent page accesses
     */
    class TranslationBuffer
    {
    private:
        /**
         * @brief define an entry inside the translation buffer,
         * all entries will have
         * bytes 0-63, low 12 unused: virtual page number
         * bytes 64-127, low 12 unused: physical address page
         * bytes 128-143: flags for every
         */
        struct tbentry
        {
            
            /** region mapped from this */
            uint64_t m_to{0};

            /**
             * @brief mask flags inside a tlb entry
            */
            enum flag_mask : uint16_t
            {
                /** is this entry used inside the line*/
                mis_used = 0x8000,

                /** is this entry invalid inside the line */
                mis_dirty = 0x4000,

                /** can this entry be read in userspace */
                muser_read = 0x2000,

                /** can this entry be written in userpace */
                muser_write = 0x1000,
                
                /** can this entry be executed in userspace */
                muser_exec = 0x0800,

                /** get an octet for all user permissions */
                muser_perm = 0x3800,

                /** can this entry be read from in sysspace */
                msys_read = 0x0400,

                /** can this entry be written to in sysspace */
                msys_write = 0x0200,

                /** can this entry be executed in sysspace */
                msys_exec = 0x0100,

                /** octet for system permissions in this memory region */
                msys_perm = 0x0700,

                /** byte for last access in this region */
                mentry_time = 0x00FF,
            };

            enum flag_offset : uint8_t
            {
                ois_used = 15,
                ois_dirty = 14,
                ouser_read = 13,
                ouser_write = 12,
                ouser_exec = 11,
                ouser_perm = 11,
                osys_read = 10,
                osys_write = 9,
                osys_exec = 8,
                osys_perm = 8,
                oentry_time = 0,
            };

            

            // Generic getter function
            [[nodiscard]] constexpr auto get_flag(flag_offset offset) const noexcept -> bool { return (m_flags & ((1 << offset) - 1)) != 0; }

            // Generic setter function
            constexpr auto set_flag(flag_offset offset, bool val = true) noexcept -> void
            {
                if (val)
                {
                    m_flags |= (1 << offset);
                    return;
                }
                m_flags &= ~(1 << offset);
            }

            [[nodiscard]] constexpr auto time() const noexcept -> uint8_t { return m_flags & flag_mask::mentry_time; }
            constexpr auto set_time(uint8_t time) noexcept -> void { m_flags = (m_flags & ~flag_mask::mentry_time) | time; }

            [[nodiscard]] constexpr auto get_usr() const noexcept -> uint8_t { return (m_flags & flag_mask::muser_perm) >> flag_offset::ouser_perm; }
            constexpr auto set_usr(uint8_t val) noexcept -> void { m_flags = (m_flags & ~flag_mask::muser_perm) | (val << flag_offset::ouser_perm); }

            [[nodiscard]] constexpr auto get_sys() const noexcept -> uint8_t { return (m_flags & flag_mask::msys_perm) >> flag_offset::osys_perm; }
            constexpr auto set_sys(uint8_t val) noexcept -> void { m_flags = (m_flags & ~flag_mask::msys_perm) | (val << flag_offset::osys_perm); }

            /**
             * @brief get the virtual page number
             * 
             * @return uint64_t page number
            */
            [[nodiscard]] constexpr auto get_vpn() const noexcept -> auto { return m_from & ~0x0FFFLLU; }
            constexpr auto set_vpn(uint64_t vpn) noexcept -> void { m_from = vpn & ~0x0FFFLLU; }

            [[nodiscard]] constexpr auto get_ppn() const noexcept -> auto { return m_to & ~0x0FFFLLU; }
            constexpr auto set_ppn(uint64_t ppn) noexcept -> void { m_to = ppn & ~0x0FFFLLU; }


            private:
            uint64_t m_from{0};
            /**
             * bit 15: is_used flag
             * bit 14: is_dirty flag
             * bit 13: user read
             * bit 12: user write
             * bit 11: user execute
             * bit 10: system read
             * bit 09: system write
             * bit 08: system execute
             *
             * bits 7-0: entry time*/
            uint16_t m_flags{flag_mask::mis_dirty};
        };

        std::array<tbentry, 64> m_entries{{}};

        /**
         * @brief get the last addressable entry in the virtual page
        */
        [[nodiscard]] constexpr auto entry_vpn(uint16_t index) { return m_entries.at(index).get_vpn() | 0x0FFF;}


    };

    /**
     * @brief pointer to a function that manages a hosted interrupt
     */
    using hosted_int = void (*)(Thread *);

    struct thread_hosted_functions
    {
        /** version of those functions */
        uint64_t version;

        /** default interrupt call*/
        hosted_int interrupt;

        /** read from fd function */
        hosted_int read;

        /** write to fd function */
        hosted_int write;

        /** open file function */
        hosted_int open;

        /** close fd function*/
        hosted_int close;
    };

    /**
     * @brief defines a function type related to interrupts
     */
    using interrupt_function_t = void (*)(int, Thread *);

    /**
     * @brief defines a function for generating instructions
     */
    using instr_dispatch_t = void (*)(Thread *, union instruction_t);

    /***/
    using thread_return = std::pair<bool, int>;

    /**
     * @brief run code from a file
     *
     * @param argc main's argc
     * @param[in] argv main's argv
     * @param thread current thread to run
     * @param step set to only run one instruction, defaults to false but used when testing
     * @param flags flags to which instructions to run
     * @return exit codet
     */
    auto run(int argc, char **argv, Thread &thread, bool step = false, config_flags_1 flags = config_flags_1(config_value)) -> thread_return;

    /** @} */ /* end of group Virtual Instrucion Set Emulation */

    /**
     * @brief defines a function type related to interrupts
     *
     */
    using interrupt_function_t = void (*)(int, Thread *);

} // namespace supernova
