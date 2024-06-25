#include "../supernova.h"
#include <iostream>
#include <memory>
#include <random>
#include <iomanip>
/// this test will go over all instructions that only affect registers, other tests are made with other specific registers

using namespace supernova;

int test_instruction(Thread &thread, RInstruction instr, std::mt19937_64 &rengine, const char *instrname, auto expected_fn)
{
    const uint64_t r1 = rengine();
    const uint64_t r2 = rengine();

    thread.progc() = 0;
    thread.registers(1) = r1;
    thread.registers(2) = r2;

    auto instr_ptr = reinterpret_cast<RInstruction *>(thread.memory().get());
    *instr_ptr = instr;

    std::cerr << "== testing instruction `" << std::setw(4) << std::setfill(' ') << instrname << "`, r1 = `" << std::setw(16) << std::setfill('0') << thread.registers(1) << "` and r2  = `" << std::setw(16) << thread.registers(2) << "`: ";

    run(0, nullptr, thread, true);

    auto result = expected_fn(r1, r2) != thread.registers(3);

    std::cerr << "expected `" << std::setw(16) << expected_fn(r1, r2) << "`, got `" << std::setw(16) << thread.registers(3) << "` " << (result ? "in" : "") << "correctly\n";

    return result;
}

int test_instruction(Thread &thread, SInstruction instr, std::mt19937_64 &rengine, const char *instrname, auto expected_fn)
{
    const uint64_t r1 = rengine();
    const uint64_t imm = rengine() & (SInstruction::mask_imm >> SInstruction::off_imm);

    thread.registers(1) = r1;
    thread.progc() = 0;
    auto instr_ptr = reinterpret_cast<SInstruction *>(thread.memory().get());
    instr = SInstruction(instr.opcode(), instr.r1(), instr.rd(), imm);
    *instr_ptr = instr;

    std::cerr << "== testing instruction `" << std::setfill(' ') << std::setw(4) << instrname << "`, r1 = `" << std::setfill('0') << std::setw(16) << thread.registers(1) << "` and imm = `" << std::setw(16) << imm << "`: ";

    run(0, nullptr, thread, true);

    auto result = expected_fn(r1, imm) != thread.registers(3);

    std::cerr << "expected `" << std::setw(16) << expected_fn(r1, imm) << "`, got `" << std::setw(16) << thread.registers(3) << "` " << (result ? "in" : "") << "correctly\n";

    return result;
}

int opcodes(int, char **)
{
    std::cerr << "== constructing thread\n";

    auto memory = std::make_unique_for_overwrite<uint8_t[]>(8);

    auto rengine = std::mt19937_64{};

    // i think this is random enough
    srand(time(NULL));
    rengine.discard(rand() % (rand() % 28657));

    thread_model_t thread_model{
        .flags = confflags_stack | confflags_intdiv | confflags_interrupts | confflags_condset | confflags_hosted,
        .interrupt_count = (1llu << 51) - 1,
        .page_level = 0,
        .page_size = 0,
        .model_name = {0x53757065, 0x726e6f76, 0x61546573, 0x74696e67}, // SupernovaTesting
        .io_address_space = 0x0000,
        .last_instruction_index = in_instrc,
    };

    Thread thread{std::move(memory), 8, &thread_model};

    std::cerr << "== thread constructed successfully!\n"
              << std::hex;

    auto test_result = 0u;

    test_result |= test_instruction(thread, RInstruction(andr_instrc, 1, 2, 3), rengine, "andr", std::bit_and{});
    test_result |= test_instruction(thread, SInstruction(andi_instrc, 1, 3, 0), rengine, "andi", std::bit_and{});
    test_result |= test_instruction(thread, RInstruction(xorr_instrc, 1, 2, 3), rengine, "xorr", std::bit_xor{});
    test_result |= test_instruction(thread, SInstruction(xori_instrc, 1, 3, 0), rengine, "xori", std::bit_xor{});
    test_result |= test_instruction(thread, RInstruction(orr_instrc, 1, 2, 3), rengine, "orr", std::bit_or{});
    test_result |= test_instruction(thread, SInstruction(ori_instrc, 1, 3, 0), rengine, "ori", std::bit_or{});
    test_result |= test_instruction(thread, RInstruction(not_instrc, 1, 2, 3), rengine, "not", [](auto left, auto){ return ~left; });
    test_result |= test_instruction(thread, SInstruction(cnt_instrc, 1, 3, 0), rengine, "cnt", [](auto left, auto) -> uint64_t { return __builtin_popcountl(left); });
    test_result |= test_instruction(thread, RInstruction(llsr_instrc, 1, 2, 3), rengine, "llsr", [](auto left, auto right) -> uint64_t { return right >= 64 ? 0 : left << right; });
    test_result |= test_instruction(thread, SInstruction(llsi_instrc, 1, 3, 0), rengine, "llsi", [](auto left, auto right) -> uint64_t { return right >= 64 ? 0 : left << right; });
    test_result |= test_instruction(thread, RInstruction(lrsr_instrc, 1, 2, 3), rengine, "lrsr", [](auto left, auto right) -> uint64_t { return right >= 64 ? 0 : left >> right; });
    test_result |= test_instruction(thread, SInstruction(lrsi_instrc, 1, 3, 0), rengine, "lrsi", [](auto left, auto right) -> uint64_t { return right >= 64 ? 0 : left >> right; });
    // test_result |= test_instruction(thread, RInstruction(alsr_instrc, 1, 2, 3), rengine, "alsr", std::bit_and{});
    // test_result |= test_instruction(thread, SInstruction(alsi_instrc, 1, 3, 0), rengine, "alsi", std::bit_and{});
    // test_result |= test_instruction(thread, RInstruction(arsr_instrc, 1, 2, 3), rengine, "arsr", std::bit_and{});
    // test_result |= test_instruction(thread, SInstruction(arsi_instrc, 1, 3, 0), rengine, "arsi", std::bit_and{});

    return test_result;
}