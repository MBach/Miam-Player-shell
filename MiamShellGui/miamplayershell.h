#ifndef MIAMPLAYERSHELL_H
#define MIAMPLAYERSHELL_H

#include "interfaces/mediaplayerplugin.h"
#include "miamcore_global.h"
#include "mediaplayer.h"
#include "listwidget.h"

#include <QWidget>

#include "ui_config.h"

/**
 * \brief       The MiamPlayerShell class is a specific plugin for Windows which extends the Shell
 * \author      Matthieu Bachelier
 * \version     0.3
 * \copyright   GNU General Public License v3
 */
class MIAMCORE_LIBRARY MiamPlayerShell : public MediaPlayerPlugin
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID MediaPlayerPlugin_iid)
	Q_INTERFACES(MediaPlayerPlugin)

private:
	Ui::MiamPlayerShellConfigPage _config;
	MediaPlayerControl *_mediaPlayerControl;

public:
	explicit MiamPlayerShell(QObject *parent = nullptr);

	virtual ~MiamPlayerShell();

	virtual void cleanUpBeforeDestroy();

	/** Reimplemented from BasicPlugin. */
	virtual QWidget* configPage();

	/** Reimplemented from MediaPlayerPlugin. */
	virtual QStringList extensions() const override { return QStringList(); }

	/** Reimplemented from BasicPlugin. */
	virtual void init() override;

	/** Reimplemented from BasicPlugin. */
	inline virtual bool isConfigurable() const override { return true; }

	/** Reimplemented from MediaPlayerPlugin. */
	inline virtual bool hasView() const override { return false; }

	/** Reimplemented from BasicPlugin. */
	inline virtual QString name() const { return "MiamPlayerShell"; }

	/** Reimplemented from BasicPlugin. */
	inline virtual QString version() const { return "0.3"; }

	/** Reimplemented from MediaPlayerPlugin. */
	virtual void setMediaPlayerControl(MediaPlayerControl *) override;

private:
	/** Adjust height of fake Context Menu. */
	void resizeListWidget(QListWidget *list);

private slots:
	void toggleFeature(bool enabled);

	void toggleSubMenu(bool disabled);
};

#endif // MIAMPLAYERSHELL_H
