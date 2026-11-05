#ifndef MIXXX_DLGBEATMAPEDITOR_H
#define MIXXX_DLGBEATMAPEDITOR_H

#include <QDialog>

class DlgBeatmapEditor : public QDialog {
public:
    explicit DlgBeatmapEditor(
            const QString& trackTitle,
            QWidget* parent = nullptr);
};

#endif // MIXXX_DLGBEATMAPEDITOR_H