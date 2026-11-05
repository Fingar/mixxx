
#include "widget/beatmap/wbeatmaptimeline.h"

#include <QPainter>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <utility>

#include "track/track.h"
#include "waveform/waveform.h"

WBeatmapTimeline::WBeatmapTimeline(
    TrackPointer pTrack,
    QWidget* parent)
    : QWidget(parent),
      m_pTrack(std::move(pTrack)) {
    setMinimumHeight(200);
}

void WBeatmapTimeline::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.fillRect(rect(), QColor(30, 30, 30));

    // Timeline dimensions.
    constexpr auto rulerHeight = 40;
    const auto waveformTop = rulerHeight;
    const auto waveformHeight = height() - waveformTop;
    const auto waveformCenter = waveformTop + waveformHeight / 2.0;

    // Draw the time ruler.
    painter.setPen(QColor(180, 180, 180));

    const auto visibleSeconds = width() / m_pixelsPerSecond;
    const auto startSecond = static_cast<int>(std::floor(m_scrollSeconds));
    const auto endSecond =
            static_cast<int>(std::ceil(m_scrollSeconds + visibleSeconds));

    for (auto second = startSecond; second <= endSecond; ++second) {
        const auto x = static_cast<int>(
                (second - m_scrollSeconds) * m_pixelsPerSecond);

        painter.drawLine(x, 0, x, 15);

        const auto minutes = std::abs(second) / 60;
        const auto seconds = std::abs(second) % 60;

        const auto timeLabel = QString("%1%2:%3")
                .arg(second < 0 ? "-" : "")
                .arg(minutes)
                .arg(seconds, 2, 10, QChar('0'));

        painter.drawText(x + 4, 30, timeLabel);
    }

    // Get the track's waveform.
    const auto pWaveform = m_pTrack->getWaveform();

    if (!pWaveform) {
        painter.setPen(Qt::white);
        painter.drawText(10, waveformCenter, tr("Waveform not loaded"));
        return;
    }

    const auto duration = m_pTrack->getDuration();
    const auto dataSize = std::min(
            pWaveform->getDataSize(),
            pWaveform->getCompletion());

    if (duration <= 0.0 || dataSize < 2 || waveformHeight <= 0) {
        return;
    }

    // Draw the center line.
    painter.setPen(QColor(75, 75, 75));
    painter.drawLine(
            QPointF(0, waveformCenter),
            QPointF(width(), waveformCenter));

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

        // Find the maximum amplitude for this pixel.
        auto amplitude = 0;

        for (auto i = indexStart; i <= indexEnd; i += 2) {
            amplitude = std::max(amplitude,
                    static_cast<int>(pWaveform->getAll(i)));

            amplitude = std::max(amplitude,
                    static_cast<int>(pWaveform->getAll(i + 1)));
        }

        const auto normalizedAmplitude = amplitude / 255.0;
        const auto halfHeight =
                normalizedAmplitude * waveformHeight / 2.0;

        painter.drawLine(
                QPointF(x, waveformCenter - halfHeight),
                QPointF(x, waveformCenter + halfHeight));
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
            10.0,
            1000.0);

        m_scrollSeconds = timeAtMouse - mouseX / m_pixelsPerSecond;
    } else {
        // Scroll horizontally.
        m_scrollSeconds -= event->angleDelta().y() / 4.0 / m_pixelsPerSecond;
    }

    update();
    event->accept();
}
