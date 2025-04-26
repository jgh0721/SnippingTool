#include <QApplication>
#include <ElaApplication.h>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

#include "snippingTool.hpp"

int main( int argc, char* argv[] )
{
    SetEnvironmentVariableW( L"QT_ENABLE_HIGHDPI_SCALING", L"1" );

    QCoreApplication::setAttribute( Qt::AA_EnableHighDpiScaling, true );
    QCoreApplication::setAttribute( Qt::AA_UseHighDpiPixmaps, true );
    QCoreApplication::setAttribute( Qt::AA_UseStyleSheetPropagationInWidgetStyles, true );

    QApplication app( argc, argv );
    eApp->init();

    QSnippingTool Tool;
    Tool.SetDisplayAffinity( WDA_EXCLUDEFROMCAPTURE );
    Tool.show();

    return app.exec();
}

// int main(int argc, char* argv[])
// {
// #if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
//     QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
// #if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
//     QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
//     QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
// #else
//     //根据实际屏幕缩放比例更改
//     qputenv("QT_SCALE_FACTOR", "1.5");
// #endif
// #endif
//     QApplication a(argc, argv);
//     eApp->init();
//     MainWindow w;
//     w.show();
// #ifdef Q_OS_WIN
//     //    HWND handle = FindWindowA(NULL, "ElaWidgetTool");
//     //    if (handle != NULL)
//     //    {
//     //        SetWindowDisplayAffinity(handle, WDA_EXCLUDEFROMCAPTURE);
//     //    }
// #endif
//     return a.exec();
// }
