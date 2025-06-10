#include "media-edit.hpp"
#include "media-data.hpp"
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QFileDialog>
#include <QStandardPaths>
#include <QMessageBox>

#include "ui_MediaEdit.h"

#define QTStr(str) QString(obs_module_text(str))
#define MainStr(str) QString(obs_frontend_get_locale_string(str))

MediaEdit::MediaEdit(QWidget *parent) : QDialog(parent), ui(new Ui_MediaEdit)
{
	obs_frontend_push_ui_translation(obs_module_get_string);
	ui->setupUi(this);
	obs_frontend_pop_ui_translation();
}

MediaEdit::~MediaEdit() {}

void MediaEdit::SetName(const QString &name)
{
	ui->name->setText(name);

	if (origText.isNull())
		origText = name;
}

QString MediaEdit::GetName()
{
	return ui->name->text();
}

void MediaEdit::SetPath(const QString &path)
{
	ui->path->setText(path);
}

QString MediaEdit::GetPath()
{
	return ui->path->text();
}

void MediaEdit::SetLoopChecked(bool checked)
{
	ui->loop->setChecked(checked);
}

bool MediaEdit::LoopChecked()
{
	return ui->loop->isChecked();
}

void MediaEdit::on_browseButton_clicked()
{
	QString folder = ui->path->text();

	if (!folder.isEmpty())
		folder = QFileInfo(folder).absoluteDir().absolutePath();
	else
		folder = QStandardPaths::writableLocation(
			QStandardPaths::HomeLocation);

	QString fileName = QFileDialog::getOpenFileName(
		this, QTStr("OpenAudioFile"), folder,
		("Audio (*.mp3 *.aac *.ogg *.wav *.flac)"));

	if (!fileName.isEmpty())
		ui->path->setText(fileName);
}

void MediaEdit::on_buttonBox_clicked(QAbstractButton *button)
{
	QDialogButtonBox::ButtonRole val = ui->buttonBox->buttonRole(button);

	if (val == QDialogButtonBox::RejectRole) {
		close();
	} else if (val == QDialogButtonBox::AcceptRole) {
		QString name = GetName();

		if (name.isEmpty()) {
			QMessageBox::warning(this, MainStr("EmptyName.Title"),
					     MainStr("EmptyName.Text"));
			return;
		}

		if (origText != name && MediaObj::FindByName(name)) {
			QMessageBox::warning(this, MainStr("NameExists.Title"),
					     MainStr("NameExists.Text"));
			return;
		}

		if (ui->path->text().isEmpty()) {
			QMessageBox::warning(this, QTStr("NoFile.Title"),
					     QTStr("NoFile.Text"));
			return;
		}

		accept();
		close();
	}
}
