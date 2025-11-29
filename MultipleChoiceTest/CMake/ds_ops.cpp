// ds_ops.cpp
#include "ds_ops.h"
#include "string_utils.h"
#include "diemthi.h"
#include <cstdio>
#include <cstdlib>

/*
 * QUY ƯỚC MÃ LỖI (return int):
 *
 * them_lop:
 *  0 = thành công
 *  1 = trùng mã lớp
 *  2 = DS lớp đã đầy
 *  3 = mã lớp không hợp lệ (không phải A-Z/0-9 sau normalize_code)
 *
 * them_sv_vao_ds:
 *  0 = thành công
 *  1 = không tìm thấy lớp
 *  2 = MASV đã tồn tại (toàn hệ thống)
 *  3 = dữ liệu SV không hợp lệ (mã sai, phái sai, password sai)
 *
 * them_sv_vao_lop:
 *  0 = thành công
 *  1 = MASV trùng trong lớp
 *  2 = dữ liệu SV không hợp lệ (phái/password)
 *
 * sua_sv_trong_lop:
 *  0 = thành công
 *  1 = không tìm thấy SV trong lớp
 *  2 = dữ liệu mới không hợp lệ (phái/password)
 *
 * doi_masv:
 *  0 = thành công
 *  1 = không tìm thấy SV
 *  2 = MASV mới đã tồn tại (global)
 *  3 = MASV mới không hợp lệ
 */

// ---------------------- Validate cơ bản ----------------------

static bool valid_password(const char* pw) {
    int len = su_strlen(pw);
    return len >= 4 && len <= 20;
}

static bool valid_gender(const char* g) {
    // chấp nhận NAM / NU (không phân biệt hoa thường – dùng su_stricmp)
    return (su_stricmp(g, "NAM") == 0 || su_stricmp(g, "NU") == 0);
}

// ---------------------- LỚP ----------------------

void init_ds_lop(DS_Lop& ds) {
    ds.n = 0;
    for (int i = 0; i < MAX_LOP; ++i) {
        ds.nodes[i] = NULL;
    }
}

// Tìm index lớp theo mã (so sánh không phân biệt hoa thường)
int find_lop_index(const DS_Lop& ds, const char* malop_ci) {
    if (!malop_ci) return -1;
    for (int i = 0; i < ds.n; ++i) {
        if (ds.nodes[i] && su_stricmp(ds.nodes[i]->malop, malop_ci) == 0) {
            return i;
        }
    }
    return -1;
}

Lop* get_lop(DS_Lop& ds, const char* malop_ci) {
    int idx = find_lop_index(ds, malop_ci);
    if (idx < 0) return NULL;
    return ds.nodes[idx];
}

const Lop* get_lop_const(const DS_Lop& ds, const char* malop_ci) {
    int idx = find_lop_index(ds, malop_ci);
    if (idx < 0) return NULL;
    return ds.nodes[idx];
}

// Thêm lớp vào danh sách
// 0: ok, 1: trùng mã, 2: DS đầy, 3: mã không hợp lệ
int them_lop(DS_Lop& ds, const char* malop, const char* tenlop) {
    if (ds.n >= MAX_LOP) {
        return 2; // full
    }

    // Chuẩn hoá mã lớp
    char ml[16];
    su_strncpy(ml, malop, 16);
    normalize_code(ml);          // trim + xoá space + upper + bỏ ký tự lạ

    // Mã lớp phải chỉ gồm [A-Z0-9], không rỗng
    if (!is_code_alnum_upper(ml)) {
        return 3;
    }

    // Check trùng (case-insensitive)
    if (find_lop_index(ds, ml) >= 0) {
        return 1; // trùng mã
    }

    // Tạo node lớp mới
    Lop* p = new Lop;
    su_strncpy(p->malop, ml, 16);

    // Chuẩn hoá tên lớp (gộp space, viết hoa chữ cái đầu)
    char tl[51];
    su_strncpy(tl, tenlop, 51);
    normalize_name(tl);
    su_strncpy(p->tenlop, tl, 51);

    p->FirstSV = NULL;

    ds.nodes[ds.n++] = p;
    return 0;
}

// Xoá 1 lớp theo mã (không phân biệt hoa thường)
// Chặn xoá nếu trong lớp có SV đã có điểm (ds_diemthi != NULL)
bool xoa_lop(DS_Lop& ds, const char* malop_ci) {
    int idx = find_lop_index(ds, malop_ci);
    if (idx < 0) return false;

    Lop* lp = ds.nodes[idx];
    if (!lp) return false;

    // Nếu có SV đã có điểm -> không cho xoá
    for (PTRSV p = lp->FirstSV; p; p = p->next) {
        if (p->data.ds_diemthi != NULL) {
            return false;
        }
    }

    // Free toàn bộ SV trong lớp
    free_list_sv(lp->FirstSV);

    // Xoá lớp
    delete lp;

    // Dồn mảng lỗ hổng
    for (int i = idx + 1; i < ds.n; ++i) {
        ds.nodes[i - 1] = ds.nodes[i];
    }
    ds.nodes[--ds.n] = NULL;

    return true;
}

// Đếm số SV trong 1 lớp
int so_sinh_vien_trong_lop(const Lop* lop) {
    if (!lop) return 0;
    int c = 0;
    for (PTRSV p = lop->FirstSV; p; p = p->next) ++c;
    return c;
}

// ---------------------- SINH VIÊN ----------------------

// Kiểm tra MASV đã tồn tại ở bất kỳ lớp nào (global)
bool exists_sv_global(const DS_Lop& ds, const char* masv_ci) {
    if (!masv_ci) return false;
    for (int i = 0; i < ds.n; ++i) {
        const Lop* lop = ds.nodes[i];
        if (!lop) continue;
        for (PTRSV p = lop->FirstSV; p; p = p->next) {
            if (su_stricmp(p->data.masv, masv_ci) == 0) {
                return true;
            }
        }
    }
    return false;
}

// Tìm SV trong 1 lớp (case-insensitive MASV), có trả về prev nếu cần
PTRSV tim_sv_trong_lop(Lop* lop, const char* masv_ci, PTRSV* prev_out) {
    if (prev_out) *prev_out = NULL;
    if (!lop || !masv_ci) return NULL;

    PTRSV prev = NULL;
    for (PTRSV p = lop->FirstSV; p; prev = p, p = p->next) {
        if (su_stricmp(p->data.masv, masv_ci) == 0) {
            if (prev_out) *prev_out = prev;
            return p;
        }
    }
    return NULL;
}

// Version const (không sửa lớp)
PTRSV tim_sv_trong_lop_const(const Lop* lop, const char* masv_ci) {
    if (!lop || !masv_ci) return NULL;
    for (PTRSV p = lop->FirstSV; p; p = p->next) {
        if (su_stricmp(p->data.masv, masv_ci) == 0) return p;
    }
    return NULL;
}

// Chỉ check: MASV, phái, password (MASV đã normalize trước khi truyền vào)
static bool valid_sv_fields(const char* masv,
                            const char* phai,
                            const char* password) {
    // MASV: normalize lại lần nữa cho chắc (phòng chỗ khác gọi trực tiếp)
    char t[16];
    su_strncpy(t, masv, 16);
    normalize_code(t);
    if (!is_code_alnum_upper(t)) return false;

    // Phái: normalize về upper rồi kiểm tra NAM/NU
    char g[8];
    su_strncpy(g, phai, 8);
    normalize_gender(g);
    if (!valid_gender(g)) return false;

    // Password: 4–20 ký tự
    if (!valid_password(password)) return false;

    return true;
}

// Thêm SV vào 1 lớp (đã xác định Lop*)
// 0: ok, 1: trùng MASV trong lớp, 2: dữ liệu SV không hợp lệ
int them_sv_vao_lop(Lop* lop, const SinhVien& sv) {
    if (!lop) return 2;

    // Kiểm tra trùng MASV trong cùng lớp
    if (tim_sv_trong_lop(lop, sv.masv, NULL)) {
        return 1;
    }

    // Validate phái/password tối thiểu (phòng TH ai đó tạo SinhVien thủ công)
    char g[8];
    su_strncpy(g, sv.phai, 8);
    normalize_gender(g);
    if (!valid_gender(g)) {
        return 2;
    }
    if (!valid_password(sv.password)) {
        return 2;
    }

    // Chèn đầu danh sách
    PTRSV node = new NodeSV;
    node->data = sv;                 // copy struct
    node->data.ds_diemthi = NULL;    // luôn khởi tạo rỗng
    node->next = lop->FirstSV;
    lop->FirstSV = node;

    return 0;
}

/*
 * Thêm SV vào DS theo mã lớp + các field thô từ UI.
 *
 *  - Mã lớp, MASV được normalize_code (trim, upper, bỏ ký tự lạ).
 *  - Kiểm tra:
 *      + Lớp tồn tại?
 *      + MASV duy nhất toàn DS?
 *      + Dữ liệu SV hợp lệ (MASV + phái + password)?
 *
 *  Return:
 *   0 = OK
 *   1 = không tìm thấy lớp
 *   2 = MASV đã tồn tại (global)
 *   3 = dữ liệu SV không hợp lệ
 */
int them_sv_vao_ds(DS_Lop& ds,
                   const char* malop,
                   const char* masv,
                   const char* ho,
                   const char* ten,
                   const char* phai,
                   const char* password) {
    // Chuẩn hoá mã lớp
    char ml[16];
    su_strncpy(ml, malop, 16);
    normalize_code(ml);

    // Tìm lớp
    Lop* pLop = get_lop(ds, ml);
    if (!pLop) {
        return 1; // không tìm thấy lớp
    }

    // Chuẩn hoá MASV
    char ms[16];
    su_strncpy(ms, masv, 16);
    normalize_code(ms);

    // Kiểm tra MASV global duy nhất
    if (exists_sv_global(ds, ms)) {
        return 2; // MASV đã tồn tại
    }

    // Validate chung (MASV, phái, password)
    if (!valid_sv_fields(ms, phai, password)) {
        return 3; // dữ liệu không hợp lệ
    }

    // Chuẩn hoá họ tên
    SinhVien sv;

    su_strncpy(sv.masv, ms, 16);

    char h[51];
    su_strncpy(h, ho, 51);
    normalize_name(h);

    char t[16];
    su_strncpy(t, ten, 16);
    normalize_name(t);

    su_strncpy(sv.ho, h, 51);
    su_strncpy(sv.ten, t, 16);

    // Chuẩn hoá phái về NAM/NU
    char g[8];
    su_strncpy(g, phai, 8);
    normalize_gender(g);
    su_strncpy(sv.phai, g, 4);

    // Password giữ nguyên (chỉ kiểm tra độ dài)
    su_strncpy(sv.password, password, 21);

    sv.ds_diemthi = NULL;

    // Thêm vào lớp
    return them_sv_vao_lop(pLop, sv);
}

// Sửa thông tin SV (họ, tên, phái, password) trong 1 lớp cụ thể
// 0: ok, 1: không tìm thấy SV, 2: dữ liệu mới không hợp lệ
int sua_sv_trong_lop(Lop* lop,
                     const char* masv_ci,
                     const char* ho,
                     const char* ten,
                     const char* phai,
                     const char* password) {
    if (!lop) return 1;

    PTRSV p = tim_sv_trong_lop(lop, masv_ci, NULL);
    if (!p) {
        return 1; // không tìm thấy
    }

    // Chuẩn hoá họ, tên
    char h[51];
    su_strncpy(h, ho, 51);
    normalize_name(h);

    char t[16];
    su_strncpy(t, ten, 16);
    normalize_name(t);

    // Chuẩn hoá phái
    char g[8];
    su_strncpy(g, phai, 8);
    normalize_gender(g);
    if (!valid_gender(g)) {
        return 2;
    }

    // Kiểm tra password
    if (!valid_password(password)) {
        return 2;
    }

    // Ghi đè dữ liệu
    su_strncpy(p->data.ho, h, 51);
    su_strncpy(p->data.ten, t, 16);
    su_strncpy(p->data.phai, g, 4);
    su_strncpy(p->data.password, password, 21);

    return 0;
}

// Đổi MASV cho 1 SV trong 1 lớp
// 0: ok, 1: không tìm thấy SV, 2: MASV mới đã tồn tại, 3: MASV mới không hợp lệ
int doi_masv(DS_Lop& ds, Lop* lop,
             const char* masv_cu_ci,
             const char* masv_moi) {
    if (!lop) return 1;

    PTRSV p = tim_sv_trong_lop(lop, masv_cu_ci, NULL);
    if (!p) {
        return 1; // không tìm thấy SV cũ
    }

    // Chuẩn hoá MASV mới
    char ms[16];
    su_strncpy(ms, masv_moi, 16);
    normalize_code(ms);

    // Mã mới phải hợp lệ
    if (!is_code_alnum_upper(ms)) {
        return 3;
    }

    // Và chưa tồn tại global
    if (exists_sv_global(ds, ms)) {
        return 2;
    }

    su_strncpy(p->data.masv, ms, 16);
    return 0;
}

// Xoá SV khỏi lớp (chặn nếu SV đã có điểm thi)
// Return: true = xoá được, false = không xoá được
bool xoa_sv_khoi_lop(Lop* lop, const char* masv_ci) {
    if (!lop || !masv_ci) return false;

    PTRSV prev = NULL;
    PTRSV p = tim_sv_trong_lop(lop, masv_ci, &prev);
    if (!p) return false;

    // Không xoá nếu SV đã có điểm thi
    if (p->data.ds_diemthi != NULL) {
        return false;
    }

    // Gỡ node ra khỏi danh sách
    if (!prev) {
        lop->FirstSV = p->next;
    } else {
        prev->next = p->next;
    }

    // ds_diemthi đã check NULL, chỉ xoá node
    delete p;
    return true;
}

// Đếm số SV trong 1 list
int dem_sv(PTRSV head) {
    int c = 0;
    for (PTRSV p = head; p; p = p->next) ++c;
    return c;
}

// ---------------------- FREE ----------------------

// Giải phóng toàn bộ danh sách SV (và danh sách điểm của từng SV)
void free_list_sv(PTRSV& head) {
    PTRSV p = head;
    while (p) {
        // giải phóng danh sách điểm thi của SV
        free_ds_diemthi(p->data.ds_diemthi);

        PTRSV q = p->next;
        delete p;
        p = q;
    }
    head = NULL;
}

// Giải phóng toàn bộ DS lớp
void free_ds_lop(DS_Lop& ds) {
    for (int i = 0; i < ds.n; ++i) {
        if (ds.nodes[i]) {
            free_list_sv(ds.nodes[i]->FirstSV);
            delete ds.nodes[i];
            ds.nodes[i] = NULL;
        }
    }
    ds.n = 0;
}
