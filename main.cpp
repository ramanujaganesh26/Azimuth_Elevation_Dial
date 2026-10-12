#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include "src/ReceiverController.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("AzimuthElevationReceiver");
    app.setOrganizationName("GroundControl");

    // Use Basic style for full custom styling control
    QQuickStyle::setStyle("Basic");

    ReceiverController controller;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("receiver", &controller);
    engine.rootContext()->setContextProperty("logModel", controller.logModel());

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("Azimuth_Elevation", "Main");

    return QGuiApplication::exec();
}
