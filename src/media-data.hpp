#pragma once

#include "obs.hpp"
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
	void PlaySound();

public:
	MediaObj(const QString &name, const QString &path);
	~MediaObj();

	static MediaObj *FindByUUID(const QString &uuid);
	static MediaObj *FindByName(const QString &name);

	QString GetUUID();

	void SetName(const QString &newName);
	QString GetName();

	void SetPath(const QString &newPath);
	QString GetPath();

	obs_hotkey_id GetHotkey();

	void SetLoopEnabled(bool enable);
	bool LoopEnabled();

	void SetVolume(float volume);
	float GetVolume();

signals:
	void Play(MediaObj *obj);
	void Renamed(MediaObj *obj, QString newName);
};
