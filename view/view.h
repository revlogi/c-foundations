#ifndef VIEW_INCLUDED
#define VIEW_INCLUDED

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SV(s) ((StringView){(s), sizeof(s) - 1})

typedef struct ByteView ByteView;
typedef struct StringView StringView;

struct ByteView {
    const unsigned char* data;
    size_t length;
};

struct StringView {
    const char* data;
    size_t length;
};

ByteView byte_view(const void* data, size_t length);
ByteView byte_view_slice(ByteView bv, size_t offset, size_t length);

bool byte_view_equal(ByteView bv1, ByteView bv2);
int byte_view_cmp(ByteView bv1, ByteView bv2);

uint64_t byte_view_hash(ByteView bv);

ByteView string_view_bytes(StringView sv);

StringView string_view(const char* s);
bool string_view_equal(StringView sv1, StringView sv2);
int string_view_cmp(StringView sv1, StringView sv2);

StringView string_view_substr(StringView sv, size_t offset, size_t length);

#endif
