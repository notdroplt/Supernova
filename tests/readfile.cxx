#include "../supernova.h"
#include <iomanip>
#include <iostream>
using namespace supernova::headers;

void print_read_return(read_return const &rret)
{
    std::cerr << "{\n"
              << "  status: " << static_cast<int>(rret.status) << '\n'
              << std::hex << std::setfill('0')
              << "  pointer: 0x" << std::setw(16) << (uint64_t)(rret.memory_pointer.get()) << '\n'
              << "  memory_size: 0x" << std::setw(16) << rret.memory_size << "\n}\n";
}

auto test_case(char const *fname, read_return expected)
{
    read_return read_ret = read_file(fname);

    if (read_ret.status == expected.status)
    {
        return false;
    }
    std::cerr << "Reading file \"" << fname << "\" returned: ";
    print_read_return(read_ret);

    std::cerr << "expected: \t";
    print_read_return(expected);
    return true;
    return false;
}

auto readfile(int, char **)
{
    read_return read_ret;
    int result = 0;
    result += test_case("01234567.89a", read_return{FileNotFound});
    result += test_case("smaller.spn", read_return{InvalidHeader});
    result += test_case("invalid_magic.spn", read_return{MagicMismatch});
    return result;
}