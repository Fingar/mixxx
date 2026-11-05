#ifndef MIXXX_DLGBEATMAPEDITOR_H
#define MIXXX_DLGBEATMAPEDITOR_H

#include <QDialog>
#include <QFutureWatcher>

#include "preferences/usersettings.h"
#include "track/track_decl.h"
#include "util/db/dbconnectionpool.h"
#include "waveform/waveform.h"

class WBeatmapTimeline;

class DlgBeatmapEditor : public QDialog {
  public:
    explicit DlgBeatmapEditor(
            TrackPointer pTrack,
            UserSettingsPointer pConfig,
            mixxx::DbConnectionPoolPtr pDbConnectionPool,
            QWidget* parent = nullptr);

    ~DlgBeatmapEditor() override;

  private:
    void loadWaveform();

    TrackPointer m_pTrack;
    UserSettingsPointer m_pConfig;
    mixxx::DbConnectionPoolPtr m_pDbConnectionPool;

    WBeatmapTimeline* m_pTimeline;
    QFutureWatcher<ConstWaveformPointer> m_waveformWatcher;
};

#endif // MIXXX_DLGBEATMAPEDITOR_H