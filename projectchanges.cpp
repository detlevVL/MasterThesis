#include "projectchanges.h"

ProjectChanges::ProjectChanges(QString projectPath, QObject *parent) : QObject(parent)
{
    this->projectPath = projectPath;
    connect(ModelManagerInterface::instance(), SIGNAL(documentChangedOnDisk(QmlJS::Document::Ptr)), this, SLOT(fileModified(QmlJS::Document::Ptr)));
    connect(ModelManagerInterface::instance(), SIGNAL(aboutToRemoveFiles(QStringList)), this, SLOT(fileDeleted(QStringList)));

    loadFileChanges();
}

void ProjectChanges::fileModified(Document::Ptr doc)
{
    if (doc->path().contains(projectPath)) {
        modifiedFiles.insert(doc->fileName());
    }
}

void ProjectChanges::fileDeleted(const QStringList &files)
{
    foreach (const QString &file, files) {
        if (file.contains(projectPath)) {
            modifiedFiles.remove(file);
            deletedFiles.insert(file);
        }
    }
}
QSet<QString> ProjectChanges::getDeletedFiles() const
{
    return deletedFiles;
}

QSet<QString> ProjectChanges::getModifiedFiles() const
{
    return modifiedFiles;
}


void ProjectChanges::loadFileChanges()
{
    modifiedFiles.clear();
    deletedFiles.clear();

    QFile changesFile(projectPath + QString::fromStdString("/MHchanges.txt"));

    if (changesFile.exists()) {
        changesFile.open(QFile::ReadOnly | QFile::Text);
        QTextStream in(&changesFile);

        QString line = in.readLine();
        while (!line.isNull()) {
            QString change = line.section(QString::fromStdString(":"), 0, 0);
            QString files = line.section(QString::fromStdString(":"), 1, 1);

            foreach (const QString &file, files.split(QString::fromStdString(","), QString::SkipEmptyParts)) {
                if (change == QString::fromStdString("modified")) {
                    modifiedFiles.insert(file);
                } else if (change == QString::fromStdString("deleted")) {
                    deletedFiles.insert(file);
                }
            }

            line = in.readLine();
        }
    }
}

void ProjectChanges::saveFileChanges()
{
    QFile changesFile(projectPath + QString::fromStdString("/MHchanges.txt"));
    changesFile.open(QFile::ReadWrite | QFile::Truncate | QFile::Text);
    QTextStream out(&changesFile);

    out << "modified" << ":";
    foreach (const QString &file, modifiedFiles) {
        out << file << ",";
    }
    out << endl;

    out << "deleted" << ":";
    foreach (const QString &file, deletedFiles) {
        out << file << ",";
    }
    out << endl;

    changesFile.close();
}

