#include "widget/beatmap/wbeatmapnavigator.h"

#include <QMouseEvent>
#include <QPainter>

#include <algorithm>
#include <cmath>

#include "widget/beatmap/wbeatmaptimeline.h"
#include "waveform/waveform.h"

namespace {
constexpr double kNavigatorMargin = 0.05;
constexpr double kNavigatorScale = 1.0 - 2.0 * kNavigatorMargin;
} // namespace

WBeatmapNavigator::WBeatmapNavigator(
        WBeatmapTimeline* pTimeline,
        QWidget* parent)
        : QWidget(parent),
          m_pTimeline(pTimeline) {
    setFixedHeight(55);
    setMouseTracking(true);

    connect(m_pTimeline,
            &WBeatmapTimeline::visibleRangeChanged,
            this,
            QOverload<>::of(&WBeatmapNavigator::update));
}

double WBeatmapNavigator::xToTime(double x) const {
    const auto duration = m_pTimeline->trackDuration();

    if (width() <= 0 || duration <= 0.0) {
        return 0.0;
    }

    const auto normalizedX = x / width();
    return (normalizedX - kNavigatorMargin) / kNavigatorScale * duration;
}

double WBeatmapNavigator::timeToX(double time) const {
    const auto duration = m_pTimeline->trackDuration();

    if (duration <= 0.0) {
        return 0.0;
    }

    return (kNavigatorMargin + time / duration * kNavigatorScale) * width();
}

void WBeatmapNavigator::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.fillRect(rect(), QColor(35, 35, 35));

    const auto duration = m_pTimeline->trackDuration();
    if (duration <= 0.0) {
        return;
    }

    // Draw the center line
    painter.setPen(QColor(100, 100, 100, 65));

    painter.drawLine(
            QPointF(0, height() / 2.0),
            QPointF(width() - 1, height() / 2.0));

    // Draw the whole-song waveform.
    const auto pWaveform = m_pTimeline->waveform();

    if (pWaveform) {
        const auto dataSize = std::min(
                pWaveform->getDataSize(),
                pWaveform->getCompletion());

        if (dataSize >= 2) {
            const auto waveformCenter = height() / 2.0;
            const auto waveformHeight = height() * 0.65;

            for (auto x = 0; x < width(); ++x) {
                const auto timeStart = xToTime(x);
                const auto timeEnd = xToTime(x + 1);

                if (timeEnd <= 0.0 || timeStart >= duration) {
                    continue;
                }

                auto indexStart = static_cast<int>(
                        std::max(0.0, timeStart) / duration * dataSize);
                auto indexEnd = static_cast<int>(
                        std::min(duration, timeEnd) / duration * dataSize);

                indexStart = std::clamp(indexStart / 2 * 2, 0, dataSize - 2);
                indexEnd = std::clamp(indexEnd / 2 * 2, 0, dataSize - 2);

                // Use peak amplitude for the waveform silhouette, while averaging
                // frequency bands to determine its RGB color.
                auto amplitude = 0;
                double lowSum = 0.0;
                double midSum = 0.0;
                double highSum = 0.0;

                for (auto i = indexStart; i <= indexEnd; i += 2) {
                    for (auto channel = 0; channel < 2; ++channel) {
                        const auto index = i + channel;

                        amplitude = std::max(amplitude,
                                static_cast<int>(pWaveform->getAll(index)));

                        lowSum += pWaveform->getLow(index);
                        midSum += pWaveform->getMid(index);
                        highSum += pWaveform->getHigh(index);
                    }
                }

                // Normalize the frequency bands to preserve bright colors
                // independently of the waveform's amplitude.
                const auto maxBand = std::max({lowSum, midSum, highSum});

                const auto color = maxBand > 0.0
                        ? QColor::fromRgbF(
                                  lowSum / maxBand,
                                  midSum / maxBand,
                                  highSum / maxBand)
                        : QColor(0, 0, 0);

                const auto halfHeight =
                        amplitude / 255.0 * waveformHeight / 2.0;

                painter.setPen(color);
                painter.drawLine(
                        QPointF(x, waveformCenter - halfHeight),
                        QPointF(x, waveformCenter + halfHeight));
            }
        }
    }

    painter.setPen(QColor(100, 100, 100));

    const auto songStartX = timeToX(0.0);
    const auto songEndX = timeToX(duration);

    painter.drawLine(
            QPointF(songStartX, 0),
            QPointF(songStartX, height() - 1));

    painter.drawLine(
            QPointF(songEndX, 0),
            QPointF(songEndX, height() - 1));

    const auto startX = timeToX(m_pTimeline->visibleStart());
    const auto endX = timeToX(m_pTimeline->visibleEnd());

    const auto clippedStartX = std::clamp(startX, 0.0, static_cast<double>(width()));
    const auto clippedEndX = std::clamp(endX, 0.0, static_cast<double>(width()));

    // Dim the waveform outside the visible timeline range.
    // A translucent gray overlay reduces contrast and saturation,
    // keeping the navigator readable without competing with the main view.
    constexpr auto outsideFade = 170;
    const QColor fadeColor(35, 35, 35, outsideFade);

    painter.fillRect(
            QRectF(0, 0, clippedStartX, height()),
            fadeColor);

    painter.fillRect(
            QRectF(clippedEndX, 0, width() - clippedEndX, height()),
            fadeColor);

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

    // Show how far the viewport extends beyond the navigator.
    const auto leftOverflow = startX < 0.0;
    const auto rightOverflow = endX > width();

    painter.setPen(QColor(120, 190, 255));

    QFont indicatorFont = painter.font();
    indicatorFont.setPixelSize(18);
    indicatorFont.setBold(true);
    painter.setFont(indicatorFont);

    if (leftOverflow) {
        const auto completelyOutside = endX <= 0.0;

        painter.drawText(
                QRectF(2, 0, 32, height()),
                Qt::AlignVCenter | Qt::AlignLeft,
                completelyOutside ? QStringLiteral("≪") : QStringLiteral("‹"));
    }

    if (rightOverflow) {
        const auto completelyOutside = startX >= width();

        painter.drawText(
                QRectF(width() - 34, 0, 32, height()),
                Qt::AlignVCenter | Qt::AlignRight,
                completelyOutside ? QStringLiteral("≫") : QStringLiteral("›"));
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
        const auto minDuration =
                m_pTimeline->width() / WBeatmapTimeline::kMaxPixelsPerSecond;
        const auto maxDuration =
                m_pTimeline->width() / WBeatmapTimeline::kMinPixelsPerSecond;

        const auto newStart = std::clamp(
                m_initialStart + delta,
                m_initialEnd - maxDuration,
                m_initialEnd - minDuration);

        m_pTimeline->setVisibleRange(newStart, m_initialEnd);
        break;
    }

    case DragMode::ResizeRight: {
        const auto minDuration =
                m_pTimeline->width() / WBeatmapTimeline::kMaxPixelsPerSecond;
        const auto maxDuration =
                m_pTimeline->width() / WBeatmapTimeline::kMinPixelsPerSecond;

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
