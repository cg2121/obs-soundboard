#include "media-data.hpp"
#include <util/platform.h>
#include <util/util.hpp>

std::unordered_map<std::string, MediaObj *> MediaObj::uuidMap;
std::unordered_map<std::string, MediaObj *> MediaObj::nameMap;

MediaObj::MediaObj(const std::string &name_, const std::string &path_)
	: name(name_),
	path(path_)
{
	BPtr<char> uuid_ = os_generate_uuid();
	uuid = uuid_.Get();
	uuidMap.insert({ uuid, this });

//	std::string hotkeyName = QTStr("SoundHotkey").arg(name);

//	auto playSound = [](void *data, obs_hotkey_id, obs_hotkey_t *,
//			    bool pressed)
//	{
//		MediaObj *sound = static_cast<MediaObj *>(data);

//		if (pressed)
//			QMetaObject::invokeMethod(sound, "PlaySound");
//	};

//	hotkey = obs_hotkey_register_frontend(QT_TO_UTF8(hotkeyName), QT_TO_UTF8(hotkeyName), playSound, this);

	nameMap.insert({ name, this });
}

MediaObj::~MediaObj()
{
	obs_hotkey_unregister(hotkey);
	nameMap.erase(name);
	uuidMap.erase(uuid);
}

MediaObj *MediaObj::FindByName(const std::string &name)
{
	auto it = nameMap.find(name);
	return it != nameMap.end() ? it->second : nullptr;
}

MediaObj *MediaObj::FindByUUID(const std::string &uuid)
{
	auto it = uuidMap.find(uuid);
	return it != uuidMap.end() ? it->second : nullptr;
}

std::string MediaObj::GetUUID()
{
	return uuid;
}

void MediaObj::SetName(const std::string &newName)
{
	if (name.empty() || name == newName)
		return;

	nameMap.erase(name);

	name = newName;

//	std::string hotkeyName = QTStr("SoundHotkey").arg(name);
//	obs_hotkey_set_name(hotkey, QT_TO_UTF8(hotkeyName));
//	obs_hotkey_set_description(hotkey, QT_TO_UTF8(hotkeyName));

	nameMap.insert({ name, this });
}

std::string MediaObj::GetName()
{
	return name;
}

void MediaObj::SetPath(const std::string &newPath)
{
	path = newPath;
}

std::string MediaObj::GetPath()
{
	return path;
}

void MediaObj::SetLoopEnabled(bool enable)
{
	loop = enable;
}

bool MediaObj::LoopEnabled()
{
	return loop;
}

void MediaObj::SetVolume(float newVolume)
{
	volume = newVolume;
}

float MediaObj::GetVolume()
{
	return volume;
}

//void MediaObj::PlaySound()
//{
//	emit Play(this);
//}
