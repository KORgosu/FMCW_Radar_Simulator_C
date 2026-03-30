#include "architecture_window.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QFile>
#include <QWebEngineView>

ArchitectureWindow::ArchitectureWindow(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("FMCW Radar Architecture");
    resize(960, 760);
    setStyleSheet("background:#060d06; color:#00ff41;");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto *hdr = new QLabel("[ FMCW 레이더 아키텍처 — 블록 클릭 시 상세 설명 및 신호 파형 ]");
    hdr->setStyleSheet(
        "background:#060d06; color:#00ff41; font-family:'Courier New'; font-size:11px;"
        " letter-spacing:2px; padding:6px 12px; border-bottom:1px solid #0d2a0d;"
    );
    root->addWidget(hdr);

    m_view = new QWebEngineView(this);
    root->addWidget(m_view);

    loadHtml();
}

void ArchitectureWindow::loadHtml()
{
    /* Qt 리소스에서 HTML fragment 읽기 */
    QFile f(":/html/fmcw_radar_architecture_v2.html");
    QString fragment;
    if (f.open(QIODevice::ReadOnly))
        fragment = QString::fromUtf8(f.readAll());

    /* Claude.ai CSS 변수 → 레이더 다크 테마로 재정의,
       완전한 HTML 문서로 래핑                          */
    const QString html =
        "<!DOCTYPE html><html><head><meta charset='utf-8'>"
        "<style>"
        ":root{"
          "--color-text-primary:#00ff41;"
          "--color-text-secondary:#00bb30;"
          "--color-text-tertiary:#005a15;"
          "--color-text-info:#00ccff;"
          "--color-background-secondary:#0d1f0d;"
          "--color-background-info:#001a22;"
          "--color-border-tertiary:#0d2a0d;"
          "--color-border-secondary:#1a5a1a;"
          "--color-border-info:#004a80;"
          "--border-radius-md:6px;"
          "--font-sans:'Courier New',monospace;"
          "--font-mono:'Courier New',monospace;"
        "}"
        "body{"
          "background:#060d06;"
          "color:#00ff41;"
          "padding:16px;"
          "font-family:'Courier New',monospace;"
        "}"
        "::-webkit-scrollbar{width:6px;}"
        "::-webkit-scrollbar-track{background:#060d06;}"
        "::-webkit-scrollbar-thumb{background:#1a5a1a;border-radius:3px;}"
        "</style></head><body>"
        + fragment +
        "</body></html>";

    m_view->setHtml(html);
}
