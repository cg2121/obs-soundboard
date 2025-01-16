#include <obs-frontend-api.h>
#include <obs-module.h>
#include <util/platform.h>
#include <obs-hotkey.h>
#include <util/util.hpp>
#include "plugin-support.h"
#include "soundboard.hpp"
#include "media-edit.hpp"
#include <QAction>
#include <QMainWindow>
#include <QObject>
#include <QMessageBox>
#include <QDockWidget>
#include <QMenu>
#include <QListWidget>
#include <QActionGroup>
#include <QLineEdit>
#include <QMimeData>
#include <QFileInfo>
#include <QDragEnterEvent>

#include "scene-tree.hpp"
#include "media-controls.hpp"

#include "media-data.hpp"

#include "ui_Soundboard.h"

#define QT_UTF8(str) QString::fromUtf8(str, -1)
#define QT_TO_UTF8(str) str.toUtf8().constData()
#define QTStr(str) QString(obs_module_text(str))
#define MainStr(str) QString(obs_frontend_get_locale_string(str))

static QString GetDefaultString(QString name = "")
{
	if (name.isEmpty())
		name = QTStr("Sound");

	if (!MediaObj::FindByName(name))
		return name;

	int i = 2;

	for (;;) {
		QString out = name + " " + QString::number(i);

		if (!MediaObj::FindByName(out))
			return out;

		i++;
	}
}

static void OnSave(obs_data_t *saveData, bool saving, void *data)
{
	Soundboard *sb = static_cast<Soundboard *>(data);

	if (saving)
		sb->Save(saveData);
	else
		sb->Load(saveData);
}

static void OnEvent(enum obs_frontend_event event, void *data)
{
	Soundboard *sb = static_cast<Soundboard *>(data);

	switch (event) {
	case OBS_FRONTEND_EVENT_FINISHED_LOADING:
	case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED:
		sb->CreateSource();
		break;
	case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CLEANUP:
		sb->Clear();
	default:
		break;
	};
}

Soundboard::Soundboard(QWidget *parent) : QWidget(parent), ui(new Ui_Soundboard)
{
	ui->setupUi(this);
	ui->list->SetGridMode(true);

	for (QAction *x : ui->toolbar->actions()) {
		QWidget *temp = ui->toolbar->widgetForAction(x);

		for (QByteArray &y : x->dynamicPropertyNames()) {
			temp->setProperty(y, x->property(y));
		}
	}

	ui->actionAdd->setToolTip(QTStr("AddSound"));
	ui->actionRemove->setToolTip(QTStr("RemoveSound.Title"));
	ui->actionEdit->setToolTip(QTStr("EditSound"));

	QActionGroup *actionGroup = new QActionGroup(this);
	actionGroup->addAction(ui->actionListMode);
	actionGroup->addAction(ui->actionGridMode);

	obs_frontend_add_event_callback(OnEvent, this);
	obs_frontend_add_save_callback(OnSave, this);

	ui->list->setItemDelegate(new MediaRenameDelegate(ui->list));

	renameMedia = new QAction(MainStr("Rename"), this);
	renameMedia->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	connect(renameMedia, &QAction::triggered, this,
		&Soundboard::EditMediaName);

#ifdef __APPLE__
	renameMedia->setShortcut({Qt::Key_Return});
#else
	renameMedia->setShortcut({Qt::Key_F2});
#endif

	addAction(renameMedia);

	connect(ui->list->itemDelegate(), &QAbstractItemDelegate::closeEditor,
		this, &Soundboard::MediaNameEdited);
}

Soundboard::~Soundboard()
{
	obs_frontend_remove_event_callback(OnEvent, this);
	obs_frontend_remove_save_callback(OnSave, this);
}

MediaObj *Soundboard::GetCurrentMediaObj()
{
	QListWidgetItem *item = ui->list->currentItem();

	if (!item)
		return nullptr;

	QString uuid = item->data(Qt::UserRole).toString();
	return MediaObj::FindByUUID(uuid);
}

QListWidgetItem *Soundboard::FindItem(MediaObj *obj)
{
	for (int i = 0; i < ui->list->count(); i++) {
		QListWidgetItem *item = ui->list->item(i);
		QString uuid = item->data(Qt::UserRole).toString();

		if (uuid == obj->GetUUID())
			return item;
	}

	return nullptr;
}

void Soundboard::CreateSource()
{
	if (obs_obj_invalid(source)) {
		source = obs_source_create("ffmpeg_source",
					   obs_module_text("Soundboard"),
					   nullptr, nullptr);
		obs_source_set_hidden(source, true);

		ui->mediaControls->SetSource(source.Get());
	}

	obs_set_output_source(8, source);
}

OBSDataArray Soundboard::SaveMedia()
{
	OBSDataArrayAutoRelease array = obs_data_array_create();

	for (int i = 0; i < ui->list->count(); i++) {
		QListWidgetItem *item = ui->list->item(i);
		QString uuid = item->data(Qt::UserRole).toString();

		MediaObj *obj = MediaObj::FindByUUID(uuid);

		OBSDataAutoRelease settings = obs_data_create();
		obs_data_set_string(settings, "name",
				    QT_TO_UTF8(obj->GetName()));
		obs_data_set_string(settings, "path",
				    QT_TO_UTF8(obj->GetPath()));
		obs_data_set_bool(settings, "loop", obj->LoopEnabled());
		obs_data_set_double(settings, "volume",
				    (double)obj->GetVolume());

		OBSDataArrayAutoRelease hotkeyArray =
			obs_hotkey_save(obj->GetHotkey());
		obs_data_set_array(settings, "sound_hotkey", hotkeyArray);

		obs_data_array_push_back(array, settings);
	};

	return array.Get();
}

void Soundboard::LoadMedia(OBSDataArray array)
{
	for (size_t i = 0; i < obs_data_array_count(array); i++) {
		OBSDataAutoRelease settings = obs_data_array_item(array, i);

		obs_data_set_default_string(settings, "name",
					    obs_module_text("Sound"));
		obs_data_set_default_double(settings, "volume", 1.0);

		QString name = obs_data_get_string(settings, "name");
		QString path = obs_data_get_string(settings, "path");
		bool loop = obs_data_get_bool(settings, "loop");
		float volume = (float)obs_data_get_double(settings, "volume");

		OBSDataArrayAutoRelease hotkeyArray =
			obs_data_get_array(settings, "sound_hotkey");

		MediaObj *obj = Add(name, path);
		obs_hotkey_load(obj->GetHotkey(), hotkeyArray);
		obj->SetLoopEnabled(loop);
		obj->SetVolume(volume);
	}
}

void Soundboard::Save(OBSData saveData)
{
	QMainWindow *window = (QMainWindow *)obs_frontend_get_main_window();
	QDockWidget *dock = static_cast<QDockWidget *>(parent());

	OBSDataArray array = SaveMedia();
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
	obs_data_set_bool(saveData, "grid_mode", ui->list->GetGridMode());

	MediaObj *obj = GetCurrentMediaObj();

	if (obj)
		obs_data_set_string(saveData, "current_sound",
				    QT_TO_UTF8(obj->GetName()));

	obs_data_set_bool(saveData, "use_countdown",
			  ui->mediaControls->countDownTimer);
}

void Soundboard::LoadSource(OBSData saveData)
{
	OBSDataAutoRelease sourceData =
		obs_data_get_obj(saveData, "soundboard_source");

	if (sourceData) {
		obs_data_set_obj(sourceData, "settings", nullptr);
		source = obs_load_source(sourceData);
		obs_source_set_hidden(source, true);

		if (obs_obj_invalid(source))
			return;

		ui->mediaControls->SetSource(source.Get());
	}
}

void Soundboard::Load(OBSData saveData)
{
	QMainWindow *window =
		static_cast<QMainWindow *>(obs_frontend_get_main_window());
	QDockWidget *dock = window->findChild<QDockWidget *>("SoundboardDock");

	LoadSource(saveData);

	OBSDataArrayAutoRelease array =
		obs_data_get_array(saveData, "soundboard_array");
	LoadMedia(array.Get());

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
	ui->list->SetGridMode(grid);

	QString lastSound = obs_data_get_string(saveData, "current_sound");

	if (!lastSound.isEmpty()) {
		MediaObj *obj = MediaObj::FindByName(lastSound);
		QListWidgetItem *item = FindItem(obj);

		if (item)
			ui->list->setCurrentItem(item);
		else
			ui->list->setCurrentRow(0);
	} else {
		ui->list->setCurrentRow(0);
	}

	bool countdown = obs_data_get_bool(saveData, "use_countdown");
	ui->mediaControls->countDownTimer = countdown;
}

void Soundboard::Clear()
{
	ui->mediaControls->countDownTimer = false;
	ui->mediaControls->SetSource(nullptr);
	source = nullptr;

	prevPath = "";

	for (int i = 0; i < ui->list->count(); i++) {
		QListWidgetItem *item = ui->list->item(i);
		QString uuid = item->data(Qt::UserRole).toString();

		MediaObj *obj = MediaObj::FindByUUID(uuid);
		delete obj;
		obj = nullptr;
	}

	ui->list->clear();

	UpdateActions();
}

void Soundboard::Play(MediaObj *obj)
{
	QString path = obj->GetPath();

	if (prevPath == path) {
		obs_source_media_restart(source);
		return;
	}

	prevPath = path;

	QListWidgetItem *item = FindItem(obj);
	obs_source_set_volume(source, obj->GetVolume());

	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_bool(settings, "looping", obj->LoopEnabled());
	obs_data_set_string(settings, "local_file", QT_TO_UTF8(path));
	obs_data_set_bool(settings, "is_local_file", true);
	obs_source_update(source, settings);

	ui->list->setCurrentItem(item);
}

void Soundboard::ItemRenamed(MediaObj *obj)
{
	QListWidgetItem *item = FindItem(obj);

	if (item)
		item->setText(obj->GetName());
}

MediaObj *Soundboard::Add(const QString &name_, const QString &path)
{
	QString name = GetDefaultString(name_);

	MediaObj *obj = new MediaObj(name, path);

	QListWidgetItem *item = new QListWidgetItem(name);
	item->setData(Qt::UserRole, obj->GetUUID());
	ui->list->addItem(item);
	ui->list->setCurrentItem(item);

	connect(obj, &MediaObj::HotkeyPressed, this, &Soundboard::Play);
	connect(obj, &MediaObj::Renamed, this, &Soundboard::ItemRenamed);

	UpdateActions();

	return obj;
}

void Soundboard::on_actionAdd_triggered()
{
	MediaEdit edit(this);

	auto added = [&, this]() {
		QString name = edit.GetName();
		QString path = edit.GetPath();
		bool loop = edit.LoopChecked();

		MediaObj *obj = Add(name, path);
		obj->SetLoopEnabled(loop);
	};

	connect(&edit, &QDialog::accepted, this, added);

	edit.SetName(GetDefaultString());
	edit.exec();
}

void Soundboard::on_actionEdit_triggered()
{
	MediaObj *obj = GetCurrentMediaObj();

	if (!obj)
		return;

	MediaEdit edit(this);

	auto edited = [&]() {
		QString name = edit.GetName();
		QString path = edit.GetPath();
		bool loop = edit.LoopChecked();

		obj->SetName(name);
		obj->SetPath(path);
		obj->SetLoopEnabled(loop);
	};

	connect(&edit, &QDialog::accepted, this, edited);

	edit.SetName(obj->GetName());
	edit.SetPath(obj->GetPath());
	edit.SetLoopChecked(obj->LoopEnabled());
	edit.exec();
}

void Soundboard::on_list_itemClicked()
{
	Play(GetCurrentMediaObj());
}

void Soundboard::UpdateActions()
{
	bool enable = ui->list->count() > 0;

	if (actionsEnabled == enable)
		return;

	ui->actionRemove->setEnabled(enable);
	ui->actionEdit->setEnabled(enable);

	for (QAction *action : ui->toolbar->actions()) {
		QWidget *widget = ui->toolbar->widgetForAction(action);

		if (!widget)
			continue;

		widget->style()->unpolish(widget);
		widget->style()->polish(widget);
	}

	actionsEnabled = enable;
}

void Soundboard::on_actionRemove_triggered()
{
	MediaObj *obj = GetCurrentMediaObj();

	if (!obj)
		return;

	QString name = obj->GetName();

	QMessageBox::StandardButton reply =
		QMessageBox::question(this, QTStr("RemoveSound.Title"),
				      QTStr("RemoveSound.Text").arg(name),
				      QMessageBox::Yes | QMessageBox::No);

	if (reply == QMessageBox::No)
		return;

	QListWidgetItem *item = FindItem(obj);
	delete ui->list->takeItem(ui->list->row(item));
	obj->deleteLater();

	UpdateActions();
}

void Soundboard::on_actionListMode_triggered()
{
	ui->list->SetGridMode(false);
}

void Soundboard::on_actionGridMode_triggered()
{
	ui->list->SetGridMode(true);
}

void Soundboard::on_actionDuplicate_triggered()
{
	MediaObj *obj = GetCurrentMediaObj();

	if (!obj)
		return;

	QString name = GetDefaultString(obj->GetName());
	QString path = obj->GetPath();
	bool loop = obj->LoopEnabled();
	MediaObj *newObj = Add(name, path);
	newObj->SetLoopEnabled(loop);
}

void Soundboard::on_list_customContextMenuRequested(const QPoint &pos)
{
	QListWidgetItem *item = ui->list->itemAt(pos);

	bool grid = ui->list->GetGridMode();

	if (grid)
		ui->actionGridMode->setChecked(true);
	else
		ui->actionListMode->setChecked(true);

	QMenu popup(this);

	popup.addAction(ui->actionAdd);
	popup.addAction(ui->actionFilters);
	popup.addSeparator();

	if (item) {
		popup.addAction(renameMedia);
		popup.addSeparator();
		popup.addAction(ui->actionEdit);
		popup.addAction(ui->actionRemove);
		popup.addAction(ui->actionDuplicate);
		popup.addSeparator();
	}

	popup.addSeparator();

	QMenu subMenu(QTStr("ListMode"), this);
	subMenu.addAction(ui->actionListMode);
	subMenu.addAction(ui->actionGridMode);

	popup.addMenu(&subMenu);

	popup.exec(QCursor::pos());
}

void Soundboard::on_actionFilters_triggered()
{
	obs_frontend_open_source_filters(source);
}

void Soundboard::dragEnterEvent(QDragEnterEvent *event)
{
	// refuse drops of our own widgets
	if (event->source() != nullptr) {
		event->setDropAction(Qt::IgnoreAction);
		return;
	}

	if (event->mimeData()->hasUrls())
		event->acceptProposedAction();
}

void Soundboard::dragLeaveEvent(QDragLeaveEvent *event)
{
	event->accept();
}

void Soundboard::dragMoveEvent(QDragMoveEvent *event)
{
	event->acceptProposedAction();
}

void Soundboard::dropEvent(QDropEvent *event)
{
	QStringList supportedExt;
	supportedExt << "mp3" << "aac" << "ogg" << "wav" << "flac";

	foreach(const QUrl &url, event->mimeData()->urls())
	{
		QString path = url.toLocalFile();
		QFileInfo fi(path);
		QString name = fi.fileName();
		QString ext = fi.suffix();

		if (!supportedExt.contains(ext))
			continue;

		Add(name, path);
	}
}

void Soundboard::EditMediaName()
{
	removeAction(renameMedia);
	QListWidgetItem *item = ui->list->currentItem();
	Qt::ItemFlags flags = item->flags();

	item->setFlags(flags | Qt::ItemIsEditable);
	ui->list->editItem(item);
	item->setFlags(flags);
}

void Soundboard::MediaNameEdited(QWidget *editor)
{
	addAction(renameMedia);
	MediaObj *obj = GetCurrentMediaObj();
	QLineEdit *edit = qobject_cast<QLineEdit *>(editor);
	QString name = edit->text().trimmed();

	if (!obj)
		return;

	QListWidgetItem *item = FindItem(obj);
	QString origName = obj->GetName();

	if (name == origName) {
		item->setText(origName);
		return;
	}

	if (name.isEmpty()) {
		item->setText(origName);
		QMessageBox::warning(this, MainStr("EmptyName.Title"),
				     MainStr("EmptyName.Text"));
		return;
	}

	if (MediaObj::FindByName(name)) {
		item->setText(origName);
		QMessageBox::warning(this, MainStr("NameExists.Title"),
				     MainStr("NameExists.Text"));
		return;
	}

	obj->SetName(name);
}

MediaRenameDelegate::MediaRenameDelegate(QObject *parent)
	: QStyledItemDelegate(parent)
{
}

void MediaRenameDelegate::setEditorData(QWidget *editor,
					const QModelIndex &index) const
{
	QStyledItemDelegate::setEditorData(editor, index);
	QLineEdit *lineEdit = qobject_cast<QLineEdit *>(editor);
	if (lineEdit)
		lineEdit->selectAll();
}

bool MediaRenameDelegate::eventFilter(QObject *editor, QEvent *event)
{
	if (event->type() == QEvent::KeyPress) {
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
		switch (keyEvent->key()) {
		case Qt::Key_Escape: {
			QLineEdit *lineEdit = qobject_cast<QLineEdit *>(editor);
			if (lineEdit)
				lineEdit->undo();
			break;
		}
		case Qt::Key_Tab:
		case Qt::Key_Backtab:
			return false;
		}
	}

	return QStyledItemDelegate::eventFilter(editor, event);
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
	obs_frontend_push_ui_translation(obs_module_get_string);

	Soundboard *sb = new Soundboard();
	obs_frontend_add_dock_by_id("SoundboardDock",
				    obs_module_text("Soundboard"), sb);

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
