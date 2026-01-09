#include "cauhoi.h"
#include "string_utils.h"
#include <cstdio>

// ================== DS câu hỏi ==================
int dem_cau(PTRCH head) {
    int c = 0;
    for (PTRCH p = head; p; p = p->next) ++c;
    return c;
}

PTRCH find_cau_by_id(PTRCH head, int id) {
    for (PTRCH p = head; p; p = p->next)
        if (p->data.id == id) return p;
    return NULL;
}

void add_cau_hoi(PTRCH& head, const CauHoi& ch) {
    PTRCH node = new NodeCH;
    node->data = ch;
    node->next = NULL;

    if (!head) { head = node; return; }
    PTRCH p = head;
    while (p->next) p = p->next;
    p->next = node;
}

bool remove_cau_by_id(PTRCH& head, int id) {
    PTRCH prev = NULL;
    PTRCH p = head;
    while (p) {
        if (p->data.id == id) {
            if (!prev) head = p->next;
            else prev->next = p->next;
            delete p;
            return true;
        }
        prev = p;
        p = p->next;
    }
    return false;
}

// ================== ID generator (global auto-increment) ==================
static int g_next_qid = 1;

int get_next_question_id() {
    return g_next_qid;
}

static int max2(int a, int b) { return (a > b) ? a : b; }

static int max_id_in_list(PTRCH h) {
    int mx = 0;
    for (PTRCH p = h; p; p = p->next) {
        if (p->data.id > mx) mx = p->data.id;
    }
    return mx;
}

static int max_id_in_tree(PTRMH r) {
    if (!r) return 0;
    int m0 = max_id_in_list(r->data.FirstCHT);
    int ml = max_id_in_tree(r->left);
    int mr = max_id_in_tree(r->right);
    return max2(m0, max2(ml, mr));
}

// Fallback: nếu meta file không có/hỏng -> quét max toàn cây 1 lần lúc load
void sync_question_id_after_load(PTRMH root) {
    int mx = max_id_in_tree(root);
    int next = mx + 1;
    if (next <= 0) next = 1;
    g_next_qid = next;
}

// O(1) load meta
bool load_question_id_meta(const char* path) {
    if (!path) return false;
    FILE* f = std::fopen(path, "rt");
    if (!f) return false;

    int next = 0;
    int ok = std::fscanf(f, "%d", &next);
    std::fclose(f);

    if (ok != 1) return false;
    if (next <= 0) return false;

    g_next_qid = next;
    return true;
}

// O(1) save meta
bool save_question_id_meta(const char* path) {
    if (!path) return false;
    FILE* f = std::fopen(path, "wt");
    if (!f) return false;

    std::fprintf(f, "%d\n", g_next_qid);
    std::fclose(f);
    return true;
}

// O(1) phát ID (KHÔNG check trùng mỗi lần)
int new_question_id_unique(PTRMH /*root_ignored*/) {
    int id = g_next_qid;
    // tránh trường hợp overflow (rất khó xảy ra với bài này)
    if (g_next_qid < 2147483647) ++g_next_qid;
    return id;
}
