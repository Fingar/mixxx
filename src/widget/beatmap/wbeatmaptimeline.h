#ifndef MIXXX_WBEATMAPTIMELINE_H
#define MIXXX_WBEATMAPTIMELINE_H

#include <QWidget>

#include "track/track_decl.h"
#include "waveform/waveform.h"

class QPaintEvent;
class QWheelEvent;
class QMouseEvent;

class WBeatmapTimeline : public QWidget {
    Q_OBJECT

  public:
    explicit WBeatmapTimeline(
        TrackPointer pTrack,
        QWidget* parent = nullptr);
    double visibleStart() const;
    double visibleEnd() const;
    void setVisibleRange(double startSeconds, double endSeconds);
    double trackDuration() const;
    ConstWaveformPointer waveform() const;
    static constexpr double kMaxPixelsPerSecond = 1000.0;
    static constexpr double kMinPixelsPerSecond = 0.01;

  protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

  private:
    TrackPointer m_pTrack;
    double m_pixelsPerSecond = 100.0;
    double m_scrollSeconds = 0.0;
    bool m_isPanning = false;
    double m_lastPanX = 0.0;

  signals:
    void visibleRangeChanged();
};

#endif // MIXXX_WBEATMAPTIMELINE_H
