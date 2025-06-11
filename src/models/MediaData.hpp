#pragma once

#include <obs.hpp>

#include <QObject>
#include <vector>

class MediaObj : public QObject {
	Q_OBJECT

private:
	static std::vector<MediaObj *> mediaItems;

	QString uuid;

	QString name = "";
	QString path = "";
	bool loop = false;
	float volume = 1.0f;

	obs_hotkey_id hotkey = OBS_INVALID_HOTKEY_ID;

private slots:
	void pressed();
	void released();

public:
	MediaObj(const QString &name, const QString &path);
	~MediaObj();

	static MediaObj *findByUUID(const QString &uuid);
	static MediaObj *findByName(const QString &name);

	QString getUUID();

	void setName(const QString &newName);
	QString getName();

	void setPath(const QString &newPath);
	QString getPath();

	obs_hotkey_id getHotkey();

	void setLoopEnabled(bool enable);
	bool loopEnabled();

	void setVolume(float volume);
	float getVolume();

signals:
	void hotkeyPressed(MediaObj *obj);
	void hotkeyReleased(MediaObj *obj);

	void renamed(MediaObj *obj);
};
