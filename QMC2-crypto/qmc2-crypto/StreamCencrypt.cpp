#include "qmc2-crypto/StreamCencrypt.h"

#include <cassert>
#include <cstring>

#include <algorithm>

void StreamCencrypt::StreamEncrypt(uint64_t offset, uint8_t *buf, size_t len)
{
  if (N > 300)
  {
    ProcessByRC4(offset, buf, len);
  }
  else
  {
    for (size_t i = 0; i < len; i++)
    {
      buf[i] ^= mapL(offset + i);
    }
  }
}

void StreamCencrypt::StreamDecrypt(uint64_t offset, uint8_t *buf, size_t len)
{
  this->StreamEncrypt(offset, buf, len);
}

void StreamCencrypt::SetKeyDec(KeyDec *key_dec)
{
  Uninit();
  rc4_key = nullptr;
  if (key_dec)
  {
    key_dec->GetKey(this->rc4_key, N);
    if (N > 300)
    {
      InitRC4KSA();
    }
  }
}

void StreamCencrypt::Uninit()
{
  // reset initial rc4 key
  if (rc4_key)
  {
    delete[] rc4_key;
    rc4_key = nullptr;
  }

  // reset sbox
  this->N = 0;

  if (S)
  {
    delete[] S;
    S = nullptr;
  }
}

#define FIRST_SEGMENT_SIZE (0x80)
#define SEGMENT_SIZE (0x1400)

void StreamCencrypt::ProcessByRC4(size_t offset, uint8_t *buf, size_t size)
{
  uint8_t *orig_buf = buf;

  uint8_t *last_addr = orig_buf + size;

  auto len = size;

  // Initial segment
  if (offset < FIRST_SEGMENT_SIZE)
  {
    auto len_segment = std::min(size, FIRST_SEGMENT_SIZE - offset);
    EncFirstSegment(offset, buf, len_segment);
    len -= len_segment;
    buf += len_segment;
    offset += len_segment;
  }

  // FIXME: Move this as a private member?
  uint8_t *S = new uint8_t[N]();

  // Align segment
  if (offset % SEGMENT_SIZE != 0)
  {
    auto len_segment = std::min(SEGMENT_SIZE - (offset % SEGMENT_SIZE), len);
    EncASegment(S, offset, buf, len_segment);
    len -= len_segment;
    buf += len_segment;
    offset += len_segment;
  }

  // Batch process segments
  while (len > SEGMENT_SIZE)
  {
    auto len_segment = std::min(size_t{SEGMENT_SIZE}, len);
    EncASegment(S, offset, buf, len_segment);
    len -= len_segment;
    buf += len_segment;
    offset += len_segment;
  }

  // Last segment (incomplete segment)
  if (len > 0)
  {
    EncASegment(S, offset, buf, len);
  }

  assert(last_addr == buf + len);

  delete[] S;
}

uint64_t StreamCencrypt::GetSegmentKey(uint64_t id, uint64_t seed)
{
  return uint64_t((double)this->key_hash / double((id + 1) * seed) * 100.0);
}

void StreamCencrypt::EncFirstSegment(size_t offset, uint8_t *buf, size_t len)
{
  for (size_t i = 0; i < len; i++)
  {
    uint64_t key = uint64_t{this->rc4_key[offset % this->N]};
    buf[i] ^= this->rc4_key[GetSegmentKey(offset, key) % this->N];
    offset++;
  }
}

void StreamCencrypt::EncASegment(uint8_t *S, size_t offset, uint8_t *buf, size_t len)
{
  if (rc4_key == nullptr)
  {
    // We need to initialise RC4 key first!
    return;
  }

  const auto N = this->N;

  // Initialise a new seedbox
  memcpy(S, this->S, N);

  // Calculate segment id
  int segment_id = (offset / SEGMENT_SIZE) & 0x1FF;

  // Calculate the number of bytes to skip.
  // The initial "key" derived from segment id, plus the current offset.
  auto skip_len = GetSegmentKey(offset / SEGMENT_SIZE, this->rc4_key[segment_id]) & 0x1FF;
  skip_len += offset % SEGMENT_SIZE;

  int j = 0;
  int k = 0;
  for (size_t i = 0; i < skip_len; i++)
  {
    j = (j + 1) % N;
    k = (S[j] + k) % N;
    std::swap(S[j], S[k]);
  }

  // Now we also manipulate the buffer:
  for (size_t i = 0; i < len; i++)
  {
    j = (j + 1) % N;
    k = (S[j] + k) % N;
    std::swap(S[j], S[k]);

    buf[i] ^= S[(S[j] + S[k]) % N];
  }
}

void StreamCencrypt::InitRC4KSA()
{
  if (!S)
  {
    S = new uint8_t[N]();
  }

  for (size_t i = 0; i < N; ++i)
  {
    S[i] = i & 0xFF;
  }

  int j = 0;
  for (size_t i = 0; i < N; ++i)
  {
    j = (S[i] + j + rc4_key[i % N]) % N;
    std::swap(S[i], S[j]);
  }

  GetHashBase();
}

void StreamCencrypt::GetHashBase()
{
  this->key_hash = 1;
  for (size_t i = 0; i < this->N; i++)
  {
    int32_t value = int32_t{this->rc4_key[i]};

    // ignore if key char is '\x00'
    if (!value)
      continue;

    auto next_hash = this->key_hash * value;
    if (next_hash == 0 || next_hash <= this->key_hash)
      break;

    this->key_hash = next_hash;
  }
}

inline uint8_t rotate(uint8_t value, int bits)
{
  int rotate = (bits + 4) % 8;
  auto left = value << rotate;
  auto right = value >> rotate;
  return uint8_t(left | right);
}

// Untested, this might be wrong.
uint8_t StreamCencrypt::mapL(uint64_t offset)
{
  if (offset > 0x7FFF)
    offset %= 0x7FFF;

  uint64_t key = (offset * offset + 71214) % this->N;

  uint8_t value = this->rc4_key[key];
  return rotate(value, key & 0b0111);
}

StreamCencrypt::~StreamCencrypt() {
    delete[] rc4_key;
    delete[] S;
}
