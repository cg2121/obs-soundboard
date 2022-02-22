#pragma once

#include "obs.hpp"
#include <QWidget>
#include <QListWidget>
#include <QStyledItemDelegate>
#include <vector>
#include <string>
#include <memory>
#include <scene-tree.hpp>
#include <volume-control.hpp>
#include <media-controls.hpp>

#include "ui_Soundboard.h"
#include "ui_SoundEdit.h"

struct SoundData {
	QString name = "";
	QString path = "";
	obs_hotkey_id hotkey = OBS_INVALID_HOTKEY_ID;
};

class ListRenameDelegate : public QStyledItemDelegate {
	Q_OBJECT

public:
	ListRenameDelegate(QObject *parent);
	virtual void setEditorData(QWidget *editor,
				   const QModelIndex &index) const override;

protected:
	virtual bool eventFilter(QObject *editor, QEvent *event) override;
};

class SoundEdit : public QDialog {
	Q_OBJECT

public:
	std::unique_ptr<Ui_SoundEdit> ui;
	SoundEdit(QWidget *parent = nullptr);

	bool editMode = false;
	QString origText = "";

public slots:
	void on_browseButton_clicked();
	void on_buttonBox_clicked(QAbstractButton *button);
};

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
	obs_hotkey_id stopHotkey = OBS_INVALID_HOTKEY_ID;
	obs_hotkey_id restartHotkey = OBS_INVALID_HOTKEY_ID;
	obs_hotkey_pair_id muteHotkeys = OBS_INVALID_HOTKEY_PAIR_ID;
	obs_hotkey_pair_id playPauseHotkeys = OBS_INVALID_HOTKEY_PAIR_ID;

	bool lockVolume = false;
	bool usePercent = false;
	bool editActive = false;

	void SaveSoundboard(obs_data_t *saveData);
	void LoadSoundboard(obs_data_t *saveData);

	MediaControls *mediaControls = nullptr;
	VolControl *vol = nullptr;

	std::vector<SoundData *> sounds;
	QString GetSoundName(SoundData *sound);
	QString GetSoundPath(SoundData *sound);
	void SetSoundName(SoundData *sound, const QString &newName);
	void SetSoundPath(SoundData *sound, const QString &newPath);
	obs_hotkey_id GetSoundHotkey(SoundData *sound);
	SoundData *FindSoundByName(const QString &name);

private:
	void AddSound(const QString &name, const QString &path,
		      obs_data_t *settings = nullptr);
	void EditSound(const QString &newName, const QString &newPath);

	QString prevName = "";

private slots:
	void on_soundList_itemClicked(QListWidgetItem *item);
	void on_actionAddSound_triggered();
	void on_actionRemoveSound_triggered();
	void on_actionEditSound_triggered();
	void SetGrid();
	void on_soundList_customContextMenuRequested(const QPoint &pos);
	void DuplicateSound();
	void SoundboardSetMuted(bool mute);
	void OpenFilters();
	void OpenAdvAudio();
	void ToggleLockVolume(bool checked);
	void EditSoundListItem();
	void SoundListNameEdited(QWidget *editor,
				 QAbstractItemDelegate::EndEditHint endHint);

	void PlaySound();
	void PauseSound();
	void RestartSound();
	void StopSound();

	void VolumeControlsToggled(bool checked);
	void MediaControlsToggled(bool checked);
	void ToolbarToggled(bool checked);

public slots:
	void PlaySound(const QString &name);
	void VolControlContextMenu();
	void ResetWidgets();
};
