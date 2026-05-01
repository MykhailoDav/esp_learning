#include <Arduino.h>
#include <esp_timer.h>
#include <esp_heap_caps.h>
#include <string.h>

static const size_t SIZES[] = {50, 100, 500, 1000};
static const uint32_t FREQS[] = {240, 160, 80, 40};
static const int N_ITER = 5;
static const uint32_t SEED = 42;

template <typename T>
static inline void swap_v(T &a, T &b) { T t = a; a = b; b = t; }

template <typename T>
static int partition_v(T *arr, int low, int high) {
    T pivot = arr[high];
    int i = low - 1;
    for (int j = low; j < high; j++) {
        if (arr[j] <= pivot) {
            i++;
            swap_v(arr[i], arr[j]);
        }
    }
    swap_v(arr[i + 1], arr[high]);
    return i + 1;
}

template <typename T>
static void quicksort(T *arr, int low, int high) {
    if (low < high) {
        int p = partition_v(arr, low, high);
        quicksort(arr, low, p - 1);
        quicksort(arr, p + 1, high);
    }
}

template <typename T>
struct Node {
    T value;
    Node *left;
    Node *right;
};

template <typename T>
static Node<T> *bst_insert(Node<T> *root, T value) {
    if (!root) {
        Node<T> *n = (Node<T> *)malloc(sizeof(Node<T>));
        n->value = value;
        n->left = nullptr;
        n->right = nullptr;
        return n;
    }
    if (value < root->value)
        root->left = bst_insert(root->left, value);
    else
        root->right = bst_insert(root->right, value);
    return root;
}

template <typename T>
static void bst_free(Node<T> *root) {
    if (!root) return;
    bst_free(root->left);
    bst_free(root->right);
    free(root);
}

template <typename T>
static void gen_array(T *arr, size_t n);

template <>
void gen_array<int>(int *arr, size_t n) {
    randomSeed(SEED);
    for (size_t i = 0; i < n; i++) arr[i] = (int)random(-100000, 100000);
}

template <>
void gen_array<float>(float *arr, size_t n) {
    randomSeed(SEED);
    for (size_t i = 0; i < n; i++) arr[i] = (float)random(-100000, 100000) / 100.0f;
}

template <>
void gen_array<double>(double *arr, size_t n) {
    randomSeed(SEED);
    for (size_t i = 0; i < n; i++) arr[i] = (double)random(-100000, 100000) / 100.0;
}

template <>
void gen_array<char>(char *arr, size_t n) {
    randomSeed(SEED);
    for (size_t i = 0; i < n; i++) arr[i] = (char)random(-128, 128);
}

template <typename T>
static void test_combo(const char *type_name, size_t n) {
    T *src = (T *)malloc(sizeof(T) * n);
    T *work = (T *)malloc(sizeof(T) * n);
    if (!src || !work) {
        Serial.printf("# alloc failed for %s n=%u\n", type_name, (unsigned)n);
        if (src) free(src);
        if (work) free(work);
        return;
    }
    gen_array<T>(src, n);

    int64_t qs_total = 0;
    for (int it = 0; it < N_ITER; it++) {
        memcpy(work, src, sizeof(T) * n);
        int64_t t0 = esp_timer_get_time();
        quicksort<T>(work, 0, (int)n - 1);
        int64_t t1 = esp_timer_get_time();
        qs_total += (t1 - t0);
    }
    int64_t qs_avg = qs_total / N_ITER;
    size_t qs_mem = sizeof(T) * n;

    Serial.printf("%u,%u,%s,quicksort,%lld,%u\n",
                  (unsigned)getCpuFrequencyMhz(),
                  (unsigned)n, type_name,
                  (long long)qs_avg, (unsigned)qs_mem);

    int64_t bt_total = 0;
    long bt_mem_total = 0;
    for (int it = 0; it < N_ITER; it++) {
        size_t free_before = ESP.getFreeHeap();
        int64_t t0 = esp_timer_get_time();
        Node<T> *root = nullptr;
        for (size_t i = 0; i < n; i++) root = bst_insert<T>(root, src[i]);
        int64_t t1 = esp_timer_get_time();
        size_t free_after = ESP.getFreeHeap();
        bt_total += (t1 - t0);
        bt_mem_total += (long)(free_before - free_after);
        bst_free<T>(root);
    }
    int64_t bt_avg = bt_total / N_ITER;
    long bt_mem = bt_mem_total / N_ITER;

    Serial.printf("%u,%u,%s,bintree,%lld,%ld\n",
                  (unsigned)getCpuFrequencyMhz(),
                  (unsigned)n, type_name,
                  (long long)bt_avg, bt_mem);

    free(src);
    free(work);
}

static void run_all() {
    for (size_t i = 0; i < sizeof(SIZES) / sizeof(SIZES[0]); i++) {
        size_t n = SIZES[i];
        test_combo<int>("int", n);
        test_combo<float>("float", n);
        test_combo<double>("double", n);
        test_combo<char>("char", n);
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("===BEGIN===");
    Serial.println("freq_mhz,size,type,algorithm,time_us,memory_bytes");

    for (size_t i = 0; i < sizeof(FREQS) / sizeof(FREQS[0]); i++) {
        uint32_t f = FREQS[i];
        setCpuFrequencyMhz(f);
        Serial.updateBaudRate(115200);
        delay(200);
        run_all();
    }

    Serial.println("===END===");
}

void loop() {
    delay(1000);
}
