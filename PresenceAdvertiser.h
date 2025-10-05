#pragma once
#include <QObject>
#include <QLowEnergyController>
#include <QLowEnergyAdvertisingData>
#include <QLowEnergyAdvertisingParameters>
#include <QTimer>

class PresenceAdvertiser : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool advertisingActive READ advertisingActive NOTIFY advertisingActiveChanged)
    Q_PROPERTY(int rollingPeriodSec READ rollingPeriodSec WRITE setRollingPeriodSec NOTIFY rollingPeriodSecChanged)
public:
    explicit PresenceAdvertiser(QObject* parent=nullptr);
    ~PresenceAdvertiser();

    Q_INVOKABLE void configure(quint16 courseId,
                               const QByteArray& nonceSessao,
                               qint64 sessionStartEpochSec,
                               const QByteArray& secretAluno);

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();

    bool advertisingActive() const { return m_active; }

    int rollingPeriodSec() const { return m_periodSec; }
    void setRollingPeriodSec(int s);

signals:
    void advertisingActiveChanged();
    void rollingPeriodSecChanged();
    void errorChanged(const QString& msg);

private slots:
    void updateAdvertising();

private:
    void applyAdvertisingPayload(const QByteArray& payload);

    bool m_active = false;
    quint16 m_courseId = 0;
    QByteArray m_nonce;
    qint64 m_sessionStart = 0;
    QByteArray m_secret;
    int m_periodSec = 20;

    QLowEnergyController* m_peripheral = nullptr;
    QTimer m_timer;
};
