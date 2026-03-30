/****************************************************************************
** Meta object code from reading C++ file 'StreamMergeEngine.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../core/StreamMergeEngine.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'StreamMergeEngine.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_StreamMergeEngine_t {
    QByteArrayData data[25];
    char stringdata0[273];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_StreamMergeEngine_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_StreamMergeEngine_t qt_meta_stringdata_StreamMergeEngine = {
    {
QT_MOC_LITERAL(0, 0, 17), // "StreamMergeEngine"
QT_MOC_LITERAL(1, 18, 15), // "progressChanged"
QT_MOC_LITERAL(2, 34, 0), // ""
QT_MOC_LITERAL(3, 35, 7), // "percent"
QT_MOC_LITERAL(4, 43, 5), // "stage"
QT_MOC_LITERAL(5, 49, 13), // "statusMessage"
QT_MOC_LITERAL(6, 63, 7), // "message"
QT_MOC_LITERAL(7, 71, 15), // "warningOccurred"
QT_MOC_LITERAL(8, 87, 7), // "warning"
QT_MOC_LITERAL(9, 95, 13), // "errorOccurred"
QT_MOC_LITERAL(10, 109, 5), // "error"
QT_MOC_LITERAL(11, 115, 19), // "InterpolationMethod"
QT_MOC_LITERAL(12, 135, 7), // "Nearest"
QT_MOC_LITERAL(13, 143, 6), // "Linear"
QT_MOC_LITERAL(14, 150, 15), // "OutputDelimiter"
QT_MOC_LITERAL(15, 166, 5), // "Space"
QT_MOC_LITERAL(16, 172, 5), // "Comma"
QT_MOC_LITERAL(17, 178, 3), // "Tab"
QT_MOC_LITERAL(18, 182, 14), // "OutputEncoding"
QT_MOC_LITERAL(19, 197, 4), // "Utf8"
QT_MOC_LITERAL(20, 202, 3), // "Gbk"
QT_MOC_LITERAL(21, 206, 12), // "FileCategory"
QT_MOC_LITERAL(22, 219, 17), // "SequentialUniform"
QT_MOC_LITERAL(23, 237, 19), // "SequentialCorrected"
QT_MOC_LITERAL(24, 257, 15) // "IndexedFallback"

    },
    "StreamMergeEngine\0progressChanged\0\0"
    "percent\0stage\0statusMessage\0message\0"
    "warningOccurred\0warning\0errorOccurred\0"
    "error\0InterpolationMethod\0Nearest\0"
    "Linear\0OutputDelimiter\0Space\0Comma\0"
    "Tab\0OutputEncoding\0Utf8\0Gbk\0FileCategory\0"
    "SequentialUniform\0SequentialCorrected\0"
    "IndexedFallback"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_StreamMergeEngine[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       4,   48, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,   34,    2, 0x06 /* Public */,
       5,    1,   39,    2, 0x06 /* Public */,
       7,    1,   42,    2, 0x06 /* Public */,
       9,    1,   45,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int, QMetaType::QString,    3,    4,
    QMetaType::Void, QMetaType::QString,    6,
    QMetaType::Void, QMetaType::QString,    8,
    QMetaType::Void, QMetaType::QString,   10,

 // enums: name, alias, flags, count, data
      11,   11, 0x2,    2,   68,
      14,   14, 0x2,    3,   72,
      18,   18, 0x2,    2,   78,
      21,   21, 0x2,    3,   82,

 // enum data: key, value
      12, uint(StreamMergeEngine::InterpolationMethod::Nearest),
      13, uint(StreamMergeEngine::InterpolationMethod::Linear),
      15, uint(StreamMergeEngine::OutputDelimiter::Space),
      16, uint(StreamMergeEngine::OutputDelimiter::Comma),
      17, uint(StreamMergeEngine::OutputDelimiter::Tab),
      19, uint(StreamMergeEngine::OutputEncoding::Utf8),
      20, uint(StreamMergeEngine::OutputEncoding::Gbk),
      22, uint(StreamMergeEngine::FileCategory::SequentialUniform),
      23, uint(StreamMergeEngine::FileCategory::SequentialCorrected),
      24, uint(StreamMergeEngine::FileCategory::IndexedFallback),

       0        // eod
};

void StreamMergeEngine::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<StreamMergeEngine *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->progressChanged((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 1: _t->statusMessage((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->warningOccurred((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 3: _t->errorOccurred((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (StreamMergeEngine::*)(int , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&StreamMergeEngine::progressChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (StreamMergeEngine::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&StreamMergeEngine::statusMessage)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (StreamMergeEngine::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&StreamMergeEngine::warningOccurred)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (StreamMergeEngine::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&StreamMergeEngine::errorOccurred)) {
                *result = 3;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject StreamMergeEngine::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_StreamMergeEngine.data,
    qt_meta_data_StreamMergeEngine,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *StreamMergeEngine::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *StreamMergeEngine::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_StreamMergeEngine.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int StreamMergeEngine::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void StreamMergeEngine::progressChanged(int _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void StreamMergeEngine::statusMessage(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void StreamMergeEngine::warningOccurred(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void StreamMergeEngine::errorOccurred(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
