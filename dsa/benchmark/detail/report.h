/**
 * @file report.h
 * @brief 报告生成模块
 * @version 1.0.0
 */

#pragma once

#include <algorithm>
#include <fstream>
#include <iostream>
#include <print>
#include <string>
#include <vector>

#include "statistics.h"

namespace benchmark {

class Reporter {
public:
    static void print_table(const std::vector<Statistics>& results) {
        if (results.empty()) {
            return;
        }

        std::size_t width = 10;
        for (const auto& s : results) {
            width = std::max(width, s.name_.length() + 2);
        }

        std::println("{:<{}} {:>12} {:>12} {:>12} {:>12}", "Benchmark", width, "Time (ns)", "CPU (ns)",
                     "Iterations", "Ops/s");
        std::println("{}", std::string(width + 48, '-'));

        for (const auto& s : results) {
            std::println("{:<{}} {:>12} {:>12} {:>12} {:>12}", s.name_, width, format_time(s.mean_),
                         format_time(s.mean_per_iter_), s.iterations_, format_throughput(s.ops_per_second()));
        }
    }

    static std::string to_json(const std::vector<Statistics>& results) {
        std::string out = "{\n  \"benchmarks\": [\n";
        for (std::size_t i = 0; i < results.size(); ++i) {
            const auto& s = results[i];
            // clang-format off
            out += std::format(R"(    
    {{
        "name": "{}",
        "iterations": {},
        "mean": {:.2f}, "stddev": {:.2f}, "min": {:.2f}, "max": {:.2f},
        "p50": {:.2f}, "p95": {:.2f}, "p99": {:.2f}, "p999": {:.2f},
        "ops_per_second": {:.2f}, "threads": {}
    }})",
            s.name_, s.iterations_, s.mean_, s.stddev_, s.min_, 
            s.max_, s.p50_, s.p95_,s.p99_, s.p999_,s.ops_per_second(), s.threads_);
            // clang-format on
            if (i < results.size() - 1) {
                out += ",";
            }
            out += "\n";
        }
        return out + "  ]\n}\n";
    }

    static std::string to_csv(const std::vector<Statistics>& results) {
        std::string out =
            "name,iterations,mean_ns,stddev_ns,min_ns,max_ns,p50_ns,p95_ns,p99_ns,p999_ns,ops_per_second,"
            "threads\n";
        for (const auto& s : results) {
            out += std::format("{},{},{:.2f},{:.2f},{:.2f},{:.2f},{:.2f},{:.2f},{:.2f},{:.2f},{:.2f},{}\n",
                               s.name_, s.iterations_, s.mean_, s.stddev_, s.min_, s.max_, s.p50_, s.p95_,
                               s.p99_, s.p999_, s.ops_per_second(), s.threads_);
        }
        return out;
    }

    static bool save_to_file(const std::string& filename, const std::string& content) {
        std::ofstream file(filename);
        if (!file) {
            std::println(std::cerr, "Error: Cannot open file {}", filename);
            return false;
        }
        file << content;
        return true;
    }

    static void print_comparison(const std::vector<Statistics>& baseline,
                                 const std::vector<Statistics>& current) {
        if (baseline.size() != current.size()) {
            return;
        }

        for (std::size_t i = 0; i < baseline.size(); ++i) {
            double change = compare_statistics(baseline[i], current[i]);
            std::println("{}: {:.2f} -> {:.2f} ns/iter ({:+.1f}%)", baseline[i].name_,
                         baseline[i].mean_per_iter_, current[i].mean_per_iter_, change);
        }
    }

private:
    static std::string format_time(double ns) {
        if (ns < 1e3) {
            return std::format("{:.2f} ns", ns);
        }

        if (ns < 1e6) {
            return std::format("{:.2f} us", ns / 1e3);
        }

        if (ns < 1e9) {
            return std::format("{:.2f} ms", ns / 1e6);
        }

        return std::format("{:.2f} s", ns / 1e9);
    }

    static std::string format_throughput(double ops) {
        if (ops < 1e3) {
            return std::format("{:.0f}", ops);
        }

        if (ops < 1e6) {
            return std::format("{:.1f}K", ops / 1e3);
        }

        if (ops < 1e9) {
            return std::format("{:.1f}M", ops / 1e6);
        }

        return std::format("{:.2f}G", ops / 1e9);
    }
};

}  // namespace benchmark
