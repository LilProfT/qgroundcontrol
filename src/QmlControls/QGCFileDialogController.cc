/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "QGCFileDialogController.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "AppSettings.h"

#include <QStandardPaths>
#include <QDebug>
#include <QDir>

QGC_LOGGING_CATEGORY(QGCFileDialogControllerLog, "QGCFileDialogControllerLog")

//Should the lookup tables be called outside the function?
QString QGCFileDialogController::removeAccent(const QString &filename)
{
    QString output;

    //Lookup tables, Vietnamese only
    //Maybe the normalization method is superior?
    QString diacriticLetters = QString::fromUtf8("ÁÃÀẢẠ"
                                                "ÂẤẪẦẨẬ"
                                                "ĂẮẴẰẲẶ"
                                                "áãàảạ"
                                                "âấẫầẩậ"
                                                "ăắẵằẳặ"
                                                "Đđ"
                                                "ÉẼÈẺẸ"
                                                "ÊẾỄỀỂỆ"
                                                "éẽèẻẹ"
                                                "êếễềểệ"
                                                "ÍĨÌỈỊ"
                                                "íĩìỉị"
                                                "ÓÕÒỎỌ"
                                                "ÔỐỖỒỔỘ"
                                                "ƠỚỠỜỞỢ"
                                                "óõòỏọ"
                                                "ôốỗồổộ"
                                                "ơớỡờởợ"
                                                "ÚŨÙỦỤ"
                                                "ƯỨỮỪỬỰ"
                                                "úũùủụ"
                                                "ưứữừửự"
                                                "ÝỸỲỶỴ"
                                                "ýỹỳỷỵ");

    QString noDiacriticLetters = QString::fromUtf8("AAAAA"
                                                   "AAAAAA"
                                                   "AAAAAA"
                                                   "aaaaa"
                                                   "aaaaaa"
                                                   "aaaaaa"
                                                   "Dd"
                                                   "EEEEE"
                                                   "EEEEEE"
                                                   "eeeee"
                                                   "eeeeee"
                                                   "IIIII"
                                                   "iiiii"
                                                   "OOOOO"
                                                   "OOOOOO"
                                                   "OOOOOO"
                                                   "ooooo"
                                                   "oooooo"
                                                   "oooooo"
                                                   "UUUUU"
                                                   "UUUUUU"
                                                   "uuuuu"
                                                   "uuuuuu"
                                                   "YYYYY"
                                                   "yyyyy");

    for (int i = 0 ; i < filename.length(); i++)
    {
        int filename_index = diacriticLetters.indexOf(filename[i]);

        //Return -1 if char is not found
        if (filename_index < 0)
        {
            output.append(filename[i]);
        } else {
            QChar replacement_char = noDiacriticLetters[filename_index];

            output.append(replacement_char);
        }
    }

    return output;
}

//Adding search function
QStringList QGCFileDialogController::getFiles(const QString& directoryPath, const QStringList& nameFilters)
{
    qCDebug(QGCFileDialogControllerLog) << "getFiles" << directoryPath << nameFilters;
    QStringList files;

    QDir fileDir(directoryPath);

    QFileInfoList fileInfoList = fileDir.entryInfoList(nameFilters,  QDir::Files, QDir::Name);

    for (const QFileInfo& fileInfo: fileInfoList) {
        qCDebug(QGCFileDialogControllerLog) << "getFiles found" << fileInfo.fileName();
        files << fileInfo.fileName();
    }

    return files;
}

bool QGCFileDialogController::fileExists(const QString& filename)
{
    return QFile(filename).exists();
}

QString QGCFileDialogController::fullyQualifiedFilename(const QString& directoryPath, const QString& filename, const QStringList& nameFilters)
{
    QString firstFileExtention;

    // Check that the filename has one of the specified file extensions

    bool extensionFound = true;
    if (nameFilters.count()) {
        extensionFound = false;
        for (const QString& nameFilter: nameFilters) {
            if (nameFilter.startsWith("*.")) {
                QString fileExtension = nameFilter.right(nameFilter.length() - 2);
                if (fileExtension != "*") {
                    if (firstFileExtention.isEmpty()) {
                        firstFileExtention = fileExtension;
                    }
                    if (filename.endsWith(fileExtension)) {
                        extensionFound = true;
                        break;
                    }
                }
            } else if (nameFilter != "*") {
                qCWarning(QGCFileDialogControllerLog) << "unsupported name filter format" << nameFilter;
            }
        }
    }

    // Add the extension if it is missing
    QString filenameWithExtension = filename;
    if (!extensionFound) {
        filenameWithExtension = QStringLiteral("%1.%2").arg(filename).arg(firstFileExtention);
    }

    return directoryPath + QStringLiteral("/") + filenameWithExtension;
}

void QGCFileDialogController::deleteFile(const QString& filename)
{
    QFile::remove(filename);
}

QStringList QGCFileDialogController::searchFiles(const QString &searchText, const QStringList& files)
{
    QStringList newlist;

    if (searchText.isEmpty())
    {
        return files;
    } else {
        for (int i = 0; i < files.size(); i++ )
        {
            if (removeAccent(files[i]).contains(removeAccent(searchText), Qt::CaseInsensitive))
            {
                newlist << files[i];
            }
        }
    }

    newlist.sort();

    return newlist;
}

QString QGCFileDialogController::fullFolderPathToShortMobilePath(const QString& fullFolderPath)
{
#ifdef __mobile__
    QString defaultSavePath = qgcApp()->toolbox()->settingsManager()->appSettings()->savePath()->rawValueString();
    if (fullFolderPath.startsWith(defaultSavePath)) {
        int lastDirSepIndex = fullFolderPath.lastIndexOf(QStringLiteral("/"));
        return qgcApp()->applicationName() + QStringLiteral("/") + fullFolderPath.right(fullFolderPath.length() - lastDirSepIndex);
    } else {
        return fullFolderPath;
    }
#else
    qWarning() << "QGCFileDialogController::fullFolderPathToShortMobilePath should only be used in mobile builds";
    return fullFolderPath;
#endif
}
