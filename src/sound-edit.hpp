#pragma once

#include <QDialog>
#include <memory>

#include "ui_SoundEdit.h"

class SoundEdit : public QDialog {
	Q_OBJECT

public:
	SoundEdit(QWidget *parent = nullptr);
	~SoundEdit();

	bool editMode = false;
	QString origText = "";

	std::unique_ptr<Ui_SoundEdit> ui;

public slots:
	void on_browseButton_clicked();
	void on_buttonBox_clicked(QAbstractButton *button);
};
