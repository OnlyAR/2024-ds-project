#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

#define CODES_FILE_SIZE (1 << 26)
#define KEEPWORD_NUM 256
#define KEEPWORD_LEN 16
#define TRIE_NODE_NUM (1 << 20)
#define IDENTIFIER_LEN 64
#define PAGE_CONTENT_LEN (1 << 13)
#define PAGE_NUM (1 << 13)
#define PAGE_FUNC_NUM (1 << 10)
#define THRESHOLD 0.95

#define max2(a, b) ((a)>(b)?(a):(b))

#ifdef DEBUG

#include <time.h>

#define LOG_FILENAME_LEN 64
#define MESSAGE_LEN (1 << 20)
#endif

typedef struct TrieNode {
    int val;
    struct TrieNode *branches[64];
} TrieNode, *TriePtr;

typedef struct FuncStream {
    unsigned invoke_sort;
    char func_name[IDENTIFIER_LEN];
    char *stream;
} FuncStream, *FuncStreamPtr;

typedef struct Page {
    unsigned id;
    char shared[PAGE_CONTENT_LEN];
    char proc_stream[PAGE_CONTENT_LEN];
    unsigned body_len;
    unsigned proc_stream_len;
    unsigned func_num;
    unsigned invoked_func_num;
    FuncStream func_streams[PAGE_FUNC_NUM];
    FuncStreamPtr sorted_funcs[PAGE_FUNC_NUM];
    TriePtr func_dict;
} Page;

struct Reader {
    unsigned idx;
    char *content;
} reader;

struct TrieAllocator {
    TriePtr base;
    unsigned off;
} allocator;

char codes_content[CODES_FILE_SIZE];
Page pages[PAGE_NUM];
unsigned page_num = 0;
TriePtr keepword_set;
unsigned dp[PAGE_CONTENT_LEN][PAGE_CONTENT_LEN];
unsigned page_set[PAGE_NUM];

void init();

TriePtr create_node();

unsigned ch2idx(char ch);

TriePtr insert(TriePtr root, char *word, int val);

int search(TriePtr root, char *word);

void read_codes_content(int, char **);

void build_keepwords_set();

int is_keepword(char *word);

int is_ignore(char ch);

int is_iden_prefix(char ch);

int is_iden_allowed(char ch);

char read_char();

void read_next();

unsigned get_proc_id();

unsigned tokenize(char *buf);

unsigned build_proc_stream();

int build_func_stream();

int func_cmp(const void *p1, const void *p2);

unsigned min3(unsigned a, unsigned b, unsigned c);

unsigned edit_dis_dp(const char *str1, const char *str2, unsigned l1, unsigned l2);

void check_sim();

int is_sim(Page *p1, Page *p2);

#ifdef DEBUG
enum LEVEL {
    INFO, ERROR
};

char message[MESSAGE_LEN];

FILE *debug_file_handler = NULL;

void create_debug_file();

void close_debug_file();

void debug(enum LEVEL level, char *msg);

#endif

int main(int argc, char *argv[]) {
    init();
    build_keepwords_set();
    read_codes_content(argc, argv);

    while (build_proc_stream());
    check_sim();

#ifdef DEBUG
    close_debug_file();
#endif
    return 0;
}


void init() {
    reader.content = codes_content;
    allocator.base = (TriePtr) malloc(sizeof(TrieNode) * TRIE_NODE_NUM);

#ifdef DEBUG
    create_debug_file();
#endif
}

inline TriePtr create_node() {
    return allocator.base + allocator.off++;
}

unsigned ch2idx(char ch) {
    if (ch == '_') return 0;
    if (isdigit(ch)) return 1 + ch - '0';
    if (isupper(ch)) return 1 + 10 + ch - 'A';
    if (islower(ch)) return 1 + 10 + 26 + ch - 'a';

#ifdef DEBUG
    sprintf(message, "Char [%c] can't be mapped", ch);
    debug(ERROR, message);
#endif

    return -1;
}

TriePtr insert(TriePtr root, char *word, int val) {
    if (root == NULL) root = create_node();
    if (*word == '\0') root->val = val;
    else root->branches[ch2idx(*word)] = insert(root->branches[ch2idx(*word)], word + 1, val);
    return root;
}

int search(TriePtr root, char *word) {
    if (root == NULL) return 0;
    if (*word == '\0') return root->val;
    return search(root->branches[ch2idx(*word)], word + 1);
}

void read_codes_content(int argc, char *argv[]) {
    char *filename = argc == 1 ? "codes.txt" : argv[1];
    FILE *in = fopen(filename, "rb");
    unsigned length = fread(codes_content, 1, CODES_FILE_SIZE, in);
    fclose(in);

#ifdef DEBUG
    sprintf(message, "Codes read, length: %u.\n\n", length);
    debug(INFO, message);
#endif
}

void build_keepwords_set() {
    FILE *in = fopen("keepwords.txt", "rb");
    char buf[KEEPWORD_NUM * KEEPWORD_LEN];
    unsigned len = fread(buf, 1, KEEPWORD_NUM * KEEPWORD_LEN, in);
    buf[len++] = '\n'; // add a '\n' at the tail manually
    fclose(in);
    char word[KEEPWORD_LEN];
    unsigned word_len = 0;

    for (unsigned i = 0; i < len; i++) {
        if (buf[i] != '\n' && buf[i] != '\r') {
            word[word_len++] = buf[i];
        } else if (word[0]) {
            word[word_len] = '\0';
            keepword_set = insert(keepword_set, word, 1);
            word_len = 0;
            word[0] = '\0';
        }
    }
}

inline int is_keepword(char *word) { return search(keepword_set, word) == 1; }

inline int is_ignore(char ch) { return ch == ' ' || ch == '\r' || ch == '\n' || ch == '\t'; }

inline int is_iden_prefix(char ch) { return ch == '_' || isalpha(ch); }

inline int is_iden_allowed(char ch) { return ch == '_' || isalnum(ch); }

inline char read_char() { return reader.content[reader.idx]; }

inline void read_next() { reader.idx++; }

unsigned get_proc_id() {
    unsigned num = 0;
    while (is_ignore(read_char())) read_next();
    while (isdigit(read_char())) {
        num = num * 10 + read_char() - '0';
        read_next();
    }
    return num;
}

unsigned tokenize(char *buf) {
    while (is_ignore(read_char())) read_next();
    if (is_iden_prefix(read_char())) {
        int len = 0;
        while (is_iden_allowed(read_char())) {
            buf[len++] = read_char();
            read_next();
        }
        buf[len] = '\0';
        return len;
    } else {
        buf[0] = read_char();
        buf[1] = '\0';
        read_next();
        return 1;
    }
}

unsigned build_proc_stream() {
    unsigned id = get_proc_id();

    if (id == 0) return 0;

    pages[page_num].id = id;

#ifdef DEBUG
    sprintf(message, "[Proc ID] %d", id);
    debug(INFO, message);
#endif

    while (build_func_stream());

    for (unsigned i = 0; i < pages[page_num].func_num; i++) {
        char *func_name = pages[page_num].func_streams[i].func_name;
        int result = search(pages[page_num].func_dict, func_name);
        if (result) {
            pages[page_num].invoked_func_num++;
            pages[page_num].func_streams[i].invoke_sort = result;
        } else {
            pages[page_num].func_streams[i].invoke_sort = 0x7fffffff;
        }
    }

    qsort(pages[page_num].sorted_funcs,
          pages[page_num].func_num,
          sizeof(FuncStreamPtr),
          func_cmp);

    char *merge = pages[page_num].proc_stream;
    for (int i = 0; pages[page_num].sorted_funcs[i] &&
                    pages[page_num].sorted_funcs[i]->invoke_sort <= pages[page_num].invoked_func_num; i++) {
        for (char *func = pages[page_num].sorted_funcs[i]->stream; *func; func++) {
            *merge++ = *func;
        }
    }

    pages[page_num].proc_stream_len = merge - pages[page_num].proc_stream;

#ifdef DEBUG
    sprintf(message, "[Func Num] %u\n", pages[page_num].invoked_func_num);
    debug(INFO, message);

    for (int i = 0; pages[page_num].sorted_funcs[i] &&
                    pages[page_num].sorted_funcs[i]->invoke_sort <= pages[page_num].invoked_func_num; i++) {
        sprintf(message, "- <%s> invoke sort: %u", pages[page_num].sorted_funcs[i]->func_name,
                pages[page_num].sorted_funcs[i]->invoke_sort);
        debug(INFO, message);
        sprintf(message, "  %s", pages[page_num].sorted_funcs[i]->stream);
        debug(INFO, message);
    }

    debug(INFO, "");
    debug(INFO, pages[page_num].proc_stream);
    debug(INFO, "\n");
#endif

    page_num++;
    return 1;
}

int build_func_stream() {
    char buf[IDENTIFIER_LEN] = "";
    unsigned stack_depth = 0;
    unsigned len;
    char current_iden[IDENTIFIER_LEN] = "";
    unsigned current_iden_len = 0;

    unsigned func_idx = pages[page_num].func_num;
    char *func_stream_str = pages[page_num].shared + pages[page_num].body_len + 1;
    FuncStreamPtr func_stream_ptr = pages[page_num].func_streams + func_idx;
    unsigned func_stream_len = 0;
    int invoke_id = 1;

    len = tokenize(buf);
    if (buf[0] == '\f') return 0;

    strcpy(current_iden, buf);
    current_iden_len = len;

    strcpy(func_stream_ptr->func_name, current_iden);
    int is_main = strcmp(current_iden, "main") == 0;
    if (is_main) pages[page_num].func_dict = insert(pages[page_num].func_dict, current_iden, invoke_id++);

    current_iden[0] = '\0';
    current_iden_len = 0;

    do {
        len = tokenize(buf);
        if (buf[0] == '(') stack_depth++;
        else if (buf[0] == ')') stack_depth--;
    } while (stack_depth != 0);

    do {
        len = tokenize(buf);

        if (is_iden_prefix(buf[0]) && !is_keepword(buf)) {
            strcpy(current_iden, buf);
            current_iden_len = len;
        } else {
            if (buf[0] == '(' && current_iden_len != 0) {
                strcpy(func_stream_str + func_stream_len, "FUNC");
                func_stream_len += 4;
                if (is_main && search(pages[page_num].func_dict, current_iden) == 0) {
                    pages[page_num].func_dict = insert(pages[page_num].func_dict, current_iden, invoke_id++);
                }
            } else if (buf[0] == '{') stack_depth++;
            else if (buf[0] == '}') stack_depth--;

            strcpy(func_stream_str + func_stream_len, buf);
            func_stream_len += len;

            current_iden[0] = '\0';
            current_iden_len = 0;
        }
    } while (stack_depth != 0);

    pages[page_num].body_len += func_stream_len + 1;
    pages[page_num].func_streams[func_idx].stream = func_stream_str;
    pages[page_num].sorted_funcs[func_idx] = func_stream_ptr;
    pages[page_num].func_num++;
    return 1;
}

inline int func_cmp(const void *p1, const void *p2) {
    return (int) ((*(FuncStreamPtr *) p1)->invoke_sort - (*(FuncStreamPtr *) p2)->invoke_sort);
}

inline unsigned min3(unsigned a, unsigned b, unsigned c) {
    unsigned min = a < b ? a : b;
    return min < c ? min : c;
}

unsigned edit_dis_dp(const char *str1, const char *str2, unsigned l1, unsigned l2) {
    unsigned i, j;
    unsigned len1, len2;

    len1 = l1 + 1;
    len2 = l2 + 1;
    unsigned max_len = max2(l1, l2);

    for (i = 0; i <= len1; i++) {
        for (j = 0; j <= len2; j++) {
            if (i == 0)
                dp[i][j] = j;
            else if (j == 0)
                dp[i][j] = i;
            else if (str1[i - 1] == str2[j - 1])
                dp[i][j] = dp[i - 1][j - 1];
            else
                dp[i][j] = 1 + min3(dp[i][j - 1], dp[i - 1][j], dp[i - 1][j - 1]);
        }
    }
    return dp[len1][len2];
}

void check_sim() {
    int flag;
    for (int i = 0; i < page_num - 1; i++) {
        if (page_set[i]) continue;
        flag = 0;
        for (int j = i + 1; j < page_num; j++) {
            if (is_sim(&pages[i], &pages[j])) {
                if (!flag) {
                    page_set[i] = 1;
                    printf("%d ", pages[i].id);
                    flag = 1;
                }
                page_set[j] = 1;
                printf("%d ", pages[j].id);
            }
        }
        if (flag) puts("");
    }
}

int is_sim(Page *p1, Page *p2) {
    unsigned max_len = max2(p1->proc_stream_len, p2->proc_stream_len);
    unsigned dis;
    double sim;

    dis = edit_dis_dp(p1->proc_stream, p2->proc_stream, p1->proc_stream_len, p2->proc_stream_len);
    sim = 1 - (double) dis / max_len;

#ifdef DEBUG
    sprintf(message, "%d <-> %d = %f", p1->id, p2->id, sim);
    debug(INFO, message);
#endif

    if (sim > THRESHOLD) return 1;
    return 0;
}

#ifdef DEBUG

void create_debug_file() {
    time_t t;
    struct tm *p;
    time(&t);
    p = localtime(&t);
    char debug_filename[LOG_FILENAME_LEN];

#ifdef _WIN32
    sprintf(debug_filename, "log\\debug-%d-%02d-%02d_%02d-%02d-%02d.log",
            1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
#else
    sprintf(debug_filename, "log/debug-%d-%02d-%02d_%02d-%02d-%02d.log",
            1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
#endif
    debug_file_handler = fopen(debug_filename, "w");

    printf("[INFO] Created log file: %s\n\n", debug_filename);
}

void close_debug_file() {
    fclose(debug_file_handler);
}

void debug(enum LEVEL level, char *msg) {
    switch (level) {
        case INFO:
            break;
        case ERROR:
            fputs("[ERROR] ", debug_file_handler);
            break;
    }
    fputs(msg, debug_file_handler);
    fputc('\n', debug_file_handler);
}

#endif