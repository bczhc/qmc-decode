//
// Created by bczhc on 25/06/22.
//

#include "qmc2.h"
#include <cstdio>
#include <cassert>
#include <iostream>
#include <fstream>
#include <cstring>

using namespace std;

StreamCencrypt *qmc2::createInstWidthEKey(const char *ekey_b64) {
    auto *stream = new StreamCencrypt();
    auto *key_dec = new KeyDec();
    key_dec->SetKey(ekey_b64, strlen(ekey_b64));
    stream->SetKeyDec(key_dec);
    delete key_dec;
    return stream;
}

int qmc2::decodeWithEKey(const char *input, const char *output, const char *ekey) {
    auto fp_in = fopen(input, "rb");
    auto fp_out = fopen(output, "wb");
    if (fp_in == nullptr || fp_out == nullptr) {
        perror("Failed to open file");
        return 1;
    }

    auto stream = createInstWidthEKey(ekey);

    size_t offset = 0;
    char buf[read_buf_len];
    size_t read_size;
    while ((read_size = fread(buf, 1, read_buf_len, fp_in)) > 0) {
        stream->StreamDecrypt(offset, (uint8_t *) buf, read_size);
        size_t size = fwrite(buf, 1, read_size, fp_out);
        assert(size == read_size);
        offset += read_size;
    }

    if (fclose(fp_in) != 0 || fclose(fp_out) != 0) {
        perror("Failed to close file");
        return 1;
    }

    delete stream;
    return 0;
}

int qmc2::decode(const char *input, const char *output) {
    ifstream stream_input(input, ios::in | ios::binary);
    if (stream_input.fail()) {
        fprintf(stderr, "ERROR: could not open input file %s\n", input);
        return 1;
    }

    auto *buf = new uint8_t[read_buf_len]();

    // embeded ekey detection & extraction
    stream_input.seekg(0, ios::end);
    auto input_file_len = size_t(stream_input.tellg());
    stream_input.seekg(input_file_len - footer_detection_size, ios::beg);
    stream_input.read(reinterpret_cast<char *>(buf), footer_detection_size);

    qmc_detection detection{};
    if (!detect_key_end_position(detection, buf, footer_detection_size)) {
        fprintf(stderr, "ERROR: could not derive embedded ekey from file.\n");
        fprintf(stderr, "       %s\n", detection.error_msg);
        stream_input.close();
        delete[] buf;
        return 1;
    }

    fprintf(stderr, "song_id: %s\n", detection.song_id[0] ? detection.song_id : "(unknown)");

    // size_t decrypted_file_size = input_file_len - footer_detection_size +
    // position - encrypted_key_size;
    size_t decrypted_file_size = input_file_len - footer_detection_size + detection.ekey_position;

    // Extract base64_encoded_ekey
    stream_input.seekg(decrypted_file_size, ios::beg);
    stream_input.read(reinterpret_cast<char *>(buf), detection.ekey_len);
    buf[detection.ekey_len] = 0;
    auto stream = createInstWidthEKey(reinterpret_cast<char *>(buf));

    ofstream stream_out(output, ios::out | ios::binary);
    if (stream_out.fail()) {
        fprintf(stderr, "ERROR: could not open output file %s\n", output);
        return 1;
    }

    // Reset input position
    stream_input.seekg(0, ios::beg);
    uint64_t offset = 0;
    size_t to_decrypt_len = decrypted_file_size;

    // Begin decryption
    while (to_decrypt_len > 0) {
        auto block_size = std::min(read_buf_len, to_decrypt_len);
        stream_input.read(reinterpret_cast<char *>(buf), block_size);
        auto bytes_read = stream_input.gcount();

        stream->StreamDecrypt(offset, buf, bytes_read);
        stream_out.write(reinterpret_cast<char *>(buf), bytes_read);

        offset += bytes_read;
        to_decrypt_len -= bytes_read;
        fprintf(stderr, ".");
        fflush(stderr);
    }

    delete[] buf;
    return 0;
}
