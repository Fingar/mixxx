#ifndef MIXXX_WBEATMAPTIMELINE_H
#define MIXXX_WBEATMAPTIMELINE_H

#include <QWidget>

#include "track/track_decl.h"

class QPaintEvent;
class QWheelEvent;

class WBeatmapTimeline : public QWidget {
  public:
    explicit WBeatmapTimeline(
        TrackPointer pTrack,
        QWidget* parent = nullptr);

  protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

  private:
    TrackPointer m_pTrack;
    double m_pixelsPerSecond = 100.0;
    double m_scrollSeconds = 0.0;
};

#endif // MIXXX_WBEATMAPTIMELINE_H
