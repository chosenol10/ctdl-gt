// cauhoi.h
#ifndef CAUHOI_H
#define CAUHOI_H

#include "structs.h"

// Đếm số câu trong danh sách
int dem_cau(PTRCH head);

// Tìm câu theo id
PTRCH find_cau_by_id(PTRCH head, int id);

// Thêm câu (chèn cuối)
void add_cau_hoi(PTRCH& head, const CauHoi& ch);

// Xoá câu theo id (trả true nếu xoá)
bool remove_cau_by_id(PTRCH& head, int id);

// Tạo id ngẫu nhiên và đảm bảo duy nhất trên toàn cây môn học
int new_question_id_unique(PTRMH root);

// Kiểm tra id đã tồn tại trên toàn cây?
bool exists_question_id_in_tree(PTRMH root, int id);

#endif // CAUHOI_H
