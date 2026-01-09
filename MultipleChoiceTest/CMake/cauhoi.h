#ifndef CAUHOI_H
#define CAUHOI_H

#include "structs.h"

// ===== CÂU HỎI: DSLK =====
int   dem_cau(PTRCH head);
PTRCH find_cau_by_id(PTRCH head, int id);
void  add_cau_hoi(PTRCH& head, const CauHoi& ch);
bool  remove_cau_by_id(PTRCH& head, int id);

// ===== ID câu hỏi duy nhất toàn hệ thống (O(1) khi phát) =====
//
// Cơ chế:
// - new_question_id_unique(): trả về ID mới và tăng bộ đếm.
// - load_question_id_meta(): đọc next_id từ file meta (O(1)).
// - save_question_id_meta(): ghi next_id ra file meta (O(1)).
// - sync_question_id_after_load(): fallback nếu file meta mất/hỏng,
//   quét max_id toàn cây rồi set next_id = max+1 (chỉ làm lúc load).
//
int  new_question_id_unique(PTRMH root_ignored);
int  get_next_question_id();
bool load_question_id_meta(const char* path);
bool save_question_id_meta(const char* path);
void sync_question_id_after_load(PTRMH root);

#endif // CAUHOI_H
