#ifndef LISTWIDGET_H
#define LISTWIDGET_H

#include <QListWidget>

#include "miamcore_global.h"

class MIAMCORE_LIBRARY ListWidget : public QListWidget
{
	Q_OBJECT
public:
	Q_ENUMS(UserRole)

	ListWidget(QWidget *parent);

	enum UserRole{UR_ItemWithArrow		= Qt::UserRole + 1,
				  UR_ItemIsSeparator	= Qt::UserRole + 2,
				  UR_ItemPosition		= Qt::UserRole + 3,
				  UR_IsMovableToSubMenu	= Qt::UserRole + 4};

protected:
	void mouseMoveEvent(QMouseEvent *e);

	void paintEvent(QPaintEvent *);
};

class MIAMCORE_LIBRARY ListWidgetItem : public QListWidgetItem
{
public:
	ListWidgetItem(int i, const QString &text, QListWidget *parent) : QListWidgetItem(text, parent) {
		if (text.isEmpty()) {
			this->setData(ListWidget::UR_ItemIsSeparator, true);
		}
		this->setData(ListWidget::UR_ItemPosition, i);
	}

	ListWidgetItem(int i, const QIcon &icon, const QString &text, QListWidget *parent) : QListWidgetItem(icon, text, parent) {
		this->setData(ListWidget::UR_ItemPosition, i);
	}

	virtual bool operator<(const QListWidgetItem &other) const {
		return data(ListWidget::UR_ItemPosition).toInt() < other.data(ListWidget::UR_ItemPosition).toInt();
	}
};

#endif // LISTWIDGET_H
