#include "listwidget.h"

#include <QApplication>
#include <QGraphicsDropShadowEffect>
#include <QStylePainter>

#include <QtDebug>

ListWidget::ListWidget(QWidget *parent)
	: QListWidget(parent)
{
	this->setMouseTracking(true);
	this->setIconSize(QSize(18, 18));
	QGraphicsDropShadowEffect *dropShadow = new QGraphicsDropShadowEffect(this);
	dropShadow->setBlurRadius(4.0);
	dropShadow->setColor(QApplication::palette().mid().color());
	dropShadow->setOffset(QPoint(2, 2));
	this->setGraphicsEffect(dropShadow);
}

/** Redefined to force update the viewport. */
void ListWidget::mouseMoveEvent(QMouseEvent *e)
{
	this->viewport()->update();
	QListWidget::mouseMoveEvent(e);
}

/** Redefined to be able to display items with the current theme. */
void ListWidget::paintEvent(QPaintEvent *)
{
	QStylePainter p(this->viewport());
	static const int itemHeight = 22;

	p.fillRect(viewport()->rect(), QColor(255, 255, 255));

	// Subdirectories in the popup menu
	int h = 0;
	for (int i = 0; i < count(); i ++) {
		QListWidgetItem *it = item(i);
		QRect r(0, h, this->width(), itemHeight);
		if (it->text().isEmpty()) {
			r.setHeight(3);
			h += 3;
		} else {
			h += itemHeight;
		}

		if (it->text().isEmpty()) {
			p.save();
			p.setPen(QApplication::palette().midlight().color());
			p.drawLine(r.x() + 33, r.y() + 1, r.width(), r.y());
			p.restore();
			continue;
		}

		// Draw: Highlight, Icon, Arrow and Text
		QRect iconRect(r.x() + 6, r.y() + 2, iconSize().width(), iconSize().height());
		bool itemIsEnabled = true;
		bool isHighLighted = false;

		// Highlighted rectangle
		p.save();
		if (r.contains(mapFromGlobal(QCursor::pos()))) {
			p.setPen(QApplication::palette().highlight().color());
			p.setBrush(QApplication::palette().highlight().color().lighter());
			QRect highlight = r.adjusted(1, 0, -6, -1);
			p.drawRect(highlight);
			p.setPen(QColor(192, 192, 192, 128));
			p.drawLine(33, r.top() + 1, 33, r.bottom());
			isHighLighted = true;
		}
		p.restore();
		p.drawPixmap(iconRect, it->icon().pixmap(iconSize()));

		// Arrow
		if (it->data(ListWidget::UR_ItemWithArrow).toBool()) {
			QStyleOption o;
			o.initFrom(this);
			o.rect = QRect(width() - 20, r.y() + 7, 9, 9);
			/// XXX: create a triangle from 3 points to allow AntiAliasing because renderHint is not working on primitive
			p.drawPrimitive(QStyle::PE_IndicatorArrowRight, o);
		}

		// Text
		QRect textRect = r.adjusted(37, 0, 0, 0);
		QString text = fontMetrics().elidedText(it->text(), Qt::ElideRight, textRect.width());
		p.save();
		if (!itemIsEnabled) {
			p.setPen(QApplication::palette().color(QPalette::Disabled, QPalette::WindowText));
		}
		p.setFont(it->font());
		if (isHighLighted) {
			p.setPen(QApplication::palette().highlightedText().color());
		}
		p.drawText(textRect, text, Qt::AlignLeft | Qt::AlignVCenter);
		p.restore();
	}
}
