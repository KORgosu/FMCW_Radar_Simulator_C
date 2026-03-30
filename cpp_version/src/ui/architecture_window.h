#pragma once
#ifndef ARCHITECTURE_WINDOW_H
#define ARCHITECTURE_WINDOW_H

#include <QDialog>

class QWebEngineView;

class ArchitectureWindow : public QDialog
{
    Q_OBJECT
public:
    explicit ArchitectureWindow(QWidget *parent = nullptr);

private:
    QWebEngineView *m_view;
    void loadHtml();
};

#endif /* ARCHITECTURE_WINDOW_H */
