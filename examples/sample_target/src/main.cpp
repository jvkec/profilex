#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {

std::size_t parse_iterations(int argc, char** argv) {
    if (argc < 2) {
        return 400000;
    }

    const std::string arg = argv[1];
    const std::size_t parsed = static_cast<std::size_t>(std::stoull(arg));
    if (parsed == 0U) {
        throw std::invalid_argument("iterations must be greater than zero");
    }
    return parsed;
}

double do_work(const std::size_t iterations) {
    volatile double accumulator = 0.0;
    for (std::size_t i = 1; i <= iterations; ++i) {
        const double x = static_cast<double>(i) * 0.001;
        accumulator += std::sqrt(x) * std::sin(x);
    }
    return accumulator;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const std::size_t iterations = parse_iterations(argc, argv);
        const auto start = std::chrono::steady_clock::now();
        const double value = do_work(iterations);
        const auto end = std::chrono::steady_clock::now();

        const std::chrono::duration<double, std::milli> elapsed = end - start;
        std::cout << "sample_bench iterations=" << iterations << " value=" << value
                  << " elapsed_ms=" << elapsed.count() << '\n';
        return EXIT_SUCCESS;
    } catch (const std::exception& ex) {
        std::cerr << "sample_bench error: " << ex.what() << '\n';
        return EXIT_FAILURE;
    }
}
