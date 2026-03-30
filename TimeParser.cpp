#include <QDebug>
#include "TimeParser.h"
#include <QRegularExpression>

namespace TimeMerge {

QString ParseResult::errorCodeToString(ParseErrorCode code) {
    switch (code) {
        case ParseErrorCode::Success:
            return "Success";
        case ParseErrorCode::InvalidFormat:
            return "Invalid format";
        case ParseErrorCode::InvalidHour:
            return "Invalid hour value";
        case ParseErrorCode::InvalidMinute:
            return "Invalid minute value";
        case ParseErrorCode::InvalidSecond:
            return "Invalid second value";
        case ParseErrorCode::InvalidFraction:
            return "Invalid fraction value";
        case ParseErrorCode::OutOfRange:
            return "Value out of range";
        case ParseErrorCode::EmptyInput:
            return "Empty input";
        case ParseErrorCode::UnknownError:
            return "Unknown error";
        default:
            return "Unknown error code";
    }
}

ParseResult TimeParser::parseColonFractionalTime(const QString& str, TimeResolution resolution) const {
    QRegularExpression re(R"(^\s*(\d{1,2}):(\d{1,2}):(\d{1,2}):(\d{1,6})\s*$)");
    QRegularExpressionMatch match = re.match(str);

    if (!match.hasMatch()) {
        return ParseResult(ParseErrorCode::InvalidFormat, "Does not match colon fractional time format");
    }

    int hour = match.captured(1).toInt();
    int minute = match.captured(2).toInt();
    int second = match.captured(3).toInt();
    QString fractionStr = match.captured(4);

    if (hour < 0 || hour > 23) {
        return ParseResult(ParseErrorCode::InvalidHour, "Hour must be 0-23");
    }
    if (minute < 0 || minute > 59) {
        return ParseResult(ParseErrorCode::InvalidMinute, "Minute must be 0-59");
    }
    if (second < 0 || second > 59) {
        return ParseResult(ParseErrorCode::InvalidSecond, "Second must be 0-59");
    }

    int digits = fractionStr.length();
    TimeResolution detectedResolution;

    if (digits <= 3) {
        detectedResolution = TimeResolution::Milliseconds;
    } else {
        detectedResolution = TimeResolution::Microseconds;
    }

    int64_t fraction = fractionStr.toLongLong();

    return ParseResult(TimePoint(hour, minute, second, fraction, detectedResolution));
}

ParseResult TimeParser::parseDotFractionalTime(const QString& str) const {
    QRegularExpression re(R"(^\s*(\d{1,2}):(\d{1,2}):(\d{1,2})\.(\d{1,6})\s*$)");
    QRegularExpressionMatch match = re.match(str);

    if (!match.hasMatch()) {
        return ParseResult(ParseErrorCode::InvalidFormat, "Does not match dot fractional time format");
    }

    int hour = match.captured(1).toInt();
    int minute = match.captured(2).toInt();
    int second = match.captured(3).toInt();
    QString fractionStr = match.captured(4);

    if (hour < 0 || hour > 23) {
        return ParseResult(ParseErrorCode::InvalidHour, "Hour must be 0-23");
    }
    if (minute < 0 || minute > 59) {
        return ParseResult(ParseErrorCode::InvalidMinute, "Minute must be 0-59");
    }
    if (second < 0 || second > 59) {
        return ParseResult(ParseErrorCode::InvalidSecond, "Second must be 0-59");
    }

    int digits = fractionStr.length();
    TimeResolution detectedResolution;
    int64_t fraction;

    if (digits <= 3) {
        detectedResolution = TimeResolution::Milliseconds;
        fraction = fractionStr.toLongLong();
    } else {
        detectedResolution = TimeResolution::Microseconds;
        fraction = fractionStr.toLongLong();
    }

    return ParseResult(TimePoint(hour, minute, second, fraction, detectedResolution));
}

void TimeParser::registerBuiltInFormats() {
    m_formats.clear();

    auto colonMsParser = [this](const QString& str) {
        return parseColonFractionalTime(str, TimeResolution::Milliseconds);
    };
    m_formats.append(TimeFormatDescriptor("ColonMilliseconds", R"(^\d{1,2}:\d{1,2}:\d{1,2}:\d{1,3}$)", colonMsParser));

    auto colonUsParser = [this](const QString& str) {
        return parseColonFractionalTime(str, TimeResolution::Microseconds);
    };
    m_formats.append(TimeFormatDescriptor("ColonMicroseconds", R"(^\d{1,2}:\d{1,2}:\d{1,2}:\d{1,6}$)", colonUsParser));

    auto dotMsParser = [this](const QString& str) {
        return parseDotFractionalTime(str);
    };
    m_formats.append(TimeFormatDescriptor("DotMilliseconds", R"(^\d{1,2}:\d{1,2}:\d{1,2}\.\d{1,3}$)", dotMsParser));

    auto dotUsParser = [this](const QString& str) {
        return parseDotFractionalTime(str);
    };
    m_formats.append(TimeFormatDescriptor("DotMicroseconds", R"(^\d{1,2}:\d{1,2}:\d{1,2}\.\d{1,6}$)", dotUsParser));

    m_builtinFormatCount = m_formats.size();
}

TimeParser& TimeParser::instance() {
    static TimeParser parser;  // 静态局部变量，线程安全
    return parser;
}

TimeParser::TimeParser() : m_builtinFormatCount(0) {
    registerBuiltInFormats();
}

ParseResult TimeParser::parse(const QString& timeStr) const {
    // 检查空输入
    if (timeStr.isEmpty() || timeStr.trimmed().isEmpty()) {
        return ParseResult(ParseErrorCode::EmptyInput, "Input string is empty");
    }
    
    // 遍历所有已注册的格式
    for (const auto& format : m_formats) {
        QRegularExpression re(format.pattern);
        if (re.match(timeStr).hasMatch()) {
            return format.parse(timeStr);  // 匹配成功，调用解析函数
        }
    }
    
    // 所有格式都不匹配
    return ParseResult(ParseErrorCode::InvalidFormat, 
                       "No matching format found for: " + timeStr);
}

void TimeParser::registerFormat(const TimeFormatDescriptor& format) {
    m_formats.append(format);
}

void TimeParser::clearFormats() {
    m_formats.clear();
    m_builtinFormatCount = 0;
}

} // namespace TimeMerge
