/**
 * @file supernova.hh
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
#include <variant>

namespace supernova
{

    /**
     * @brief supernova opcodes (64 bit version)
     *
     * instruction types:
     *
     * R type (registers only) => register-register-register
     *
     * bitwise representation:
     * @code
     * [63-24][23-19][18-14][13-8][7-0]
     *    |      |      |     |      |
     *  [pad]  [r2]   [r1]   [rd] [opcode]
     * @endcode
     *
     * S type ("small" immediate) => register-register-immediate
     *
     * bitwise representation:
     * @code
     * [63-19][18-14][13-8][7-0]
     *    |      |     |     |
     *  [imm]  [r1]  [rd] [opcode]
     * @endcode
     *
     * L type ("long" immediate) => register-immediate
     *
     * bitwise representation:
     * @code
     * [63-14][13-8][7-0]
     *    |     |     |
     *  [imm]  [rd] [opcode]
     * @endcode
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
     *
     *
     * on groups 0 and 1, the LSB defines if the instruction is an R type (when 0)
     * or an S type (when 1) group 2, has only S types and the `jal` instruction
     * (L type) group 3 is always L type
     */
    enum instruction_prefixes : uint8_t
    {
        /**
         * @defgroup InP instruction prefixes
         *
         * @brief those groups define instructions that have similar behaviors
         * between themselves
         *
         *
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
        alsr_instrc = 0x0C, /**< `als r#, r#, r#`  : R type */
        alsi_instrc = 0x0D, /**< `als r#, r#, imm` : S type */
        arsr_instrc = 0x0E, /**< `ars r#, r#, r#`  : R type */
        arsi_instrc = 0x0F, /**< `ars r#, r#, imm` : S type */
        /** @} */           /* InPG0 */

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
        call_instrc = 0x1C,  /**< `call r#, r#, r#`  : R type */
        push_instrc = 0x1D,  /**< `push r#, r#, imm` : S type */
        retn_instrc = 0x1E,  /**< `retn r#, r#, r#`  : R type */
        pull_instrc = 0x1F,  /**< `pull r#, r#, imm` : S type */
        /** @} */            /* InPG1 */

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
        pbreak_instrc = 0x3B,  /**< `pbreak 0`           : L type */
        bout_instrc = 0x3C,    /**< `outb r#, r#, 0`     : S type */
        out_instrc = 0x3D,     /**< `outw r#, r#, 0`     : S type */
        bin_instrc = 0x3E,     /**< `inb r#, r#, 0`      : S type */
        in_instrc = 0x3F,      /**< `inw r#, r#, 0`      : S type */
        /** @} */              /** InPG3*/

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
        /** TODO: group 5, conditional move instructions */
        /** TODO: group 6, memory fences */
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
     * @brief Sign extend immediates on small integer instructions
     * 
     * @param number number to sign extend
     * 
     * @returns int64_t sign extended number
     */
    [[nodiscard, gnu::const]] constexpr int64_t ssextend(uint64_t number) noexcept
    {
        constexpr auto mask = 0xffffc00000000000LU;
        constexpr auto index = 46U;
        return number >> index != 0 ? static_cast<int64_t>(number) | mask : static_cast<int64_t>(number);
    }


    /**
     * @brief Sign extend immediates on long integer instructions
     * 
     * @param number number to sign extend
     * 
     * @returns int64_t sign extended number
     */
    [[nodiscard, gnu::const]] constexpr int64_t lsextend(uint64_t number) noexcept
    {
        constexpr auto mask = 0xfff8000000000000LU;
        constexpr auto index = 51U;
        return number >> index != 0 ? static_cast<int64_t>(number) | mask : static_cast<int64_t>(number);
    }


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
        static const constexpr auto mask_r1 = 0x0000000000001F00U;

        /** mask for the r2 index on a raw uint64_t */
        static const constexpr auto mask_r2 = 0x000000000003E000U;

        /** mask for the rd index on a raw uint64_t */
        static const constexpr auto mask_rd = 0x00000000007C0000U;

        /** offset for the opcode on a raw uint64_t */
        static const constexpr auto off_op = 0U;

        /** offset for the r1 index on a raw uint64_t */
        static const constexpr auto off_r1 = off_op + 8U;

        /** offset for the r2 index on a raw uint64_t */
        static const constexpr auto off_r2 = off_r1 + 5U;

        /** offset for the rd index on a raw uint64_t */
        static const constexpr auto off_rd = off_r2 + 5U;

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
        constexpr RInstruction(instruction_prefixes opcode, uint64_t reg1, uint64_t reg2, uint64_t regd)
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
        [[nodiscard]] constexpr auto opcode() const noexcept
        {
            return static_cast<instruction_prefixes>(this->m_instruction & mask_op);
        }

        /**
         * @brief get the first register in this instruction
         * @return first register index
         */
        [[nodiscard]] constexpr auto r1() const noexcept
        {
            return (this->m_instruction & mask_r1) >> off_r1;
        }

        /**
         * @brief get the second register in this instruction
         * @return second register index
         */
        [[nodiscard]] constexpr auto r2() const noexcept
        {
            return (this->m_instruction & mask_r2) >> off_r2;
        }

        /**
         * @brief get the destination register in this instruction
         * @return destination register index
         */
        [[nodiscard]] constexpr auto rd() const noexcept
        {
            return (this->m_instruction & mask_rd) >> off_rd;
        }

        /**
         * @brief cast this instruction to an `uint64_t`
         */
        [[nodiscard]] constexpr explicit operator uint64_t() const noexcept
        {
            return this->m_instruction;
        }

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
        static const constexpr auto mask_r1 = 0x0000000000001F00U;

        /** mask for the r2 index on a raw `uint64_t` */
        static const constexpr auto mask_rd = 0x000000000003E000U;

        /** mask for the immediate on a raw `uint64_t` */
        static const constexpr auto mask_imm = 0xFFFFFFFFFFFC0000U;

        /** offset for the opcode on a raw `uint64_t` */
        static const constexpr auto off_op = 0U;

        /** offset for the r1 index on a raw `uint64_t` */
        static const constexpr auto off_r1 = off_op + 8U;

        /** offset for the r2 index on a raw `uint64_t` */
        static const constexpr auto off_rd = off_r1 + 5U;

        /** offset for the immediate on a raw `uint64_t` */
        static const constexpr auto off_imm = off_rd + 5U;

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
        constexpr SInstruction(instruction_prefixes opcode, uint64_t reg1, uint64_t regd, uint64_t imm)
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
        [[nodiscard]] constexpr auto opcode() const noexcept
        {
            return static_cast<instruction_prefixes>(this->m_instruction & mask_op);
        }

        /**
         * @brief get the first register in this instruction
         * @return first register index
         */
        [[nodiscard]] constexpr auto r1() const noexcept
        {
            return (this->m_instruction & mask_r1) >> off_r1;
        }

        /**
         * @brief get the destination register in this instruction
         * @return destination register index
         */
        [[nodiscard]] constexpr auto rd() const noexcept
        {
            return (this->m_instruction & mask_rd) >> off_rd;
        }

        /**
         * @brief get the second register in this instruction, sign extended
         * @return second register index
         */
        [[nodiscard]] constexpr auto imm() const noexcept
        {
            return ssextend((this->m_instruction & mask_imm) >> off_imm);
        }

        /**
         * @brief get the second register in this instruction, do not extend sign
         * @return second register index
         */
        [[nodiscard]] constexpr auto uimm() const noexcept
        {
            return (this->m_instruction & mask_imm) >> off_imm;
        }

        /**
         * @brief cast this instruction as an `uint64_t`
         */
        [[nodiscard]] constexpr explicit operator uint64_t() const noexcept
        {
            return this->m_instruction;
        }

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
        static const constexpr auto mask_r1 = 0x0000000000001F00U;

        /** mask for the immediate index on a raw `uint64_t` */
        static const constexpr auto mask_imm = 0xFFFFFFFFFFFFE000U;

        /** offset for the opcode on a raw `uint64_t` */
        static const constexpr auto off_op = 0U;

        /** offset for the r1 index on a raw `uint64_t` */
        static const constexpr auto off_r1 = off_op + 8U;

        /** offset for the r2 index on a raw `uint64_t` */
        static const constexpr auto off_imm = off_r1 + 5U;

        /**
         * @brief Generate an L instruction from multiple values
         * @param opcode L instruction opcode
         * @param reg1 first register 
         * @param imm immediate value
         * 
         * @note the constructor does not check if the opcode is a valid L instruction opcode
         */
        constexpr LInstruction(instruction_prefixes opcode, uint64_t reg1, uint64_t imm)
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
        [[nodiscard]] constexpr auto opcode() const noexcept
        {
            return static_cast<instruction_prefixes>(this->m_instruction & mask_op);
        }

        /**
         * @brief get the first register in this instruction
         * @return first register index
         */
        [[nodiscard]] constexpr auto r1() const noexcept
        {
            return (this->m_instruction & mask_r1) >> off_r1;
        }

        /**
         * @brief get the immediate in this instruction, sign extended
         * @return immediate value
         */
        [[nodiscard]] constexpr auto imm() const noexcept
        {
            return lsextend((this->m_instruction & mask_imm) >> off_imm);
        }

        /**
         * @brief get the second register in this instruction
         * @return immediate value
         */
        [[nodiscard]] constexpr auto uimm() const noexcept
        {
            return (this->m_instruction & mask_imm) >> off_imm;
        }

        /**
         * @brief cast this instruction as an `uint64_t`
         */
        [[nodiscard]] constexpr explicit operator uint64_t() const noexcept
        {
            return this->m_instruction;
        }

    private:
        /** raw instruction value */
        uint64_t m_instruction{0};
    };

    /**
     * @brief defines a castable type between a raw `uint64_t` and
     * instruction blocks
     *
     * Size : 8 bytes
     */

    using Instruction = std::variant<uint64_t, RInstruction, SInstruction, LInstruction>;

    /**
     * @brief first configuration register, readonly
     */
    enum config_flags_1 : uint16_t
    {
        confflags_paging = 0x0000,     /**< support for memory paging */
        confflags_stack = 0x0001,      /**< support for stack instructions */
        confflags_intdiv = 0x0002,     /**< support for integer division instructions */
        confflags_interrupts = 0x0004, /**< support for software interrupts */
        confflags_floats = 0x0008,     /**< support for hardware floating point */
        confflags_fences = 0x0010,     /**< support for memory fences */
        confflags_condset = 0x0020,    /**< support for conditional get/set */
        confflags_condmove = 0x0040,   /**< support for conditional move */
        confflags_multi64 = 0x0080,    /**< multiple execution instructions, 64 bit */
        confflags_multi128 = 0x0100,   /**< multiple execution instructions, 128 bit */
        confflags_multi256 = 0x0200,   /**< multiple execution instructions, 256 bit */
        confflags_multi512 = 0x0400,   /**< multiple execution instructions, 512 bit */
        confflags_ioint = 0x0800,      /**< @b programmable hardware interrupts */
        confflags_hosted = 0x1000      /**< supports hosted environment functions */
    };

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
     * threads have 32 registers, but 31 are actually usable (r0 is reset to zero
     * every cycle). besides that, the architecture design only worries about stack
     * registers to save state on interrupts,
     * but the compiler does set some conventions, being:
     *
     * r1: 1st return register
     * r2: stack pointer
     * r3: base pointer
     *
     * r28: second processor call argument (function switch)
     * r29: first processor call argument (interrupt space)
     *
     * r31 going upwards: function arguments
     *
     */
    class Thread
    {
    public:
        /** register index for the function switch inside processor calls */
        static const constexpr auto pcall_fswitch = 28;

        /** register index for the interrupt space inside processor calls */
        static const constexpr auto pcall_intspace = 29;

        /** register index for the first return value on processor calls */
        static const constexpr auto pcall_1stret = 30;

        /** register index for the second return value on processor calls*/
        static const constexpr auto pcall_2ndnret = 31;

        /** count all registers inside the processor */
        static const constexpr auto register_count = 32;

        Thread(uint8_t* memory, uint64_t memory_size, struct thread_model_t *model)
            : m_memory{memory}, m_memory_size{memory_size}, m_model{model}
        {
        }

        /**
         * @brief get a register by index
         * @param index index of register to get value from
         * @return reference of the register on given index
         */
        [[nodiscard]] constexpr auto &registers(int index) noexcept { return m_registers[index]; }

        /**
         * @brief get all registers in an array
         * @return reference to the array of registers
         */
        [[nodiscard]] constexpr auto &allregs() noexcept { return m_registers; }

        /**
         * @brief get the program counter register
         * @return program counter reference
         */
        [[nodiscard]] constexpr auto &progc() noexcept { return m_program_counter; }

        /**
         * @brief get the interrupt vector register
         * @return interrupt vector reference
         */
        [[nodiscard]] constexpr auto &intvec() noexcept { return m_int_vector; }

        /**
         * @brief get the size of the memory in bytes
         * @return memory byte size
         */
        [[nodiscard]] constexpr auto memsize() const noexcept { return m_memory_size; }

        /**
         * @brief get the pointer to the working memory region
         * @return memory pointer
         *
         * @note `fetch` is not implemented as a member function as it requires processor calls
         */
        [[nodiscard]] constexpr auto &memory() noexcept { return m_memory; }

        /**
         * @brief get the model information register
         * @return model information register value
         */
        [[nodiscard]] constexpr auto model() const noexcept { return m_model; }

        /**
         * @brief get the current processor call status
         * @return processor call reference
         */
        [[nodiscard]] constexpr auto &pcall() noexcept { return m_pcall; }

        /**
         * @brief get the current processor signal
         * @return processor signal reference
         */
        [[nodiscard]] constexpr auto &signal() noexcept { return m_signal; }

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
            this->registers(instr.rd()) =
                func(this->registers(instr.r1()), this->registers(instr.r2()));
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
            this->registers(instr.rd()) =
                func(this->registers(instr.r1()), instr.imm());
        }

    private:
        std::array<uint64_t, register_count> m_registers{{0}}; /**< thread registers */
        uint8_t* m_memory{};                 /**< thread memory pointer */
        uint64_t m_program_counter{0};                         /**< thread instructon pointer */
        uint64_t m_int_vector{0};                              /**< interrupt vector pointer*/
        uint64_t m_memory_size;                                /**< thread memory size */
        struct thread_model_t *m_model;                        /**< thread model pointer */
        ProcessorCall m_pcall{NormalExecution};                /**< which execution state the cpu is in */
        ThreadDestruction m_signal{DoNotDestroy};              /**< thread destruction signal */
    };

    /**
     * @brief pointer to a function that manages a hosted interrupt
     */
    using hosted_int = void (*)(Thread *);

    struct thread_hosted_functions
    {
        /** version of those functions */
        uint64_t version;

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

    /**
     * \defgroup virtset Virtual Instruction Set Emulation
     *
     * \brief all the instruction prefixes used on the emulated vm cpu
     *
     * @{
     */

    /**
     * @brief defines a header for a runnable code on the virtual machine
     *
     * @note all positions should \b not consider the header offset
     */
    struct virtmacheader_t
    {
        /** file magic "Zenithvm" */
        uint64_t magic;

        /** current header version */
        uint64_t version;

        /** initialized data size */
        uint64_t data_size;

        /** data section start on the file */
        uint64_t data_start;

        /** data section start on memory */
        uint64_t data_offset;

        /** runnable code size */
        uint64_t code_size;

        /** code section start on the file */
        uint64_t code_start;

        /** code section start on memory */
        uint64_t code_offset;

        /** code entry point */
        uint64_t entry_point;

        /** padding value */
        uint64_t pad;
    };

    struct thread_return
    {
        bool gracefully_exit;
        int status;
    };

    /**
     * @brief run code from a file
     *
     * @param argc main's argc
     * @param[in] argv main's argv
     * @param thread current thread to run 
     * @param step set to only run one instruction, defaults to false but used when testing
     * @return exit codet
     */
    thread_return run(int argc, char **argv, Thread &thread, bool step = false);

    

    /** @} */ /* end of group Virtual Instrucion Set Emulation */

    /**
     * @brief defines a function type related to interrupts
     *
     */
    using interrupt_function_t = void (*)(int, Thread *);

} // namespace zenith::supernova
