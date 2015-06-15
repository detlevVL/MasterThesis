#ifndef EVENTINFO
#define EVENTINFO

#endif // EVENTINFO

#include <QString>

struct eventInfo {
    qint64 startTime;
    qint64 length;
    QString data;
    QString filename;
    qint64 line;
    qint64 column;

    eventInfo(qint64 startTime, qint64 length, QString data,
              QString file, int lineNumber, int columnNumber) :
                startTime(startTime), length(length), data(data),
                filename(file), line(lineNumber), column(columnNumber) {}

    bool operator<(const eventInfo& other) const {
        return startTime < other.startTime; // sort by startTime
    }
};
