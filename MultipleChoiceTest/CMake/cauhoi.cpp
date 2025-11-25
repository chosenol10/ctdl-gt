// cauhoi.cpp
#include "cauhoi.h"
#include "rng.h"
#include <cstdio>

int dem_cau(PTRCH head) {
    int c = 0; for (PTRCH p = head; p; p = p->next) ++c; return c;
}

PTRCH find_cau_by_id(PTRCH head, int id) {
    for (PTRCH p = head; p; p = p->next) if (p->data.id == id) return p;
    return NULL;
}

void add_cau_hoi(PTRCH& head, const CauHoi& ch) {
    PTRCH node = new NodeCH; node->data = ch; node->next = NULL;
    if (!head) { head = node; return; }
    PTRCH p = head;
    while (p->next) p = p->next;
    p->next = node;
}

bool remove_cau_by_id(PTRCH& head, int id) {
    PTRCH prev = NULL; PTRCH p = head;
    while (p) {
        if (p->data.id == id) {
            if (!prev) head = p->next; else prev->next = p->next;
            delete p;
            return true;
        }
        prev = p; p = p->next;
    }
    return false;
}

static bool exists_id_in_list(PTRCH h, int id) {
    for (PTRCH p = h; p; p = p->next) if (p->data.id == id) return true;
    return false;
}
bool exists_question_id_in_tree(PTRMH root, int id) {
    if (!root) return false;
    if (exists_id_in_list(root->data.FirstCHT, id)) return true;
    return exists_question_id_in_tree(root->left, id) || exists_question_id_in_tree(root->right, id);
}

int new_question_id_unique(PTRMH root) {
    // Thử ngẫu nhiên 6 chữ số. Nếu xui va chạm nhiều, fallback tăng dần.
    rng_seed_once();
    for (int tries = 0; tries < 1000; ++tries) {
        int cand = rand_in(100000, 999999);
        if (!exists_question_id_in_tree(root, cand)) return cand;
    }
    // fallback: tuyến tính từ 1 lên
    int id = 1;
    while (exists_question_id_in_tree(root, id)) ++id;
    return id;
}
