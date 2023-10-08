#include <string.h>
#include <zlib.h>

#include "data.h"
#include "lutro.h"

static int data_compress(lua_State *L);
static int data_decompress(lua_State *L);
static int data_encode(lua_State *L);
static int data_decode(lua_State *L);
// static int data_getPackedSize(lua_State *L);
static int data_hash(lua_State *L);
// static int data_newByteData(lua_State *L);
// static int data_newDataView(lua_State *L);
// static int data_pack(lua_State *L);
// static int data_unpack(lua_State *L);

int lutro_data_preload(lua_State *L)
{
  static luaL_Reg data_funcs[] = {
    { "compress", data_compress },
    { "decompress", data_decompress },
    { "encode", data_encode },
    { "decode", data_decode },
    // { "getPackedSize", data_getPackedSize },
    { "hash", data_hash },
    // { "newByteData", data_newByteData },
    // { "newDataView", data_newDataView },
    // { "pack", data_pack },
    // { "unpack", data_unpack },
    {NULL, NULL}
  };

  lutro_ensure_global_table(L, "lutro");
  luaL_newlib(L, data_funcs);
  lua_setfield(L, -2, "data");

  return 1;
}

// compress/decompress functions ported from https://github.com/love2d/love/blob/main/src/modules/data/Compressor.cpp

static uLong zlibCompressBound(uLong sourceLen, bool gzip)
{
  uLong size = sourceLen + (sourceLen >> 12) + (sourceLen >> 14) + (sourceLen >> 25) + 13;

  // The gzip header is slightly larger than the zlib header.
  //return gzip ? (size + 18 - 6) : size;
  return gzip ? (size + 12) : size;
}

// specify windowbits, 15 for zlib, 31 for gzip, -15 for deflate
static int zlibCompress(int windowbits, Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen, int level)
{
  z_stream stream;

  stream.zalloc = Z_NULL;
  stream.zfree = Z_NULL;
  stream.opaque = Z_NULL;

  stream.next_in = (Bytef *) source;
  stream.avail_in = (uInt) sourceLen;

  stream.next_out = dest;
  stream.avail_out = (uInt) (*destLen);

  int err = deflateInit2(&stream, level, Z_DEFLATED, windowbits, 8, Z_DEFAULT_STRATEGY);

  if (err != Z_OK)
    return err;

  err = deflate(&stream, Z_FINISH);

  if (err != Z_STREAM_END)
  {
    deflateEnd(&stream);
    return err == Z_OK ? Z_BUF_ERROR : err;
  }

  *destLen = stream.total_out;

  return deflateEnd(&stream);
}

// specify windowbits, (15 + 32) for zlib & gzip (auto detect), -15 for deflate
static int zlibDecompress(int windowbits, Bytef * dest, uLongf *destLen, const Bytef *source, uLong sourceLen)
{
  z_stream stream;

  stream.zalloc = Z_NULL;
  stream.zfree = Z_NULL;
  stream.opaque = Z_NULL;

  stream.next_in = (Bytef *) source;
  stream.avail_in = (uInt) sourceLen;

  stream.next_out = dest;
  stream.avail_out = (uInt) (*destLen);

  int err = inflateInit2(&stream, windowbits);

  if (err != Z_OK)
    return err;

  err = inflate(&stream, Z_FINISH);

  if (err != Z_STREAM_END)
  {
    inflateEnd(&stream);
    if (err == Z_NEED_DICT || (err == Z_BUF_ERROR && stream.avail_in == 0))
      return Z_DATA_ERROR;
    return err;
  }

  *destLen = stream.total_out;

  return inflateEnd(&stream);
}

static const char CONTAINER_string[] = "string";
static const char CONTAINER_data[] = "data";

static const char COMPRESS_zlib[] = "zlib";
static const char COMPRESS_gzip[] = "gzip";
static const char COMPRESS_deflate[] = "deflate";
static const char COMPRESS_lz4[] = "lz4";

int data_compress(lua_State *L)
{
  const char *ctype = luaL_checkstring(L, 1);

  if (strncmp(ctype, CONTAINER_data, sizeof(CONTAINER_data)) == 0)
  {
    return luaL_error(L, "'data' container type not supported");
  }

  if (strncmp(ctype, CONTAINER_string, sizeof(CONTAINER_string)) != 0)
  {
    return luaL_error(L, "Invalid container type: %s\n", ctype);
  }

  size_t rawsize;
  const char *rawbytes = luaL_checklstring(L, 3, &rawsize);
  int level = luaL_optinteger(L, 4, -1);
  const char *formatstr = luaL_checkstring(L, 2);
  int windowbits;
  bool gzip;

  if (strncmp(formatstr, COMPRESS_zlib, sizeof(COMPRESS_zlib)) == 0)
  {
    windowbits = 15;
    gzip = false;
  }
  else if (strncmp(formatstr, COMPRESS_gzip, sizeof(COMPRESS_gzip)) == 0)
  {
    windowbits = 31;
    gzip = true;
  }
  else if (strncmp(formatstr, COMPRESS_deflate, sizeof(COMPRESS_deflate)) == 0)
  {
    windowbits = -15;
    gzip = false;
  }
  else if (strncmp(formatstr, COMPRESS_lz4, sizeof(COMPRESS_lz4)) == 0)
  {
    return luaL_error(L, "'lz4' compress format not supported");
  }
  else
  {
    return luaL_error(L, "Invalid compress format: %s\n", formatstr);
  }

  if (level < 0)
    level = Z_DEFAULT_COMPRESSION;
  else if (level > 9)
    level = 9;

  uLong maxsize = zlibCompressBound((uLong)rawsize, gzip);
  char *cbytes = lutro_malloc(maxsize);

  uLongf destlen = maxsize;
  int status = zlibCompress(windowbits, (Bytef*)cbytes, &destlen, (const Bytef*)rawbytes, (uLong)rawsize, level);

  if (status != Z_OK)
  {
    lutro_free(cbytes);
    return luaL_error(L, "Could not zlib/gzip-compress data.");
  }

  lua_pushlstring(L, cbytes, destlen);
  lutro_free(cbytes);

  return 1;
}

int data_decompress(lua_State *L)
{
  const char *ctype = luaL_checkstring(L, 1);

  if (strncmp(ctype, CONTAINER_data, sizeof(CONTAINER_data)) == 0)
  {
    return luaL_error(L, "'data' container type not supported");
  }

  if (strncmp(ctype, CONTAINER_string, sizeof(CONTAINER_string)) != 0)
  {
    return luaL_error(L, "Invalid container type: %s\n", ctype);
  }

  if (lua_gettop(L) < 3)
  {
    // argv: container, compressedString
    return luaL_error(L, "Must specify compress format");
  }

  size_t csize;
  const char *cbytes = luaL_checklstring(L, 3, &csize);
  const char *formatstr = luaL_checkstring(L, 2);
  int windowbits;

  if (strncmp(formatstr, COMPRESS_zlib, sizeof(COMPRESS_zlib)) == 0)
  {
    windowbits = 15 + 32; // auto detect
  }
  else if (strncmp(formatstr, COMPRESS_gzip, sizeof(COMPRESS_gzip)) == 0)
  {
    windowbits = 15 + 32; // auto detect
  }
  else if (strncmp(formatstr, COMPRESS_deflate, sizeof(COMPRESS_deflate)) == 0)
  {
    windowbits = -15;
  }
  else if (strncmp(formatstr, COMPRESS_lz4, sizeof(COMPRESS_lz4)) == 0)
  {
    return luaL_error(L, "'lz4' compress format not supported");
  }
  else
  {
    return luaL_error(L, "Invalid compress format: %s\n", formatstr);
  }

  size_t rawsize = csize * 2; // guess
  char *rawbytes = NULL;

  while (true)
  {
    rawbytes = lutro_malloc(rawsize);

    uLongf destLen = (uLongf)rawsize;
    int status = zlibDecompress(windowbits, (Bytef*)rawbytes, &destLen, (const Bytef*)cbytes, (uLong)csize);

    if (status == Z_OK)
    {
      rawsize = (size_t)destLen;
      break;
    }
    else if (status != Z_BUF_ERROR)
    {
      // For any error other than "not enough room", error
      lutro_free(rawbytes);
      return luaL_error(L, "Could not decompress zlib/gzip-compressed data.");
    }

    lutro_free(rawbytes);
    rawsize *= 2;
  }

  lua_pushlstring(L, rawbytes, rawsize);
  lutro_free(rawbytes);

  return 1;
}

// hex encode/decode functions ported from https://github.com/love2d/love/blob/main/src/modules/data/DataModule.cpp
static const char hexchars[] = "0123456789abcdef";

static char *bytesToHex(const uint8_t *src, size_t srclen, size_t *dstlen)
{
  *dstlen = srclen * 2;

  if (*dstlen == 0) return NULL;

  char *dst = lutro_malloc(*dstlen + 1);

  for (size_t i = 0; i < srclen; i++)
  {
    uint8_t b = src[i];
    dst[i * 2 + 0] = hexchars[b >> 4];
    dst[i * 2 + 1] = hexchars[b & 0xF];
  }

  dst[*dstlen] = '\0';
  return dst;
}

static uint8_t nibble(char c)
{
  if (c >= '0' && c <= '9')
    return (uint8_t) (c - '0');

  if (c >= 'A' && c <= 'F')
    return (uint8_t) (c - 'A' + 0x0a);

  if (c >= 'a' && c <= 'f')
    return (uint8_t) (c - 'a' + 0x0a);

  return 0;
}

static uint8_t *hexToBytes(const char *src, size_t srclen, size_t *dstlen)
{
  if (srclen >= 2 && src[0] == '0' && (src[1] == 'x' || src[1] == 'X'))
  {
    src += 2;
    srclen -= 2;
  }

  *dstlen = (srclen + 1) / 2;

  if (*dstlen == 0) return NULL;

  uint8_t *dst = lutro_malloc(*dstlen);

  for (size_t i = 0; i < *dstlen; i++)
  {
    dst[i] = nibble(src[i * 2]) << 4;

    if (i * 2 + 1 < srclen)
      dst[i] |= nibble(src[i * 2 + 1]);
  }

  return dst;
}

// base64 encode/decode functions ported from https://github.com/love2d/love/blob/main/src/common/b64.cpp
// Translation table as described in RFC1113
static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Translation table to decode (created by Bob Trower)
static const char cd64[]="|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";

/**
 * encode 3 8-bit binary bytes as 4 '6-bit' characters
 **/
static void b64_encode_block(char in[3], char out[4], int len)
{
  out[0] = (char) cb64[(int)((in[0] & 0xfc) >> 2)];
  out[1] = (char) cb64[(int)(((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4))];
  out[2] = (char) (len > 1 ? cb64[(int)(((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6))] : '=');
  out[3] = (char) (len > 2 ? cb64[(int)(in[2] & 0x3f)] : '=');
}

static char *b64_encode(const char *src, size_t srclen, size_t linelen, size_t *dstlen)
{
  if (linelen == 0) linelen = SIZE_MAX;

  size_t blocksout = 0;
  size_t srcpos = 0;

  size_t adjustment = (srclen % 3) ? (3 - (srclen % 3)) : 0;
  size_t paddedlen = ((srclen + adjustment) / 3) * 4;

  *dstlen = paddedlen + paddedlen / linelen;

  if (*dstlen == 0) return NULL;

  char *dst = lutro_malloc(*dstlen + 1);
  size_t dstpos = 0;

  while (srcpos < srclen)
  {
    char in[3]  = {0};
    char out[4] = {0};

    int len = 0;

    for (int i = 0; i < 3; i++)
    {
      if (srcpos >= srclen)
        break;

      in[i] = src[srcpos++];
      len++;
    }

    if (len > 0)
    {
      b64_encode_block(in, out, len);

      for (int i = 0; i < 4 && dstpos < *dstlen; i++, dstpos++)
        dst[dstpos] = out[i];

      blocksout++;
    }

    if (blocksout >= linelen / 4 || srcpos >= srclen)
    {
      if (blocksout > 0 && dstpos < *dstlen)
        dst[dstpos++] = '\n';

      blocksout = 0;
    }
  }

  dst[dstpos] = '\0';
  return dst;
}

static void b64_decode_block(char in[4], char out[3])
{
  out[0] = (char)(in[0] << 2 | in[1] >> 4);
  out[1] = (char)(in[1] << 4 | in[2] >> 2);
  out[2] = (char)(((in[2] << 6) & 0xc0) | in[3]);
}

static char *b64_decode(const char *src, size_t srclen, size_t *size)
{
  size_t paddedsize = (srclen / 4) * 3;

  char *dst = lutro_malloc(paddedsize);
  char *d = dst;

  char in[4]  = {0};
  char out[3] = {0};
  size_t i, len, srcpos = 0;

  while (srcpos <= srclen)
  {
    for (len = 0, i = 0; i < 4 && srcpos <= srclen; i++)
    {
      char v = 0;

      while (srcpos <= srclen && v == 0)
      {
        v = src[srcpos++];
        v = (char)((v < 43 || v > 122) ? 0 : cd64[v - 43]);
        if (v != 0)
          v = (char)((v == '$') ? 0 : v - 61);
      }

      if (srcpos <= srclen)
      {
        len++;
        if (v != 0)
          in[i] = (char)(v - 1);
      }
      else
        in[i] = 0;
    }

    if (len)
    {
      b64_decode_block(in, out);
      for (i = 0; i < len - 1; i++)
        *(d++) = out[i];
    }
  }

  *size = (size_t)(ptrdiff_t) (d - dst);
  return dst;
}

static const char ENCODE_base64[] = "base64";
static const char ENCODE_hex[] = "hex";

// encode/decode function ported from https://github.com/love2d/love/blob/main/src/modules/data/wrap_DataModule.cpp:w_encode
int data_encode(lua_State *L)
{
  const char *ctype = luaL_checkstring(L, 1);

  if (strncmp(ctype, CONTAINER_data, sizeof(CONTAINER_data)) == 0)
  {
    return luaL_error(L, "'data' container type not supported");
  }

  if (strncmp(ctype, CONTAINER_string, sizeof(CONTAINER_string)) != 0)
  {
    return luaL_error(L, "Invalid container type: %s\n", ctype);
  }

  size_t srclen, dstlen;
  const char *src = luaL_checklstring(L, 3, &srclen);
  size_t linelen = (size_t)luaL_optinteger(L, 4, 0);
  const char *formatstr = luaL_checkstring(L, 2);
  char* dst;

  if (strncmp(formatstr, ENCODE_base64, sizeof(ENCODE_base64)) == 0)
  {
    dst = b64_encode(src, srclen, linelen, &dstlen);
  }
  else if (strncmp(formatstr, ENCODE_hex, sizeof(ENCODE_hex)) == 0)
  {
    dst = bytesToHex((const uint8_t*)src, srclen, &dstlen);
  }
  else
  {
    return luaL_error(L, "Invalid encode format: %s\n", formatstr);
  }

  if (dst == NULL)
  {
    lua_pushstring(L, "");
  }
  else
  {
    lua_pushlstring(L, dst, dstlen);
    lutro_free(dst);
  }

  return 1;
}

int data_decode(lua_State *L)
{
  const char *ctype = luaL_checkstring(L, 1);

  if (strncmp(ctype, CONTAINER_data, sizeof(CONTAINER_data)) == 0)
  {
    return luaL_error(L, "'data' container type not supported");
  }

  if (strncmp(ctype, CONTAINER_string, sizeof(CONTAINER_string)) != 0)
  {
    return luaL_error(L, "Invalid container type: %s\n", ctype);
  }

  size_t srclen, dstlen;
  const char *src = luaL_checklstring(L, 3, &srclen);
  const char *formatstr = luaL_checkstring(L, 2);
  char* dst;

  // encode(format, src, srclen, dstlen, linelen)
  if (strncmp(formatstr, ENCODE_base64, sizeof(ENCODE_base64)) == 0)
  {
    dst = b64_decode(src, srclen, &dstlen);
  }
  else if (strncmp(formatstr, ENCODE_hex, sizeof(ENCODE_hex)) == 0)
  {
    dst = (char*)hexToBytes(src, srclen, &dstlen);
  }
  else
  {
    return luaL_error(L, "Invalid encode format: %s\n", formatstr);
  }

  if (dst == NULL)
  {
    lua_pushstring(L, "");
  }
  else
  {
    lua_pushlstring(L, dst, dstlen);
    lutro_free(dst);
  }

  return 1;
}

// hash functions ported from https://github.com/love2d/love/blob/main/src/modules/data/HashFunction.cpp
static inline uint32_t leftrot32(uint32_t x, uint8_t amount)
{
  return (x << amount) | (x >> (32 - amount));
}

static inline uint32_t rightrot32(uint32_t x, uint8_t amount)
{
  return (x >> amount) | (x << (32 - amount));
}

static inline uint64_t rightrot64(uint64_t x, uint8_t amount)
{
  return (x >> amount) | (x << (64 - amount));
}

// Extend the value of `a` to make it a multiple of `n`
static inline uint64_t extend_multiple(uint64_t a, uint64_t n)
{
  uint64_t r = a % n;
  return r == 0 ? a : a + (n - r);
}

static const char MD5_name[] = "md5";

static const uint8_t MD5_shifts[64] =
{
  7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
  5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
  4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
  6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 
};

static const uint32_t MD5_constants[64] =
{
  0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
  0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
  0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
  0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
  0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
  0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
  0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
  0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
  0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
  0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
  0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
  0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
  0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
  0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
  0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
  0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
};

/**
 * The following implementation is based on the pseudocode provided by multiple
 * authors on wikipedia: https://en.wikipedia.org/wiki/MD5
 * The pseudocode is licensed under the CC-BY-SA license, but no authorship
 * information is present. I believe this note, and the zlib license of this
 * project satisfy the conditions of the license.
 **/

static size_t MD5_hash(const char* input, uint64_t length, uint8_t* output)
{
  uint32_t a0 = 0x67452301;
  uint32_t b0 = 0xefcdab89;
  uint32_t c0 = 0x98badcfe;
  uint32_t d0 = 0x10325476;

  // Compute final padded length, accounting for the appended bit (byte) and size
  uint64_t paddedLength = extend_multiple(length + 1 + 8, 64);

  uint32_t *padded = lutro_malloc(paddedLength);
  memcpy(padded, input, length);
  memset(((uint8_t*)padded) + length, 0, paddedLength - length);
  *(((uint8_t*)padded) + length) = 0x80; // append bit

  // Append length in bits
  uint64_t bit_length = length * 8;
  memcpy(((uint8_t*)padded) + paddedLength - 8, &bit_length, 8);

  // Process chunks
  for (uint64_t i = 0; i < paddedLength/4; i += 16)
  {
    uint32_t *chunk = &padded[i];

    uint32_t A = a0;
    uint32_t B = b0;
    uint32_t C = c0;
    uint32_t D = d0;
    uint32_t F;
    uint32_t g;

    for (int j = 0; j < 64; j++)
    {
      if (j < 16)
      {
        F = (B & C) | (~B & D);
        g = j;
      }
      else if (j < 32)
      {
        F = (D & B) | (~D & C);
        g = (5*j + 1) % 16;
      }
      else if (j < 48)
      {
        F = B ^ C ^ D;
        g = (3*j + 5) % 16;
      }
      else
      {
        F = C ^ (B | ~D);
        g = (7*j) % 16;
      }

      uint32_t temp = D;
      D = C;
      C = B;
      B += leftrot32(A + F + MD5_constants[j] + chunk[g], MD5_shifts[j]);
      A = temp;
    }

    a0 += A;
    b0 += B;
    c0 += C;
    d0 += D;
  }

  lutro_free(padded);

  memcpy(output, &a0, 4);
  memcpy(output + 4, &b0, 4);
  memcpy(output + 8, &c0, 4);
  memcpy(output + 12, &d0, 4);

  return 16;
}

/**
 * The following implementation was based on the text, not the code listings,
 * in RFC3174. I believe this means no copyright other than that of the L�VE
 * Development Team applies.
 **/
static const char SHA1_name[] = "sha1";

static size_t SHA1_hash(const char* input, uint64_t length, uint8_t* output)
{
  uint32_t intermediate[5] = {
    0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0
  };

  // Compute final padded length, accounting for the appended bit (byte) and size
  uint64_t paddedLength = extend_multiple(length + 1 + 8, 64);

  uint32_t *padded = lutro_malloc(paddedLength);
  memcpy(padded, input, length);
  memset(((uint8_t*)padded) + length, 0, paddedLength - length);
  *(((uint8_t*)padded) + length) = 0x80; // append bit

  // Append length in bits (big endian)
  uint64_t bit_length = length * 8;
  for (int i = 0; i < 8; ++i)
    *(((uint8_t*)padded) + (paddedLength - 8 + i)) = (bit_length >> (56 - i * 8)) & 0xFF;

  // Allocate our extended words
  uint32_t words[80];

  for (uint64_t i = 0; i < paddedLength/4; i += 16)
  {
    uint32_t *chunk = &padded[i];
    for (int j = 0; j < 16; j++)
    {
      char *c = (char*) &words[j];
      uint32_t cj = chunk[j];
      c[0] = (cj >> 24) & 0xFF;
      c[1] = (cj >> 16) & 0xFF;
      c[2] = (cj >>  8) & 0xFF;
      c[3] = (cj >>  0) & 0xFF;
    }
    for (int j = 16; j < 80; j++)
      words[j] = leftrot32(words[j-3] ^ words[j-8] ^ words[j-14] ^ words[j-16], 1);

    uint32_t A = intermediate[0];
    uint32_t B = intermediate[1];
    uint32_t C = intermediate[2];
    uint32_t D = intermediate[3];
    uint32_t E = intermediate[4];

    for (int j = 0; j < 80; j++)
    {
      uint32_t temp = leftrot32(A, 5) + E + words[j];

      if (j < 20)
        temp += 0x5A827999 + ((B & C) | (~B & D));
      else if (j < 40)
        temp += 0x6ED9EBA1 + (B ^ C ^ D);
      else if (j < 60)
        temp += 0x8F1BBCDC + ((B & C) | (B & D) | (C & D));
      else
        temp += 0xCA62C1D6 + (B ^ C ^ D);

      E = D;
      D = C;
      C = leftrot32(B, 30);
      B = A;
      A = temp;
    }

    intermediate[0] += A;
    intermediate[1] += B;
    intermediate[2] += C;
    intermediate[3] += D;
    intermediate[4] += E;
  }

  lutro_free(padded);

  for (int i = 0; i < 20; i += 4)
  {
    uint32_t interm = intermediate[i / 4];
    output[i+0] = (interm >> 24) & 0xFF;
    output[i+1] = (interm >> 16) & 0xFF;
    output[i+2] = (interm >>  8) & 0xFF;
    output[i+3] = (interm >>  0) & 0xFF;
  }

  return 20;
}

/**
 * This implementation was based on the description in RFC-6234.
 **/
// SHA-2: SHA-224 and SHA-256
static const char SHA224_name[] = "sha224";
static const char SHA256_name[] = "sha256";

static const uint32_t SHA224_initial[8] =
{
  0xc1059ed8, 0x367cd507, 0x3070dd17, 0xf70e5939,
  0xffc00b31, 0x68581511, 0x64f98fa7, 0xbefa4fa4,
};

static const uint32_t SHA256_initial[8] =
{
  0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
  0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
};

static const uint32_t SHA256_constants[64] =
{
  0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
  0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
  0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
  0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
  0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
  0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
  0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
  0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
  0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
  0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
  0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
  0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
  0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
  0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
  0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
  0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};

static void SHA2xx_hash(const uint32_t* initial, int hashlength, const char* input, uint64_t length, uint8_t* output)
{
  // Compute final padded length, accounting for the appended bit (byte) and size
  uint64_t paddedLength = extend_multiple(length + 1 + 8, 64);

  uint32_t *padded = lutro_malloc(paddedLength);
  memcpy(padded, input, length);
  memset(((uint8_t*)padded) + length, 0, paddedLength - length);
  *(((uint8_t*)padded) + length) = 0x80; // append bit

  // Append length in bits (big endian)
  uint64_t bit_length = length * 8;
  for (int i = 0; i < 8; ++i)
    *(((uint8_t*)padded) + (paddedLength - 8 + i)) = (bit_length >> (56 - i * 8)) & 0xFF;

  uint32_t intermediate[8];

  memcpy(intermediate, initial, sizeof(intermediate));

  // Allocate our extended words
  uint32_t words[64];

  for (uint64_t i = 0; i < paddedLength/4; i += 16)
  {
    uint32_t *chunk = &padded[i];
    for (int j = 0; j < 16; j++)
    {
      char *c = (char*) &words[j];
      c[0] = (chunk[j] >> 24) & 0xFF;
      c[1] = (chunk[j] >> 16) & 0xFF;
      c[2] = (chunk[j] >>  8) & 0xFF;
      c[3] = (chunk[j] >>  0) & 0xFF;
    }
    for (int j = 16; j < 64; j++)
    {
      words[j] = rightrot32(words[j-2], 17) ^ rightrot32(words[j-2], 19) ^ (words[j-2] >> 10);
      words[j] += rightrot32(words[j-15], 7) ^ rightrot32(words[j-15], 18) ^ (words[j-15] >> 3);
      words[j] += words[j-7] + words[j-16];
    }

    uint32_t A = intermediate[0];
    uint32_t B = intermediate[1];
    uint32_t C = intermediate[2];
    uint32_t D = intermediate[3];
    uint32_t E = intermediate[4];
    uint32_t F = intermediate[5];
    uint32_t G = intermediate[6];
    uint32_t H = intermediate[7];

    for (int j = 0; j < 64; j++)
    {
      uint32_t temp1 = H + SHA256_constants[j] + words[j];
      temp1 += rightrot32(E, 6) ^ rightrot32(E, 11) ^ rightrot32(E, 25);
      temp1 += (E & F) ^ (~E & G);
      uint32_t temp2 = rightrot32(A, 2) ^ rightrot32(A, 13) ^ rightrot32(A, 22);
      temp2 += (A & B) ^ (A & C) ^ (B & C);

      H = G;
      G = F;
      F = E;
      E = D + temp1;
      D = C;
      C = B;
      B = A;
      A = temp1 + temp2;
    }

    intermediate[0] += A;
    intermediate[1] += B;
    intermediate[2] += C;
    intermediate[3] += D;
    intermediate[4] += E;
    intermediate[5] += F;
    intermediate[6] += G;
    intermediate[7] += H;
  }

  lutro_free(padded);

  for (int i = 0; i < hashlength; i += 4)
  {
    uint32_t interm = intermediate[i / 4];
    output[i+0] = (interm >> 24) & 0xFF;
    output[i+1] = (interm >> 16) & 0xFF;
    output[i+2] = (interm >>  8) & 0xFF;
    output[i+3] = (interm >>  0) & 0xFF;
  }
}

static inline size_t SHA224_hash(const char* input, uint64_t length, uint8_t* output)
{
  SHA2xx_hash(SHA224_initial, 28, input, length, output);
  return 28;
}

static inline size_t SHA256_hash(const char* input, uint64_t length, uint8_t* output)
{
  SHA2xx_hash(SHA256_initial, 32, input, length, output);
  return 32;
}

/**
 * This implementation was based on the description in RFC-6234.
 **/
// SHA-2: SHA-384 and SHA-512
static const char SHA384_name[] = "sha384";
static const char SHA512_name[] = "sha512";

static const uint64_t SHA384_initial[8] =
{
  0xcbbb9d5dc1059ed8, 0x629a292a367cd507, 0x9159015a3070dd17, 0x152fecd8f70e5939,
  0x67332667ffc00b31, 0x8eb44a8768581511, 0xdb0c2e0d64f98fa7, 0x47b5481dbefa4fa4,
};

static const uint64_t SHA512_initial[8] =
{
  0x6a09e667f3bcc908, 0xbb67ae8584caa73b, 0x3c6ef372fe94f82b, 0xa54ff53a5f1d36f1,
  0x510e527fade682d1, 0x9b05688c2b3e6c1f, 0x1f83d9abfb41bd6b, 0x5be0cd19137e2179,
};

static const uint64_t SHA512_constants[80] =
{
  0x428a2f98d728ae22, 0x7137449123ef65cd, 0xb5c0fbcfec4d3b2f, 0xe9b5dba58189dbbc,
  0x3956c25bf348b538, 0x59f111f1b605d019, 0x923f82a4af194f9b, 0xab1c5ed5da6d8118,
  0xd807aa98a3030242, 0x12835b0145706fbe, 0x243185be4ee4b28c, 0x550c7dc3d5ffb4e2,
  0x72be5d74f27b896f, 0x80deb1fe3b1696b1, 0x9bdc06a725c71235, 0xc19bf174cf692694,
  0xe49b69c19ef14ad2, 0xefbe4786384f25e3, 0x0fc19dc68b8cd5b5, 0x240ca1cc77ac9c65,
  0x2de92c6f592b0275, 0x4a7484aa6ea6e483, 0x5cb0a9dcbd41fbd4, 0x76f988da831153b5,
  0x983e5152ee66dfab, 0xa831c66d2db43210, 0xb00327c898fb213f, 0xbf597fc7beef0ee4,
  0xc6e00bf33da88fc2, 0xd5a79147930aa725, 0x06ca6351e003826f, 0x142929670a0e6e70,
  0x27b70a8546d22ffc, 0x2e1b21385c26c926, 0x4d2c6dfc5ac42aed, 0x53380d139d95b3df,
  0x650a73548baf63de, 0x766a0abb3c77b2a8, 0x81c2c92e47edaee6, 0x92722c851482353b,
  0xa2bfe8a14cf10364, 0xa81a664bbc423001, 0xc24b8b70d0f89791, 0xc76c51a30654be30,
  0xd192e819d6ef5218, 0xd69906245565a910, 0xf40e35855771202a, 0x106aa07032bbd1b8,
  0x19a4c116b8d2d0c8, 0x1e376c085141ab53, 0x2748774cdf8eeb99, 0x34b0bcb5e19b48a8,
  0x391c0cb3c5c95a63, 0x4ed8aa4ae3418acb, 0x5b9cca4f7763e373, 0x682e6ff3d6b2b8a3,
  0x748f82ee5defb2fc, 0x78a5636f43172f60, 0x84c87814a1f0ab72, 0x8cc702081a6439ec,
  0x90befffa23631e28, 0xa4506cebde82bde9, 0xbef9a3f7b2c67915, 0xc67178f2e372532b,
  0xca273eceea26619c, 0xd186b8c721c0c207, 0xeada7dd6cde0eb1e, 0xf57d4f7fee6ed178,
  0x06f067aa72176fba, 0x0a637dc5a2c898a6, 0x113f9804bef90dae, 0x1b710b35131c471b,
  0x28db77f523047d84, 0x32caab7b40c72493, 0x3c9ebe0a15c9bebc, 0x431d67c49c100d4c,
  0x4cc5d4becb3e42b6, 0x597f299cfc657e2a, 0x5fcb6fab3ad6faec, 0x6c44198c4a475817,
};

static void SHA5xx_hash(const uint64_t* initial, int hashlength, const char* input, uint64_t length, uint8_t* output)
{
  uint64_t intermediates[8];
  memcpy(intermediates, initial, sizeof(intermediates));

  // Compute final padded length, accounting for the appended bit (byte) and size
  uint64_t paddedLength = extend_multiple(length + 1 + 16, 128);

  uint64_t *padded = lutro_malloc(paddedLength);
  memcpy(padded, input, length);
  memset(((uint8_t*)padded) + length, 0, paddedLength - length);
  *(((uint8_t*)padded) + length) = 0x80; // append bit

  // Append length in bits (big endian), note we only write a 64-bit int, so
  // we have filled the first 8 bytes with zeroes
  uint64_t bit_length = length * 8;
  for (int i = 0; i < 8; ++i)
    *(((uint8_t*)padded) + (paddedLength - 8 + i)) = (bit_length >> (56 - i * 8)) & 0xFF;

  // Allocate our extended words
  uint64_t words[80];

  for (uint64_t i = 0; i < paddedLength/8; i += 16)
  {
    uint64_t *chunk = &padded[i];
    for (int j = 0; j < 16; ++j)
    {
      char *c = (char*) &words[j];
      c[0] = (chunk[j] >> 56) & 0xFF;
      c[1] = (chunk[j] >> 48) & 0xFF;
      c[2] = (chunk[j] >> 40) & 0xFF;
      c[3] = (chunk[j] >> 32) & 0xFF;
      c[4] = (chunk[j] >> 24) & 0xFF;
      c[5] = (chunk[j] >> 16) & 0xFF;
      c[6] = (chunk[j] >>  8) & 0xFF;
      c[7] = (chunk[j] >>  0) & 0xFF;
    }
    for (int j = 16; j < 80; ++j)
    {
      words[j] = words[j-7] + words[j-16];
      words[j] += rightrot64(words[j-2], 19) ^ rightrot64(words[j-2], 61) ^ (words[j-2] >> 6);
      words[j] += rightrot64(words[j-15], 1) ^ rightrot64(words[j-15], 8) ^ (words[j-15] >> 7);
    }

    uint64_t A = intermediates[0];
    uint64_t B = intermediates[1];
    uint64_t C = intermediates[2];
    uint64_t D = intermediates[3];
    uint64_t E = intermediates[4];
    uint64_t F = intermediates[5];
    uint64_t G = intermediates[6];
    uint64_t H = intermediates[7];

    for (int j = 0; j < 80; ++j)
    {
      uint64_t temp1 = H + SHA512_constants[j] + words[j];
      temp1 += rightrot64(E, 14) ^ rightrot64(E, 18) ^ rightrot64(E, 41);
      temp1 += (E & F) ^ (~E & G);
      uint64_t temp2 = rightrot64(A, 28) ^ rightrot64(A, 34) ^ rightrot64(A, 39);
      temp2 += (A & B) ^ (A & C) ^ (B & C);
      H = G;
      G = F;
      F = E;
      E = D + temp1;
      D = C;
      C = B;
      B = A;
      A = temp1 + temp2;
    }

    intermediates[0] += A;
    intermediates[1] += B;
    intermediates[2] += C;
    intermediates[3] += D;
    intermediates[4] += E;
    intermediates[5] += F;
    intermediates[6] += G;
    intermediates[7] += H;
  }

  lutro_free(padded);

  for (int i = 0; i < hashlength; i += 8)
  {
    uint64_t interm = intermediates[i / 8];
    output[i+0] = (interm >> 56) & 0xFF;
    output[i+1] = (interm >> 48) & 0xFF;
    output[i+2] = (interm >> 40) & 0xFF;
    output[i+3] = (interm >> 32) & 0xFF;
    output[i+4] = (interm >> 24) & 0xFF;
    output[i+5] = (interm >> 16) & 0xFF;
    output[i+6] = (interm >>  8) & 0xFF;
    output[i+7] = (interm >>  0) & 0xFF;
  }
}

static inline size_t SHA384_hash(const char* input, uint64_t length, uint8_t* output)
{
  SHA5xx_hash(SHA384_initial, 48, input, length, output);
  return 48;
}

static inline size_t SHA512_hash(const char* input, uint64_t length, uint8_t* output)
{
  SHA5xx_hash(SHA512_initial, 64, input, length, output);
  return 64;
}

int data_hash(lua_State *L)
{
  size_t length;
  const char *hashFunc = luaL_checkstring(L, 1);
  const char *input = luaL_checklstring(L, 2, &length);
  uint8_t hashResult[64];
  size_t hashLen;

  if (strncmp(hashFunc, MD5_name, sizeof(MD5_name)) == 0)
  {
    hashLen = MD5_hash(input, length, hashResult);
  }
  else if (strncmp(hashFunc, SHA1_name, sizeof(SHA1_name)) == 0)
  {
    hashLen = SHA1_hash(input, length, hashResult);
  }
  else if (strncmp(hashFunc, SHA224_name, sizeof(SHA224_name)) == 0)
  {
    hashLen = SHA224_hash(input, length, hashResult);
  }
  else if (strncmp(hashFunc, SHA256_name, sizeof(SHA256_name)) == 0)
  {
    hashLen = SHA256_hash(input, length, hashResult);
  }
  else if (strncmp(hashFunc, SHA384_name, sizeof(SHA384_name)) == 0)
  {
    hashLen = SHA384_hash(input, length, hashResult);
  }
  else if (strncmp(hashFunc, SHA512_name, sizeof(SHA512_name)) == 0)
  {
    hashLen = SHA512_hash(input, length, hashResult);
  }
  else
  {
    return luaL_error(L, "Invalid hash function: %s\n", hashFunc);
  }

  lua_pushlstring(L, (const char*)hashResult, hashLen);

  return 1;
}
