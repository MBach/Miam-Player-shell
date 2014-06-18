#ifndef MIAMPLAYERSHELL_H
#define MIAMPLAYERSHELL_H

#include "mediaplayerplugininterface.h"
#include "miamcore_global.h"
#include "mediaplayer.h"
#include "listwidget.h"

#include <QWidget>

#include "ui_config.h"

/**
 * \brief       The MiamPlayerShell class is a specific plugin for Windows which extends the Shell
 * \author      Matthieu Bachelier
 * \version     0.1
 * \copyright   GNU General Public License v3
 */
class MIAMCORE_LIBRARY MiamPlayerShell : public QWidget, public MediaPlayerPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID MediaPlayerPluginInterface_iid)
	Q_INTERFACES(MediaPlayerPluginInterface)

private:
	Ui::MiamPlayerShellConfigPage _config;
	QWeakPointer<MediaPlayer> _mediaPlayer;

public:
	MiamPlayerShell();

	virtual ~MiamPlayerShell() {}

	virtual QWidget* configPage();

	inline virtual bool isConfigurable() const { return true; }

	inline virtual QString name() const { return "MiamPlayerShell"; }

	inline virtual QString version() const { return "0.1"; }

	inline virtual bool providesView() const { return false; }

	void setMediaPlayer(QWeakPointer<MediaPlayer>);
};

#endif // MIAMPLAYERSHELL_H
