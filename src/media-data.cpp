#include "media-data.hpp"

std::vector<MediaData *> MediaData::mediaItems;

MediaData::MediaData(const QString &name_, const QString &path_, bool loop_)
	: name(name_), path(path_), loop(loop_)
{
	mediaItems.emplace_back(this);
}

MediaData::~MediaData()
{
	obs_hotkey_unregister(hotkey);
	mediaItems.erase(std::remove(mediaItems.begin(), mediaItems.end(),
				     this),
			 mediaItems.end());
}

QString MediaData::GetName(const MediaData &media)
{
	return media.name;
}

QString MediaData::GetPath(const MediaData &media)
{
	return media.path;
}

obs_hotkey_id MediaData::GetHotkey(const MediaData &media)
{
	return media.hotkey;
}

void MediaData::SetName(MediaData &media, const QString &newName)
{
	media.name = newName;
}

void MediaData::SetPath(MediaData &media, const QString &newPath)
{
	media.path = newPath;
}

void MediaData::SetHotkey(MediaData &media, const obs_hotkey_id &hotkey)
{
	media.hotkey = hotkey;
}

MediaData *MediaData::FindMediaByName(const QString &name)
{
	for (auto &media : mediaItems) {
		if (media->name == name)
			return media;
	}

	return nullptr;
}

void MediaData::SetLoopingEnabled(MediaData &media, bool loop)
{
	media.loop = loop;
}

bool MediaData::LoopingEnabled(const MediaData &media)
{
	return media.loop;
}

std::vector<MediaData *> MediaData::GetMedia()
{
	return mediaItems;
}
