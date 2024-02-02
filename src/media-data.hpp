#pragma once

#include "obs.hpp"
#include <QString>
#include <vector>

class MediaData {
private:
	static std::vector<MediaData *> mediaItems;
	QString name = "";
	QString path = "";
	obs_hotkey_id hotkey = OBS_INVALID_HOTKEY_ID;
	bool loop = false;

public:
	MediaData(const QString &name_, const QString &path_, bool loop_);
	~MediaData();

	static void SetName(MediaData &media, const QString &newName);
	static void SetPath(MediaData &media, const QString &newPath);
	static void SetHotkey(MediaData &media, const obs_hotkey_id &hotkey);

	static QString GetName(const MediaData &media);
	static QString GetPath(const MediaData &media);
	static obs_hotkey_id GetHotkey(const MediaData &media);
	static MediaData *FindMediaByName(const QString &name);

	static void SetLoopingEnabled(MediaData &media, bool loop);
	static bool LoopingEnabled(const MediaData &media);
	static std::vector<MediaData *> GetMedia();
};
