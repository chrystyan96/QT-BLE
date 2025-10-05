#pragma once
#include <QObject>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QMap>
#include <QSet>

struct Student {
    QString id;         // matrícula/identificador
    QByteArray secret;  // chave HMAC
};

class PresenceScanner : public QObject {
    Q_OBJECT
    Q_PROPERTY(int scanTimeoutMs READ scanTimeoutMs WRITE setScanTimeoutMs NOTIFY scanTimeoutMsChanged)
    Q_PROPERTY(int rssiMin READ rssiMin WRITE setRssiMin NOTIFY rssiMinChanged)
    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)
public:
    explicit PresenceScanner(QObject* parent=nullptr);

    Q_INVOKABLE void configure(quint16 courseId,
                               const QByteArray& nonceSessao,
                               qint64 sessionStartEpochSec,
                               int rollingPeriodSec);

    Q_INVOKABLE void setRoster(const QList<QVariantMap>& students); // { id: string, secret: base64/hex/texto }
    Q_INVOKABLE void startScan();
    Q_INVOKABLE void stopScan();

    int scanTimeoutMs() const { return m_scanTimeoutMs; }
    void setScanTimeoutMs(int ms);

    int rssiMin() const { return m_rssiMin; }
    void setRssiMin(int v);

    bool scanning() const { return m_agent.isActive(); }

signals:
    void presenceMarked(const QString& studentId, int rssi);
    void scanFinished();
    void scanStarted();
    void errorChanged(const QString& msg);
    void scanTimeoutMsChanged();
    void rssiMinChanged();
    void scanningChanged();

private slots:
    void onDiscovered(const QBluetoothDeviceInfo& info);
    void onFinished();

private:
    bool validateAndResolveStudent(const QByteArray& payload, int rssi, QString& outStudentId) const;

    QBluetoothDeviceDiscoveryAgent m_agent;
    quint16 m_courseId = 0;
    QByteArray m_nonce;
    qint64 m_sessionStart = 0;
    int m_periodSec = 20;

    QList<Student> m_students;
    QSet<QString> m_alreadyMarked;

    int m_scanTimeoutMs = 8000;
    int m_rssiMin = -70;
};
