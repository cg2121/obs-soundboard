#pragma once

#include "obs.hpp"
#include <QWidget>
#include <QPointer>
#include <memory>

#include "ui_Soundboard.h"

class QListWidget;
class SceneTree;
class MediaControls;

class Soundboard : public QWidget {
	Q_OBJECT

	friend class SoundEdit;

public:
	std::unique_ptr<Ui_Soundboard> ui;
	Soundboard(QWidget *parent = nullptr);
	~Soundboard();

	obs_data_array_t *SaveSounds();
	void LoadSounds(obs_data_array_t *array);
	void LoadSource(obs_data_t *saveData);
	void ClearSoundboard();

	OBSSourceAutoRelease source;

	void SaveSoundboard(obs_data_t *saveData);
	void LoadSoundboard(obs_data_t *saveData);

private:
	void AddSound(const QString &name, const QString &path, bool loop,
		      obs_data_t *settings = nullptr);
	void EditSound(const QString &newName, const QString &newPath,
		       bool loop);

	QString prevName = "";
	QString prevSound = "";

private slots:
	void on_soundList_itemClicked(QListWidgetItem *item);
	void on_actionAddSound_triggered();
	void on_actionRemoveSound_triggered();
	void on_actionEditSound_triggered();
	void SetGrid();
	void on_soundList_customContextMenuRequested(const QPoint &pos);
	void DuplicateSound();

	void PlaySound();
	void PauseSound();
	void RestartSound();
	void StopSound();

	void OpenFilters();

public slots:
	void SoundboardSetMuted(bool mute);
	void PlaySound(const QString &name);
};
