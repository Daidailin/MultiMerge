#ifndef TIMEPARSER_H
#define TIMEPARSER_H

#include "TimePoint.h"
#include <QString>
#include <QVector>
#include <memory>
#include <functional>

namespace TimeMerge {

enum class ParseErrorCode {
    Success = 0,         //< 解析成功
    InvalidFormat,       //< 格式无效（不匹配任何已注册的格式）
    InvalidHour,         //< 小时值无效（非数字或格式错误）
    InvalidMinute,       //< 分钟值无效
    InvalidSecond,       //< 秒值无效
    InvalidFraction,     //< 小数部分无效
    OutOfRange,          //< 数值超出范围（如小时>23）
    EmptyInput,          //< 输入为空字符串
    UnknownError         //< 未知错误
};

class ParseResult {
public:

    ParseResult() : m_errorCode(ParseErrorCode::Success) {}
    

    ParseResult(const TimePoint& timePoint, ParseErrorCode code = ParseErrorCode::Success)
        : m_timePoint(timePoint), m_errorCode(code) {}
    

    ParseResult(ParseErrorCode code, const QString& message = QString())
        : m_errorCode(code), m_errorMessage(message) {}
    

    bool isSuccess() const { return m_errorCode == ParseErrorCode::Success; }
    

    const TimePoint& timePoint() const { return m_timePoint; }
    

    ParseErrorCode errorCode() const { return m_errorCode; }
    

    const QString& errorMessage() const { return m_errorMessage; }
    

    static QString errorCodeToString(ParseErrorCode code);
    
private:
    TimePoint m_timePoint;
    ParseErrorCode m_errorCode;
    QString m_errorMessage;
};


struct TimeFormatDescriptor {
    QString formatName;
    QString pattern;
    std::shared_ptr<std::function<ParseResult(const QString&)>> parserFunc;  ///< 解析函数
    

    TimeFormatDescriptor() : parserFunc(nullptr) {}
    

    TimeFormatDescriptor(const QString& name, const QString& pattern,
                        std::function<ParseResult(const QString&)> parser)
        : formatName(name), pattern(pattern), 
          parserFunc(std::make_shared<std::function<ParseResult(const QString&)>>(parser)) {}
    

    ParseResult parse(const QString& input) const {
        if (parserFunc) {
            return (*parserFunc)(input);
        }
        return ParseResult(ParseErrorCode::InvalidFormat, "No parser available");
    }
};


class TimeParser {
private:
    TimeParser();
    
    void registerBuiltInFormats();

    ParseResult parseColonFractionalTime(const QString& str, TimeResolution resolution) const;

    ParseResult parseDotFractionalTime(const QString& str) const;

    int m_builtinFormatCount;
    QVector<TimeFormatDescriptor> m_formats;

public:

    static TimeParser& instance();
    

    ParseResult parse(const QString& timeStr) const;
    

    void registerFormat(const TimeFormatDescriptor& format);
    

    void clearFormats();
    

    int registeredFormatCount() const { return m_formats.size(); }
    

    int builtinFormatCount() const { return m_builtinFormatCount; }
    

    const QVector<TimeFormatDescriptor>& registeredFormats() const { return m_formats; }
};

} // namespace TimeMerge

#endif // TIMEPARSER_H
