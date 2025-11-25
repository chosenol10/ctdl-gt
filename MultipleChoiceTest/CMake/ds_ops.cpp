// ds_ops.cpp
#include "ds_ops.h"
#include "string_utils.h"
#include "diemthi.h"
#include <cstdio>
#include <cstdlib>

static bool valid_password(const char* pw) {
    int len = su_strlen(pw);
    return len >= 4 && len <= 20;
}
static bool valid_gender(const char* g) {
    return (su_stricmp(g, "NAM") == 0 || su_stricmp(g, "NU") == 0);
}

// ======= LỚP =======
void init_ds_lop(DS_Lop& ds) {
    ds.n = 0;
    for (int i = 0; i < MAX_LOP; ++i) ds.nodes[i] = NULL;
}

int find_lop_index(const DS_Lop& ds, const char* malop_ci) {
    for (int i = 0; i < ds.n; ++i) {
        if (ds.nodes[i] && su_stricmp(ds.nodes[i]->malop, malop_ci) == 0) return i;
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

int them_lop(DS_Lop& ds, const char* malop, const char* tenlop) {
    if (ds.n >= MAX_LOP) return 2;
    char ml[16]; su_strncpy(ml, malop, 16); normalize_code(ml);
    if (!is_code_alnum_upper(ml)) return 3;
    if (find_lop_index(ds, ml) >= 0) return 1;

    Lop* p = new Lop;
    su_strncpy(p->malop, ml, 16);
    char tl[51]; su_strncpy(tl, tenlop, 51); normalize_name(tl);
    su_strncpy(p->tenlop, tl, 51);
    p->FirstSV = NULL;

    ds.nodes[ds.n++] = p;
    return 0;
}

// Chặn xoá lớp nếu có SV có điểm -> đảm bảo nghiệp vụ
bool xoa_lop(DS_Lop& ds, const char* malop_ci) {
    int idx = find_lop_index(ds, malop_ci);
    if (idx < 0) return false;
    Lop* lp = ds.nodes[idx];
    // nếu có SV có điểm -> chặn
    for (PTRSV p = lp->FirstSV; p; p = p->next) {
        if (p->data.ds_diemthi != NULL) {
            // chặn xoá (y/c người dùng xoá điểm/bài thi trước)
            return false;
        }
    }
    // free list SV
    free_list_sv(lp->FirstSV);
    delete lp;
    // dồn mảng
    for (int i = idx + 1; i < ds.n; ++i) ds.nodes[i - 1] = ds.nodes[i];
    ds.nodes[--ds.n] = NULL;
    return true;
}

int so_sinh_vien_trong_lop(const Lop* lop) {
    if (!lop) return 0;
    int c = 0; for (PTRSV p = lop->FirstSV; p; p = p->next) ++c; return c;
}

// ======= SINH VIÊN =======
bool exists_sv_global(const DS_Lop& ds, const char* masv_ci) {
    for (int i = 0; i < ds.n; ++i) {
        PTRSV p = ds.nodes[i]->FirstSV;
        for (; p; p = p->next) {
            if (su_stricmp(p->data.masv, masv_ci) == 0) return true;
        }
    }
    return false;
}

PTRSV tim_sv_trong_lop(Lop* lop, const char* masv_ci, PTRSV* prev_out) {
    if (prev_out) *prev_out = NULL;
    if (!lop) return NULL;
    PTRSV prev = NULL;
    for (PTRSV p = lop->FirstSV; p; prev = p, p = p->next) {
        if (su_stricmp(p->data.masv, masv_ci) == 0) {
            if (prev_out) *prev_out = prev;
            return p;
        }
    }
    return NULL;
}

// version const: không sửa lop, chỉ tìm SV
PTRSV tim_sv_trong_lop_const(const Lop* lop, const char* masv_ci) {
    if (!lop) return NULL;
    for (PTRSV p = lop->FirstSV; p; p = p->next) {
        if (su_stricmp(p->data.masv, masv_ci) == 0) return p;
    }
    return NULL;
}

// chỉ check MASV, phái, password (ho/ten được normalize riêng)
static bool valid_sv_fields(const char* masv,
                            const char* phai,
                            const char* password) {
    char t[16];
    su_strncpy(t, masv, 16); normalize_code(t);
    if (!is_code_alnum_upper(t)) return false;

    char g[8];
    su_strncpy(g, phai, 8); normalize_gender(g);
    if (!valid_gender(g)) return false;

    if (!valid_password(password)) return false;
    return true;
}

int them_sv_vao_lop(Lop* lop, const SinhVien& sv) {
    if (!lop) return 2;
    // Kiểm tra trùng trong lớp
    if (tim_sv_trong_lop(lop, sv.masv, NULL)) return 1;

    // Validate phái/password tối thiểu
    char g[8]; su_strncpy(g, sv.phai, 8); normalize_gender(g);
    if (!valid_gender(g)) return 2;
    if (!valid_password(sv.password)) return 2;

    // Chèn đầu
    PTRSV node = new NodeSV;
    node->data = sv; // copy
    node->data.ds_diemthi = NULL; // danh sách điểm tách riêng, đảm bảo không copy nhầm
    node->next = lop->FirstSV;
    lop->FirstSV = node;
    return 0;
}

int them_sv_vao_ds(DS_Lop& ds,
                   const char* malop,
                   const char* masv,
                   const char* ho,
                   const char* ten,
                   const char* phai,
                   const char* password) {
    char ml[16]; su_strncpy(ml, malop, 16); normalize_code(ml);
    Lop* pLop = get_lop(ds, ml);
    if (!pLop) return 1;

    // MASV toàn DS duy nhất
    char ms[16]; su_strncpy(ms, masv, 16); normalize_code(ms);
    if (exists_sv_global(ds, ms)) return 2;

    // Validate chung (MASV, phái, password)
    if (!valid_sv_fields(ms, phai, password)) return 3;

    SinhVien sv;
    su_strncpy(sv.masv, ms, 16);

    char h[51]; su_strncpy(h, ho, 51); normalize_name(h);
    char t[16]; su_strncpy(t, ten, 16); normalize_name(t);
    su_strncpy(sv.ho, h, 51);
    su_strncpy(sv.ten, t, 16);

    char g[8]; su_strncpy(g, phai, 8); normalize_gender(g);
    su_strncpy(sv.phai, g, 4);

    su_strncpy(sv.password, password, 21);
    sv.ds_diemthi = NULL;

    return them_sv_vao_lop(pLop, sv);
}

int sua_sv_trong_lop(Lop* lop,
                     const char* masv_ci,
                     const char* ho,
                     const char* ten,
                     const char* phai,
                     const char* password) {
    if (!lop) return 1;
    PTRSV p = tim_sv_trong_lop(lop, masv_ci, NULL);
    if (!p) return 1;

    // validate dữ liệu mới
    char h[51]; su_strncpy(h, ho, 51); normalize_name(h);
    char t[16]; su_strncpy(t, ten, 16); normalize_name(t);
    char g[8]; su_strncpy(g, phai, 8); normalize_gender(g);
    if (!valid_gender(g)) return 2;
    if (!valid_password(password)) return 2;

    su_strncpy(p->data.ho, h, 51);
    su_strncpy(p->data.ten, t, 16);
    su_strncpy(p->data.phai, g, 4);
    su_strncpy(p->data.password, password, 21);
    return 0;
}

int doi_masv(DS_Lop& ds, Lop* lop, const char* masv_cu_ci, const char* masv_moi) {
    if (!lop) return 1;
    PTRSV p = tim_sv_trong_lop(lop, masv_cu_ci, NULL);
    if (!p) return 1;

    char ms[16]; su_strncpy(ms, masv_moi, 16); normalize_code(ms);
    if (!is_code_alnum_upper(ms)) return 3;
    if (exists_sv_global(ds, ms)) return 2;

    su_strncpy(p->data.masv, ms, 16);
    return 0;
}

bool xoa_sv_khoi_lop(Lop* lop, const char* masv_ci) {
    if (!lop) return false;
    PTRSV prev = NULL;
    PTRSV p = tim_sv_trong_lop(lop, masv_ci, &prev);
    if (!p) return false;

    // Chặn xoá nếu đã có điểm
    if (p->data.ds_diemthi != NULL) {
        return false; // cần xoá dữ liệu thi trước (examlog + điểm)
    }

    if (!prev) lop->FirstSV = p->next;
    else prev->next = p->next;

    // điểm đã kiểm tra NULL; free SV node
    delete p;
    return true;
}

int dem_sv(PTRSV head) {
    int c = 0; for (PTRSV p = head; p; p = p->next) ++c; return c;
}

// ======= FREE =======
void free_list_sv(PTRSV& head) {
    PTRSV p = head;
    while (p) {
        free_ds_diemthi(p->data.ds_diemthi);
        PTRSV q = p->next;
        delete p;
        p = q;
    }
    head = NULL;
}

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
