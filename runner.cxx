#include "supernova.h"
#include <iostream>
#include <bitset>
#ifndef SUPERNOVA_VERSION
#define SUPERNOVA_VERSION ""
#endif

[[gnu::cold]]
void print_help() {
    std::cout <<
        "Supernova v" SUPERNOVA_VERSION ": Zenith virtual machine runtime\n"
        " usage: snvm [options] \"executable name\" -- [executable args]\n"
        "options:\n"
        "  -h --help           | display this help\n"
        "  -v --version        | print current version\n"
        "  -p --properties     | get current virtual machine properties\n";
}

[[gnu::cold]]
void print_properties() {
    std::cout << 
        "Properties:\n"
        "===================\n"
        "thread model:\n" 
        "\tflags: 0b" << std::bitset<16>(supernova::config_value) << "\n"
        "\tpossible interrupt count: " << supernova::int_count << "\n"
        "======================================\n"
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

int main(int argc, char ** argv) {
    if (argc == 1 || argv[1] == std::string_view("-h") || argv[1] == std::string_view("--help")) {
        print_help();
        return 0;
    }
    if (argv[1] == std::string_view("-v") || argv[1] == std::string_view("--version")) {
        std::cout << SUPERNOVA_VERSION << '\n';
        return 0;
    }
    if (argv[1] == std::string_view("-p") || argv[1] == std::string_view("--properties")) {
        print_properties();
        return 0;
    }

    auto file_info = supernova::headers::read_file(argv[1]);

    if (file_info.status != supernova::headers::read_status::ReadOk) {
        std::cerr << "could run file, status code = " << static_cast<int>(file_info.status) << '\n';
        return file_info.status;
    }

    auto thread = supernova::Thread(std::move(file_info.memory_pointer), file_info.memory_size, nullptr, file_info.entry_point);

    supernova::run(0, nullptr, thread);
}