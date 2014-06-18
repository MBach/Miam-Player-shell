#include "miamplayershell.h"

MiamPlayerShell::MiamPlayerShell()
	: QWidget(NULL)
{
}

QWidget* MiamPlayerShell::configPage()
{
	QWidget *widget = new QWidget();
	_config.setupUi(widget);
	_config.listWidget_2->raise();

	return widget;
}

void MiamPlayerShell::setMediaPlayer(QWeakPointer<MediaPlayer> mediaPlayer)
{
	_mediaPlayer = mediaPlayer;

	connect(_config.sendToCurrentPlaylistCheckBox, &QCheckBox::toggled, this, [=]() {

	});
}
