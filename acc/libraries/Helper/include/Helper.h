#pragma once

#include <cstdint>

// wrapper functions for ms timestamp retrieval, the functions help obtaining the 
// timestamp field in the messages
namespace acc
{
[[ nodiscard ]] uint64_t getTimestampMs(void);                              // gets current timestamp in units of milliseconds
[[ nodiscard ]] uint32_t getTimestampMsSinceBaseline(uint64_t baselineMs);  // gets elapsed milliseconds since a time baseline
}