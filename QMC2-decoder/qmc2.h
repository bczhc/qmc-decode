//
// Created by bczhc on 25/06/22.
//

#ifndef QMC2_QMC2_H
#define QMC2_QMC2_H

#include <qmc2-crypto/IKeyDec.h>
#include <qmc2-crypto/QMCDetection.h>
#include <qmc2-crypto/StreamCencrypt.h>

// 1M buffer
constexpr size_t read_buf_len = 1 * 1024 * 1024;

static_assert(read_buf_len > footer_detection_size && read_buf_len > encrypted_key_size_v1 &&
              read_buf_len > encrypted_key_size_v2,
              "'read_buf_len' should be larger than 'footer_detection_size' "
              "and 'encrypted_key_size'.");

namespace qmc2 {
    StreamCencrypt *createInstWidthEKey(const char *ekey_b64);

    int decodeWithEKey(const char *input, const char *output, const char *ekey);

    int decode(const char *input, const char *output);
}

#endif //QMC2_QMC2_H
