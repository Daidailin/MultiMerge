#include "Interpolator.h"
#include <cmath>

using namespace std;

double Interpolator::interpolate(const QVector<double>& x, const QVector<double>& y, double xTarget, InterpolationType type) {
    switch (type) {
    case NEAREST_NEIGHBOR:
        return nearestNeighbor(x, y, xTarget);
    case LINEAR:
        return linearInterpolation(x, y, xTarget);
    default:
        return 0.0;
    }
}

double Interpolator::nearestNeighbor(const QVector<double>& x, const QVector<double>& y, double xTarget) {
    if (x.isEmpty() || y.isEmpty() || x.size() != y.size()) {
        return 0.0;
    }

    int nearestIndex = 0;
    double minDistance = abs(x[0] - xTarget);

    for (int i = 1; i < x.size(); ++i) {
        double distance = abs(x[i] - xTarget);
        if (distance < minDistance) {
            minDistance = distance;
            nearestIndex = i;
        }
    }

    return y[nearestIndex];
}

double Interpolator::linearInterpolation(const QVector<double>& x, const QVector<double>& y, double xTarget) {
    if (x.isEmpty() || y.isEmpty() || x.size() != y.size()) {
        return 0.0;
    }

    // 找到xTarget所在的区间
    int i = 0;
    while (i < x.size() - 1 && x[i] < xTarget) {
        ++i;
    }

    // 处理边界情况
    if (i == 0) {
        return y[0];
    }
    if (i == x.size()) {
        return y.last();
    }

    // 线性插值
    double x1 = x[i-1], y1 = y[i-1];
    double x2 = x[i], y2 = y[i];
    return y1 + (y2 - y1) * (xTarget - x1) / (x2 - x1);
}

double Interpolator::stringToDouble(const QString& str) {
    bool ok;
    double value = str.toDouble(&ok);
    return ok ? value : 0.0;
}

QString Interpolator::doubleToString(double value) {
    return QString::number(value, 'g', 10);
}