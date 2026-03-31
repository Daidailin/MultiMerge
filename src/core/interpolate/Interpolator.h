#ifndef INTERPOLATOR_H
#define INTERPOLATOR_H

#include <QString>
#include <QVector>

class Interpolator {
public:
    enum InterpolationType {
        NEAREST_NEIGHBOR,
        LINEAR
    };

    // 插值方法
    static double interpolate(const QVector<double>& x, const QVector<double>& y, double xTarget, InterpolationType type);
    
    // 最近邻插值
    static double nearestNeighbor(const QVector<double>& x, const QVector<double>& y, double xTarget);
    
    // 线性插值
    static double linearInterpolation(const QVector<double>& x, const QVector<double>& y, double xTarget);
    
    // 将字符串转换为数值
    static double stringToDouble(const QString& str);
    
    // 将数值转换为字符串
    static QString doubleToString(double value);
};

#endif // INTERPOLATOR_H