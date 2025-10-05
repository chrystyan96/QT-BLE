#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QBluetoothPermission>
#include <QPermission>
#include <QQmlEngine>

#include "PresenceAdvertiser.h"
#include "PresenceScanner.h"

static void ensureBlePermissions(QObject* receiver) {
    QBluetoothPermission btPerm;
    btPerm.setCommunicationModes(QBluetoothPermission::Access
                                 | QBluetoothPermission::Advertise);
    auto status = qApp->checkPermission(btPerm);
    if (status == Qt::PermissionStatus::Undetermined) {
        qApp->requestPermission(btPerm, receiver, [](const QPermission&){});
    }
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

#ifdef Q_OS_ANDROID
    ensureBlePermissions(&app);
#endif

    // >>> REGISTRO DO MÓDULO/TIPOS "BlePresence"
    qmlRegisterType<PresenceAdvertiser>("BlePresence", 1, 0, "PresenceAdvertiser");
    qmlRegisterType<PresenceScanner>("BlePresence", 1, 0, "PresenceScanner");

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, [](){ QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    // Seu módulo de UI (o que contém Main.qml) é "BLE_Test"
    engine.loadFromModule("BLE_Test", "Main");
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
