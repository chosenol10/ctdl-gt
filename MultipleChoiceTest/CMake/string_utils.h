// string_utils.h
#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <cstdio>
#include <cctype>

// Tự cài đặt hàm chuỗi cơ bản (tránh phụ thuộc <cstring> nếu muốn)
int su_strlen(const char* s);
void su_strcpy(char* dest, const char* src);
void su_strncpy(char* dest, const char* src, int maxLen); // luôn null-terminate
int su_strcmp(const char* a, const char* b);
int su_stricmp(const char* a, const char* b); // không phân biệt hoa/thường ASCII
int su_strncmp(const char* a, const char* b, int n);

// Chuẩn hoá mã: trim 2 đầu, xoá toàn bộ khoảng trắng giữa, chuyển A–Z, giữ [A-Z0-9].
void normalize_code(char* s);

// Chuẩn hoá tên: trim 2 đầu, gộp khoảng trắng, hạ thường toàn chuỗi rồi viết hoa chữ cái đầu mỗi từ.
void normalize_name(char* s);

// Chuẩn hoá phái: trim + upper; hợp lệ "NAM"/"NU"
void normalize_gender(char* s);

// Trim 2 đầu (xoá space/tabs…), không đụng giữa
void trim_inplace(char* s);

// Gộp chuỗi: tất cả khoảng trắng liên tiếp thành 1 dấu cách (giữ nguyên thứ tự)
void squeeze_spaces_inplace(char* s);

// Upper/lower ASCII
void to_upper_ascii_inplace(char* s);
void to_lower_ascii_inplace(char* s);

// CSV: tách bởi dấu phẩy (',') + bỏ 1 khoảng trắng sau dấu phẩy nếu có. 
// Chỉnh sửa buffer tại chỗ, trả về số token và mảng con trỏ tokens[].
// Yêu cầu: tokensCapacity >= số trường.
int split_csv_inplace(char* line, char* tokens[], int tokensCapacity);

// Xoá ký tự BOM UTF-8 nếu có ở đầu buffer
void strip_bom_inplace(char* s);

// Xoá \r \n ở cuối dòng (nếu có)
void chomp_line(char* s);

// Kiểm tra mã đã chuẩn: chỉ [A-Z0-9], không rỗng
bool is_code_alnum_upper(const char* s);

// So sánh mã không phân biệt hoa/thường (dùng cho tìm kiếm nhanh)
int compare_code_ci(const char* a, const char* b);

#endif // STRING_UTILS_H
