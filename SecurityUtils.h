#pragma once
#include <QtGlobal>
#include <QByteArray>
#include <QCryptographicHash>

namespace SecurityUtils {

    inline constexpr int ROLLING_LEN  = 4;           // bytes do rollingId
    inline constexpr int SIG_LEN      = 8;           // bytes da assinatura truncada
    inline constexpr int PAYLOAD_LEN  = 2 + ROLLING_LEN + SIG_LEN;  // 2 = courseId(LE16)

    // HMAC-SHA256 (implementação direta com QCryptographicHash)
    QByteArray hmacSha256(const QByteArray& key, const QByteArray& message);

    // Trunca bytes (primeiros n bytes)
    inline QByteArray truncBytes(const QByteArray& bytes, int n) {
        return bytes.left(n);
    }

    // Comparação em tempo constante
    bool constTimeEquals(const QByteArray& a, const QByteArray& b);

    // Calcula slot relativo à âncora de sessão:
    // slot = floor(max(now - sessionStart, 0) / period)
    quint32 timeSlot(qint64 nowEpochSec, qint64 sessionStartEpochSec, int periodSec);

    // Helpers LE
    inline QByteArray le16(quint16 v) {
        QByteArray out; out.resize(2);
        out[0] = char(v & 0xFF);
        out[1] = char((v >> 8) & 0xFF);
        return out;
    }

    inline QByteArray le32(quint32 v) {
        QByteArray out; out.resize(4);
        out[0] = char(v & 0xFF);
        out[1] = char((v >> 8) & 0xFF);
        out[2] = char((v >> 16) & 0xFF);
        out[3] = char((v >> 24) & 0xFF);
        return out;
    }

} // namespace SecurityUtils
