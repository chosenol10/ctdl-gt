#ifndef MONHOC_AVL_H
#define MONHOC_AVL_H

#include "structs.h"

// Khởi tạo cây rỗng
void init_avl(PTRMH& root);

// Tìm theo mã (không phân biệt hoa/thường)
PTRMH find_monhoc(PTRMH root, const char* mamh_ci);

// Thêm môn học (mã duy nhất). Trả 0 OK; 1 trùng mã; 2 mã không hợp lệ.
int insert_monhoc(PTRMH& root, const char* mamh, const char* tenmh);

// Sửa tên môn học
int update_monhoc_name(PTRMH root, const char* mamh_ci, const char* tenmh);

// Xoá môn học theo mã: Chặn nếu môn còn câu hỏi
bool delete_monhoc_safe(PTRMH& root, const char* mamh_ci);

// Đếm số môn trong cây AVL
int dem_monhoc(PTRMH root);

// Duyệt LNR (in-order)
void traverse_inorder(PTRMH root, void (*visit)(const MonHoc&));

// Giải phóng cây + danh sách câu hỏi từng môn
void free_all_monhoc(PTRMH& root);

#endif // MONHOC_AVL_H
