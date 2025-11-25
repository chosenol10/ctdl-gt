// string_utils.cpp
#include "string_utils.h"

int su_strlen(const char* s) {
    if (!s) return 0;
    int n = 0; while (s[n] != '\0') ++n;
    return n;
}

void su_strcpy(char* dest, const char* src) {
    if (!dest) return;
    if (!src) { dest[0] = '\0'; return; }
    int i = 0; do { dest[i] = src[i]; } while (src[i++] != '\0');
}

void su_strncpy(char* dest, const char* src, int maxLen) {
    if (!dest || maxLen <= 0) return;
    if (!src) { dest[0] = '\0'; return; }
    int i = 0;
    for (; i < maxLen - 1 && src[i] != '\0'; ++i) dest[i] = src[i];
    dest[i] = '\0';
}

int su_strcmp(const char* a, const char* b) {
    if (a == b) return 0;
    if (!a) return -1;
    if (!b) return 1;
    int i = 0;
    while (a[i] && b[i]) {
        if (a[i] != b[i]) return (unsigned char)a[i] < (unsigned char)b[i] ? -1 : 1;
        ++i;
    }
    if (a[i] == b[i]) return 0;
    return a[i] == '\0' ? -1 : 1;
}

static char to_upper_ascii(char c) {
    if (c >= 'a' && c <= 'z') return (char)(c - 'a' + 'A');
    return c;
}
static char to_lower_ascii(char c) {
    if (c >= 'A' && c <= 'Z') return (char)(c - 'A' + 'a');
    return c;
}

int su_stricmp(const char* a, const char* b) {
    if (a == b) return 0;
    if (!a) return -1;
    if (!b) return 1;
    int i = 0;
    while (a[i] && b[i]) {
        char ca = to_upper_ascii(a[i]);
        char cb = to_upper_ascii(b[i]);
        if (ca != cb) return (unsigned char)ca < (unsigned char)cb ? -1 : 1;
        ++i;
    }
    if (a[i] == b[i]) return 0;
    return a[i] == '\0' ? -1 : 1;
}

int su_strncmp(const char* a, const char* b, int n) {
    if (n <= 0) return 0;
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;
    for (int i = 0; i < n; ++i) {
        if (a[i] != b[i] || a[i] == '\0' || b[i] == '\0') {
            if (a[i] == b[i]) return 0;
            return (unsigned char)a[i] < (unsigned char)b[i] ? -1 : 1;
        }
    }
    return 0;
}

void trim_inplace(char* s) {
    if (!s) return;
    int len = su_strlen(s);
    int i = 0;
    while (i < len && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n')) ++i;
    int j = len - 1;
    while (j >= i && (s[j] == ' ' || s[j] == '\t' || s[j] == '\r' || s[j] == '\n')) --j;
    int k = 0;
    for (int t = i; t <= j; ++t) s[k++] = s[t];
    s[k] = '\0';
}

void squeeze_spaces_inplace(char* s) {
    if (!s) return;
    int len = su_strlen(s);
    char* d = s;
    int inSpace = 0;
    for (int i = 0; i < len; ++i) {
        char c = s[i];
        if (c == ' ' || c == '\t') {
            if (!inSpace) { *d++ = ' '; inSpace = 1; }
        } else {
            *d++ = c;
            inSpace = 0;
        }
    }
    *d = '\0';
}

void to_upper_ascii_inplace(char* s) {
    if (!s) return;
    for (int i = 0; s[i]; ++i) s[i] = to_upper_ascii(s[i]);
}

void to_lower_ascii_inplace(char* s) {
    if (!s) return;
    for (int i = 0; s[i]; ++i) s[i] = to_lower_ascii(s[i]);
}

void normalize_code(char* s) {
    if (!s) return;
    trim_inplace(s);
    // loại tất cả khoảng trắng và ký tự không phải A–Z, 0–9
    char* d = s; 
    for (int i = 0; s[i]; ++i) {
        char c = s[i];
        if (c == ' ' || c == '\t') continue;
        if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) *d++ = c;
        // bỏ các kí tự khác
    }
    *d = '\0';
}

void normalize_name(char* s) {
    if (!s) return;
    trim_inplace(s);
    squeeze_spaces_inplace(s);
    to_lower_ascii_inplace(s);
    // Viết hoa chữ cái đầu mỗi từ
    int newWord = 1;
    for (int i = 0; s[i]; ++i) {
        if (s[i] == ' ') newWord = 1;
        else if (newWord) { 
            if (s[i] >= 'a' && s[i] <= 'z') s[i] = (char)(s[i] - 'a' + 'A');
            newWord = 0;
        }
    }
}

void normalize_gender(char* s) {
    if (!s) return;
    trim_inplace(s);
    to_upper_ascii_inplace(s);
}

int split_csv_inplace(char* line, char* tokens[], int tokensCapacity) {
    if (!line || !tokens || tokensCapacity <= 0) return 0;
    // chuẩn: "A, B, C" => tách theo ',' và trim từng token
    int count = 0;
    char* p = line;
    while (*p && count < tokensCapacity) {
        // token bắt đầu
        tokens[count++] = p;
        // tìm dấu phẩy
        while (*p && *p != ',') ++p;
        if (!*p) break;
        // cắt token tại ','
        *p = '\0';
        ++p;
        // nếu có một khoảng trắng sau dấu phẩy, bỏ nó
        if (*p == ' ') ++p;
    }
    // trim từng token
    for (int i = 0; i < count; ++i) trim_inplace(tokens[i]);
    return count;
}

void strip_bom_inplace(char* s) {
    if (!s) return;
    // BOM UTF-8 = 0xEF 0xBB 0xBF
    unsigned char* u = (unsigned char*)s;
    if (u[0] == 0xEF && u[1] == 0xBB && u[2] == 0xBF) {
        int len = su_strlen(s);
        for (int i = 3; i <= len; ++i) s[i - 3] = s[i];
    }
}

void chomp_line(char* s) {
    if (!s) return;
    int len = su_strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[len - 1] = '\0';
        --len;
    }
}

bool is_code_alnum_upper(const char* s) {
    if (!s || s[0] == '\0') return false;
    for (int i = 0; s[i]; ++i) {
        char c = s[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))) return false;
    }
    return true;
}

int compare_code_ci(const char* a, const char* b) {
    return su_stricmp(a, b);
}
