#include "widget/beatmap/wbeatmapnavigator.h"

#include <QMouseEvent>
#include <QPainter>

#include <algorithm>
#include <cmath>

#include "widget/beatmap/wbeatmaptimeline.h"
#include "waveform/waveform.h"

WBeatmapNavigator::WBeatmapNavigator(
        WBeatmapTimeline* pTimeline,
        QWidget* parent)
        : QWidget(parent),
          m_pTimeline(pTimeline) {
    setFixedHeight(70);
    setMouseTracking(true);

    connect(m_pTimeline,
            &WBeatmapTimeline::visibleRangeChanged,
            this,
            QOverload<>::of(&WBeatmapNavigator::update));
}

double WBeatmapNavigator::xToTime(double x) const {
    const auto duration = m_pTimeline->trackDuration();
    return width() > 0 ? x / width() * duration : 0.0;
}

double WBeatmapNavigator::timeToX(double time) const {
    const auto duration = m_pTimeline->trackDuration();
    return duration > 0.0 ? time / duration * width() : 0.0;
}

void WBeatmapNavigator::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.fillRect(rect(), QColor(35, 35, 35));

    const auto duration = m_pTimeline->trackDuration();
    if (duration <= 0.0) {
        return;
    }

    // Draw the whole-song waveform.
    const auto pWaveform = m_pTimeline->waveform();

    if (pWaveform) {
        const auto dataSize = std::min(
                pWaveform->getDataSize(),
                pWaveform->getCompletion());

        if (dataSize >= 2) {
            const auto waveformCenter = height() / 2.0;
            const auto waveformHeight = height() * 0.65;

            painter.setPen(QColor(70, 180, 220));

            for (auto x = 0; x < width(); ++x) {
                const auto timeStart = xToTime(x);
                const auto timeEnd = xToTime(x + 1);

                auto indexStart = static_cast<int>(
                        timeStart / duration * dataSize);
                auto indexEnd = static_cast<int>(
                        timeEnd / duration * dataSize);

                indexStart = std::clamp(indexStart / 2 * 2, 0, dataSize - 2);
                indexEnd = std::clamp(indexEnd / 2 * 2, 0, dataSize - 2);

                auto amplitude = 0;

                for (auto i = indexStart; i <= indexEnd; i += 2) {
                    amplitude = std::max(amplitude,
                            static_cast<int>(pWaveform->getAll(i)));
                    amplitude = std::max(amplitude,
                            static_cast<int>(pWaveform->getAll(i + 1)));
                }

                const auto halfHeight =
                        amplitude / 255.0 * waveformHeight / 2.0;

                painter.drawLine(
                        QPointF(x, waveformCenter - halfHeight),
                        QPointF(x, waveformCenter + halfHeight));
            }
        }
    }

    const auto startX = timeToX(m_pTimeline->visibleStart());
    const auto endX = timeToX(m_pTimeline->visibleEnd());

    const auto clippedStartX = std::clamp(startX, 0.0, static_cast<double>(width()));
    const auto clippedEndX = std::clamp(endX, 0.0, static_cast<double>(width()));

    if (clippedEndX > clippedStartX) {
        painter.fillRect(
                QRectF(clippedStartX, 0, clippedEndX - clippedStartX, height()),
                QColor(70, 140, 220, 65));

        painter.setPen(QColor(100, 170, 240));
        painter.drawRect(QRectF(
                clippedStartX,
                0,
                clippedEndX - clippedStartX - 1,
                height() - 1));
    }

    // Only draw handles when their actual positions are visible.
    if (startX >= 0.0 && startX <= width()) {
        painter.fillRect(
                QRectF(startX - 3, 0, 6, height()),
                QColor(120, 190, 255));
    }

    if (endX >= 0.0 && endX <= width()) {
        painter.fillRect(
                QRectF(endX - 3, 0, 6, height()),
                QColor(120, 190, 255));
    }
}

void WBeatmapNavigator::mousePressEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton) {
        return;
    }

    const auto duration = m_pTimeline->trackDuration();
    if (duration <= 0.0) {
        return;
    }

    const auto x = event->position().x();
    const auto startX = timeToX(m_pTimeline->visibleStart());
    const auto endX = timeToX(m_pTimeline->visibleEnd());

    constexpr auto handleWidth = 8.0;

    m_initialStart = m_pTimeline->visibleStart();
    m_initialEnd = m_pTimeline->visibleEnd();
    m_dragStartTime = xToTime(x);

    if (std::abs(x - startX) <= handleWidth) {
        m_dragMode = DragMode::ResizeLeft;
    } else if (std::abs(x - endX) <= handleWidth) {
        m_dragMode = DragMode::ResizeRight;
    } else if (x >= startX && x <= endX) {
        m_dragMode = DragMode::Pan;
    } else {
        // Jump to clicked position, keeping the current zoom.
        const auto visibleDuration = m_initialEnd - m_initialStart;
        const auto newStart = xToTime(x) - visibleDuration / 2.0;

        m_pTimeline->setVisibleRange(
                newStart,
                newStart + visibleDuration);

        m_initialStart = m_pTimeline->visibleStart();
        m_initialEnd = m_pTimeline->visibleEnd();
        m_dragStartTime = xToTime(x);
        m_dragMode = DragMode::Pan;
    }

    event->accept();
}

void WBeatmapNavigator::mouseMoveEvent(QMouseEvent* event) {
    if (m_dragMode == DragMode::None) {
        return;
    }

    const auto delta = xToTime(event->position().x()) - m_dragStartTime;

    switch (m_dragMode) {
    case DragMode::Pan:
        m_pTimeline->setVisibleRange(
                m_initialStart + delta,
                m_initialEnd + delta);
        break;

    case DragMode::ResizeLeft: {
        const auto minDuration = m_pTimeline->width() / 1000.0;
        const auto maxDuration = m_pTimeline->width() / 10.0;

        const auto newStart = std::clamp(
                m_initialStart + delta,
                m_initialEnd - maxDuration,
                m_initialEnd - minDuration);

        m_pTimeline->setVisibleRange(newStart, m_initialEnd);
        break;
    }

    case DragMode::ResizeRight: {
        const auto minDuration = m_pTimeline->width() / 1000.0;
        const auto maxDuration = m_pTimeline->width() / 10.0;

        const auto newEnd = std::clamp(
                m_initialEnd + delta,
                m_initialStart + minDuration,
                m_initialStart + maxDuration);

        m_pTimeline->setVisibleRange(m_initialStart, newEnd);
        break;
    }

    case DragMode::None:
        break;
    }

    event->accept();
}

void WBeatmapNavigator::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragMode = DragMode::None;
        event->accept();
    }
}

#include "moc_wbeatmapnavigator.cpp"
