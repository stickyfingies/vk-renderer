#pragma once

#include <cstddef>
#include <cstdint>

constexpr inline uint64_t string_hash(const char * input)
{
    return
		(*input)
		? static_cast<uint64_t>(*input) + 33 * string_hash(input + 1)
		: 5381;
}

constexpr inline uint64_t operator "" _h(const char * string, size_t)
{
    return string_hash(string);
}
