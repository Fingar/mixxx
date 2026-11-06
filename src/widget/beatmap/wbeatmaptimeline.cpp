
#include "widget/beatmap/wbeatmaptimeline.h"

#include <QPainter>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <utility>

#include "track/beats.h"
#include "track/track.h"
#include "waveform/waveform.h"

WBeatmapTimeline::WBeatmapTimeline(
    TrackPointer pTrack,
    QWidget* parent)
    : QWidget(parent),
      m_pTrack(std::move(pTrack)) {
    setMinimumHeight(200);
}

double WBeatmapTimeline::visibleStart() const {
    return m_scrollSeconds;
}

double WBeatmapTimeline::visibleEnd() const {
    return m_scrollSeconds + width() / m_pixelsPerSecond;
}

void WBeatmapTimeline::setVisibleRange(
        double startSeconds,
        double endSeconds) {
    if (endSeconds <= startSeconds) {
        return;
    }

    const auto visibleDuration = endSeconds - startSeconds;

    m_pixelsPerSecond = std::clamp(
            width() / visibleDuration,
            kMinPixelsPerSecond,
            kMaxPixelsPerSecond);

    m_scrollSeconds = startSeconds;

    update();
    emit visibleRangeChanged();
}

double WBeatmapTimeline::trackDuration() const {
    return m_pTrack->getDuration();
}

ConstWaveformPointer WBeatmapTimeline::waveform() const {
    return m_pTrack->getWaveform();
}

void WBeatmapTimeline::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.fillRect(rect(), QColor(30, 30, 30));

    // Timeline dimensions.
    constexpr auto rulerHeight = 40;
    const auto waveformTop = rulerHeight;
    const auto waveformAreaHeight = height() - waveformTop;
    const auto waveformHeight = waveformAreaHeight * 0.75;
    const auto waveformCenter = waveformTop + waveformAreaHeight / 2.0;

    // Draw the time ruler.
    const auto duration = m_pTrack->getDuration();
    constexpr auto minTickSpacing = 80.0;

    auto tickInterval = 1;
    while (tickInterval * m_pixelsPerSecond < minTickSpacing) {
        tickInterval *= (tickInterval == 1 || tickInterval == 5) ? 5 : 2;
    }

    const auto visibleSeconds = width() / m_pixelsPerSecond;
    const auto firstTick = static_cast<int>(
            std::ceil(m_scrollSeconds / tickInterval)) * tickInterval;

    const auto endSecond = static_cast<int>(
            std::ceil(m_scrollSeconds + visibleSeconds));

    for (auto second = firstTick; second <= endSecond; second += tickInterval) {
        const auto x = static_cast<int>(
                (second - m_scrollSeconds) * m_pixelsPerSecond);

        const auto isOutsideTrack = second < 0 || second > duration;

        painter.setPen(isOutsideTrack
                        ? QColor(100, 100, 100)
                        : QColor(180, 180, 180));

        painter.drawLine(x, 0, x, 15);

        const auto minutes = std::abs(second) / 60;
        const auto seconds = std::abs(second) % 60;

        const auto timeLabel = QString("%1%2:%3")
                .arg(second < 0 ? "-" : "")
                .arg(minutes)
                .arg(seconds, 2, 10, QChar('0'));

        painter.drawText(x + 4, 30, timeLabel);
    }

    // Draw the center line.
    painter.setPen(QColor(75, 75, 75));
    painter.drawLine(
            QPointF(0, waveformCenter),
            QPointF(width(), waveformCenter));

    // Get the track's waveform.
    const auto pWaveform = m_pTrack->getWaveform();

    if (pWaveform && duration > 0.0 && waveformHeight > 0) {
        const auto dataSize = std::min(
                pWaveform->getDataSize(),
                pWaveform->getCompletion());

        if (dataSize >= 2) {
            // Draw waveform.
            painter.setPen(QColor(70, 180, 220));

            for (auto x = 0; x < width(); ++x) {
                const auto timeStart =
                        m_scrollSeconds + x / m_pixelsPerSecond;

                const auto timeEnd =
                        m_scrollSeconds + (x + 1) / m_pixelsPerSecond;

                if (timeEnd <= 0.0) {
                    continue;
                }

                if (timeStart >= duration) {
                    break;
                }

                // Convert timeline time to waveform indices.
                auto indexStart = static_cast<int>(
                        std::max(0.0, timeStart) / duration * dataSize);

                auto indexEnd = static_cast<int>(
                        std::min(duration, timeEnd) / duration * dataSize);

                // Align indices to stereo pairs.
                indexStart = std::clamp(indexStart / 2 * 2, 0, dataSize - 2);
                indexEnd = std::clamp(indexEnd / 2 * 2, 0, dataSize - 2);

                // Average amplitude across the pixel's stereo samples to represent
                // its overall loudness, while averaging low/mid/high frequency bands
                // to determine its RGB color (bass = red, mids = green, highs = blue).
                double amplitudeSum = 0.0;
                double lowSum = 0.0;
                double midSum = 0.0;
                double highSum = 0.0;
                int sampleCount = 0;

                for (auto i = indexStart; i <= indexEnd; i += 2) {
                    for (auto channel = 0; channel < 2; ++channel) {
                        const auto index = i + channel;

                        amplitudeSum += pWaveform->getAll(index);
                        lowSum += pWaveform->getLow(index);
                        midSum += pWaveform->getMid(index);
                        highSum += pWaveform->getHigh(index);
                        ++sampleCount;
                    }
                }

                const auto amplitude = sampleCount > 0
                        ? amplitudeSum / sampleCount
                        : 0.0;

                // Normalize frequency bands independently of amplitude so that
                // color represents frequency balance rather than loudness.
                const auto maxBand = std::max({lowSum, midSum, highSum});

                const auto color = maxBand > 0.0
                        ? QColor::fromRgbF(
                                  static_cast<float>(lowSum / maxBand),
                                  static_cast<float>(midSum / maxBand),
                                  static_cast<float>(highSum / maxBand))
                        : QColor(0, 0, 0);

                const auto normalizedAmplitude = amplitude / 255.0;
                const auto halfHeight =
                        normalizedAmplitude * waveformHeight / 2.0;

                painter.setPen(color);
                painter.drawLine(
                        QPointF(x, waveformCenter - halfHeight),
                        QPointF(x, waveformCenter + halfHeight));
            }


        }
    } else if (!pWaveform) {
        painter.setPen(Qt::white);
        painter.drawText(
            QPointF(10, waveformCenter),
            tr("Waveform not loaded"));
    }

    // Draw the pre-track pattern.
    const auto trackStartX = -m_scrollSeconds * m_pixelsPerSecond;
    const auto preTrackEndX = std::min(trackStartX, static_cast<double>(width()));

    if (preTrackEndX > 0.0) {
        painter.save();

        painter.setClipRect(QRectF(
                0, rulerHeight,
                preTrackEndX,
                height() - rulerHeight));

        constexpr auto triangleSpacingSeconds = 0.5;
        const auto triangleWidth = triangleSpacingSeconds * m_pixelsPerSecond;
        const auto triangleHeight = waveformHeight * 0.3;

        painter.setPen(QColor(180, 150, 115));
        painter.setBrush(QColor(100, 80, 60, 100));

        if (triangleWidth >= 8.0) {
            for (auto time = -triangleSpacingSeconds; time >= visibleStart() - triangleSpacingSeconds;
                    time -= triangleSpacingSeconds) {
                const auto x = (time - m_scrollSeconds) * m_pixelsPerSecond;

                QPolygonF triangle;
                triangle << QPointF(x, waveformCenter - triangleHeight / 2.0)
                         << QPointF(x + triangleWidth, waveformCenter)
                         << QPointF(x, waveformCenter + triangleHeight / 2.0);

                painter.drawPolygon(triangle);
                    }
        }

        painter.restore();
    }

    // Draw analyzed beatgrid.
    const auto pBeats = m_pTrack->getBeats();

    if (pBeats) {
        const auto sampleRate = pBeats->getSampleRate().value();

        const auto startFrame =
                mixxx::audio::FramePos(visibleStart() * sampleRate);
        const auto endFrame =
                mixxx::audio::FramePos(visibleEnd() * sampleRate);

        painter.setPen(QColor(255, 255, 255, 75));

        for (auto it = pBeats->iteratorFrom(startFrame);
                it != pBeats->cend() && *it <= endFrame;
                ++it) {
            const auto seconds = it->value() / sampleRate;
            const auto x = (seconds - m_scrollSeconds) * m_pixelsPerSecond;

            painter.drawLine(
                    QPointF(x, rulerHeight),
                    QPointF(x, height()));
                }
    }

    // Draw track boundaries.
    painter.setPen(QColor(130, 130, 130));

    for (const auto time : {0.0, duration}) {
        const auto x = (time - m_scrollSeconds) * m_pixelsPerSecond;

        if (x >= 0.0 && x < width()) {
            painter.drawLine(
                    QPointF(x, rulerHeight),
                    QPointF(x, height()));
        }
    }
}

void WBeatmapTimeline::wheelEvent(QWheelEvent* event) {
    const auto steps = event->angleDelta().y() / 120.0;

    if (event->modifiers() & Qt::ControlModifier) {
        // Zoom around the mouse cursor.
        const auto mouseX = event->position().x();
        const auto timeAtMouse = m_scrollSeconds + mouseX / m_pixelsPerSecond;

        m_pixelsPerSecond = std::clamp(
            m_pixelsPerSecond * std::pow(1.2, steps),
            kMinPixelsPerSecond,
            kMaxPixelsPerSecond);

        m_scrollSeconds = timeAtMouse - mouseX / m_pixelsPerSecond;
    } else {
        // Scroll horizontally.
        m_scrollSeconds -= event->angleDelta().y() / 4.0 / m_pixelsPerSecond;
    }

    update();
    emit visibleRangeChanged();
    event->accept();
}

void WBeatmapTimeline::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        m_isPanning = true;
        m_lastPanX = event->position().x();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    QWidget::mousePressEvent(event);
}

void WBeatmapTimeline::mouseMoveEvent(QMouseEvent* event) {
    if (m_isPanning) {
        const auto x = event->position().x();
        const auto deltaX = x - m_lastPanX;

        m_scrollSeconds -= deltaX / m_pixelsPerSecond;
        m_lastPanX = x;

        update();
        emit visibleRangeChanged();

        event->accept();
        return;
    }

    QWidget::mouseMoveEvent(event);
}

void WBeatmapTimeline::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton && m_isPanning) {
        m_isPanning = false;
        unsetCursor();
        event->accept();
        return;
    }

    QWidget::mouseReleaseEvent(event);
}

#include "moc_wbeatmaptimeline.cpp"