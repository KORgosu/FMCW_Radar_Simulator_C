#include <QApplication>
#include <QFile>
#include <QFont>
#include "src/ui/main_window.h"

int main(int argc, char *argv[])
{
    /* QWebEngineView는 OpenGL 컨텍스트 공유 필요 — QApplication 생성 전 설정 */
    QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    QApplication app(argc, argv);
    app.setApplicationName("FMCW Radar Simulator");
    app.setOrganizationName("Sejong University Defense Radar Lab");

    /* 기본 폰트 */
    QFont font("Courier New", 10);
    app.setFont(font);

    /* 다크 레이더 스타일시트 */
    QFile qss(":/style/dark_radar.qss");
    if (!qss.open(QFile::ReadOnly)) {
        /* 빌드 디렉토리에서 상대 경로로 재시도 */
        qss.setFileName("resources/style/dark_radar.qss");
        (void)qss.open(QFile::ReadOnly);
    }
    if (qss.isOpen())
        app.setStyleSheet(QString::fromUtf8(qss.readAll()));

    MainWindow window;
    window.show();

    return app.exec();
}
