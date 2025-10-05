#include "PresenceAdvertiser.h"
#include "SecurityUtils.h"
#include "BlePresenceConfig.h"
#include <QDateTime>
#include <QDebug>

PresenceAdvertiser::PresenceAdvertiser(QObject* parent)
    : QObject(parent)
{
    connect(&m_timer, &QTimer::timeout, this, &PresenceAdvertiser::updateAdvertising);
}

PresenceAdvertiser::~PresenceAdvertiser() {
    stop();
}

// Aceita secret em texto/hex/base64 e retorna os BYTES corretos
static QByteArray parseSecretFlexible(const QByteArray& in)
{
    QByteArray trimmed = in.trimmed();
    const bool looksB64 = trimmed.contains('=') || trimmed.contains('+') || trimmed.contains('/');
    auto isHexChars = [](const QByteArray& s){
        for (auto c : s) {
            if (!((c>='0'&&c<='9')||(c>='A'&&c<='F')||(c>='a'&&c<='f'))) return false;
        }
        return true;
    };
    if (looksB64) {
        QByteArray b = QByteArray::fromBase64(trimmed);
        if (!b.isEmpty()) return b;
    }
    if ((trimmed.size()%2)==0 && isHexChars(trimmed)) {
        QByteArray b = QByteArray::fromHex(trimmed);
        if (!b.isEmpty()) return b;
    }
    return trimmed; // texto puro
}

void PresenceAdvertiser::configure(quint16 courseId,
                                   const QByteArray& nonceSessao,
                                   qint64 sessionStartEpochSec,
                                   const QByteArray& secretAluno) {
    if (courseId == 0) {
        emit errorChanged(QStringLiteral("courseId inválido"));
    }
    if (nonceSessao.size() < 4) {
        emit errorChanged(QStringLiteral("nonce muito curto (min 4 bytes)"));
    }
    m_courseId     = courseId;
    m_nonce        = nonceSessao;
    m_sessionStart = sessionStartEpochSec;
    m_secret = parseSecretFlexible(secretAluno);
    qInfo().nospace() << "[ADV] secret len=" << m_secret.size() << " bytes";
}

void PresenceAdvertiser::start() {
    if (m_active) return;
    if (!m_peripheral) {
        m_peripheral = QLowEnergyController::createPeripheral(this);
    }
    updateAdvertising();
    m_timer.start(m_periodSec * 1000);
    m_active = true;
    emit advertisingActiveChanged();
}

void PresenceAdvertiser::stop() {
    if (m_peripheral) {
        m_peripheral->stopAdvertising();
    }
    m_timer.stop();
    if (m_active) {
        m_active = false;
        emit advertisingActiveChanged();
    }
}

void PresenceAdvertiser::setRollingPeriodSec(int s) {
    if (s < 5) s = 5;
    if (m_periodSec == s) return;
    m_periodSec = s;
    emit rollingPeriodSecChanged();
    if (m_active) {
        m_timer.start(m_periodSec * 1000);
        updateAdvertising();
    }
}

void PresenceAdvertiser::updateAdvertising() {
    // Payload: [courseId(2)][rollingId(ROLLING_LEN)][signature(SIG_LEN)]
    const qint64 now  = QDateTime::currentSecsSinceEpoch();
    const quint32 slot = SecurityUtils::timeSlot(now, m_sessionStart, m_periodSec);

    QByteArray msg;
    msg += SecurityUtils::le16(m_courseId);
    msg += m_nonce;
    msg += SecurityUtils::le32(slot);

    const QByteArray hmac      = SecurityUtils::hmacSha256(m_secret, msg);
    const QByteArray rollingId = SecurityUtils::truncBytes(hmac, SecurityUtils::ROLLING_LEN);
    const QByteArray signature = SecurityUtils::truncBytes(hmac, SecurityUtils::SIG_LEN);

    QByteArray payload;
    payload += SecurityUtils::le16(m_courseId);
    payload += rollingId;
    payload += signature;

    qInfo().nospace()
        << "[ADV] slot=" << slot
        << " rolling=" << QString::fromLatin1(rollingId.toHex().toUpper())
        << " sig="     << QString::fromLatin1(signature.toHex().toUpper());

    applyAdvertisingPayload(payload);
}

void PresenceAdvertiser::applyAdvertisingPayload(const QByteArray& presenceBytes) {
    if (!m_peripheral) return;

    qInfo().nospace()
        << "[ADV] companyId=0x"
        << QString("%1").arg(BlePresenceConfig::COMPANY_ID, 4, 16, QLatin1Char('0')).toUpper()
        << " payload(" << Qt::dec << presenceBytes.size() << " B)="
        << QString::fromLatin1(presenceBytes.toHex().toUpper());

    QLowEnergyAdvertisingData adv;
    adv.setDiscoverability(QLowEnergyAdvertisingData::DiscoverabilityGeneral);
    adv.setIncludePowerLevel(false);
    adv.setLocalName(QString()); // economiza bytes
    adv.setManufacturerData(BlePresenceConfig::COMPANY_ID, presenceBytes);

    QLowEnergyAdvertisingParameters params;
    params.setMode(QLowEnergyAdvertisingParameters::AdvNonConnInd);
    // params.setInterval(QLowEnergyAdvertisingParameters::Interval::Medium); // opcional

    m_peripheral->stopAdvertising();
    m_peripheral->startAdvertising(params, adv, QLowEnergyAdvertisingData{}); // scan response vazio
}
