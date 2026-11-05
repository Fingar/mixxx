#include "widget/beatmap/dlgbeatmapeditor.h"

#include <QLabel>
#include <QVBoxLayout>

DlgBeatmapEditor::DlgBeatmapEditor(
        const QString& trackTitle,
        QWidget* parent)
        : QDialog(parent) {
    setWindowTitle(tr("Beatmap Editor"));
    resize(900, 500);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(trackTitle, this));
}