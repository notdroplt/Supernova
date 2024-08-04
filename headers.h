#pragma once
#include <cstdint>
#include <memory>
#include "args.h"
namespace supernova::headers
{
    /**
     * @brief first header inside a snova file, responsible for coordinating other headers
     */
    struct main_header
    {
        /** file magic "Zenithvm" */
        uint64_t magic;

        /** current header version */
        uint64_t version;

        /** cpu flags necessary for executing the code */
        uint64_t flags;

        /** memory allocated to the virtual machine, skipped if this is a real processor */
        uint64_t memory_size;

        /** code entry point */
        uint64_t entry_point;

        /** amount of memory regions inside the current file */
        uint64_t memory_regions;
    };

    /**
     * @brief flags for memory areas inside the file
     */
    enum memory_flags : uint8_t
    {
        /** this area is readable */
        mem_read = 0x01,

        /** this area is writable */
        mem_write = 0x02,

        /** this area is executable */
        mem_execute = 0x04,

        /** allocate memory for this area, it does not exist on the file */
        mem_clear = 0x08,

        /** this memory region should go to the executable code memory*/
        mem_exists = 0x10
    };

    /**
     * @brief map for a region of the memory
     */
    struct alignas(uint64_t) memory_map
    {
        /** magic for the memory map area to be detected as a valid memory map*/
        uint64_t magic;

        /** start of the memory map inside the file, if `clear` is set it is just ignored */
        uint64_t start;

        /** size of the memory map both in the file and the virtual memory, in bytes*/
        uint64_t size;

        /** start of the memory map inside the virtual memory*/
        uint64_t offset;

        /** flags for the memory region defined*/
        memory_flags flags;
    };

    /** master magic: "Zenithvm" */
    constexpr auto const master_magic = 0x6D766874696E655ALLU;

    /** memory map magic: "mem_map!" */
    constexpr auto const memmap_magic = 0x2170616D5f6D656DLLU;

    /** version: major(16bit):minor(16bit):patch(32bit)*/
    constexpr auto const snvm_version = SUPERNOVA_VERSION_MAJOR << 48U | SUPERNOVA_VERSION_MINOR << 32U | SUPERNOVA_VERSION_PATCH;

    enum read_status : uint8_t
    {
        ReadOk,
        FileNotFound,
        InvalidHeader,
        InvalidEntryPoint,
        VersionMismatch,
        MagicMismatch,
        InvalidMemoryRegion,
        FileError
    };

    struct read_return
    {
        std::unique_ptr<uint8_t[]> memory_pointer{nullptr};
        uint64_t memory_size{0};
        read_status status{ReadOk};
        uint64_t entry_point{-1LLU};
        read_return() = default;
        explicit read_return(read_status stat, uint64_t mem_size = 0, uint64_t entry = 0, std::unique_ptr<uint8_t[]> memory = nullptr)
            : memory_pointer(std::move(memory)), memory_size{mem_size}, status{stat}, entry_point{entry} {}
    };

    auto read_file(arguments::arguments const& filename) -> read_return;
};