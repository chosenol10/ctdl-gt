// diemthi.cpp
#include "diemthi.h"
#include "string_utils.h"
#include <cstdio>

PTRDiemThi tim_diem(PTRDiemThi head, const char* mamh_ci) {
    for (PTRDiemThi p = head; p != NULL; p = p->next) {
        if (compare_code_ci(p->data.mamh, mamh_ci) == 0) return p;
    }
    return NULL;
}

void upsert_diem(PTRDiemThi& head, const char* mamh_ci, float diem) {
    PTRDiemThi p = tim_diem(head, mamh_ci);
    if (p) {
        p->data.diem = diem;
        return;
    }
    PTRDiemThi node = new NodeDiem;
    su_strncpy(node->data.mamh, mamh_ci, 16);
    node->data.diem = diem;
    node->next = head;
    head = node;
}

int dem_diem(PTRDiemThi head) {
    int c = 0; for (PTRDiemThi p = head; p; p = p->next) ++c; return c;
}

void free_ds_diemthi(PTRDiemThi& head) {
    PTRDiemThi p = head;
    while (p) {
        PTRDiemThi q = p->next;
        delete p;
        p = q;
    }
    head = NULL;
}
