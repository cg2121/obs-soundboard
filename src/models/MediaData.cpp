#include "MediaData.hpp"
#include <util/platform.h>
#include <util/util.hpp>
#include <obs-module.h>

#define QTStr(str) QString(obs_module_text(str))
#define QT_UTF8(str) QString::fromUtf8(str, -1)
#define QT_TO_UTF8(str) str.toUtf8().constData()

std::vector<MediaObj *> MediaObj::mediaItems;

MediaObj::MediaObj(const QString &name_, const QString &path_) : name(name_), path(path_)
{
	BPtr<char> uuid_ = os_generate_uuid();
	uuid = uuid_.Get();

	QString hotkeyName = QTStr("SoundHotkey").arg(name);

	auto playSound = [](void *data, obs_hotkey_id, obs_hotkey_t *, bool pressed) {
		MediaObj *sound = static_cast<MediaObj *>(data);

		if (pressed)
			QMetaObject::invokeMethod(sound, &MediaObj::pressed);
		else
			QMetaObject::invokeMethod(sound, &MediaObj::released);
	};

	hotkey = obs_hotkey_register_frontend(QT_TO_UTF8(hotkeyName), QT_TO_UTF8(hotkeyName), playSound, this);

	mediaItems.emplace_back(this);
}

MediaObj::~MediaObj()
{
	obs_hotkey_unregister(hotkey);
	mediaItems.erase(std::remove(mediaItems.begin(), mediaItems.end(), this), mediaItems.end());
}

MediaObj *MediaObj::findByName(const QString &name)
{
	for (auto &media : mediaItems) {
		if (media->name == name)
			return media;
	}

	return nullptr;
}

MediaObj *MediaObj::findByUUID(const QString &uuid)
{
	for (auto &media : mediaItems) {
		if (media->uuid == uuid)
			return media;
	}

	return nullptr;
}

QString MediaObj::getUUID()
{
	return uuid;
}

void MediaObj::setName(const QString &newName)
{
	if (newName.isEmpty() || name == newName)
		return;

	name = newName;

	QString hotkeyName = QTStr("SoundHotkey").arg(name);
	obs_hotkey_set_name(hotkey, QT_TO_UTF8(hotkeyName));
	obs_hotkey_set_description(hotkey, QT_TO_UTF8(hotkeyName));

	emit renamed(this);
}

QString MediaObj::getName()
{
	return name;
}

void MediaObj::setPath(const QString &newPath)
{
	path = newPath;
}

QString MediaObj::getPath()
{
	return path;
}

obs_hotkey_id MediaObj::getHotkey()
{
	return hotkey;
}

void MediaObj::setLoopEnabled(bool enable)
{
	loop = enable;
}

bool MediaObj::loopEnabled()
{
	return loop;
}

void MediaObj::setVolume(float newVolume)
{
	volume = newVolume;
}

float MediaObj::getVolume()
{
	return volume;
}

void MediaObj::pressed()
{
	emit hotkeyPressed(this);
}

void MediaObj::released()
{
	emit hotkeyReleased(this);
}
