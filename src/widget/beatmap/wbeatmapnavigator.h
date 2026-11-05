#ifndef MIXXX_WBEATMAPNAVIGATOR_H
#define MIXXX_WBEATMAPNAVIGATOR_H

#include <QWidget>

class QMouseEvent;
class QPaintEvent;
class WBeatmapTimeline;

class WBeatmapNavigator : public QWidget {
    Q_OBJECT

  public:
    explicit WBeatmapNavigator(
        WBeatmapTimeline* pTimeline,
        QWidget *parent = nullptr);

  protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

  private:
    enum class DragMode {
        None,
        Pan,
        ResizeLeft,
        ResizeRight,
    };

    double xToTime(double x) const;
    double timeToX(double time) const;

    WBeatmapTimeline* m_pTimeline;
    DragMode m_dragMode = DragMode::None;

    double m_dragStartTime = 0.0;
    double m_initialStart = 0.0;
    double m_initialEnd = 0.0;
};

#endif // MIXXX_WBEATMAPNAVIGATOR_H
