#include "miamplayershell.h"

#include "settings.h"

MiamPlayerShell::MiamPlayerShell()
	: QWidget(NULL)
{
}

QWidget* MiamPlayerShell::configPage()
{
	QWidget *widget = new QWidget(this);
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
	_config.menu->addItem(new ListWidgetItem(4, QIcon(":/MiamPlayerShell/mp.ico"), "Miam-PLayer", _config.menu));
	_config.menu->addItem(new ListWidgetItem(9, tr("Include in Library"), _config.menu));
	_config.menu->addItem(new ListWidgetItem(10, tr("Pin to Start"), _config.menu));
	_config.menu->addItem(new ListWidgetItem(11, "", _config.menu));
	_config.menu->addItem(new ListWidgetItem(12, tr("Send to"), _config.menu));
	_config.menu->addItem(new ListWidgetItem(13, "", _config.menu));

	_config.subMenu->addItem(new ListWidgetItem(5, _config.sendToCurrentPlaylist->text(), _config.subMenu));
	_config.subMenu->addItem(new ListWidgetItem(6, _config.sendToNewPlaylist->text(), _config.subMenu));
	_config.subMenu->addItem(new ListWidgetItem(7, _config.sendToEditor->text(), _config.subMenu));
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

	Settings *settings = Settings::getInstance();

	// Swap items from one context menu to another
	connect(_config.radioButtonDisableSubMenu, &QCheckBox::toggled, this, [=](bool b) {
		_config.subMenu->setVisible(!b);
		if (b) {
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
		settings->setValue("MiamPlayerShell/hasSubMenu", !b);
		this->resizeListWidget(_config.menu);
		this->resizeListWidget(_config.subMenu);
	});

	connect(_config.sendToCurrentPlaylist, &QCheckBox::toggled, this, &MiamPlayerShell::toggleFeature);
	connect(_config.sendToNewPlaylist, &QCheckBox::toggled, this, &MiamPlayerShell::toggleFeature);
	connect(_config.sendToEditor, &QCheckBox::toggled, this, &MiamPlayerShell::toggleFeature);
	connect(_config.addToLibrary, &QCheckBox::toggled, this, &MiamPlayerShell::toggleFeature);

	// Init values and trigger signal
	if (!settings->value("MiamPlayerShell/hasSubMenu", true).toBool()) {
		_config.radioButtonDisableSubMenu->toggle();
	}

	// Hide last item
	if (!settings->value("MiamPlayerShell/addToLibrary", false).toBool()) {
		_config.addToLibrary->toggle();
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
	Settings *settings = Settings::getInstance();
	settings->setValue("MiamPlayerShell/" + checkBox->objectName(), enabled);

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

	this->resizeListWidget(list);
	list->sortItems();

	// Avoid to have all checkboxes unchecked: if one wants to uncheck all, he should disable the plugin instead
	QList<QCheckBox*> checkedOnes;
	QList<QCheckBox*> checkBoxes = QList<QCheckBox*>() << _config.sendToCurrentPlaylist << _config.sendToNewPlaylist
													   << _config.sendToEditor << _config.addToLibrary;
	foreach (QCheckBox *checkBox, checkBoxes) {
		if (checkBox->isChecked()) {
			checkedOnes << checkBox;
		}
	}
	// Only one checkbox is still checked, we need to protect it
	if (checkedOnes.size() == 1) {
		checkedOnes.first()->setEnabled(false);
	} else if (checkedOnes.size() > 1) {
		// Reset protected status
		foreach (QCheckBox *checkBox, checkBoxes) {
			checkBox->setEnabled(true);
		}
	}
}

void MiamPlayerShell::setMediaPlayer(QWeakPointer<MediaPlayer> mediaPlayer)
{
	_mediaPlayer = mediaPlayer;
}
