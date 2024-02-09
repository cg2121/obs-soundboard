#include "sound-edit.hpp"
#include "soundboard.hpp"
#include "media-data.hpp"
#include <obs-module.h>
#include <QFileDialog>
#include <QStandardPaths>
#include <QMessageBox>

#define QT_UTF8(str) QString::fromUtf8(str, -1)
#define QT_TO_UTF8(str) str.toUtf8().constData()
#define QTStr(str) QT_UTF8(obs_module_text(str))
#define MainStr(str) QT_UTF8(obs_frontend_get_locale_string(str))

SoundEdit::SoundEdit(QWidget *parent) : QDialog(parent), ui(new Ui_SoundEdit)
{
	ui->setupUi(this);
	ui->fileEdit->setReadOnly(true);
}

SoundEdit::~SoundEdit() {}

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
	if (MediaData::FindMediaByName(name))
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

