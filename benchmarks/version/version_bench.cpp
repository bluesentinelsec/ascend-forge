/**
 * @file version_bench.cpp
 * @brief Microbenchmarks for ascend_forge::Version.
 */

#include "ascend_forge/version.hpp"

#include <benchmark/benchmark.h>
#include <string_view>

namespace
{

void BM_Version(benchmark::State &state)
{
    for (auto _ : state)
    {
        // Mutable lvalue required: const-ref DoNotOptimize is deprecated under -Werror.
        std::string_view version = ascend_forge::Version();
        benchmark::DoNotOptimize(version);
    }
}
BENCHMARK(BM_Version);

} // namespace
