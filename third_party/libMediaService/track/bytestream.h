#ifndef AVCODEC_BYTESTREAM_H
#define AVCODEC_BYTESTREAM_H

#include <stdint.h>
#include <string.h>
#include "intreadwrite.h"

#define FFMIN(a,b) ((a) > (b) ? (b) : (a))

#define AV_BSWAP16C(x) (((x) << 8 & 0xff00)  | ((x) >> 8 & 0x00ff))
#define AV_BSWAP32C(x) (AV_BSWAP16C(x) << 16 | AV_BSWAP16C((x) >> 16))
#define AV_BSWAP64C(x) (AV_BSWAP32C(x) << 32 | AV_BSWAP32C((x) >> 32))

#define AV_BSWAPC(s, x) AV_BSWAP##s##C(x)

#ifndef av_bswap16
static av_always_inline av_const uint16_t av_bswap16(uint16_t x)
{
    x= (x>>8) | (x<<8);
    return x;
}
#endif

#ifndef av_bswap32
static av_always_inline av_const uint32_t av_bswap32(uint32_t x)
{
    return AV_BSWAP32C(x);
}
#endif

#ifndef av_bswap64
static inline uint64_t av_const av_bswap64(uint64_t x)
{
    return (uint64_t)av_bswap32(x) << 32 | av_bswap32(x >> 32);
}
#endif

typedef struct GetByteContext {
    const uint8_t *buffer, *buffer_end, *buffer_start;
} GetByteContext;

typedef struct PutByteContext {
    uint8_t *buffer, *buffer_end, *buffer_start;
    int eof;
} PutByteContext;

#define DEF(type, name, bytes, read, write)                                  \
static  type bytestream_get_ ## name(const uint8_t **b)        \
{                                                                              \
    (*b) += bytes;                                                             \
    return read(*b - bytes);                                                   \
}                                                                              \
static void bytestream_put_ ## name(uint8_t **b,              \
                                                     const type value)         \
{                                                                              \
    write(*b, value);                                                          \
    (*b) += bytes;                                                             \
}                                                                              \
static void bytestream2_put_ ## name ## u(PutByteContext *p,  \
                                                           const type value)   \
{                                                                              \
    bytestream_put_ ## name(&p->buffer, value);                                \
}                                                                              \
static void bytestream2_put_ ## name(PutByteContext *p,       \
                                                      const type value)        \
{                                                                              \
    if (!p->eof && (p->buffer_end - p->buffer >= bytes)) {                     \
        write(p->buffer, value);                                               \
        p->buffer += bytes;                                                    \
    } else                                                                     \
        p->eof = 1;                                                            \
}                                                                              \
static type bytestream2_get_ ## name ## u(GetByteContext *g)  \
{                                                                              \
    return bytestream_get_ ## name(&g->buffer);                                \
}                                                                              \
static type bytestream2_get_ ## name(GetByteContext *g)       \
{                                                                              \
    if (g->buffer_end - g->buffer < bytes) {                                   \
        g->buffer = g->buffer_end;                                             \
        return 0;                                                              \
    }                                                                          \
    return bytestream2_get_ ## name ## u(g);                                   \
}                                                                              \
static type bytestream2_peek_ ## name(GetByteContext *g)      \
{                                                                              \
    if (g->buffer_end - g->buffer < bytes)                                     \
        return 0;                                                              \
    return read(g->buffer);                                                    \
}

DEF(uint64_t,     le64, 8, AV_RL64, AV_WL64)
DEF(unsigned int, le32, 4, AV_RL32, AV_WL32)
DEF(unsigned int, le24, 3, AV_RL24, AV_WL24)
DEF(unsigned int, le16, 2, AV_RL16, AV_WL16)
DEF(uint64_t,     be64, 8, AV_RB64, AV_WB64)
DEF(unsigned int, be32, 4, AV_RB32, AV_WB32)
DEF(unsigned int, be24, 3, AV_RB24, AV_WB24)
DEF(unsigned int, be16, 2, AV_RB16, AV_WB16)
DEF(unsigned int, byte, 1, AV_RB8 , AV_WB8)

static void bytestream2_init(GetByteContext *g,
                                              const uint8_t *buf,
                                              int buf_size)
{
    g->buffer       = buf;
    g->buffer_start = buf;
    g->buffer_end   = buf + buf_size;
}

static void bytestream2_init_writer(PutByteContext *p,
                                                     uint8_t *buf,
                                                     int buf_size)
{
    p->buffer       = buf;
    p->buffer_start = buf;
    p->buffer_end   = buf + buf_size;
    p->eof          = 0;
}

static  int bytestream2_get_bytes_left(GetByteContext *g)
{
    return g->buffer_end - g->buffer;
}

static int bytestream2_get_bytes_left_p(PutByteContext *p)
{
    return p->buffer_end - p->buffer;
}

static void bytestream2_skip(GetByteContext *g,
                                              unsigned int size)
{
    g->buffer += FFMIN(g->buffer_end - g->buffer, size);
}

static void bytestream2_skipu(GetByteContext *g,
                                               unsigned int size)
{
    g->buffer += size;
}

static void bytestream2_skip_p(PutByteContext *p,
                                                unsigned int size)
{
    int size2;
    if (p->eof)
        return;
    size2 = FFMIN(p->buffer_end - p->buffer, size);
    if (size2 != size)
        p->eof = 1;
    p->buffer += size2;
}

static int bytestream2_tell(GetByteContext *g)
{
    return (int)(g->buffer - g->buffer_start);
}

static int bytestream2_tell_p(PutByteContext *p)
{
    return (int)(p->buffer - p->buffer_start);
}

static int bytestream2_size(GetByteContext *g)
{
    return (int)(g->buffer_end - g->buffer_start);
}

static int bytestream2_size_p(PutByteContext *p)
{
    return (int)(p->buffer_end - p->buffer_start);
}


static unsigned int bytestream2_get_buffer(GetByteContext *g,
                                                            uint8_t *dst,
                                                            unsigned int size)
{
    int size2 = FFMIN(g->buffer_end - g->buffer, size);
    memcpy(dst, g->buffer, size2);
    g->buffer += size2;
    return size2;
}

static unsigned int bytestream2_get_bufferu(GetByteContext *g,
                                                             uint8_t *dst,
                                                             unsigned int size)
{
    memcpy(dst, g->buffer, size);
    g->buffer += size;
    return size;
}

static unsigned int bytestream2_put_buffer(PutByteContext *p,
                                                            const uint8_t *src,
                                                            unsigned int size)
{
    int size2;
    if (p->eof)
        return 0;
    size2 = FFMIN(p->buffer_end - p->buffer, size);
    if (size2 != size)
        p->eof = 1;
    memcpy(p->buffer, src, size2);
    p->buffer += size2;
    return size2;
}

static unsigned int bytestream2_put_bufferu(PutByteContext *p,
                                                             const uint8_t *src,
                                                             unsigned int size)
{
    memcpy(p->buffer, src, size);
    p->buffer += size;
    return size;
}

static void bytestream2_set_buffer(PutByteContext *p,
                                                    const uint8_t c,
                                                    unsigned int size)
{
    int size2;
    if (p->eof)
        return;
    size2 = FFMIN(p->buffer_end - p->buffer, size);
    if (size2 != size)
        p->eof = 1;
    memset(p->buffer, c, size2);
    p->buffer += size2;
}

static void bytestream2_set_bufferu(PutByteContext *p,
                                                     const uint8_t c,
                                                     unsigned int size)
{
    memset(p->buffer, c, size);
    p->buffer += size;
}

static unsigned int bytestream2_get_eof(PutByteContext *p)
{
    return p->eof;
}

static unsigned int bytestream2_copy_bufferu(PutByteContext *p,
                                                              GetByteContext *g,
                                                              unsigned int size)
{
    memcpy(p->buffer, g->buffer, size);
    p->buffer += size;
    g->buffer += size;
    return size;
}

static unsigned int bytestream2_copy_buffer(PutByteContext *p,
                                                             GetByteContext *g,
                                                             unsigned int size)
{
    int size2;

    if (p->eof)
        return 0;
    size  = FFMIN(g->buffer_end - g->buffer, size);
    size2 = FFMIN(p->buffer_end - p->buffer, size);
    if (size2 != size)
        p->eof = 1;

    return bytestream2_copy_bufferu(p, g, size2);
}

static unsigned int bytestream_get_buffer(const uint8_t **b,
                                                           uint8_t *dst,
                                                           unsigned int size)
{
    memcpy(dst, *b, size);
    (*b) += size;
    return size;
}

static void bytestream_put_buffer(uint8_t **b,
                                                   const uint8_t *src,
                                                   unsigned int size)
{
    memcpy(*b, src, size);
    (*b) += size;
}

#endif /* AVCODEC_BYTESTREAM_H */
