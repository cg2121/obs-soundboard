#pragma once

#include "obs.hpp"
#include <string>
#include <unordered_map>

class MediaObj {
private:
	static std::unordered_map<std::string, MediaObj *> uuidMap;
	static std::unordered_map<std::string, MediaObj *> nameMap;

	std::string uuid;

	std::string name = "";
	std::string path = "";
	bool loop = false;
	float volume = 1.0f;

	obs_hotkey_id hotkey = OBS_INVALID_HOTKEY_ID;

public:
	MediaObj(const std::string &name, const std::string &path);
	~MediaObj();

	static MediaObj *FindByUUID(const std::string &uuid);
	static MediaObj *FindByName(const std::string &name);

	std::string GetUUID();

	void SetName(const std::string &newName);
	std::string GetName();

	void SetPath(const std::string &newPath);
	std::string GetPath();

	void SetLoopEnabled(bool enable);
	bool LoopEnabled();

	void SetVolume(float volume);
	float GetVolume();
};
