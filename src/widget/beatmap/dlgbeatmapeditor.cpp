#include "widget/beatmap/dlgbeatmapeditor.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QtConcurrentRun>

#include <utility>

#include "library/dao/analysisdao.h"
#include "track/track.h"
#include "util/db/dbconnectionpooler.h"
#include "util/db/dbconnectionpooled.h"
#include "waveform/waveformfactory.h"
#include "widget/beatmap/wbeatmaptimeline.h"

DlgBeatmapEditor::DlgBeatmapEditor(
        TrackPointer pTrack,
        UserSettingsPointer pConfig,
        mixxx::DbConnectionPoolPtr pDbConnectionPool,
        QWidget* parent)
        : QDialog(parent),
          m_pTrack(std::move(pTrack)),
          m_pConfig(std::move(pConfig)),
          m_pDbConnectionPool(std::move(pDbConnectionPool)),
          m_pTimeline(nullptr) {
    setWindowTitle(tr("Beatmap Editor"));
    resize(900, 500);

    auto* layout = new QVBoxLayout(this);

    layout->addWidget(new QLabel(m_pTrack->getTitle(), this));

    m_pTimeline = new WBeatmapTimeline(m_pTrack, this);
    layout->addWidget(m_pTimeline);

    connect(&m_waveformWatcher,
            &QFutureWatcher<ConstWaveformPointer>::finished,
            this,
            [this]() {
                const auto pWaveform = m_waveformWatcher.result();
                if (pWaveform) {
                    m_pTrack->setWaveform(pWaveform);
                    m_pTimeline->update();
                }
            });

    loadWaveform();
}

DlgBeatmapEditor::~DlgBeatmapEditor() {
    m_waveformWatcher.waitForFinished();
}

void DlgBeatmapEditor::loadWaveform() {
    if (m_pTrack->getWaveform()) {
        return;
    }

    const auto trackId = m_pTrack->getId();
    const auto pConfig = m_pConfig;
    const auto pDbConnectionPool = m_pDbConnectionPool;

    auto future = QtConcurrent::run(
            [trackId, pConfig, pDbConnectionPool]() -> ConstWaveformPointer {
                if (!trackId.isValid()) {
                    return {};
                }

                mixxx::DbConnectionPooler dbConnectionPooler(pDbConnectionPool);

                AnalysisDao analysisDao(pConfig);
                analysisDao.initialize(
                        mixxx::DbConnectionPooled(pDbConnectionPool));

                const auto analyses = analysisDao.getAnalysesForTrackByType(
                        trackId,
                        AnalysisDao::TYPE_WAVEFORM);

                for (const auto& analysis : analyses) {
                    if (WaveformFactory::waveformVersionToVersionClass(
                                analysis.version) == WaveformFactory::VC_USE) {
                        return ConstWaveformPointer(
                                WaveformFactory::loadWaveformFromAnalysis(analysis));
                    }
                }

                return {};
            });

    m_waveformWatcher.setFuture(future);
}