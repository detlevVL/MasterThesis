#ifndef NAMINGANALYSIS_H
#define NAMINGANALYSIS_H

#include <QList>
#include <QMultiHash>
#include <QString>

class NamingAnalysis
{
public:
    NamingAnalysis();
    QMultiHash<QString, QString> analyse(QList<QString> sources, QList<QString> tests, QList<QString> modifiedSources, QList<QString> modifiedTests);

};

#endif // NAMINGANALYSIS_H
