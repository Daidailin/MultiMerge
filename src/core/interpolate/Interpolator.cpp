#include "Interpolator.h"

#include <algorithm>
#include <stdexcept>

// 线性插值函数
double Interpolator::interpolate(double x0, double y0, double x1, double y1, double x) {
    if (x0 == x1) {
        return y0;
    }
    return y0 + (y1 - y0) * (x - x0) / (x1 - x0);
}

// 对数值类型进行插值
std::vector<double> Interpolator::interpolateValues(const std::vector<double>& times, const std::vector<double>& values, double targetTime, Method method) {
    if (times.empty() || values.empty() || times.size() != values.size()) {
        throw std::invalid_argument("Invalid input vectors");
    }

    std::vector<double> result;
    
    // 找到目标时间所在的区间
    auto it = std::lower_bound(times.begin(), times.end(), targetTime);
    
    if (it == times.begin()) {
        // 目标时间早于所有数据点
        for (size_t i = 0; i < values.size(); ++i) {
            result.push_back(values[i]);
        }
    } else if (it == times.end()) {
        // 目标时间晚于所有数据点
        for (size_t i = 0; i < values.size(); ++i) {
            result.push_back(values[values.size() - 1]);
        }
    } else {
        // 目标时间在数据点之间
        size_t index = it - times.begin();
        double t0 = times[index - 1];
        double t1 = times[index];
        
        for (size_t i = 0; i < values.size(); ++i) {
            double v0 = values[i * times.size() + index - 1];
            double v1 = values[i * times.size() + index];
            
            switch (method) {
                case Method::NEAREST:
                    result.push_back((targetTime - t0) < (t1 - targetTime) ? v0 : v1);
                    break;
                case Method::LINEAR:
                    result.push_back(interpolate(t0, v0, t1, v1, targetTime));
                    break;
                case Method::NONE:
                    result.push_back(0.0); // 或者其他默认值
                    break;
            }
        }
    }
    
    return result;
}

// 对字符串类型进行插值（主要用于最近邻）
std::vector<std::string> Interpolator::interpolateStrings(const std::vector<double>& times, const std::vector<std::string>& values, double targetTime, Method method) {
    if (times.empty() || values.empty() || times.size() != values.size()) {
        throw std::invalid_argument("Invalid input vectors");
    }

    std::vector<std::string> result;
    
    // 找到目标时间所在的区间
    auto it = std::lower_bound(times.begin(), times.end(), targetTime);
    
    if (it == times.begin()) {
        // 目标时间早于所有数据点
        for (size_t i = 0; i < values.size(); ++i) {
            result.push_back(values[i]);
        }
    } else if (it == times.end()) {
        // 目标时间晚于所有数据点
        for (size_t i = 0; i < values.size(); ++i) {
            result.push_back(values[values.size() - 1]);
        }
    } else {
        // 目标时间在数据点之间
        size_t index = it - times.begin();
        double t0 = times[index - 1];
        double t1 = times[index];
        
        for (size_t i = 0; i < values.size(); ++i) {
            std::string v0 = values[i * times.size() + index - 1];
            std::string v1 = values[i * times.size() + index];
            
            switch (method) {
                case Method::NEAREST:
                    result.push_back((targetTime - t0) < (t1 - targetTime) ? v0 : v1);
                    break;
                case Method::LINEAR:
                case Method::NONE:
                    result.push_back(v0); // 字符串不支持线性插值
                    break;
            }
        }
    }
    
    return result;
}