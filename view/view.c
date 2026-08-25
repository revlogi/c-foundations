#include "view.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

ByteView string_view_bytes(StringView sv) {
    return (ByteView){
        .data = (const unsigned char *)sv.data,
        .length = sv.length,
    };
}

ByteView byte_view(const void *data, size_t length) {
    return (ByteView){
        .data = data,
        .length = length,
    };
}

ByteView byte_view_slice(ByteView bv, size_t offset, size_t length) {
    assert(offset <= bv.length);
    assert(length <= bv.length - offset);

    return (ByteView){
        .data = bv.data + offset,
        .length = length,
    };
}

bool byte_view_equal(ByteView bv1, ByteView bv2) {
    return bv1.length == bv2.length && memcmp(bv1.data, bv2.data, bv1.length) == 0;
}

int byte_view_cmp(ByteView bv1, ByteView bv2) {
    size_t length = bv1.length < bv2.length ? bv1.length : bv2.length;
    int result = memcmp(bv1.data, bv2.data, length);

    if (result != 0) {
        return result;
    }
    return (bv1.length > bv2.length) - (bv1.length < bv2.length);
}

uint64_t byte_view_hash(ByteView bv) {
    const unsigned char *bytes = bv.data;

    uint64_t hash = 14695981039346656037ULL;
    for (size_t i = 0; i < bv.length; i++) {
        hash ^= bytes[i];
        hash *= 1099511628211ULL;
    }

    return hash;
}

StringView string_view(const char *s) {
    return (StringView){
        .data = s,
        .length = strlen(s),
    };
}

bool string_view_equal(StringView sv1, StringView sv2) {
    return byte_view_equal(string_view_bytes(sv1), string_view_bytes(sv2));
}

int string_view_cmp(StringView sv1, StringView sv2) {
    return byte_view_cmp(string_view_bytes(sv1), string_view_bytes(sv2));
}

StringView string_view_substr(StringView sv, size_t offset, size_t length) {
    assert(offset <= sv.length);
    assert(length <= sv.length - offset);

    return (StringView){
        .data = sv.data + offset,
        .length = length,
    };
}
