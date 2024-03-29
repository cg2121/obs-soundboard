#include <obs-frontend-api.h>
#include <obs-module.h>
#include "util/platform.h"
#include "obs-hotkey.h"
#include "util/util.hpp"
#include "plugin-support.h"
#include "soundboard.hpp"
#include "sound-edit.hpp"
#include <QAction>
#include <QMainWindow>
#include <QObject>
#include <QMessageBox>
#include <QDockWidget>
#include <QMenu>
#include <QListWidget>

#include <scene-tree.hpp>
#include <media-controls.hpp>

#include <media-data.hpp>

#define QT_UTF8(str) QString::fromUtf8(str, -1)
#define QT_TO_UTF8(str) str.toUtf8().constData()
#define QTStr(str) QT_UTF8(obs_module_text(str))
#define MainStr(str) QT_UTF8(obs_frontend_get_locale_string(str))

static bool NameExists(const QString &name)
{
	if (MediaData::FindMediaByName(name))
		return true;

	return false;
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
			source = obs_source_create(
				"ffmpeg_source",
				QT_TO_UTF8(QTStr("Soundboard")), nullptr,
				nullptr);
			obs_source_set_hidden(source, true);

			sb->ui->mediaControls->SetSource(source);
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

	obs_frontend_add_event_callback(OBSEvent, this);
	obs_frontend_add_save_callback(OnSave, this);
}

Soundboard::~Soundboard()
{
	obs_frontend_remove_event_callback(OBSEvent, this);
	obs_frontend_remove_save_callback(OnSave, this);
}

obs_data_array_t *Soundboard::SaveSounds()
{
	obs_data_array_t *soundArray = obs_data_array_create();

	for (int i = 0; i < ui->soundList->count(); i++) {
		QListWidgetItem *item = ui->soundList->item(i);
		MediaData *sound = MediaData::FindMediaByName(item->text());

		OBSDataAutoRelease d = obs_data_create();
		obs_data_set_string(d, "name",
				    QT_TO_UTF8(MediaData::GetName(*sound)));
		obs_data_set_string(d, "path",
				    QT_TO_UTF8(MediaData::GetPath(*sound)));
		obs_data_set_bool(d, "loop", MediaData::LoopingEnabled(*sound));

		OBSDataArrayAutoRelease hotkeyArray =
			obs_hotkey_save(MediaData::GetHotkey(*sound));
		obs_data_set_array(d, "sound_hotkey", hotkeyArray);

		obs_data_array_push_back(soundArray, d);
	}

	return soundArray;
}

void Soundboard::LoadSounds(obs_data_array_t *array)
{
	size_t num = obs_data_array_count(array);

	for (size_t i = 0; i < num; i++) {
		OBSDataAutoRelease d = obs_data_array_item(array, i);
		const char *name = obs_data_get_string(d, "name");
		const char *path = obs_data_get_string(d, "path");
		bool loop = obs_data_get_bool(d, "loop");

		AddSound(QT_UTF8(name), QT_UTF8(path), loop, d);
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

	obs_data_set_bool(saveData, "dock_visible", dock->isVisible());
	obs_data_set_bool(saveData, "dock_floating", dock->isFloating());
	obs_data_set_string(saveData, "dock_geometry",
			    dock->saveGeometry().toBase64().constData());
	obs_data_set_int(saveData, "dock_area", window->dockWidgetArea(dock));
	obs_data_set_bool(saveData, "grid_mode", ui->soundList->GetGridMode());

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
		source = obs_load_source(sourceData);
		obs_source_set_hidden(source, true);

		if (obs_obj_invalid(source))
			return;

		ui->mediaControls->SetSource(source);
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
	source = nullptr;

	ui->soundList->setCurrentItem(nullptr, QItemSelectionModel::Clear);

	for (int i = 0; i < ui->soundList->count(); i++) {
		QListWidgetItem *item = ui->soundList->item(i);
		delete item;
		item = nullptr;
	}

	std::vector<MediaData *> sounds = MediaData::GetMedia();

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

	MediaData *sound = MediaData::FindMediaByName(name);

	if (!sound)
		return;

	QListWidgetItem *item = FindItem(ui->soundList, name);
	QString path = MediaData::GetPath(*sound);

	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_bool(settings, "looping",
			  MediaData::LoopingEnabled(*sound));
	obs_data_set_string(settings, "local_file", QT_TO_UTF8(path));
	obs_data_set_bool(settings, "is_local_file", true);
	obs_source_update(source, settings);

	ui->soundList->setCurrentItem(item);
}

static void PlaySoundHotkey(void *data, obs_hotkey_id, obs_hotkey_t *,
			    bool pressed)
{
	MediaData *sound = static_cast<MediaData *>(data);

	QMainWindow *window = (QMainWindow *)obs_frontend_get_main_window();
	Soundboard *sb = window->findChild<Soundboard *>("Soundboard");

	if (pressed)
		QMetaObject::invokeMethod(sb, "PlaySound", Qt::AutoConnection,
					  Q_ARG(QString,
						MediaData::GetName(*sound)));
}

void Soundboard::AddSound(const QString &name, const QString &path, bool loop,
			  obs_data_t *settings)
{
	if (name.isEmpty() || path.isEmpty())
		return;

	QListWidgetItem *item = new QListWidgetItem(name);

	MediaData *sound = new MediaData(name, path, loop);

	QString hotkeyName = QTStr("SoundHotkey").arg(name);
	obs_hotkey_id hotkey = obs_hotkey_register_frontend(
		QT_TO_UTF8(hotkeyName), QT_TO_UTF8(hotkeyName), PlaySoundHotkey,
		sound);
	MediaData::SetHotkey(*sound, hotkey);

	if (settings) {
		OBSDataArrayAutoRelease hotkeyArray =
			obs_data_get_array(settings, "sound_hotkey");
		obs_hotkey_load(MediaData::GetHotkey(*sound), hotkeyArray);
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

	MediaData *sound = MediaData::FindMediaByName(si->text());

	MediaData::SetPath(*sound, newPath);
	MediaData::SetName(*sound, newName);
	MediaData::SetLoopingEnabled(*sound, loop);

	si->setText(newName);

	QString hotkeyName = QTStr("SoundHotkey").arg(newName);
	obs_hotkey_id hotkey = MediaData::GetHotkey(*sound);
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
	edit.ui->nameEdit->setText(GetDefaultString(QTStr("Sound")));
	edit.setWindowTitle(QTStr("AddSound"));
	edit.adjustSize();
	edit.exec();
}

void Soundboard::on_actionEditSound_triggered()
{
	QListWidgetItem *si = ui->soundList->currentItem();

	if (!si)
		return;

	SoundEdit edit(this);
	edit.setWindowTitle(QTStr("EditSound"));
	edit.origText = si->text();
	edit.editMode = true;

	MediaData *sound = MediaData::FindMediaByName(si->text());
	edit.ui->nameEdit->setText(MediaData::GetName(*sound));
	edit.ui->fileEdit->setText(MediaData::GetPath(*sound));
	edit.ui->loop->setChecked(MediaData::LoopingEnabled(*sound));

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

	MediaData *sound = MediaData::FindMediaByName(si->text());

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

	MediaData *sound = MediaData::FindMediaByName(si->text());
	AddSound(GetDefaultString(MediaData::GetName(*sound)),
		 MediaData::GetPath(*sound), MediaData::LoopingEnabled(*sound));
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

OBS_DECLARE_MODULE()
OBS_MODULE_AUTHOR("cg2121");
OBS_MODULE_USE_DEFAULT_LOCALE("Soundboard", "en-US")

bool obs_module_load(void)
{
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
