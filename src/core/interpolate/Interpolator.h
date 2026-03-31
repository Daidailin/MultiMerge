#ifndef INTERPOLATOR_H
#define INTERPOLATOR_H

#include <vector>
#include <string>

class Interpolator {
public:
    enum class Method {
        NEAREST,  // 最近邻
        LINEAR,   // 线性插值
        NONE      // 不插值
    };

    static double interpolate(double x0, double y0, double x1, double y1, double x);
    static std::vector<double> interpolateValues(const std::vector<double>& times, const std::vector<double>& values, double targetTime, Method method);
    static std::vector<std::string> interpolateStrings(const std::vector<double>& times, const std::vector<std::string>& values, double targetTime, Method method);
};

#endif // INTERPOLATOR_H