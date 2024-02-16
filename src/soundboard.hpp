#pragma once

#include "obs.hpp"
#include <QWidget>
#include <QPointer>
#include <memory>

class SceneTree;
class MediaControls;
class MediaObj;
class QListWidgetItem;
class Ui_Soundboard;

class Soundboard : public QWidget {
	Q_OBJECT

private:
	QString prevPath;
	std::unique_ptr<Ui_Soundboard> ui;

	MediaObj *GetCurrentMediaObj();
	QListWidgetItem *FindItem(MediaObj *obj);

	OBSSourceAutoRelease source;

	bool actionsEnabled = false;

private slots:
	void on_list_itemClicked();
	void on_actionAdd_triggered();
	void on_actionRemove_triggered();
	void on_actionEdit_triggered();
	void UpdateActions();
	void on_list_customContextMenuRequested(const QPoint &pos);
	void on_actionDuplicate_triggered();

	void on_actionFilters_triggered();

	void on_actionListMode_triggered();
	void on_actionGridMode_triggered();

	MediaObj *Add(const QString &name, const QString &path);
	void Play(MediaObj *obj);

public:
	Soundboard(QWidget *parent = nullptr);
	~Soundboard();

	OBSDataArray SaveMedia();
	void LoadMedia(OBSDataArray array);
	void LoadSource(OBSData saveData);
	void Clear();

	void Save(OBSData saveData);
	void Load(OBSData saveData);

	void CreateSource();
};
