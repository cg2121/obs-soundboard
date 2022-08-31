#include <obs-frontend-api.h>
#include <obs-module.h>
#include "util/platform.h"
#include "obs-hotkey.h"
#include "util/util.hpp"
#include "plugin-macros.generated.h"
#include "soundboard.hpp"
#include "adv-audio-control.hpp"
#include <QAction>
#include <QMainWindow>
#include <QObject>
#include <QFileDialog>
#include <QStandardPaths>
#include <QMessageBox>
#include <QDockWidget>
#include <QMenu>
#include <QListWidget>

#include <scene-tree.hpp>
#include <volume-control.hpp>
#include <media-controls.hpp>

#include <sound-data.hpp>

#define QT_UTF8(str) QString::fromUtf8(str, -1)
#define QT_TO_UTF8(str) str.toUtf8().constData()
#define QTStr(str) QT_UTF8(obs_module_text(str))
#define MainStr(str) QT_UTF8(obs_frontend_get_locale_string(str))

SoundEdit::SoundEdit(QWidget *parent) : QDialog(parent), ui(new Ui_SoundEdit)
{
	ui->setupUi(this);
	ui->fileEdit->setReadOnly(true);
}

void SoundEdit::on_browseButton_clicked()
{
	QString folder = ui->fileEdit->text();

	if (!folder.isEmpty())
		folder = QFileInfo(folder).absoluteDir().absolutePath();
	else
		folder = QStandardPaths::writableLocation(
			QStandardPaths::HomeLocation);

	QString fileName = QFileDialog::getOpenFileName(
		this, QTStr("OpenAudioFile"), folder,
		("Audio (*.mp3 *.aac *.ogg *.wav *.flac)"));

	if (!fileName.isEmpty())
		ui->fileEdit->setText(fileName);
}

static bool NameExists(const QString &name)
{
	if (SoundData::FindSoundByName(name))
		return true;

	return false;
}

void SoundEdit::on_buttonBox_clicked(QAbstractButton *button)
{
	Soundboard *sb = static_cast<Soundboard *>(parent());

	if (!sb)
		return;

	QDialogButtonBox::ButtonRole val = ui->buttonBox->buttonRole(button);

	if (val == QDialogButtonBox::RejectRole) {
		goto close;
	} else if (val == QDialogButtonBox::AcceptRole) {
		if (ui->nameEdit->text() == "") {
			QMessageBox::warning(this, QTStr("EmptyName.Title"),
					     QTStr("EmptyName.Text"));
			return;
		}

		bool nameExists = false;

		if ((editMode && ui->nameEdit->text() != origText &&
		     NameExists(ui->nameEdit->text())) ||
		    (!editMode && NameExists(ui->nameEdit->text())))
			nameExists = true;

		if (nameExists) {
			QMessageBox::warning(this, QTStr("NameExists.Title"),
					     QTStr("NameExists.Text"));
			return;
		}

		if (ui->fileEdit->text() == "") {
			QMessageBox::warning(this, QTStr("NoFile.Title"),
					     QTStr("NoFile.Text"));
			return;
		}

		if (editMode)
			sb->EditSound(ui->nameEdit->text(),
				      ui->fileEdit->text(),
				      ui->loop->isChecked());
		else
			sb->AddSound(ui->nameEdit->text(), ui->fileEdit->text(),
				     ui->loop->isChecked(), nullptr);
	}

close:
	close();
}

static void OnSave(obs_data_t *saveData, bool saving, void *data)
{
	Soundboard *sb = static_cast<Soundboard *>(data);

	if (saving)
		sb->SaveSoundboard(saveData);
	else
		sb->LoadSoundboard(saveData);
}

static void OBSEvent(enum obs_frontend_event event, void *data)
{
	Soundboard *sb = static_cast<Soundboard *>(data);
	obs_source_t *source = nullptr;

	switch (event) {
	case OBS_FRONTEND_EVENT_FINISHED_LOADING:
	case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED:
		if (obs_obj_invalid(sb->source)) {
			source = obs_source_create_private(
				"ffmpeg_source",
				QT_TO_UTF8(QTStr("Soundboard")), nullptr);

			sb->ui->mediaControls->SetSource(source);
			sb->vol->SetSource(source);
			sb->source = source;
		}

		obs_set_output_source(8, sb->source);
		break;
	case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CLEANUP:
		sb->ClearSoundboard();
	default:
		break;
	};
}

static QListWidgetItem *FindItem(QListWidget *list, const QString &name)
{
	for (int i = 0; i < list->count(); i++) {
		QListWidgetItem *item = list->item(i);

		if (item->text() == name)
			return item;
	}

	return nullptr;
}

Soundboard::Soundboard(QWidget *parent) : QWidget(parent), ui(new Ui_Soundboard)
{
	ui->setupUi(this);
	ui->soundList->SetGridMode(true);

	setObjectName("Soundboard");

	for (QAction *x : ui->soundboardToolbar->actions()) {
		QWidget *temp = ui->soundboardToolbar->widgetForAction(x);

		for (QByteArray &y : x->dynamicPropertyNames()) {
			temp->setProperty(y, x->property(y));
		}
	}

	ui->actionAddSound->setToolTip(QTStr("AddSound"));
	ui->actionRemoveSound->setToolTip(QTStr("RemoveSound.Title"));
	ui->actionEditSound->setToolTip(QTStr("EditSound"));

	auto stop = [](void *data, obs_hotkey_id, obs_hotkey_t *,
		       bool pressed) {
		Soundboard *sb = static_cast<Soundboard *>(data);

		if (pressed)
			QMetaObject::invokeMethod(sb, "StopSound");
	};

	stopHotkey = obs_hotkey_register_frontend(
		"Soundboard.Stop", QT_TO_UTF8(QTStr("StopHotkey")), stop, this);

	auto restart = [](void *data, obs_hotkey_id, obs_hotkey_t *,
			  bool pressed) {
		Soundboard *sb = static_cast<Soundboard *>(data);

		if (pressed)
			QMetaObject::invokeMethod(sb, "RestartSound");
	};

	restartHotkey = obs_hotkey_register_frontend(
		"Soundboard.Restart", QT_TO_UTF8(QTStr("RestartHotkey")),
		restart, this);

	auto mute = [](void *data, obs_hotkey_pair_id, obs_hotkey_t *,
		       bool pressed) {
		Soundboard *sb = static_cast<Soundboard *>(data);
		bool muted = obs_source_muted(sb->source);

		if (!muted && pressed) {
			QMetaObject::invokeMethod(sb, "SoundboardSetMuted",
						  Qt::AutoConnection,
						  Q_ARG(bool, true));

			return true;
		}

		return false;
	};

	auto unmute = [](void *data, obs_hotkey_pair_id, obs_hotkey_t *,
			 bool pressed) {
		Soundboard *sb = static_cast<Soundboard *>(data);
		bool muted = obs_source_muted(sb->source);

		if (muted && pressed) {
			QMetaObject::invokeMethod(sb, "SoundboardSetMuted",
						  Qt::AutoConnection,
						  Q_ARG(bool, false));

			return true;
		}

		return false;
	};

	muteHotkeys = obs_hotkey_pair_register_frontend(
		"Soundboard.Mute", QT_TO_UTF8(QTStr("MuteHotkey")),
		"Soundboard.Unmute", QT_TO_UTF8(QTStr("UnmuteHotkey")), mute,
		unmute, this, this);

	auto play = [](void *data, obs_hotkey_pair_id, obs_hotkey_t *,
		       bool pressed) {
		Soundboard *sb = static_cast<Soundboard *>(data);
		bool paused = sb->ui->mediaControls->MediaPaused();

		if (paused && pressed) {
			QMetaObject::invokeMethod(sb, "PlaySound");
			return true;
		}

		return false;
	};

	auto pause = [](void *data, obs_hotkey_pair_id, obs_hotkey_t *,
			bool pressed) {
		Soundboard *sb = static_cast<Soundboard *>(data);
		bool paused = sb->ui->mediaControls->MediaPaused();

		if (!paused && pressed) {
			QMetaObject::invokeMethod(sb, "PauseSound");
			return true;
		}

		return false;
	};

	playPauseHotkeys = obs_hotkey_pair_register_frontend(
		"Soundboard.Play", QT_TO_UTF8(QTStr("PlayHotkey")),
		"Soundboard.Pause", QT_TO_UTF8(QTStr("PauseHotkey")), play,
		pause, this, this);

	vol = new VolControl(source, true, true);
	ui->horizontalLayout->addWidget(vol);
	QObject::connect(vol->config, SIGNAL(clicked()), this,
			 SLOT(VolControlContextMenu()));

	obs_frontend_add_event_callback(OBSEvent, this);
	obs_frontend_add_save_callback(OnSave, this);
}

Soundboard::~Soundboard()
{
	obs_hotkey_unregister(stopHotkey);
	obs_hotkey_unregister(restartHotkey);
	obs_hotkey_pair_unregister(muteHotkeys);
	obs_hotkey_pair_unregister(playPauseHotkeys);
	obs_frontend_remove_event_callback(OBSEvent, this);
	obs_frontend_remove_save_callback(OnSave, this);
}

obs_data_array_t *Soundboard::SaveSounds()
{
	obs_data_array_t *soundArray = obs_data_array_create();

	for (int i = 0; i < ui->soundList->count(); i++) {
		QListWidgetItem *item = ui->soundList->item(i);
		SoundData *sound = SoundData::FindSoundByName(item->text());

		OBSDataAutoRelease data = obs_data_create();
		obs_data_set_string(data, "name",
				    QT_TO_UTF8(SoundData::GetName(*sound)));
		obs_data_set_string(data, "path",
				    QT_TO_UTF8(SoundData::GetPath(*sound)));
		obs_data_set_bool(data, "loop",
				  SoundData::LoopingEnabled(*sound));

		OBSDataArrayAutoRelease hotkeyArray =
			obs_hotkey_save(SoundData::GetHotkey(*sound));
		obs_data_set_array(data, "sound_hotkey", hotkeyArray);

		obs_data_array_push_back(soundArray, data);
	}

	return soundArray;
}

void Soundboard::LoadSounds(obs_data_array_t *array)
{
	size_t num = obs_data_array_count(array);

	for (size_t i = 0; i < num; i++) {
		OBSDataAutoRelease data = obs_data_array_item(array, i);
		const char *name = obs_data_get_string(data, "name");
		const char *path = obs_data_get_string(data, "path");
		bool loop = obs_data_get_bool(data, "loop");

		AddSound(QT_UTF8(name), QT_UTF8(path), loop, data);
	}
}

void Soundboard::SaveSoundboard(obs_data_t *saveData)
{
	QMainWindow *window = (QMainWindow *)obs_frontend_get_main_window();
	QDockWidget *dock = static_cast<QDockWidget *>(parent());

	OBSDataArrayAutoRelease array = SaveSounds();
	obs_data_set_array(saveData, "soundboard_array", array);

	if (!obs_obj_invalid(source)) {
		OBSDataAutoRelease sourceData = obs_save_source(source);
		obs_data_set_obj(saveData, "soundboard_source", sourceData);
	}

	OBSDataArrayAutoRelease hotkeyArray = obs_hotkey_save(stopHotkey);
	obs_data_set_array(saveData, "stop_hotkey", hotkeyArray);

	hotkeyArray = obs_hotkey_save(restartHotkey);
	obs_data_set_array(saveData, "restart_hotkey", hotkeyArray);

	obs_data_array_t *data0 = nullptr;
	obs_data_array_t *data1 = nullptr;

	obs_hotkey_pair_save(muteHotkeys, &data0, &data1);
	obs_data_set_array(saveData, "mute_hotkey", data0);
	obs_data_set_array(saveData, "unmute_hotkey", data1);

	obs_data_array_release(data0);
	obs_data_array_release(data1);

	obs_data_array_t *data2 = nullptr;
	obs_data_array_t *data3 = nullptr;

	obs_hotkey_pair_save(playPauseHotkeys, &data2, &data3);
	obs_data_set_array(saveData, "play_hotkey", data2);
	obs_data_set_array(saveData, "pause_hotkey", data3);

	obs_data_array_release(data2);
	obs_data_array_release(data3);

	obs_data_set_bool(saveData, "dock_visible", dock->isVisible());
	obs_data_set_bool(saveData, "dock_floating", dock->isFloating());
	obs_data_set_string(saveData, "dock_geometry",
			    dock->saveGeometry().toBase64().constData());
	obs_data_set_int(saveData, "dock_area", window->dockWidgetArea(dock));
	obs_data_set_bool(saveData, "grid_mode", ui->soundList->GetGridMode());
	obs_data_set_bool(saveData, "lock_volume", lockVolume);

	obs_data_set_bool(saveData, "props_use_percent", usePercent);

	QListWidgetItem *item = ui->soundList->currentItem();

	if (item)
		obs_data_set_string(saveData, "current_sound",
				    QT_TO_UTF8(item->text()));

	obs_data_set_bool(saveData, "use_countdown",
			  ui->mediaControls->countDownTimer);
}

void Soundboard::LoadSource(obs_data_t *saveData)
{
	OBSDataAutoRelease sourceData =
		obs_data_get_obj(saveData, "soundboard_source");

	if (sourceData) {
		obs_data_set_obj(sourceData, "settings", nullptr);
		source = obs_load_private_source(sourceData);

		if (obs_obj_invalid(source))
			return;

		ui->mediaControls->SetSource(source);
		vol->SetSource(source);
	}
}

void Soundboard::LoadSoundboard(obs_data_t *saveData)
{
	QMainWindow *window = (QMainWindow *)obs_frontend_get_main_window();
	QDockWidget *dock = static_cast<QDockWidget *>(parent());

	LoadSource(saveData);

	OBSDataArrayAutoRelease array =
		obs_data_get_array(saveData, "soundboard_array");
	LoadSounds(array);

	OBSDataArrayAutoRelease hotkeyArray =
		obs_data_get_array(saveData, "stop_hotkey");
	obs_hotkey_load(stopHotkey, hotkeyArray);

	hotkeyArray = obs_data_get_array(saveData, "restart_hotkey");
	obs_hotkey_load(restartHotkey, hotkeyArray);

	OBSDataArrayAutoRelease data0 =
		obs_data_get_array(saveData, "mute_hotkey");
	OBSDataArrayAutoRelease data1 =
		obs_data_get_array(saveData, "unmute_hotkey");
	obs_hotkey_pair_load(muteHotkeys, data0, data1);

	data0 = obs_data_get_array(saveData, "play_hotkey");
	data1 = obs_data_get_array(saveData, "pause_hotkey");
	obs_hotkey_pair_load(playPauseHotkeys, data0, data1);

	const char *geometry = obs_data_get_string(saveData, "dock_geometry");

	if (geometry && *geometry)
		dock->restoreGeometry(
			QByteArray::fromBase64(QByteArray(geometry)));

	const auto dockArea = static_cast<Qt::DockWidgetArea>(
		obs_data_get_int(saveData, "dock_area"));

	if (dockArea)
		window->addDockWidget(dockArea, dock);

	bool visible = obs_data_get_bool(saveData, "dock_visible");
	dock->setVisible(visible);

	obs_data_set_default_bool(saveData, "dock_floating", true);
	bool floating = obs_data_get_bool(saveData, "dock_floating");
	dock->setFloating(floating);

	obs_data_set_default_bool(saveData, "grid_mode", true);
	bool grid = obs_data_get_bool(saveData, "grid_mode");
	ui->soundList->SetGridMode(grid);

	bool lock = obs_data_get_bool(saveData, "lock_volume");
	ToggleLockVolume(lock);

	bool percent = obs_data_get_bool(saveData, "props_use_percent");
	usePercent = percent;

	QString lastSound =
		QT_UTF8(obs_data_get_string(saveData, "current_sound"));

	if (!lastSound.isEmpty()) {
		QListWidgetItem *item = FindItem(ui->soundList, lastSound);

		if (item)
			ui->soundList->setCurrentItem(item);
		else
			ui->soundList->setCurrentRow(0);
	} else {
		ui->soundList->setCurrentRow(0);
	}

	bool countdown = obs_data_get_bool(saveData, "use_countdown");
	ui->mediaControls->countDownTimer = countdown;
}

void Soundboard::ClearSoundboard()
{
	ui->mediaControls->countDownTimer = false;
	ui->mediaControls->SetSource(nullptr);
	vol->SetSource(nullptr);
	source = nullptr;

	ui->soundList->setCurrentItem(nullptr, QItemSelectionModel::Clear);

	for (int i = 0; i < ui->soundList->count(); i++) {
		QListWidgetItem *item = ui->soundList->item(i);
		delete item;
		item = nullptr;
	}

	std::vector<SoundData *> sounds = SoundData::GetSounds();

	for (auto &sound : sounds) {
		delete sound;
		sound = nullptr;
	}

	sounds.clear();
	ui->soundList->clear();
}

void Soundboard::PlaySound(const QString &name)
{
	prevSound = name;

	if (name.isEmpty()) {
		OBSDataAutoRelease settings = obs_source_get_settings(source);
		obs_source_reset_settings(source, settings);
		obs_source_update(source, settings);
		return;
	}

	SoundData *sound = SoundData::FindSoundByName(name);

	if (!sound)
		return;

	QListWidgetItem *item = FindItem(ui->soundList, name);
	QString path = SoundData::GetPath(*sound);

	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_bool(settings, "looping",
			  SoundData::LoopingEnabled(*sound));
	obs_data_set_string(settings, "local_file", QT_TO_UTF8(path));
	obs_data_set_bool(settings, "is_local_file", true);
	obs_source_update(source, settings);

	ui->soundList->setCurrentItem(item);
}

static void PlaySoundHotkey(void *data, obs_hotkey_id, obs_hotkey_t *,
			    bool pressed)
{
	SoundData *sound = static_cast<SoundData *>(data);

	QMainWindow *window = (QMainWindow *)obs_frontend_get_main_window();
	Soundboard *sb = window->findChild<Soundboard *>("Soundboard");

	if (pressed)
		QMetaObject::invokeMethod(sb, "PlaySound", Qt::AutoConnection,
					  Q_ARG(QString,
						SoundData::GetName(*sound)));
}

void Soundboard::AddSound(const QString &name, const QString &path, bool loop,
			  obs_data_t *settings)
{
	if (name.isEmpty() || path.isEmpty())
		return;

	QListWidgetItem *item = new QListWidgetItem(name);

	SoundData *sound = new SoundData(name, path, loop);

	QString hotkeyName = QTStr("SoundHotkey").arg(name);
	obs_hotkey_id hotkey = obs_hotkey_register_frontend(
		QT_TO_UTF8(hotkeyName), QT_TO_UTF8(hotkeyName), PlaySoundHotkey,
		sound);
	SoundData::SetHotkey(*sound, hotkey);

	if (settings) {
		OBSDataArrayAutoRelease hotkeyArray =
			obs_data_get_array(settings, "sound_hotkey");
		obs_hotkey_load(SoundData::GetHotkey(*sound), hotkeyArray);
	}

	ui->soundList->addItem(item);
	ui->soundList->setCurrentItem(item);
}

void Soundboard::EditSound(const QString &newName, const QString &newPath,
			   bool loop)
{
	if (newName.isEmpty() || newPath.isEmpty())
		return;

	QListWidgetItem *si = ui->soundList->currentItem();

	if (!si)
		return;

	SoundData *sound = SoundData::FindSoundByName(si->text());

	SoundData::SetPath(*sound, newPath);
	SoundData::SetName(*sound, newName);
	SoundData::SetLoopingEnabled(*sound, loop);

	si->setText(newName);

	QString hotkeyName = QTStr("SoundHotkey").arg(newName);
	obs_hotkey_id hotkey = SoundData::GetHotkey(*sound);
	obs_hotkey_set_name(hotkey, QT_TO_UTF8(hotkeyName));
	obs_hotkey_set_description(hotkey, QT_TO_UTF8(hotkeyName));
	PlaySound("");
}

static QString GetDefaultString(const QString &name)
{
	if (!NameExists(name))
		return name;

	int i = 2;

	for (;;) {
		QString out = name + " " + QString::number(i);

		if (!NameExists(out))
			return out;

		i++;
	}
}

void Soundboard::on_actionAddSound_triggered()
{
	SoundEdit edit(this);
	OBSAdvAudioCtrl advAudio(&edit, source, usePercent);
	edit.ui->nameEdit->setText(GetDefaultString(QTStr("Sound")));
	edit.setWindowTitle(QTStr("AddSound"));
	edit.ui->advAudioLayout->addWidget(&advAudio);
	edit.adjustSize();
	edit.exec();
}

void Soundboard::on_actionEditSound_triggered()
{
	QListWidgetItem *si = ui->soundList->currentItem();

	if (!si)
		return;

	SoundEdit edit(this);
	OBSAdvAudioCtrl advAudio(&edit, source, usePercent);
	edit.setWindowTitle(QTStr("EditSound"));
	edit.origText = si->text();
	edit.ui->advAudioLayout->addWidget(&advAudio);
	edit.editMode = true;

	SoundData *sound = SoundData::FindSoundByName(si->text());
	edit.ui->nameEdit->setText(SoundData::GetName(*sound));
	edit.ui->fileEdit->setText(SoundData::GetPath(*sound));
	edit.ui->loop->setChecked(SoundData::LoopingEnabled(*sound));

	edit.adjustSize();
	edit.exec();

	edit.origText = "";
}

void Soundboard::on_soundList_itemClicked(QListWidgetItem *item)
{
	QString text = item->text();

	if (prevSound == text)
		text = "";

	PlaySound(text);
}

void Soundboard::on_actionRemoveSound_triggered()
{
	QListWidgetItem *si = ui->soundList->currentItem();

	if (!si)
		return;

	QMessageBox::StandardButton reply =
		QMessageBox::question(this, QTStr("RemoveSound.Title"),
				      QTStr("RemoveSound.Text").arg(si->text()),
				      QMessageBox::Yes | QMessageBox::No);

	if (reply == QMessageBox::No)
		return;

	SoundData *sound = SoundData::FindSoundByName(si->text());

	if (!sound)
		return;

	delete sound;
	sound = nullptr;

	delete si;
	si = nullptr;

	PlaySound("");
}

void Soundboard::PlaySound()
{
	obs_source_media_play_pause(source, false);
}

void Soundboard::PauseSound()
{
	obs_source_media_play_pause(source, true);
}

void Soundboard::StopSound()
{
	PlaySound("");
}

void Soundboard::RestartSound()
{
	PlaySound(ui->soundList->currentItem()->text());
}

void Soundboard::SetGrid()
{
	bool grid = ui->soundList->GetGridMode();
	ui->soundList->SetGridMode(!grid);
}

void Soundboard::DuplicateSound()
{
	QListWidgetItem *si = ui->soundList->currentItem();

	if (!si)
		return;

	SoundData *sound = SoundData::FindSoundByName(si->text());
	AddSound(GetDefaultString(SoundData::GetName(*sound)),
		 SoundData::GetPath(*sound), SoundData::LoopingEnabled(*sound));
}

void Soundboard::on_soundList_customContextMenuRequested(const QPoint &pos)
{
	QListWidgetItem *item = ui->soundList->itemAt(pos);

	bool grid = ui->soundList->GetGridMode();

	QMenu popup(this);

	popup.addAction(MainStr("Add"), this,
			SLOT(on_actionAddSound_triggered()));
	popup.addAction(MainStr("Filters"), this, SLOT(OpenFilters()));
	popup.addSeparator();

	if (item) {
		popup.addAction(MainStr("Edit"), this,
				SLOT(on_actionEditSound_triggered()));
		popup.addAction(MainStr("Remove"), this,
				SLOT(on_actionRemoveSound_triggered()));
		popup.addAction(MainStr("Duplicate"), this,
				SLOT(DuplicateSound()));
		popup.addSeparator();
	}

	popup.addSeparator();
	popup.addAction(grid ? MainStr("Basic.Main.ListMode")
			     : MainStr("Basic.Main.GridMode"),
			this, SLOT(SetGrid()));

	popup.exec(QCursor::pos());
}

void Soundboard::SoundboardSetMuted(bool mute)
{
	obs_source_set_muted(source, mute);
}

void Soundboard::OpenFilters()
{
	obs_frontend_open_source_filters(source);
}

void Soundboard::ToggleLockVolume(bool checked)
{
	vol->EnableSlider(!checked);
	lockVolume = checked;
}

void Soundboard::VolControlContextMenu()
{
	QAction lockAction(MainStr("LockVolume"));
	lockAction.setCheckable(true);
	lockAction.setChecked(lockVolume);
	QObject::connect(&lockAction, SIGNAL(toggled(bool)), this,
			 SLOT(ToggleLockVolume(bool)));

	QMenu popup(this);
	popup.addAction(&lockAction);
	popup.addSeparator();
	popup.addAction(MainStr("Filters"), this, SLOT(OpenFilters()));
	popup.exec(QCursor::pos());
}

OBS_DECLARE_MODULE()
OBS_MODULE_AUTHOR("cg2121");
OBS_MODULE_USE_DEFAULT_LOCALE("Soundboard", "en-US")

bool obs_module_load(void)
{
	if (LIBOBS_API_VER < MAKE_SEMANTIC_VERSION(28, 0, 0)) {
		blog(LOG_ERROR,
		     "Soundboard plugin requires OBS 28.0.0 or newer");
		return false;
	}

	blog(LOG_INFO, "Soundboard plugin version %s is loaded",
	     PLUGIN_VERSION);

	return true;
}

void obs_module_post_load(void)
{
	QMainWindow *window = (QMainWindow *)obs_frontend_get_main_window();
	obs_frontend_push_ui_translation(obs_module_get_string);

	QDockWidget *dock = new QDockWidget(window);
	Soundboard *sb = new Soundboard(dock);
	dock->setWidget(sb);

	dock->setWindowTitle(QTStr("Soundboard"));
	dock->resize(400, 400);
	dock->setFloating(true);
	dock->hide();

	obs_frontend_add_dock(dock);

	obs_frontend_pop_ui_translation();
}

void obs_module_unload(void) {}

MODULE_EXPORT const char *obs_module_description(void)
{
	return obs_module_text("Description");
}

MODULE_EXPORT const char *obs_module_name(void)
{
	return obs_module_text("Soundboard");
}
