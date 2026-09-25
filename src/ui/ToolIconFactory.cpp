#include "ui/ToolIconFactory.h"

#include <cmath>

#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>
#include <QPolygonF>

namespace freeshape::ui {

namespace {

QIcon drawIcon(const QString& id)
{
    constexpr int size = 32;
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing, true);

    QPen pen(QColor(55, 61, 66));
    pen.setWidthF(2.15);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    auto line = [&](qreal x1, qreal y1, qreal x2, qreal y2) {
        p.drawLine(QPointF(x1, y1), QPointF(x2, y2));
    };

    if (id == QStringLiteral("sketch")) {
        p.drawRect(QRectF(6, 7, 17, 17));
        line(20, 22, 27, 15);
        line(22, 24, 28, 18);
        line(6, 27, 17, 27);
    }
    else if (id == QStringLiteral("extrude")) {
        p.drawRect(QRectF(5, 17, 15, 10));
        line(20, 17, 26, 12);
        line(20, 27, 26, 22);
        line(26, 12, 26, 22);
        line(14, 16, 14, 6);
        line(14, 6, 10, 10);
        line(14, 6, 18, 10);
    }
    else if (id == QStringLiteral("revolve")) {
        line(16, 5, 16, 27);
        QPainterPath path;
        path.arcMoveTo(QRectF(6, 7, 20, 18), 65);
        path.arcTo(QRectF(6, 7, 20, 18), 65, 245);
        p.drawPath(path);
        line(7, 9, 11, 7);
        line(7, 9, 8, 13);
    }
    else if (id == QStringLiteral("sweep")) {
        QPainterPath path;
        path.moveTo(5, 23);
        path.cubicTo(10, 6, 18, 28, 27, 10);
        p.drawPath(path);
        p.drawEllipse(QPointF(7, 22), 3.5, 3.5);
    }
    else if (id == QStringLiteral("loft")) {
        p.drawEllipse(QRectF(5, 20, 11, 6));
        p.drawEllipse(QRectF(17, 6, 10, 5));
        line(7, 22, 19, 8);
        line(14, 25, 26, 10);
    }
    else if (id == QStringLiteral("hole")) {
        p.drawRect(QRectF(5, 6, 22, 20));
        p.drawEllipse(QPointF(16, 16), 5.3, 5.3);
        line(16, 8, 16, 24);
        line(8, 16, 24, 16);
    }
    else if (id == QStringLiteral("fillet")) {
        line(7, 25, 7, 10);
        QPainterPath path;
        path.moveTo(7, 10);
        path.quadTo(7, 7, 10, 7);
        path.lineTo(25, 7);
        p.drawPath(path);
        line(13, 25, 25, 13);
    }
    else if (id == QStringLiteral("chamfer")) {
        QPainterPath path;
        path.moveTo(6, 25);
        path.lineTo(6, 12);
        path.lineTo(12, 6);
        path.lineTo(26, 6);
        p.drawPath(path);
        line(14, 25, 26, 13);
    }
    else if (id == QStringLiteral("shell")) {
        p.drawRoundedRect(QRectF(5, 6, 22, 20), 2, 2);
        p.drawRoundedRect(QRectF(9, 10, 14, 12), 1.5, 1.5);
        line(16, 6, 16, 10);
    }
    else if (id == QStringLiteral("draft")) {
        p.drawRect(QRectF(7, 8, 18, 16));
        line(7, 24, 12, 5);
        line(25, 24, 20, 5);
    }
    else if (id == QStringLiteral("pattern")) {
        for (int y : {8, 18}) {
            for (int x : {8, 18}) {
                p.drawRect(QRectF(x, y, 6, 6));
            }
        }
    }
    else if (id == QStringLiteral("mirror")) {
        line(16, 4, 16, 28);
        p.drawRect(QRectF(5, 9, 7, 14));
        p.drawRect(QRectF(20, 9, 7, 14));
    }
    else if (id == QStringLiteral("boolean")) {
        p.drawEllipse(QRectF(5, 8, 14, 14));
        p.drawEllipse(QRectF(13, 8, 14, 14));
    }
    else if (id == QStringLiteral("transform")) {
        line(16, 4, 16, 28);
        line(4, 16, 28, 16);
        line(16, 4, 12, 8); line(16, 4, 20, 8);
        line(28, 16, 24, 12); line(28, 16, 24, 20);
        line(16, 28, 12, 24); line(16, 28, 20, 24);
        line(4, 16, 8, 12); line(4, 16, 8, 20);
    }
    else if (id == QStringLiteral("line")) {
        line(6, 25, 26, 7);
        p.drawEllipse(QPointF(6, 25), 2, 2);
        p.drawEllipse(QPointF(26, 7), 2, 2);
    }
    else if (id == QStringLiteral("circle")) {
        p.drawEllipse(QPointF(16, 16), 10, 10);
        line(13, 16, 19, 16);
        line(16, 13, 16, 19);
    }
    else if (id == QStringLiteral("rectangle")) {
        p.drawRect(QRectF(5, 8, 22, 17));
        p.drawEllipse(QPointF(5, 8), 1.8, 1.8);
        p.drawEllipse(QPointF(27, 25), 1.8, 1.8);
    }
    else if (id == QStringLiteral("center_rectangle")) {
        p.drawRect(QRectF(5, 8, 22, 17));
        line(13, 16.5, 19, 16.5);
        line(16, 13.5, 16, 19.5);
    }
    else if (id == QStringLiteral("arc")) {
        QRectF r(6, 7, 20, 20);
        p.drawArc(r, 15 * 16, 150 * 16);
        p.drawEllipse(QPointF(25, 12), 1.8, 1.8);
        p.drawEllipse(QPointF(8, 20), 1.8, 1.8);
    }
    else if (id == QStringLiteral("spline")) {
        QPainterPath path;
        path.moveTo(4, 22);
        path.cubicTo(10, 4, 18, 28, 28, 8);
        p.drawPath(path);
        for (const QPointF pt : {QPointF(4,22), QPointF(13,12), QPointF(21,19), QPointF(28,8)}) {
            p.drawEllipse(pt, 1.6, 1.6);
        }
    }
    else if (id == QStringLiteral("dimension")) {
        line(5, 8, 27, 8);
        line(5, 5, 5, 12); line(27, 5, 27, 12);
        line(7, 8, 11, 6); line(7, 8, 11, 10);
        line(25, 8, 21, 6); line(25, 8, 21, 10);
        line(8, 23, 24, 23);
    }
    else if (id == QStringLiteral("construction")) {
        pen.setStyle(Qt::DashLine);
        p.setPen(pen);
        line(5, 24, 27, 8);
    }
    else if (id == QStringLiteral("trim")) {
        p.drawEllipse(QRectF(5, 7, 9, 9));
        p.drawEllipse(QRectF(5, 18, 9, 9));
        line(12, 13, 27, 25);
        line(12, 22, 27, 7);
    }
    else if (id == QStringLiteral("use")) {
        p.drawRect(QRectF(5, 7, 13, 13));
        line(13, 24, 27, 10);
        line(27, 10, 22, 10);
        line(27, 10, 27, 15);
    }
    else if (id == QStringLiteral("offset")) {
        QPainterPath a; a.moveTo(5,22); a.cubicTo(10,7,20,25,27,8); p.drawPath(a);
        QPainterPath b; b.moveTo(7,26); b.cubicTo(12,11,22,29,29,12); p.drawPath(b);
    }
    else if (id == QStringLiteral("constraints")) {
        p.drawEllipse(QPointF(9, 16), 4.5, 4.5);
        p.drawEllipse(QPointF(23, 16), 4.5, 4.5);
        line(13.5, 16, 18.5, 16);
    }
    else if (id == QStringLiteral("fit")) {
        line(5, 12, 5, 5); line(5, 5, 12, 5);
        line(20, 5, 27, 5); line(27, 5, 27, 12);
        line(5, 20, 5, 27); line(5, 27, 12, 27);
        line(20, 27, 27, 27); line(27, 20, 27, 27);
    }
    else if (id == QStringLiteral("planes")) {
        p.drawPolygon(QPolygonF{
            QPointF(5,18), QPointF(17,6), QPointF(27,12), QPointF(15,25)
        });
        line(16, 6, 16, 27);
    }
    else {
        p.drawRoundedRect(QRectF(6, 6, 20, 20), 3, 3);
        p.drawEllipse(QPointF(16, 16), 2, 2);
    }

    return QIcon(pixmap);
}

}  // namespace

QIcon ToolIconFactory::icon(const QString& id)
{
    return drawIcon(id);
}

}  // namespace freeshape::ui
