#ifndef LISTWIDGET_H
#define LISTWIDGET_H

#include <QListWidget>

#include "miamcore_global.h"

class MIAMCORE_LIBRARY ListWidget : public QListWidget
{
public:
	ListWidget(QWidget *parent);

protected:
	void mouseMoveEvent(QMouseEvent *e);

	void paintEvent(QPaintEvent *);
};

#endif // LISTWIDGET_H
