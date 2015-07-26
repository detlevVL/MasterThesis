#include "naminganalysis.h"

NamingAnalysis::NamingAnalysis()
{

}

QMultiHash<QString, QString> NamingAnalysis::analyse(QList<QString> sources, QList<QString> tests, QList<QString> modifiedSources, QList<QString> modifiedTests)
{
    QMultiHash<QString, QString> links;

    // for each modified source file basename (i.e. "/home/somewhere/something.qml" > "something.qml")
    foreach (const QString &source, modifiedSources) {
        QString possibleTest = QString::fromStdString("tst_").append(source.section(QString::fromStdString("/"), -1));

        // check if there is a "tst_something.qml" test file
        foreach (const QString &test, tests) {
            if (test.section(QString::fromStdString("/"),-1).compare(possibleTest, Qt::CaseInsensitive) == 0) {
                links.insert(source, test);
            }
        }
    }

    // for each modified test file basename (i.e. "/home/somewhere/tst_something.qml" > "tst_something.qml")
    foreach (const QString &test, modifiedTests) {
        QString possibleSource = test.section(QString::fromStdString("_"), 1);

        // check if there is a "something.qml" source file
        foreach (const QString &source, sources) {
            if (source.section(QString::fromStdString("/"),-1).compare(possibleSource, Qt::CaseInsensitive) == 0) {
                links.insert(source, test);
            }
        }
    }

    return links;
}

