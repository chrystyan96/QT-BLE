#include "SecurityUtils.h"

namespace SecurityUtils {

    QByteArray hmacSha256(const QByteArray& keyIn, const QByteArray& message)
    {
        const int blockSize = 64; // SHA-256
        QByteArray key = keyIn;

        if (key.size() > blockSize)
            key = QCryptographicHash::hash(key, QCryptographicHash::Sha256);
        if (key.size() < blockSize)
            key.append(QByteArray(blockSize - key.size(), '\0'));

        QByteArray o_key_pad(blockSize, char(0x5c));
        QByteArray i_key_pad(blockSize, char(0x36));
        for (int i = 0; i < blockSize; ++i) {
            o_key_pad[i] = char(o_key_pad[i] ^ key[i]);
            i_key_pad[i] = char(i_key_pad[i] ^ key[i]);
        }

        QByteArray inner = i_key_pad;
        inner.append(message);
        QByteArray innerHash = QCryptographicHash::hash(inner, QCryptographicHash::Sha256);

        QByteArray outer = o_key_pad;
        outer.append(innerHash);
        return QCryptographicHash::hash(outer, QCryptographicHash::Sha256);
    }

    bool constTimeEquals(const QByteArray& a, const QByteArray& b)
    {
        if (a.size() != b.size()) return false;
        unsigned char acc = 0;
        for (int i = 0; i < a.size(); ++i)
            acc |= (unsigned char)(a[i] ^ b[i]);
        return acc == 0;
    }

    quint32 timeSlot(qint64 nowEpochSec, qint64 sessionStartEpochSec, int periodSec)
    {
        if (periodSec <= 0) periodSec = 20;
        qint64 delta = nowEpochSec - sessionStartEpochSec;
        if (delta < 0) delta = 0; // se relógio do aluno estiver atrasado
        return quint32(delta / periodSec);
    }

} // namespace SecurityUtils
