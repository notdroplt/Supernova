#include "args.h"
#include "supernova.h"
#include <algorithm>
#include <array>
#include <iostream>
#include <bitset>
#include <iterator>
#include <string_view>
#ifndef SUPERNOVA_VERSION
#define SUPERNOVA_VERSION ""
#endif

[[gnu::cold]]
void print_help()
{
    std::cout << "Supernova v" SUPERNOVA_VERSION ": Zenith virtual machine runtime\n"
                 " usage: snvm [options] [flags] -- [executable args]\n"
                 "options:\n"
                 "  -h --help           | display this help\n"
                 "  -v --version        | print current version\n"
                 "  -p --properties     | get current virtual machine properties\n"
                 "  --run [path]        | run file specified by path"
                 "\n ---- config flags ----\n"
                 " --thread-count=[count]   | amount of threads to have, defaults to 1, max 32\n"
                 " --start-thread=[id]      | which thread to start, no real efect changing but defaults to 1\n"
                 " --add-search-path [path] | path to find new modules, defaults to `.`, `./snmod` and [INSTALLDIR]/snmod\n"
                 " --add-module [name]      | add a module by name"
                 "\n ---- sandbox flags ----\n"
                 " --memory-limit=[size][prefix]    | allocate memory to at most `size` bytes, prefixes need to be one of: b, k[b], m[b], g[b]\n"
                 " --load-modules=(true|false)      | all hypervisor requests to load a library in the host will forcefully fail\n"
                 " --enable-[instr]=(true|false)    | tune certain instructions, instr can be any of: div, int, float, ioint, stack\n"
                 " note: --enable-stack is the only option which already defaults to false \n";
}

[[gnu::cold]]
void print_properties()
{
    std::cout << "Properties:\n"
                 "===================\n"
                 "thread model:\n"
                 "\tflags: 0b"
              << std::bitset<16>(supernova::config_value) << "\n\tpossible interrupt count: "
              << supernova::int_count 
              << "\n======================================\n"
                    "instruction group implementations:\n"
                    "\tgroup 0: fully implemented\n"
                    "\tgroup 1: fully implemented\n"
                    "\tgroup 2: fully implemented\n"
                    "\tgroup 3: no i/o\n"
                    "\tgroup 4: not implemented\n"
                    "\tgroup 5: not implemented\n"
                    "\tgroup 6: not implemented\n"
                    "==============================\n"
                    "pcall -1:\n"
                    "\t0:0 -> r31 = 2, r30 = 2^51 - 1\n"
                    "\t0:1 implemented\n"
                    "\t1:0 -> r31 = 0 paging not yet implemented\n"
                    "\t2:0 -> r31 = 0 (will change shortly)\n";
}

[[nodiscard]] constexpr auto str_starts_with(std::string_view const &big, std::string_view const &small) noexcept
{
    if (small.size() > big.size()) return false;

    for (size_t i = 0; i < small.size(); ++i)
        if (big[i] != small[i])
            return false;

    return true;
}

[[nodiscard]] constexpr auto svtoi(std::string_view const &str, uint64_t &result) noexcept
{
    result = 0;
    for (auto &&c : str)
    {
        if (!isdigit(c))
        {
            switch (c)
            {
            case 'b':
                return true;
            case 'k':
                result *= 1000;
                return true;
            case 'm':
                result *= 1000000;
                return true;
            case 'g':
                result *= 1000000000;
                return true;
            case 't':
                result *= 1000000000000;
                return true;
            case 'B':
                return true;
            case 'K':
                result *= 0x1000;
                return true;
            case 'M':
                result *= 0x1000000;
                return true;
            case 'G':
                result *= 0x1000000000;
                return true;
            case 'T':
                result *= 0x1000000000000;
                return true;
            default:
                return false;
            }
        }
        result = result * 10 + c - '0';
    }
    return true;
}
namespace supernova::arguments
{

    arguments load_argv(int argc, char **argv)
    {
        arguments args{};

        if (argc == 1)
        {
            args.should_continue = false;
            print_help();
            return args;
        }

        for (int i = 1; i < argc; i++)
        {
            auto cur_arg = std::string_view(argv[i]);

            if (str_starts_with(cur_arg, "-h") || str_starts_with(cur_arg, "--help"))
            {
                args.should_continue = false;
                print_help();
                break;
            }

            if (str_starts_with(cur_arg, "-v") || str_starts_with(cur_arg, "--version"))
            {
                args.should_continue = false;
                std::cout << SUPERNOVA_VERSION << '\n';
                break;
            }

            if (str_starts_with(cur_arg, "-p") || str_starts_with(cur_arg, "--properties"))
            {
                args.should_continue = false;
                print_properties();
                break;
            }

            if (str_starts_with(cur_arg, "--run"))
            {
                if (i + 1 >= argc)
                {
                    std::cerr << "run file was not specified\n";
                }

                i++;
                args.filename = argv[i];
                continue;
            }

            if (str_starts_with(cur_arg, "--memory-limit="))
            {
                auto newstr = cur_arg.substr(15, cur_arg.size()); // sizeof the "--memory-limit=" string
                uint64_t value = 0;
                auto res = svtoi(newstr, value);

                if (!res)
                {
                    args.should_continue = false;
                    break;
                }

                args.memort_limit = value;
                continue;
            }

            if (str_starts_with(cur_arg, "--load-modules="))
            {
                args.load_libs = cur_arg.substr(13, cur_arg.size()) == "true";
                continue;
            }

            if (str_starts_with(cur_arg, "--enable"))
            {
                auto equals = cur_arg.find_first_of('=');
                auto element = cur_arg.substr(9, equals - 9);
                auto value = cur_arg.substr(equals+1);
                auto names = std::array<std::string_view, 5>({"div", "int", "float", "ioint", "stack"});
                auto flags = std::array<supernova::config_flags_1, 5>({confflags_idiv, confflags_int, confflags_flt, confflags_ioint, confflags_stack});

                auto flag_idx = std::find(names.begin(), names.end(), element);
                if (flag_idx == names.end()) continue;

                auto flagval = flags[std::distance(names.begin(), flag_idx)];

                if (value == "true")
                    args.should_enable = static_cast<config_flags_1>(flagval | args.should_enable);
                else 
                    args.should_enable = static_cast<config_flags_1>(args.should_enable & ~flagval);
                
    
            }
        }

        return args;
    }
}