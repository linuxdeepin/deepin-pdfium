// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "dpdfdoc.h"
#include "dpdfpage.h"

#include "public/fpdfview.h"
#include "public/fpdf_doc.h"
#include "public/fpdf_save.h"
#include "public/fpdf_dataavail.h"
#include "public/fpdf_formfill.h"

#include "core/fpdfdoc/cpdf_bookmark.h"
#include "core/fpdfdoc/cpdf_bookmarktree.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfdoc/cpdf_pagelabel.h"

#include <QFile>
#include <QTemporaryDir>
#include <QUuid>
#include <unistd.h>
#include <QStorageInfo>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

/**
 * @brief The PDFIumLoader class for FPDF_FILEACCESS
 */
class PDFIumLoader
{
public:
    PDFIumLoader() {}
    PDFIumLoader(const char *pBuf, size_t len);
    const char *m_pBuf;
    size_t m_Len;
};

/**
 * @brief GetBlock for FPDF_FILEACCESS
 * @param param 为PDFIumLoader类型
 */
static int GetBlock(void *param, unsigned long pos, unsigned char *pBuf, unsigned long size)
{
    PDFIumLoader *pLoader = static_cast<PDFIumLoader *>(param);
    if (pos + size < pos || pos + size > pLoader->m_Len) return 0;
    memcpy(pBuf, pLoader->m_pBuf + pos, size);
    return 1;
}

/**
 * @brief IsDataAvail for FX_FILEAVAIL
 */
static FPDF_BOOL IsDataAvail(FX_FILEAVAIL * /*pThis*/, size_t /*offset*/, size_t /*size*/)
{
    return true;
}

/**
 * @brief Check if the file is an Remote file
 * @param filePath file path
 * @return returns true if it's an Remote file, false otherwise
 */
static bool isRemoteFile(const QString &filePath)
{
    // Currently only SMB files are supported for remote file handling
    QFileInfo fileInfo(filePath);
    QStorageInfo storage = QStorageInfo(fileInfo.absolutePath());
    
    QString fsType = storage.fileSystemType().toLower();
    
    bool isRemote = (fsType == "cifs" || fsType == "smb" || fsType == "smbfs");
    qDebug() << filePath << "isRemote:" << isRemote;
    return isRemote;
}

DPdfDoc::Status parseError(int error)
{
    DPdfDoc::Status err_code = DPdfDoc::SUCCESS;
    // Translate FPDFAPI error code to FPDFVIEW error code
    switch (error) {
    case FPDF_ERR_SUCCESS:
        err_code = DPdfDoc::SUCCESS;
        break;
    case FPDF_ERR_FILE:
        err_code = DPdfDoc::FILE_ERROR;
        break;
    case FPDF_ERR_FORMAT:
        err_code = DPdfDoc::FORMAT_ERROR;
        break;
    case FPDF_ERR_PASSWORD:
        err_code = DPdfDoc::PASSWORD_ERROR;
        break;
    case FPDF_ERR_SECURITY:
        err_code = DPdfDoc::HANDLER_ERROR;
        break;
    }
    qDebug() << "Parsed error code:" << error << "to status:" << err_code;
    return err_code;
}

class DPdfDocPrivate
{
    friend class DPdfDoc;
public:
    DPdfDocPrivate();
    ~DPdfDocPrivate();

public:
    DPdfDoc::Status loadFile(const QString &filePath, const QString &password);

private:
    DPdfDocHandler *m_docHandler;
    QVector<DPdfPage *> m_pages;
    QString m_filePath;        // Original file path
    QString m_tempFilePath;    // Temporary file path
    bool m_isRemoteFile;          // Whether it is a Remote file
    int m_pageCount = 0;
    DPdfDoc::Status m_status;
};

DPdfDocPrivate::DPdfDocPrivate()
{
    m_docHandler = nullptr;
    m_pageCount = 0;
    m_status = DPdfDoc::NOT_LOADED;
    m_isRemoteFile = false;
}

DPdfDocPrivate::~DPdfDocPrivate()
{
    DPdfMutexLocker locker("DPdfDocPrivate::~DPdfDocPrivate()");
    // qDebug() << "Cleaning up DPdfDocPrivate resources";

    qDeleteAll(m_pages);

    if (nullptr != m_docHandler) {
        // qDebug() << "Closing PDF document handler";
        FPDF_CloseDocument(reinterpret_cast<FPDF_DOCUMENT>(m_docHandler));
    }
    
    if (!m_tempFilePath.isEmpty() && QFile::exists(m_tempFilePath)) {
        QFile::remove(m_tempFilePath);
        // qDebug() << "Temporary file deleted:" << m_tempFilePath;
    }
}

DPdfDoc::Status DPdfDocPrivate::loadFile(const QString &filePath, const QString &password)
{
    qDebug() << "Loading PDF file:" << filePath;
    m_filePath = filePath;
    m_tempFilePath.clear();
    m_isRemoteFile = false;
    m_pages.clear();    

    if (!QFile::exists(m_filePath)) {
        qWarning() << "File not found:" << m_filePath;
        m_status = DPdfDoc::FILE_NOT_FOUND_ERROR;
        return m_status;
    }

    // Check if it is a Remote file
    m_isRemoteFile = isRemoteFile(m_filePath);
    
    // Determine the file path to load
    QString fileToLoad = m_filePath;
    
    if (m_isRemoteFile) {
        qDebug() << "Detected Remote file, creating local copy:" << m_filePath;
        
        // Create a remote file cache directory in the application cache directory
        QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/remote_cache";
        QDir cacheDirObj(cacheDir);
        if (!cacheDirObj.exists()) {
            cacheDirObj.mkpath(".");
            qDebug() << "Created remote cache directory:" << cacheDir;
        }

        // Create a temporary file name using the original file name and a unique ID
        QFileInfo fileInfo(m_filePath);
        QString uniqueId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        m_tempFilePath = QString("%1/%2.%3").arg(cacheDir, fileInfo.completeBaseName(), uniqueId);

        // If the original file has a suffix, keep the original suffix
        if (!fileInfo.suffix().isEmpty()) {
            m_tempFilePath += "." + fileInfo.suffix();
        }
        
        // Copy the remote file to the local temporary file
        if (!QFile::copy(m_filePath, m_tempFilePath)) {
            qWarning() << "Failed to create local copy of Remote file:" << m_filePath;
            m_status = DPdfDoc::FILE_ERROR;
            return m_status;
        }
        
        qDebug() << "Local temporary file created:" << m_tempFilePath;
        fileToLoad = m_tempFilePath;
    }

    DPdfMutexLocker locker("DPdfDocPrivate::loadFile");

    qDebug() << "deepin-pdfium正在加载PDF文档... 文档名称:" << fileToLoad;
    void *ptr = FPDF_LoadDocument(fileToLoad.toUtf8().constData(),
                                 password.toUtf8().constData());

    m_docHandler = static_cast<DPdfDocHandler *>(ptr);

    m_status = m_docHandler ? DPdfDoc::SUCCESS : parseError(static_cast<int>(FPDF_GetLastError()));
    qDebug() << "文档（" << filePath << "）文档加载状态: " << m_status;

    if (m_docHandler) {
        m_pageCount = FPDF_GetPageCount(reinterpret_cast<FPDF_DOCUMENT>(m_docHandler));
        qDebug() << "Document loaded successfully with" << m_pageCount << "pages";
        m_pages.fill(nullptr, m_pageCount);
    }

    return m_status;
}


DPdfDoc::DPdfDoc(QString filename, QString password)
    : d_ptr(new DPdfDocPrivate())
{
    d_func()->loadFile(filename, password);
}

DPdfDoc::~DPdfDoc()
{

}

bool DPdfDoc::isValid() const
{
    bool valid = d_func()->m_docHandler != nullptr;
    qDebug() << "Checking document validity:" << valid;
    return valid;
}

bool DPdfDoc::isEncrypted() const
{
    if (!isValid()) {
        qDebug() << "Document is not valid, cannot check encryption";
        return false;
    }

    DPdfMutexLocker locker("DPdfDoc::isEncrypted()");
    bool encrypted = FPDF_GetDocPermissions(reinterpret_cast<FPDF_DOCUMENT>(d_func()->m_docHandler)) != 0xFFFFFFFF;
    qDebug() << "Document encryption status:" << encrypted;
    return encrypted;
}

DPdfDoc::Status DPdfDoc::tryLoadFile(const QString &filename, const QString &password)
{
    qDebug() << "Attempting to load file:" << filename;
    Status status = NOT_LOADED;
    if (!QFile::exists(filename)) {
        qWarning() << "File not found:" << filename;
        status = FILE_NOT_FOUND_ERROR;
        return status;
    }

    DPdfMutexLocker locker("DPdfDoc::tryLoadFile");

    void *ptr = FPDF_LoadDocument(filename.toUtf8().constData(),
                                  password.toUtf8().constData());

    DPdfDocHandler *docHandler = static_cast<DPdfDocHandler *>(ptr);
    status = docHandler ? SUCCESS : parseError(static_cast<int>(FPDF_GetLastError()));
    qDebug() << "File load attempt status:" << status;

    if (docHandler) {
        qDebug() << "Closing test document handler";
        FPDF_CloseDocument(reinterpret_cast<FPDF_DOCUMENT>(docHandler));
    }

    return status;
}

bool DPdfDoc::isLinearized(const QString &fileName)
{
    qDebug() << "Checking if file is linearized:" << fileName;
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly)) {
        qInfo() << "file open failed when isLinearized" << fileName;
    }
    QByteArray content = file.readAll();
    int len = content.length();
    const char *pBuf = content.data();

    PDFIumLoader m_PDFIumLoader;
    m_PDFIumLoader.m_pBuf = pBuf;//pdf content
    m_PDFIumLoader.m_Len = size_t(len);//content len

    FPDF_FILEACCESS m_fileAccess;
    memset(&m_fileAccess, '\0', sizeof(m_fileAccess));
    m_fileAccess.m_FileLen = static_cast<unsigned long>(len);
    m_fileAccess.m_GetBlock = GetBlock;
    m_fileAccess.m_Param = &m_PDFIumLoader;

    FX_FILEAVAIL m_fileAvail;
    memset(&m_fileAvail, '\0', sizeof(m_fileAvail));
    m_fileAvail.version = 1;
    m_fileAvail.IsDataAvail = IsDataAvail;

    FPDF_AVAIL m_PdfAvail;
    m_PdfAvail = FPDFAvail_Create(&m_fileAvail, &m_fileAccess);

    bool linearized = FPDFAvail_IsLinearized(m_PdfAvail) > 0;
    qDebug() << "File linearization status:" << linearized;
    return linearized;
}

static QFile saveWriter;

int writeFile(struct FPDF_FILEWRITE_* pThis, const void *pData, unsigned long size)
{
    Q_UNUSED(pThis)
    return 0 != saveWriter.write(static_cast<char *>(const_cast<void *>(pData)), static_cast<qint64>(size));
}

bool DPdfDoc::saveRemoteFile()
{
    qDebug() << "Saving remote file:" << d_func()->m_filePath;
    return saveAs(d_func()->m_filePath);
}

bool DPdfDoc::saveLocalFile()
{
    qDebug() << "Saving local file:" << d_func()->m_filePath;
    FPDF_FILEWRITE write;

    write.WriteBlock = writeFile;

    QTemporaryDir tempDir;

    QString tempFilePath = tempDir.path() + "/" + QUuid::createUuid().toString();
    
    saveWriter.setFileName(tempFilePath);
    qDebug() << "Using temporary file for save:" << tempFilePath;

    if (!saveWriter.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open temporary file for writing";
        return false;
    }
    
    DPdfMutexLocker locker("DPdfDoc::save");
    bool result = FPDF_SaveAsCopy(reinterpret_cast<FPDF_DOCUMENT>(d_func()->m_docHandler), &write, FPDF_NO_INCREMENTAL);
    locker.unlock();
    
    saveWriter.close();
    
    QFile tempFile(tempFilePath);
    if (!tempFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open temporary file for reading";
        return false;
    }
    
    QByteArray array = tempFile.readAll();

    tempFile.close();
    
    QFile file(d_func()->m_filePath);
    
    file.remove();          //不remove会出现第二次导出丢失数据问题 (保存动作完成之后，如果当前文档是当初打开那个，下一次导出会出错)
    
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "Failed to open target file for writing";
        return false;
    }
    
    if (array.size() != file.write(array)) {
        qWarning() << "Failed to write all data to target file";
        result = false;
    }
    
    file.flush();//函数将用户缓存中的内容写入内核缓冲区
    fsync(file.handle());//将内核缓冲写入文件(磁盘)
    file.close();
    
    qDebug() << "Local file save completed with status:" << result;
    return result;
}

bool DPdfDoc::save()
{
    qDebug() << "Saving document, isRemoteFile:" << d_func()->m_isRemoteFile;
    if (d_func()->m_isRemoteFile) {
        return saveRemoteFile();
    } else {
        return saveLocalFile();
    }
}

bool DPdfDoc::saveAs(const QString &filePath)
{
    qDebug() << "Saving document as:" << filePath;
    FPDF_FILEWRITE write;

    write.WriteBlock = writeFile;

    saveWriter.setFileName(filePath);

    if (!saveWriter.open(QIODevice::ReadWrite)) {
        qWarning() << "Failed to open file for saving:" << filePath;
        return false;
    }

    DPdfMutexLocker locker("DPdfDoc::saveAs");
    bool result = FPDF_SaveAsCopy(reinterpret_cast<FPDF_DOCUMENT>(d_func()->m_docHandler), &write, FPDF_NO_INCREMENTAL);
    locker.unlock();

    saveWriter.close();
    qDebug() << "Save as completed with status:" << result;
    return result;
}

QString DPdfDoc::filePath() const
{
    return d_func()->m_filePath;
}

int DPdfDoc::pageCount() const
{
    return d_func()->m_pageCount;
}

DPdfDoc::Status DPdfDoc::status() const
{
    return d_func()->m_status;
}

DPdfPage *DPdfDoc::page(int i, qreal xRes, qreal yRes)
{
    qDebug() << "Getting page" << i << "with resolution" << xRes << "x" << yRes;
    if (i < 0 || i >= d_func()->m_pageCount) {
        qWarning() << "Invalid page index:" << i;
        return nullptr;
    }

    if (!d_func()->m_pages[i]) {
        qDebug() << "Creating new page object for index:" << i;
        d_func()->m_pages[i] = new DPdfPage(d_func()->m_docHandler, i, xRes, yRes);
    }

    return d_func()->m_pages[i];
}

void collectBookmarks(DPdfDoc::Outline &outline, const CPDF_BookmarkTree &tree, CPDF_Bookmark This, qreal xRes, qreal yRes)
{
    DPdfDoc::Section section;

    const WideString &title = This.GetTitle();

    section.title = QString::fromWCharArray(title.c_str(), static_cast<int>(title.GetLength()));

    bool hasx = false, hasy = false, haszoom = false;
    float x = 0.0, y = 0.0, z = 0.0;

    const CPDF_Dest &dest = This.GetDest(tree.GetDocument()).GetArray() ? This.GetDest(tree.GetDocument()) : This.GetAction().GetDest(tree.GetDocument());
    section.nIndex = dest.GetDestPageIndex(tree.GetDocument());
    dest.GetXYZ(&hasx, &hasy, &haszoom, &x, &y, &z);
    section.offsetPointF = QPointF(static_cast<qreal>(x) * xRes / 72, static_cast<qreal>(y) * yRes / 72);

    const CPDF_Bookmark &Child = tree.GetFirstChild(&This);
    if (Child.GetDict() != nullptr) {
        collectBookmarks(section.children, tree, Child, xRes, yRes);
    }
    outline << section;

    const CPDF_Bookmark &SibChild = tree.GetNextSibling(&This);
    if (SibChild.GetDict() != nullptr) {
        collectBookmarks(outline, tree, SibChild, xRes, yRes);
    }
}

DPdfDoc::Outline DPdfDoc::outline(qreal xRes, qreal yRes)
{
    qDebug() << "Getting document outline with resolution" << xRes << "x" << yRes;
    DPdfMutexLocker locker("DPdfDoc::outline");

    Outline outline;
    CPDF_BookmarkTree tree(reinterpret_cast<CPDF_Document *>(d_func()->m_docHandler));
    CPDF_Bookmark cBookmark;
    const CPDF_Bookmark &firstRootChild = tree.GetFirstChild(&cBookmark);
    if (firstRootChild.GetDict() != nullptr) {
        qDebug() << "Collecting bookmarks from document";
        collectBookmarks(outline, tree, firstRootChild, xRes, yRes);
    }

    return outline;
}

DPdfDoc::Properies DPdfDoc::proeries()
{
    qDebug() << "Getting document properties";
    DPdfMutexLocker locker("DPdfDoc::proeries");

    Properies properies;
    int fileversion = 1;
    properies.insert("Version", "1");
    if (FPDF_GetFileVersion(reinterpret_cast<FPDF_DOCUMENT>(d_func()->m_docHandler), &fileversion)) {
        properies.insert("Version", QString("%1.%2").arg(fileversion / 10).arg(fileversion % 10));
    }
    properies.insert("Encrypted", isEncrypted());
    properies.insert("Linearized", FPDF_GetFileLinearized(reinterpret_cast<FPDF_DOCUMENT>(d_func()->m_docHandler)));
    properies.insert("KeyWords", QString());
    properies.insert("Title", QString());
    properies.insert("Creator", QString());
    properies.insert("Producer", QString());
    CPDF_Document *pDoc = reinterpret_cast<CPDF_Document *>(d_func()->m_docHandler);

    const CPDF_Dictionary *pInfo = pDoc->GetInfo();
    if (pInfo) {
        qDebug() << "Reading document metadata from info dictionary";
        const WideString &KeyWords = pInfo->GetUnicodeTextFor("KeyWords");
        properies.insert("KeyWords", QString::fromWCharArray(KeyWords.c_str()));

        //windows和mac上生成的pdf此处编码格式不同,需要嗅探查找
        const ByteString &Title = pInfo->GetStringFor("Title");
        if ("utf-8" == DPdfGlobal::textCodeType(Title.c_str())) {
            properies.insert("Title", QString::fromUtf8(Title.c_str()));
        } else {
            const WideString &WTitle = pInfo->GetUnicodeTextFor("Title");
            properies.insert("Title", QString::fromWCharArray(WTitle.c_str()));
        }

        const WideString &Creator = pInfo->GetUnicodeTextFor("Creator");
        properies.insert("Creator", QString::fromWCharArray(Creator.c_str()));

        const WideString &Producer = pInfo->GetUnicodeTextFor("Producer");
        properies.insert("Producer", QString::fromWCharArray(Producer.c_str()));
    }

    return properies;
}

QString DPdfDoc::label(int index) const
{
    qDebug() << "Getting page label for index:" << index;
    DPdfMutexLocker locker("DPdfDoc::label index = " + QString::number(index));

    CPDF_PageLabel label(reinterpret_cast<CPDF_Document *>(d_func()->m_docHandler));
    const Optional<WideString> &str = label.GetLabel(index);
    if (str.has_value()) {
        QString result = QString::fromWCharArray(str.value().c_str(), static_cast<int>(str.value().GetLength()));
        qDebug() << "Found page label:" << result;
        return result;
    }
    qDebug() << "No page label found for index:" << index;
    return QString();
}
