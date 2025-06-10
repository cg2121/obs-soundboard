#pragma once

#include "obs.hpp"
#include <QWidget>
#include <QStyledItemDelegate>
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

	QAction *renameMedia = nullptr;

private slots:
	void on_list_itemClicked();
	void on_actionAdd_triggered();
	void on_actionRemove_triggered();
	void on_actionEdit_triggered();
	void UpdateActions();
	void on_list_customContextMenuRequested(const QPoint &pos);
	void on_actionDuplicate_triggered();

	MediaObj *Add(const QString &name, const QString &path);
	void Play(MediaObj *obj);

	void EditMediaName();
	void MediaNameEdited(QWidget *editor);

	void ItemRenamed(MediaObj *obj);

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

protected:
	virtual void dragEnterEvent(QDragEnterEvent *event) override;
	virtual void dragLeaveEvent(QDragLeaveEvent *event) override;
	virtual void dragMoveEvent(QDragMoveEvent *event) override;
	virtual void dropEvent(QDropEvent *event) override;
};

class MediaRenameDelegate : public QStyledItemDelegate {
	Q_OBJECT

public:
	MediaRenameDelegate(QObject *parent);
	virtual void setEditorData(QWidget *editor,
				   const QModelIndex &index) const override;

protected:
	virtual bool eventFilter(QObject *editor, QEvent *event) override;
};
