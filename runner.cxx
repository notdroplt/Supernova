#include <supernova.h>
#include "args.h"
#include "headers.h"
#include <iostream>

int main(int argc, char ** argv) {
    auto const args = supernova::arguments::load_argv(argc, argv);

    if (!args.should_continue) {
        return 0;
    }

    auto file_info = supernova::headers::read_file(args);

    if (file_info.status != supernova::headers::read_status::ReadOk) {
        std::cerr << "could run file, status code = " << static_cast<int>(file_info.status) << '\n';
        return file_info.status;
    }

    auto thread = supernova::Thread(std::move(file_info.memory_pointer), file_info.memory_size, nullptr, file_info.entry_point);

    return supernova::run(0, nullptr, thread, false, args.should_enable).second;
}