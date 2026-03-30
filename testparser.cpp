#include "TimePoint.h"
#include "TimeParser.h"
#include <QCoreApplication>
#include <QDebug>
#include <iostream>

using namespace TimeMerge;

void testTimePoint() {
    std::wcout << L"\n=== Testing TimePoint ===" << std::endl;
    
    std::wcout << L"Creating TimePoint(12, 30, 45)..." << std::endl;
    TimePoint tp1(12, 30, 45);
    std::wcout << L"  Hour: " << tp1.hour() << L", Minute: " << tp1.minute() << L", Second: " << tp1.second() << std::endl;
    
    std::wcout << L"Testing toString()..." << std::endl;
    QString str = tp1.toString();
    std::wcout << L"  toString() returned: " << str.toStdWString() << std::endl;
    
    std::wcout << L"Testing toMicroseconds()..." << std::endl;
    int64_t us = tp1.toMicroseconds();
    std::wcout << L"  toMicroseconds: " << us << std::endl;
    
    std::wcout << L"Creating TimePoint with microseconds..." << std::endl;
    TimePoint tp2(12, 30, 45, 123456, TimeResolution::Microseconds);
    std::wcout << L"  Fraction: " << tp2.fraction() << std::endl;
    std::wcout << L"  toString: " << tp2.toString().toStdWString() << std::endl;
    
    std::wcout << L"TimePoint test completed." << std::endl;
}

void testParser() {
    std::wcout << L"\n=== Testing TimeParser ===" << std::endl;
    
    std::wcout << L"Getting parser instance..." << std::endl;
    TimeParser& parser = TimeParser::instance();
    std::wcout << L"Parser instance OK. Format count: " << parser.builtinFormatCount() << std::endl;
    
    std::vector<QString> testCases = {
        "12:30:45",
        "12:30:45.123",
        "12:30:45.123456",
        "00:00:00",
        "23:59:59",
        "invalid"
    };
    
    int passCount = 0;
    int failCount = 0;
    
    for (const auto& testCase : testCases) {
        std::wcout << L"\nTesting: " << testCase.toStdWString() << std::endl;
        
        ParseResult result = parser.parse(testCase);
        
        if (result.isSuccess()) {
            std::wcout << L"  PASS: " << result.timePoint().toString().toStdWString() << std::endl;
            passCount++;
        } else {
            std::wcout << L"  FAIL: " << ParseResult::errorCodeToString(result.errorCode()).toStdWString() << std::endl;
            failCount++;
        }
    }
    
    std::wcout << L"\nParser test completed." << std::endl;
    std::wcout << L"Results: " << passCount << L" passed, " << failCount << L" failed" << std::endl;
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    std::wcout << L"========================================" << std::endl;
    std::wcout << L"Time Parser Test - Qt Project" << std::endl;
    std::wcout << L"========================================" << std::endl;
    
    try {
        testTimePoint();
        testParser();
        
        std::wcout << L"\n========================================" << std::endl;
        std::wcout << L"All tests completed successfully!" << std::endl;
        std::wcout << L"========================================" << std::endl;
    } catch (const std::exception& e) {
        std::wcout << L"\nFATAL EXCEPTION: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::wcout << L"\nFATAL UNKNOWN EXCEPTION" << std::endl;
        return 1;
    }
    
    return 0;
}
