#pragma once
#include <cstdint>
#include <supernova.h>
namespace supernova::arguments
{
    /**
     * @brief arguments passed in the command line into the virtual machine
     */
    struct arguments
    {
        /** the most that the virtual machine should allocate, the actual memory size might be smaller*/
        uint64_t memort_limit{0};

        /** places of the virtual machine that will be or wont be enabled by the code*/
        supernova::config_flags_1 should_enable{supernova::config_value};

        /** filename to run*/
        char *filename{nullptr};

        bool load_libs{false};
        bool should_continue{false};
    };

    /**
     * @brief load all argv into their respective values in the struct
     * 
     * @param argc main's argc
     * @param [in] argv main's argv
     * 
     * @return formatted arguments
    */
    arguments load_argv(int argc, char** argv);


} // namespace supernova::arguments
