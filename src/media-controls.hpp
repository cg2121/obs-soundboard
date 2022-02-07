#pragma once

#include <QWidget>
#include <QTimer>
#include <vector>
#include <memory>
#include <obs.hpp>

#include "ui_media-controls.h"

class Ui_MediaControls;

class MediaControls : public QWidget {
	Q_OBJECT

	friend class Soundboard;

private:
	std::vector<OBSSignal> sigs;
	OBSWeakSource weakSource = nullptr;
	QTimer mediaTimer;
	QTimer seekTimer;
	int seek;
	int lastSeek;
	bool prevPaused = false;
	bool isSlideshow = false;

	QString FormatSeconds(int totalSeconds);
	void StartMediaTimer();
	void StopMediaTimer();
	void RefreshControls();
	void SetScene(OBSScene scene);
	int64_t GetSliderTime(int val);

	static void OBSMediaStopped(void *data, calldata_t *calldata);
	static void OBSMediaPlay(void *data, calldata_t *calldata);
	static void OBSMediaPause(void *data, calldata_t *calldata);
	static void OBSMediaStarted(void *data, calldata_t *calldata);

	std::unique_ptr<Ui_MediaControls> ui;

private slots:
	void on_playPauseButton_clicked();
	void on_stopButton_clicked();
	void on_nextButton_clicked();
	void on_previousButton_clicked();
	void on_durationLabel_clicked();

	void MediaSliderClicked();
	void MediaSliderReleased();
	void MediaSliderHovered(int val);
	void MediaSliderMoved(int val);
	void SetSliderPosition();
	void SetPlayingState();
	void SetPausedState();
	void SetRestartState();
	void RestartMedia();
	void StopMedia();
	void PlaylistNext();
	void PlaylistPrevious();

	void SeekTimerCallback();

	void MoveSliderFoward(int seconds = 5);
	void MoveSliderBackwards(int seconds = 5);

public slots:
	void PlayMedia();
	void PauseMedia();

public:
	MediaControls(QWidget *parent = nullptr);
	~MediaControls();

	obs_source_t *GetSource();
	void SetSource(obs_source_t *newSource);
	bool MediaPaused();

	bool countDownTimer = false;
};
