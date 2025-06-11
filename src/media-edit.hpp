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

	void setName(const QString &name);
	QString getName();

	void setPath(const QString &path);
	QString getPath();

	void setLoopChecked(bool checked);
	bool loopChecked();
};
