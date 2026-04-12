/**
 * @file statistics.h
 * @brief 统计分析模块
 * @version 1.0.0
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "core.h"

namespace benchmark {

struct Statistics {
    std::string name_;
    std::size_t iterations_{0}, repetitions_{0}, threads_{1};
    NanoSeconds total_time_{0};

    double mean_{0}, variance_{0}, stddev_{0}, min_{0}, max_{0};
    double p25_{0}, p50_{0}, p75_{0}, p90_{0}, p95_{0}, p99_{0}, p999_{0};
    double mean_per_iter_{0}, stddev_per_iter_{0};

    double rsd() const noexcept { return mean_ > 0 ? stddev_ / mean_ * 100.0 : 0.0; }
    double ops_per_second() const noexcept { return mean_per_iter_ > 0 ? 1e9 / mean_per_iter_ : 0.0; }
    double mops() const noexcept { return ops_per_second() / 1e6; }
};

class Statistics_Analyzer {
public:
    void add_sample(NanoSeconds time_ns, IterationCount iters) {
        samples_.push_back({time_ns, iters});
        total_iterations_ += iters;
        total_time_ += time_ns;
    }

    Statistics compute(const std::string& name, std::size_t threads = 1) {
        Statistics s;
        s.name_ = name;
        s.iterations_ = total_iterations_;
        s.repetitions_ = samples_.size();
        s.total_time_ = total_time_;
        s.threads_ = threads;

        if (samples_.empty()) {
            return s;
        }

        std::vector<double> times;
        times.reserve(samples_.size());
        for (const auto& [t, n] : samples_) {
            times.push_back(static_cast<double>(t) / n);
        }

        std::sort(times.begin(), times.end());

        s.min_ = times.front();
        s.max_ = times.back();
        s.mean_ = compute_mean(times);
        s.variance_ = compute_variance(times, s.mean_);
        s.stddev_ = std::sqrt(s.variance_);

        s.p25_ = percentile(times, 25);
        s.p50_ = percentile(times, 50);
        s.p75_ = percentile(times, 75);
        s.p90_ = percentile(times, 90);
        s.p95_ = percentile(times, 95);
        s.p99_ = percentile(times, 99);
        s.p999_ = percentile(times, 99.9);

        s.mean_per_iter_ = s.mean_;
        s.stddev_per_iter_ = s.stddev_;

        return s;
    }

    void clear() noexcept {
        samples_.clear();
        total_iterations_ = total_time_ = 0;
    }

private:
    struct Sample {
        NanoSeconds time_ns_;
        IterationCount iterations_;
    };
    std::vector<Sample> samples_;
    IterationCount total_iterations_{0};
    NanoSeconds total_time_{0};

    static double compute_mean(const std::vector<double>& v) {
        if (v.empty()) {
            return 0;
        }

        double sum = 0;
        for (double x : v) {
            sum += x;
        }
        return sum / v.size();
    }

    static double compute_variance(const std::vector<double>& v, double mean) {
        if (v.size() < 2) {
            return 0;
        }

        double sum = 0;
        for (double x : v) {
            sum += (x - mean) * (x - mean);
        }
        return sum / (v.size() - 1);
    }

    static double percentile(std::vector<double> sorted, double p) {
        if (sorted.empty()) {
            return 0;
        }

        double idx = p / 100.0 * (sorted.size() - 1);
        auto lower = static_cast<std::size_t>(idx);
        auto upper = static_cast<std::size_t>(std::ceil(idx));
        if (lower == upper || upper >= sorted.size()) {
            return sorted[lower];
        }
        double frac = idx - lower;
        return sorted[lower] * (1 - frac) + sorted[upper] * frac;
    }
};

inline double compare_statistics(const Statistics& base, const Statistics& cur) noexcept {
    return base.mean_per_iter_ > 0 ? (base.mean_per_iter_ - cur.mean_per_iter_) / base.mean_per_iter_ * 100
                                   : 0;
}

}  // namespace benchmark
