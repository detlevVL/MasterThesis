#include "projectchanges.h"

#include <QDir>

ProjectChanges::ProjectChanges(QString projectPath, QObject *parent) : QObject(parent)
{
    this->projectPath = projectPath;
    connect(ModelManagerInterface::instance(), SIGNAL(documentChangedOnDisk(QmlJS::Document::Ptr)), this, SLOT(fileModified(QmlJS::Document::Ptr)));
    connect(ModelManagerInterface::instance(), SIGNAL(aboutToRemoveFiles(QStringList)), this, SLOT(fileDeleted(QStringList)));

    QFileInfoList fileInfoList = QDir(projectPath).entryInfoList(QStringList(QString::fromStdString("*.qml")),
                                                                 QDir::NoDotAndDotDot | QDir::Files | QDir::AllDirs,
                                                                 QDir::DirsLast);
    scanProject(fileInfoList);
    loadDeletedFiles();
}

void ProjectChanges::fileModified(Document::Ptr doc)
{
    if (isInProject(doc->fileName())) {
        if (isSource(doc->fileName())) {
            sourceFiles.insert(doc->fileName());
        } else {
            testFiles.insert(doc->fileName());
        }
    }
}

void ProjectChanges::fileDeleted(const QStringList &files)
{
    foreach (const QString &file, files) {
        if (isInProject(file)) {
            if (isSource(file)) {
                sourceFiles.remove(file);
                deletedSourceFiles.insert(file, QDateTime::currentDateTime());
            } else {
                testFiles.remove(file);
                deletedTestFiles.insert(file, QDateTime::currentDateTime());
            }
        }
    }
}

QList<QString> ProjectChanges::getDeletedFiles(QDateTime since) const
{
    QList<QString> result;
    result.append(getDeletedSourceFiles(since));
    result.append(getDeletedTestFiles(since));
    return result;
}

QList<QString> ProjectChanges::getDeletedSourceFiles(QDateTime since) const
{
    QList<QString> result;
    foreach(const QString &file, deletedSourceFiles.keys()) {
        if (since < deletedSourceFiles.value(file)) {
            result.append(file);
        }
    }
    return result;
}

QList<QString> ProjectChanges::getDeletedTestFiles(QDateTime since) const
{
    QList<QString> result;
    foreach(const QString &file, deletedTestFiles.keys()) {
        if (since < deletedTestFiles.value(file)) {
            result.append(file);
        }
    }
    return result;
}

QList<QString> ProjectChanges::getFiles() const
{
    QList<QString> result;
    result.append(getSourceFiles());
    result.append(getTestFiles());
    return result;
}

QList<QString> ProjectChanges::getSourceFiles() const
{
    return sourceFiles.toList();
}

QList<QString> ProjectChanges::getTestFiles() const
{
    return testFiles.toList();
}

QList<QString> ProjectChanges::getModifiedFiles(QDateTime since) const
{
    QList<QString> result;
    result.append(getModifiedSourceFiles(since));
    result.append(getModifiedTestFiles(since));
    return result;
}

QList<QString> ProjectChanges::getModifiedSourceFiles(QDateTime since) const
{
    QList<QString> result;
    foreach (const QString &file, sourceFiles) {
        if (since < QFileInfo(file).lastModified()) {
            result.append(file);
        }
    }
    return result;
}

QList<QString> ProjectChanges::getModifiedTestFiles(QDateTime since) const
{
    QList<QString> result;
    foreach (const QString &file, testFiles) {
        if (since < QFileInfo(file).lastModified()) {
            result.append(file);
        }
    }
    return result;
}

void ProjectChanges::loadDeletedFiles()
{
    QFile changesFile(projectPath + QString::fromStdString("/MHchanges.txt"));

    if (changesFile.exists()) {
        changesFile.open(QFile::ReadOnly | QFile::Text);
        QTextStream in(&changesFile);

        QString line = in.readLine();
        while (!line.isNull()) {
            QString type = line.section(QString::fromStdString(":"), 0, 0);
            QString files = line.section(QString::fromStdString(":"), 1, -1);

            if (type == QString::fromStdString("source")) {
                foreach (const QString &fileAndDate, files.split(QString::fromStdString(";"), QString::SkipEmptyParts)) {
                    QString file = fileAndDate.section(QString::fromStdString(","), 0, 0);
                    QString date = fileAndDate.section(QString::fromStdString(","), 1, 1);
                    deletedSourceFiles.insert(file, QDateTime::fromString(date));
                }
            } else if (type == QString::fromStdString("test")) {
                foreach (const QString &fileAndDate, files.split(QString::fromStdString(";"), QString::SkipEmptyParts)) {
                    QString file = fileAndDate.section(QString::fromStdString(","), 0, 0);
                    QString date = fileAndDate.section(QString::fromStdString(","), 1, 1);
                    deletedTestFiles.insert(file, QDateTime::fromString(date));
                }
            }

            line = in.readLine();
        }
    }
}

bool ProjectChanges::isInProject(QString fileName)
{
    return (fileName.contains(projectPath));
}

bool ProjectChanges::isSource(QString fileName)
{
    return !QFileInfo(fileName).baseName().startsWith(QString::fromStdString("tst_"));
}

void ProjectChanges::saveDeletedFiles()
{
    QFile changesFile(projectPath + QString::fromStdString("/MHchanges.txt"));
    changesFile.open(QFile::ReadWrite | QFile::Truncate | QFile::Text);
    QTextStream out(&changesFile);

    out << "source:";
    foreach (const QString &file, deletedSourceFiles.keys()) {
        out << file << "," << deletedSourceFiles.value(file).toString() << ";";
    }
    out << endl;

    out << "test:";
    foreach (const QString &file, deletedTestFiles.keys()) {
        out << file << "," << deletedTestFiles.value(file).toString() << ";";
    }
    out << endl;

    changesFile.close();
}

void ProjectChanges::scanProject(const QFileInfoList &fileInfoList)
{
    foreach (const QFileInfo &fileInfo, fileInfoList) {
        if (fileInfo.isDir()) {
            // scan subdirectory
            QDir subDir = QDir(fileInfo.absoluteFilePath());
            QFileInfoList subFileInfoList = subDir.entryInfoList(QStringList(QString::fromStdString("*.qml")),
                                                                 QDir::NoDotAndDotDot | QDir::Files | QDir::AllDirs,
                                                                 QDir::DirsLast);
            scanProject(subFileInfoList);
        } else {
            // add to sources/tests list
            if (isSource(fileInfo.filePath())) {
                sourceFiles.insert(fileInfo.filePath());
            } else {
                testFiles.insert(fileInfo.filePath());
            }
        }
    }
}

