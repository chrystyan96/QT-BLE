#include "PresenceScanner.h"
#include "SecurityUtils.h"
#include <QDateTime>
#include <QByteArray>
#include <QVariantMap>

#ifndef COMPANY_ID
#define COMPANY_ID 0x1234
#endif

static QString hexStr(const QByteArray& b, char sep=' ') {
    return QString::fromLatin1(b.toHex(sep).toUpper());
}

PresenceScanner::PresenceScanner(QObject* parent)
    : QObject(parent) {
    connect(&m_agent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, &PresenceScanner::onDiscovered);
    connect(&m_agent, &QBluetoothDeviceDiscoveryAgent::finished,
            this, &PresenceScanner::onFinished);
    m_agent.setLowEnergyDiscoveryTimeout(m_scanTimeoutMs);
}

void PresenceScanner::configure(quint16 courseId,
                                const QByteArray& nonceSessao,
                                qint64 sessionStartEpochSec,
                                int rollingPeriodSec) {
    m_courseId = courseId;
    m_nonce = nonceSessao;
    m_sessionStart = sessionStartEpochSec;
    m_periodSec = rollingPeriodSec > 0 ? rollingPeriodSec : 20;

    qInfo().nospace()
        << "[CFG] courseId=" << courseId
        << " nonce=\"" << QString::fromLatin1(nonceSessao.toHex().toUpper()) << "\""
        << " sessionStart=" << sessionStartEpochSec
        << " period=" << m_periodSec;
}

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

void PresenceScanner::setRoster(const QList<QVariantMap>& students) {
    m_students.clear();
    for (const auto& vm : students) {
        Student s;
        s.id = vm.value("id").toString();
        const QByteArray secretField = vm.value("secret").toByteArray();
        s.secret = parseSecretFlexible(secretField);
        if (!s.id.isEmpty() && !s.secret.isEmpty())
            m_students.push_back(std::move(s));
    }
    qInfo() << "[UI] roster carregado:" << m_students.size() << "alunos";
}

void PresenceScanner::startScan() {
    if (m_agent.isActive()) return;
    m_alreadyMarked.clear();
    m_agent.setLowEnergyDiscoveryTimeout(m_scanTimeoutMs);
    emit scanStarted();
    m_agent.start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
    emit scanningChanged();
}

void PresenceScanner::stopScan() {
    if (m_agent.isActive()) { m_agent.stop(); emit scanningChanged(); }
}

void PresenceScanner::setScanTimeoutMs(int ms) {
    if (ms < 1000) ms = 1000;
    if (m_scanTimeoutMs == ms) return;
    m_scanTimeoutMs = ms;
    emit scanTimeoutMsChanged();
}

void PresenceScanner::setRssiMin(int v) {
    if (m_rssiMin == v) return;
    m_rssiMin = v;
    emit rssiMinChanged();
}

void PresenceScanner::onDiscovered(const QBluetoothDeviceInfo& info) {
    if (info.rssi() < m_rssiMin) return;

    QByteArray payload = info.manufacturerData(COMPANY_ID);

    qInfo().nospace()
        << "[SCAN] dev=\"" << info.address().toString() << "\""
        << " RSSI=" << info.rssi()
        << " mdata(" << payload.size() << "B)="
        << (payload.isEmpty() ? "-" : hexStr(payload));

    if (payload.isEmpty()) return;

    QString studentId;
    if (validateAndResolveStudent(payload, info.rssi(), studentId)) {
        if (!m_alreadyMarked.contains(studentId)) {
            m_alreadyMarked.insert(studentId);
            qInfo() << "[SCAN] PRESENCE OK =>" << studentId;
            emit presenceMarked(studentId, info.rssi());
        }
    }
}

bool PresenceScanner::validateAndResolveStudent(const QByteArray& payload, int rssi, QString& outStudentId) const {
    Q_UNUSED(rssi);

    // Payload esperado: [courseId(2)][rollingId(4)][signature(8)]
    if (payload.size() < 14) return false;

    const quint16 cid = quint8(payload[0]) | (quint8(payload[1]) << 8);
    if (cid != m_courseId) return false;

    const QByteArray rollingId = payload.mid(2,4);
    const QByteArray signature = payload.mid(6,8);

    const qint64 now = QDateTime::currentSecsSinceEpoch();
    const int SLOP = 3; // ±3 slots (com período 20s => ±60s)
    const int center = int(SecurityUtils::timeSlot(now, m_sessionStart, m_periodSec));

    qInfo().nospace()
        << "[SCAN] recv rolling=\"" << hexStr(rollingId)
        << "\" sig=\"" << hexStr(signature)
        << "\" centerSlot=" << center
        << " try=[" << (center - SLOP) << ".." << (center + SLOP) << "]";

    for (const auto& stu : m_students) {
        for (int s = center - SLOP; s <= center + SLOP; ++s) {
            if (s < 0) continue;

            QByteArray msg;
            msg += SecurityUtils::le16(m_courseId);
            msg += m_nonce;
            msg += SecurityUtils::le32(quint32(s));

            const QByteArray hmac = SecurityUtils::hmacSha256(stu.secret, msg);
            const QByteArray expRolling = SecurityUtils::truncBytes(hmac, 4);
            const QByteArray expSig     = SecurityUtils::truncBytes(hmac, 8);

            // Debug opcional (soltar se precisar ver por aluno/slot):
            // qInfo().nospace() << "[DBG] id=\"" << stu.id << "\" slot=" << s
            //   << " expRolling=\"" << hexStr(expRolling) << "\" expSig=\"" << hexStr(expSig) << "\"";

            if (SecurityUtils::constTimeEquals(expRolling, rollingId) &&
                SecurityUtils::constTimeEquals(expSig, signature)) {
                outStudentId = stu.id;
                return true;
            }
        }
    }
    return false;
}

void PresenceScanner::onFinished() {
    emit scanFinished();
    emit scanningChanged();
}
