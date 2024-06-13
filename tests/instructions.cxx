#include "../supernova.h"
#include <random>
#include <iostream>
#include <iomanip>
using namespace supernova;


int instructions(int argc, char ** argv)
{
    auto rengine = std::mt19937_64{};

    srand(time(NULL));
    rengine.discard(rand() % (rand() % 28657));

    if (argc == 0) 
    {
        std::cerr << "expected instruction type to test";
    }

    ///
    /// R instruction test
    ///
    if (argv[1] == std::string("rinst")) {
        const auto opc = static_cast<inspx>(rengine() & RInstruction::mask_op);
        const auto r1 = (rengine() & RInstruction::mask_r1) >> RInstruction::off_r1;
        const auto r2 = (rengine() & RInstruction::mask_r2) >> RInstruction::off_r2;
        const auto rd = (rengine() & RInstruction::mask_rd) >> RInstruction::off_rd;

        std::cerr << std::hex << std::boolalpha << std::setfill('0') << std::setw(2)
                << "constructing R instruction with opcode = `" << static_cast<int>(opc) << "`, r1 = `" << std::setw(2) << r1 << "`, r2 = `" << std::setw(2) << r2 << "`, and rd = `" << std::setw(2) << rd << "`\n"
                << "and expecting the result to be " << std::setw(16) << (static_cast<int>(opc) | (r1 << RInstruction::off_r1) | (r2 << RInstruction::off_r2) | (rd << RInstruction::off_rd)) ;

        auto rinst = RInstruction(opc, r1, r2, rd);

        if (static_cast<uint64_t>(rinst) != (static_cast<int>(opc) | (r1 << RInstruction::off_r1) | (r2 << RInstruction::off_r2) | (rd << RInstruction::off_rd)))
        {
            return 1;
        }

        std::cerr << " (got `" << static_cast<uint64_t>(rinst) << "`)\n" 
                << "comparing getters: \n" 
                << "RInstruction::opcode() -> " << static_cast<int>(rinst.opcode()) << "(getter) == " << static_cast<int>(opc) << "(setval) ? " << (static_cast<int>(rinst.opcode()) == static_cast<int>(opc)) << "\n"
                << "RInstruction::r1() -> 0x" << std::setw(2) << rinst.r1() << "(getter) == 0x" << std::setw(2) << r1 << "(setval) ? " << (rinst.r1() == r1) << "\n"
                << "RInstruction::r2() -> 0x" << std::setw(2) << rinst.r2() << "(getter) == 0x" << std::setw(2) << r2 << "(setval) ? " << (rinst.r2() == r2) << "\n"
                << "RInstruction::rd() -> 0x" << std::setw(2) << rinst.rd() << "(getter) == 0x" << std::setw(2) << rd << "(setval) ? " << (rinst.rd() == rd) << "\n";

        return rinst.r1() != r1 || rinst.r2() != r2 || rinst.rd() != rd;
    }

    if (argv[1] == std::string("sinst")) {
        const auto opc = static_cast<inspx>(rengine() & SInstruction::mask_op);
        const auto r1 = (rengine() & SInstruction::mask_r1) >> SInstruction::off_r1;
        const auto rd = (rengine() & SInstruction::mask_rd) >> SInstruction::off_rd;
        const auto imm = (rengine() & SInstruction::mask_imm) >> SInstruction::off_imm;

        std::cerr << std::hex << std::boolalpha << std::setfill('0') << std::setw(2)
                << "constructing S instruction with opcode = `" << static_cast<int>(opc) << "`, r1 = `" << std::setw(2) << r1 << "`, rd = `" << std::setw(2) << rd << "`, and imm = `" << std::setw(12) << imm << "`\n"
                << "and expecting the result to be " << std::setw(16) << (static_cast<int>(opc) | (r1 << SInstruction::off_r1) | (rd << SInstruction::off_rd) | (imm << SInstruction::off_imm)) ;

        auto sinst = SInstruction(opc, r1, rd, imm);

        std::cerr << " (got `" << static_cast<uint64_t>(sinst) << "`)\n";
        if (static_cast<uint64_t>(sinst) != (opc | (r1 << SInstruction::off_r1) | (rd << SInstruction::off_rd) | (imm << SInstruction::off_imm)))
        {
            return 1;
        }

        std::cerr << "comparing getters: \n" 
                << "SInstruction::opcode() -> " << static_cast<int>(sinst.opcode()) << "(getter) == " << static_cast<int>(opc) << "(setval) ? " << (static_cast<int>(sinst.opcode()) == static_cast<int>(opc)) << "\n"
                << "SInstruction::r1() -> 0x" << std::setw(2) << sinst.r1() << "(getter) == 0x" << std::setw(2) << r1 << "(setval) ? " << (sinst.r1() == r1) << "\n"
                << "SInstruction::rd() -> 0x" << std::setw(2) << sinst.rd() << "(getter) == 0x" << std::setw(2) << rd << "(setval) ? " << (sinst.rd() == rd) << "\n"
                << "SInstruction::imm() -> 0x" << std::setw(12) << sinst.uimm() << "(getter) == 0x" << std::setw(12) << imm << "(setval) ? " << (sinst.uimm() == imm) << "\n";

        return sinst.r1() != r1 || sinst.uimm() != imm || sinst.rd() != rd;
    }

    if (argv[1] == std::string("linst")) {
        const auto opc = static_cast<inspx>(rengine() & LInstruction::mask_op);
        const auto r1 = (rengine() & LInstruction::mask_r1) >> LInstruction::off_r1;
        const auto imm = (rengine() & LInstruction::mask_imm) >> LInstruction::off_imm;

        std::cerr << std::hex << std::boolalpha << std::setfill('0') << std::setw(2)
                << "constructing L instruction with opcode = `" << static_cast<int>(opc) << "`, r1 = `" << std::setw(2) << r1 << "`, and imm = `" << std::setw(13) << imm << "`\n"
                << "and expecting the result to be " << std::setw(16) << (static_cast<int>(opc) | (r1 << LInstruction::off_r1) | (imm << LInstruction::off_imm)) ;

        auto linst = LInstruction(opc, r1, imm);

        std::cerr << " (got `" << static_cast<uint64_t>(linst) << "`)\n";
        if (static_cast<uint64_t>(linst) != (opc | (r1 << LInstruction::off_r1) | (imm << LInstruction::off_imm)))
        {
            return 1;
        }

        std::cerr << "comparing getters: \n" 
                << "LInstruction::opcode() -> " << static_cast<int>(linst.opcode()) << "(getter) == " << static_cast<int>(opc)   << "(setval) ? " << (static_cast<int>(linst.opcode()) == static_cast<int>(opc)) << "\n"
                << "LInstruction::r1() -> 0x"   << std::setw(2)  << linst.r1()      << "(getter) == 0x" << std::setw(2) << r1   << "(setval) ? " << (linst.r1() == r1) << "\n"
                << "LInstruction::imm() -> 0x"  << std::setw(12) << linst.uimm()    << "(getter) == 0x" << std::setw(13) << imm << "(setval) ? " << (linst.uimm() == imm) << "\n";

        return linst.r1() != r1 || linst.uimm() != imm;
    }


    return 0;
}