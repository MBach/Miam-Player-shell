#include "miamplayershell.h"

#include "settings.h"

#include <QDir>

#include <QtDebug>

MiamPlayerShell::MiamPlayerShell(QObject *parent)
	: MediaPlayerPlugin(parent)
	, _mediaPlayerControl(nullptr)
{}

MiamPlayerShell::~MiamPlayerShell()
{}

void MiamPlayerShell::cleanUpBeforeDestroy()
{
	// Disable shell extension
	Settings::instance()->setValue("MiamPlayerShell/IsActive", 0l);
}

void MiamPlayerShell::init()
{
	Settings *settings = Settings::instance();
	settings->beginGroup("MiamPlayerShell");
	settings->setValue("IsActive", 1l);
	settings->endGroup();
}

QWidget* MiamPlayerShell::configPage()
{
	QWidget *widget = new QWidget;
	_config.setupUi(widget);

	// Add items
	ListWidgetItem *open = new ListWidgetItem(0, tr("Open"), _config.menu);
	QFont f = open->font();
	f.setBold(true);
	open->setFont(f);
	_config.menu->addItem(open);
	_config.menu->addItem(new ListWidgetItem(1, tr("Open in new window"), _config.menu));
	_config.menu->addItem(new ListWidgetItem(2, "", _config.menu));
	_config.menu->addItem(new ListWidgetItem(3, tr("Share with"), _config.menu));
	_config.menu->addItem(new ListWidgetItem(4, QIcon(":/MiamPlayerShell/mp.ico"), "Miam-Player", _config.menu));
	_config.menu->addItem(new ListWidgetItem(9, tr("Include in Library"), _config.menu));
	_config.menu->addItem(new ListWidgetItem(10, tr("Pin to Start"), _config.menu));
	_config.menu->addItem(new ListWidgetItem(11, "", _config.menu));
	_config.menu->addItem(new ListWidgetItem(12, tr("Send to"), _config.menu));
	_config.menu->addItem(new ListWidgetItem(13, "", _config.menu));

	_config.subMenu->addItem(new ListWidgetItem(5, _config.sendToCurrentPlaylist->text(), _config.subMenu));
	_config.subMenu->addItem(new ListWidgetItem(6, _config.sendToNewPlaylist->text(), _config.subMenu));
	_config.subMenu->addItem(new ListWidgetItem(7, _config.sendToTagEditor->text(), _config.subMenu));
	_config.subMenu->addItem(new ListWidgetItem(8, _config.addToLibrary->text(), _config.subMenu));

	// Create fake arrow sign
	for (int i = 0; i < _config.menu->count(); i++) {
		if (i == 3 || i == 4 || i == 5 || i == 8) {
			_config.menu->item(i)->setData(ListWidget::UR_ItemWithArrow, true);
		}
	}

	// Mark items movable from one list to another
	for (int i = 0; i < _config.subMenu->count(); i++) {
		_config.subMenu->item(i)->setData(ListWidget::UR_IsMovableToSubMenu, true);
	}
	_config.subMenu->raise();

	Settings *settings = Settings::instance();

	// Swap items from one context menu to another
	connect(_config.radioButtonDisableSubMenu, &QCheckBox::toggled, this, &MiamPlayerShell::toggleSubMenu);
	connect(_config.sendToCurrentPlaylist, &QCheckBox::toggled, this, &MiamPlayerShell::toggleFeature);
	connect(_config.sendToNewPlaylist, &QCheckBox::toggled, this, &MiamPlayerShell::toggleFeature);
	connect(_config.sendToTagEditor, &QCheckBox::toggled, this, &MiamPlayerShell::toggleFeature);
	connect(_config.addToLibrary, &QCheckBox::toggled, this, &MiamPlayerShell::toggleFeature);

	// Init values and trigger signal
	if (!settings->value("MiamPlayerShell/hasSubMenu", true).toBool()) {
		_config.radioButtonDisableSubMenu->toggle();
	}

	// Hide last item
//	if (!settings->value("MiamPlayerShell/addToLibrary", 0).toInt()) {
//		_config.addToLibrary->toggle();
//	}

	for (QCheckBox *checkBox : _config.actionsGroupBox->findChildren<QCheckBox*>()) {
		settings->setValue("MiamPlayerShell/" + checkBox->objectName(), checkBox->text());
	}

	this->resizeListWidget(_config.menu);
	this->resizeListWidget(_config.subMenu);
	return widget;
}

/** Adjust height of fake Context Menu. */
void MiamPlayerShell::resizeListWidget(QListWidget *list)
{
	int h = 6;
	for (int i = 0; i < list->count(); i++) {
		if (list->item(i)->data(ListWidget::UR_ItemIsSeparator).toBool()) {
			h += 3;
		} else {
			h += 22;
		}
	}
	list->setMinimumHeight(h);
	list->setMaximumHeight(h);
}

void MiamPlayerShell::toggleFeature(bool enabled)
{
	QListWidget *list;
	int row;
	QCheckBox *checkBox = qobject_cast<QCheckBox*>(sender());

	Settings *settings = Settings::instance();
	if (settings->value("MiamPlayerShell/hasSubMenu").toBool()) {
		list = _config.subMenu;
		row = 0;
	} else {
		list = _config.menu;
		row = 4;
	}
	// Move items from one list to another
	static QList<QListWidgetItem*> cachedItems;
	if (enabled) {
		for (int i = 0; i < cachedItems.count(); i++) {
			QListWidgetItem *item = cachedItems.at(i);
			if (item->text() == checkBox->text()) {
				cachedItems.takeAt(i);
				list->addItem(item);
				break;
			}
		}
	} else {
		// Remove item
		for (int i = 0; i < list->count(); i++) {
			if (checkBox->text() == list->item(i)->text()) {
				cachedItems << list->takeItem(i);
				break;
			}
		}
	}
	settings->setValue("MiamPlayerShell/Has" + checkBox->objectName(), enabled ? 1l : 0l);

	this->resizeListWidget(list);
	list->sortItems();

	// Avoid to have all checkboxes unchecked: if one wants to uncheck all, he should disable the plugin instead
	QList<QCheckBox*> checkedOnes;
	QList<QCheckBox*> checkBoxes = QList<QCheckBox*>() << _config.sendToCurrentPlaylist << _config.sendToNewPlaylist
													   << _config.sendToTagEditor << _config.addToLibrary;
	for (QCheckBox *checkBox : checkBoxes) {
		if (checkBox->isChecked()) {
			checkedOnes << checkBox;
		}
	}
	// Only one checkbox is still checked, we need to protect it
	if (checkedOnes.size() == 1) {
		checkedOnes.first()->setEnabled(false);
	} else if (checkedOnes.size() > 1) {
		// Reset protected status
		for (QCheckBox *checkBox : checkBoxes) {
			checkBox->setEnabled(true);
		}
	}
}

void MiamPlayerShell::toggleSubMenu(bool disabled)
{
	_config.subMenu->setVisible(!disabled);
	if (disabled) {
		// Move items from Submenu to Menu
		QListWidgetItem *item = _config.menu->takeItem(4);
		for (int i = _config.subMenu->count() - 1; i >= 0; i--) {
			_config.menu->insertItem(4, _config.subMenu->takeItem(i));
		}
		_config.subMenu->addItem(item);
	} else {
		// Move back items from Menu to Submenu
		QListWidgetItem *item = _config.subMenu->takeItem(0);
		for (int i = _config.menu->count() - 1; i >= 0; i--) {
			QListWidgetItem *it = _config.menu->item(i);
			if (it->data(ListWidget::UR_IsMovableToSubMenu).toBool()) {
				_config.subMenu->insertItem(0, _config.menu->takeItem(i));
			}
		}
		_config.menu->insertItem(4, item);
	}
	Settings::instance()->setValue("MiamPlayerShell/HasSubMenu", disabled ? 0l : 1l);
	this->resizeListWidget(_config.menu);
	this->resizeListWidget(_config.subMenu);
}

void MiamPlayerShell::setMediaPlayerControl(MediaPlayerControl *mpc)
{
	_mediaPlayerControl = mpc;
}
