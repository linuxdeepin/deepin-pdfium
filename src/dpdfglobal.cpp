// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <QtCore>
#include <QString>
#include <QDebug>
#include "dpdfglobal.h"
#include "public/fpdfview.h"

#include <chardet.h>

static bool initialized = false;

const static DPdfGlobal instance = DPdfGlobal();

void DPdfGlobal::init()
{
    qDebug() << "Initializing PDF library";
    if (!initialized) {
        FPDF_InitLibrary();
        initialized = true;
        qDebug() << "PDF library initialized successfully";
    } else {
        qDebug() << "PDF library already initialized";
    }
}

void DPdfGlobal::destory()
{
    qDebug() << "Destroying PDF library";
    if (initialized) {
        FPDF_DestroyLibrary();
        initialized = false;
        qDebug() << "PDF library destroyed successfully";
    } else {
        qDebug() << "PDF library was not initialized";
    }
}

DPdfGlobal::DPdfGlobal()
{
    qDebug() << "Creating DPdfGlobal instance";
    init();
}

DPdfGlobal::~DPdfGlobal()
{
    qDebug() << "Destroying DPdfGlobal instance";
    destory();
}

QString DPdfGlobal::textCodeType(const char *text)
{
    qDebug() << "Detecting text encoding";
    DetectObj *obj = detect_obj_init();
    detect(text, &obj);
    const QString &encodeind = QString(obj->encoding).toLower();
    qDebug() << "Detected encoding:" << encodeind;
    
    detect_obj_free(&obj);
    return encodeind;
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
Q_GLOBAL_STATIC_WITH_ARGS(QMutex, pdfMutex, (QMutex::Recursive));

DPdfMutexLocker::DPdfMutexLocker(const QString &tmpLog): QMutexLocker(pdfMutex())
#else
Q_GLOBAL_STATIC(QRecursiveMutex, pdfMutex);

DPdfMutexLocker::DPdfMutexLocker(const QString &tmpLog): QMutexLocker<QRecursiveMutex>(pdfMutex())
#endif
{
    m_log = tmpLog;
    qInfo() << m_log + " begin ";
    m_timer.start();
}

DPdfMutexLocker::~DPdfMutexLocker()
{
    qInfo() << m_log + " end time = " << m_timer.elapsed();
}
