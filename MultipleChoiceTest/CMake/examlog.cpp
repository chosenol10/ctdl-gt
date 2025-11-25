// examlog.cpp
#include "examlog.h"
#include "string_utils.h"
#include <cstdlib>

void init_examlog(PTRExamLog& head) { head = NULL; }

static void free_items(ExamQItem*& items) {
    if (items) { delete[] items; items = NULL; }
}

void free_examlog(PTRExamLog& head) {
    ExamRecord* p = head;
    while (p) {
        ExamRecord* q = p->next;
        free_items(p->items);
        delete p;
        p = q;
    }
    head = NULL;
}

PTRExamLog find_exam(PTRExamLog head, const char* masv_ci, const char* mamh_ci) {
    for (ExamRecord* p = head; p; p = p->next) {
        if (su_stricmp(p->masv, masv_ci) == 0 && su_stricmp(p->mamh, mamh_ci) == 0) return p;
    }
    return NULL;
}

void upsert_exam(PTRExamLog& head,
                 const char* masv_ci,
                 const char* mamh_ci,
                 const ExamQItem* items, int soCau,
                 int soCauDung, float diem) {
    ExamRecord* p = find_exam(head, masv_ci, mamh_ci);
    if (!p) {
        p = new ExamRecord;
        su_strncpy(p->masv, masv_ci, 16);
        su_strncpy(p->mamh, mamh_ci, 16);
        p->soCau = 0;
        p->items = NULL;
        p->soCauDung = 0;
        p->diem = 0.0f;
        p->next = head;
        head = p;
    }
    // copy items
    free_items(p->items);
    p->soCau = soCau;
    p->items = new ExamQItem[soCau];
    for (int i = 0; i < soCau; ++i) p->items[i] = items[i];
    p->soCauDung = soCauDung;
    p->diem = diem;
}

bool examlog_contains_question(PTRExamLog head, const char* mamh_ci, int id) {
    for (ExamRecord* p = head; p; p = p->next) {
        if (su_stricmp(p->mamh, mamh_ci) != 0) continue;
        for (int i = 0; i < p->soCau; ++i) if (p->items[i].id == id) return true;
    }
    return false;
}

void remove_all_exam_of_sv(PTRExamLog& head, const char* masv_ci) {
    ExamRecord* p = head; ExamRecord* prev = NULL;
    while (p) {
        if (su_stricmp(p->masv, masv_ci) == 0) {
            ExamRecord* del = p;
            if (!prev) head = p->next; else prev->next = p->next;
            p = p->next;
            free_items(del->items);
            delete del;
            continue;
        }
        prev = p; p = p->next;
    }
}
