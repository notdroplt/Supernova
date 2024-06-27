#include "supernova.h"
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>

supernova::headers::read_return supernova::headers::read_file(char const *filename)
{
    auto file = std::ifstream(filename, std::ios::binary | std::ios::in | std::ios::ate);
    main_header main{};

    if (!file)
    {
        return {read_status::FileNotFound};
    }

    uint64_t size = file.tellg();
    file.seekg(0);

    if (size < sizeof(main_header))
    {
        return {read_status::InvalidHeader};
    }

    if (main.magic != headers::master_magic)
    {
        return {read_status::MagicMismatch};
    }

    constexpr auto patch_mask = 0xFFFFFFFF;

    // patch versions should not have worring issues
    if ((main.version | patch_mask) < (snvm_version | patch_mask))
    {
        return {read_status::VersionMismatch};
    }

    if (main.entry_point > main.memory_size)
    {
        return {read_status::InvalidEntryPoint};
    }

    // NOLINTNEXTLINE: there is not much to do
    file.read(reinterpret_cast<char *>(&main), sizeof(main));

    if (size < (sizeof(main_header) + sizeof(memory_map) * main.memory_regions))
    {
        return {read_status::InvalidHeader};
    }

    auto memory_maps = std::make_unique<memory_map[]>(main.memory_regions);

    // NOLINTNEXTLINE: there is also not much to do
    file.read(reinterpret_cast<char *>(memory_maps.get()), sizeof(memory_map) * main.memory_regions);

    // loop twice so we don't actually need to allocate on error
    // and also, main.memory_regions is not *that* big of a number
    // so it is fine
    for (size_t i = 0; i < main.memory_regions; ++i)
    {
        auto &region = memory_maps[i];
        if (region.magic != memmap_magic)
        {
            return {read_status::MagicMismatch};
        }

        if (region.offset + region.size > main.memory_size)
        {
            return {read_status::InvalidMemoryRegion};
        }

        if (region.start + region.size > size)
        {
            return {read_status::InvalidMemoryRegion};
        }
    }

    auto memory = std::unique_ptr<uint8_t[]>(new uint8_t[main.memory_size]);

    for (size_t i = 0; i < main.memory_regions; ++i)
    {
        auto &region = memory_maps[i];

        // comment sections, debug sections and such
        if ((region.flags & memory_flags::mem_exists) == 0)
        {
            continue;
        }

        if (region.flags & memory_flags::mem_clear)
        {
            std::memset(memory.get() + region.start, 0, region.size);
            continue;
        }

        file.seekg(region.start);

        // NOLINTNEXTLINE: you got the idea
        file.read(reinterpret_cast<char *>(memory.get() + region.offset), region.size);
    }

    return {
        ReadOk,
        main.memory_size,
        main.entry_point,
        std::move(memory),
    };
}