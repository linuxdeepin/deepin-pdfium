// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "dpdfannot.h"
#include <QDebug>

DPdfAnnot::AnnotType DPdfAnnot::type()
{
    qDebug() << "Getting annotation type:" << m_type;
    return m_type;
}

void DPdfAnnot::setText(QString text)
{
    qDebug() << "Setting annotation text:" << text;
    m_text = text;
}

QString DPdfAnnot::text()
{
    qDebug() << "Getting annotation text:" << m_text;
    return m_text;
}

DPdfTextAnnot::DPdfTextAnnot()
{
    m_type = AText;
}

bool DPdfTextAnnot::pointIn(QPointF pos)
{
    return m_rect.contains(pos);
}

QList<QRectF> DPdfTextAnnot::boundaries()
{
    qDebug() << "Getting text annotation boundaries";
    QList<QRectF> list;

    list << m_rect;

    return list;
}

void DPdfTextAnnot::setRectF(const QRectF &rectf)
{
    qDebug() << "Setting text annotation rectangle:" << rectf;
    m_rect = rectf;
}

DPdfSquareAnnot::DPdfSquareAnnot()
{
    m_type = ASQUARE;
}

bool DPdfSquareAnnot::pointIn(QPointF pos)
{
    return m_rect.contains(pos);
}

QList<QRectF> DPdfSquareAnnot::boundaries()
{
    qDebug() << "Getting square annotation boundaries";
    return QList<QRectF>() << m_rect;
}

void DPdfSquareAnnot::setRectF(const QRectF &rectf)
{
    qDebug() << "Setting square annotation rectangle:" << rectf;
    m_rect = rectf;
}

DPdfHightLightAnnot::DPdfHightLightAnnot()
{
    m_type = AHighlight;
}

bool DPdfHightLightAnnot::pointIn(QPointF pos)
{
    for (QRectF rect : m_rectList) {
        if (rect.contains(pos))
            return true;
    }

    return false;
}

void DPdfHightLightAnnot::setColor(QColor color)
{
    qDebug() << "Setting highlight annotation color:" << color;
    m_color = color;
}

QColor DPdfHightLightAnnot::color()
{
    qDebug() << "Getting highlight annotation color:" << m_color;
    return m_color;
}

void DPdfHightLightAnnot::setBoundaries(QList<QRectF> rectList)
{
    qDebug() << "Setting highlight annotation boundaries, count:" << rectList.size();
    m_rectList = rectList;
}

QList<QRectF> DPdfHightLightAnnot::boundaries()
{
    qDebug() << "Getting highlight annotation boundaries";
    return m_rectList;
}

DPdfAnnot::~DPdfAnnot()
{

}

DPdfUnknownAnnot::DPdfUnknownAnnot()
{
    m_type = AUnknown;
}

bool DPdfUnknownAnnot::pointIn(QPointF pos)
{
    Q_UNUSED(pos)
    return false;
}

QList<QRectF> DPdfUnknownAnnot::boundaries()
{
    return QList<QRectF>();
}

DPdfLinkAnnot::DPdfLinkAnnot()
{
    m_type = ALink;
}

bool DPdfLinkAnnot::pointIn(QPointF pos)
{
    if (m_rect.contains(pos))
        return true;

    return false;
}

QList<QRectF> DPdfLinkAnnot::boundaries()
{
    qDebug() << "Getting link annotation boundaries";
    QList<QRectF> list;

    list << m_rect;

    return list;
}

void DPdfLinkAnnot::setRectF(const QRectF &rect)
{
    qDebug() << "Setting link annotation rectangle:" << rect;
    m_rect = rect;
}

void DPdfLinkAnnot::setUrl(QString url)
{
    qDebug() << "Setting link annotation URL:" << url;
    m_url = url;

    if (!m_url.contains("http://") && !m_url.contains("https://")) {
        qDebug() << "Prepending http:// to URL";
        m_url.prepend("http://");
    }
}

QString DPdfLinkAnnot::url() const
{
    qDebug() << "Getting link annotation URL:" << m_url;
    return m_url;
}

void DPdfLinkAnnot::setFilePath(QString filePath)
{
    qDebug() << "Setting link annotation file path:" << filePath;
    m_filePath = filePath;
}

QString DPdfLinkAnnot::filePath() const
{
    qDebug() << "Getting link annotation file path:" << m_filePath;
    return m_filePath;
}

void DPdfLinkAnnot::setPage(int index, float left, float top)
{
    qDebug() << "Setting link annotation page properties - index:" << index << "left:" << left << "top:" << top;
    m_index = index;
    m_left = left;
    m_top = top;
}

int DPdfLinkAnnot::pageIndex() const
{
    qDebug() << "Getting link annotation page index:" << m_index;
    return m_index;
}

QPointF DPdfLinkAnnot::offset() const
{
    return QPointF(static_cast<qreal>(m_left), static_cast<qreal>(m_top));
}

void DPdfLinkAnnot::setLinkType(int type)
{
    qDebug() << "Setting link annotation type:" << type;
    m_linkType = type;
}

int DPdfLinkAnnot::linkType() const
{
    qDebug() << "Getting link annotation type:" << m_linkType;
    return m_linkType;
}

bool DPdfLinkAnnot::isValid() const
{
    bool valid = (Goto == m_linkType) ? (m_index != -1) : true;
    qDebug() << "Checking link annotation validity:" << valid;
    return valid;
}

DPdfCIRCLEAnnot::DPdfCIRCLEAnnot()
{
    m_type = ACIRCLE;
}

bool DPdfCIRCLEAnnot::pointIn(QPointF pos)
{
    return m_rect.contains(pos);
}

QList<QRectF> DPdfCIRCLEAnnot::boundaries()
{
    qDebug() << "Getting circle annotation boundaries";
    return QList<QRectF>() << m_rect;
}

void DPdfCIRCLEAnnot::setRectF(const QRectF &rectf)
{
    qDebug() << "Setting circle annotation rectangle:" << rectf;
    m_rect = rectf;
}

void DPdfCIRCLEAnnot::setBoundaries(QList<QRectF> rectList)
{
    qDebug() << "Setting circle annotation boundaries, count:" << rectList.size();
    m_rectList = rectList;
}

DPdfUnderlineAnnot::DPdfUnderlineAnnot()
{
    m_type = AUNDERLINE;
}

bool DPdfUnderlineAnnot::pointIn(QPointF pos)
{
    return m_rect.contains(pos);
}

QList<QRectF> DPdfUnderlineAnnot::boundaries()
{
    qDebug() << "Getting underline annotation boundaries";
    return QList<QRectF>() << m_rect;
}

void DPdfUnderlineAnnot::setRectF(const QRectF &rectf)
{
    qDebug() << "Setting underline annotation rectangle:" << rectf;
    m_rect = rectf;
}

DPdfWidgetAnnot::DPdfWidgetAnnot()
{
    m_type = AWIDGET;
}

bool DPdfWidgetAnnot::pointIn(QPointF pos)
{
    Q_UNUSED(pos)
    return false;
}

QList<QRectF> DPdfWidgetAnnot::boundaries()
{
    qDebug() << "Getting widget annotation boundaries";
    return QList<QRectF>();
}
