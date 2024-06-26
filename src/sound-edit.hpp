#pragma once

#include <QDialog>
#include <memory>

class Ui_MediaEdit;
class QAbstractButton;

class MediaEdit : public QDialog {
	Q_OBJECT

private:
	QString origText;
	std::unique_ptr<Ui_MediaEdit> ui;

private slots:
	void on_browseButton_clicked();
	void on_buttonBox_clicked(QAbstractButton *button);

public:
	MediaEdit(QWidget *parent = nullptr);
	~MediaEdit();

	void SetName(const QString &name);
	QString GetName();

	void SetPath(const QString &path);
	QString GetPath();

	void SetLoopChecked(bool check);
	bool LoopChecked();
};
