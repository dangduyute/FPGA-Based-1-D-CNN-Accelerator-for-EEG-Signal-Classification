// fpga_eeg_websocket.c
// Build: gcc fpga_eeg_websocket.c -o fpga_eeg_websocket -O2 $(pkg-config --cflags --libs libwebsockets) -lpthread -lm
// Usage: ./fpga_eeg_websocket [X.txt] [y.txt]
//
// WS protocol:
//   beat_start : {beat_id, beat_start, gt, gt_label, pred:-1, pred_label}
//   sample     : {beat_id, index, value}
//   beat_pred  : {beat_id, pred, pred_label, accuracy, total, correct}
//   result     : {total, correct, accuracy, time}
//   status     : {message}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <time.h>
#include <stdint.h>
#include <libwebsockets.h>

#define BILLION             1000000000ULL
#define START_BASE          (0x00000)
#define LDM_INPUT_BASE_PHYS (0x10000>>2)
#define CRAM_INPUT_BASE_PHYS (0x20000>>2)
#define WRAM_INPUT_BASE_PHYS (0x30000>>2)
#define BRAM_INPUT_BASE_PHYS (0x40000>>2)
#define DONE_BASE_PHYS      (0x00000)
#define LDM_OUTPUT_BASE_PHYS (0x10000>>2)

#define SCALE_FACTOR        (1 << 6)
#define N_BEATS             100
#define SEG_LEN             320
#define GAP_CH              16
#define GAP_LEN             80
#define N_CLASS             3
#define SAMPLE_DELAY_US     5000

#include "CGRA.h"
#include "FPGA_Driver.c"

typedef uint32_t U32;

#define LOG(fmt, ...) do { printf(fmt "\n", ##__VA_ARGS__); fflush(stdout); } while(0)

static const char *LABELS[N_CLASS] = {
    "Bình thường", "Giai đoạn giữa cơn", "Cơn động kinh"
};

static const char *safe_label(int i) {
    return (i >= 0 && i < N_CLASS) ? LABELS[i] : "Unknown";
}

static const char *g_signals_path = "X.txt";
static const char *g_labels_path  = "y.txt";

static volatile int force_exit = 0;
static struct lws_context *context = NULL;

/* ---- WS queue ---- */
struct ws_msg { char *payload; size_t len; struct ws_msg *next; };
static pthread_mutex_t q_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct ws_msg *q_head = NULL, *q_tail = NULL;
static struct lws *g_wsi = NULL;

static void ws_enqueue(char *payload, size_t len) {
    struct ws_msg *m = malloc(sizeof *m);
    if (!m) { free(payload); return; }
    m->payload = payload; m->len = len; m->next = NULL;
    pthread_mutex_lock(&q_mutex);
    if (q_tail) q_tail->next = m; else q_head = m;
    q_tail = m;
    pthread_mutex_unlock(&q_mutex);

    if (g_wsi) {
        lws_callback_on_writable(g_wsi);
        if (context) lws_cancel_service(context);
    }
}

static void send_data(const char *type, const char *data) {
    int n = snprintf(NULL, 0, "{\"type\":\"%s\",\"data\":%s}", type, data);
    if (n <= 0) return;
    char *s = malloc((size_t)n + 1);
    if (!s) return;
    snprintf(s, (size_t)n + 1, "{\"type\":\"%s\",\"data\":%s}", type, data);
    ws_enqueue(s, (size_t)n);
}

static void send_status(const char *msg) {
    char buf[512];
    snprintf(buf, sizeof buf, "{\"message\":\"%s\"}", msg);
    send_data("status", buf);
}

static void send_sample(int id, int idx, float v) {
    char buf[256];
    snprintf(buf, sizeof buf, "{\"beat_id\":%d,\"index\":%d,\"value\":%.6f}", id, idx, v);
    send_data("sample", buf);
}

static void send_beat_start(int id, int start, int gt) {
    char buf[512];
    snprintf(buf, sizeof buf,
        "{\"beat_id\":%d,\"beat_start\":%d,\"gt\":%d,\"gt_label\":\"%s\","
        "\"pred\":-1,\"pred_label\":\"Dang du doan...\"}",
        id, start, gt, safe_label(gt));
    send_data("beat_start", buf);
}

static void send_beat_pred(int id, int pred, float acc, int total, int correct) {
    char buf[512];
    snprintf(buf, sizeof buf,
        "{\"beat_id\":%d,\"pred\":%d,\"pred_label\":\"%s\","
        "\"accuracy\":%.2f,\"total\":%d,\"correct\":%d}",
        id, pred, safe_label(pred), acc, total, correct);
    send_data("beat_pred", buf);
}

static void send_result(int total, int correct, float acc, float t) {
    char buf[256];
    snprintf(buf, sizeof buf,
        "{\"total\":%d,\"correct\":%d,\"accuracy\":%.2f,\"time\":%.3f}",
        total, correct, acc, t);
    send_data("result", buf);
}

/* ---- WS callback ---- */
static int callback_eeg(struct lws *wsi, enum lws_callback_reasons reason,
                        void *user, void *in, size_t len) {
    (void)user; (void)in; (void)len;
    switch (reason) {
    case LWS_CALLBACK_ESTABLISHED:
        LOG("Client connected");
        g_wsi = wsi;
        lws_callback_on_writable(wsi);
        break;
    case LWS_CALLBACK_SERVER_WRITEABLE: {
        struct ws_msg *m = NULL;
        pthread_mutex_lock(&q_mutex);
        if (q_head) { m = q_head; q_head = q_head->next; if (!q_head) q_tail = NULL; }
        pthread_mutex_unlock(&q_mutex);
        if (m) {
            unsigned char *buf = malloc(LWS_PRE + m->len);
            if (buf) {
                memcpy(buf + LWS_PRE, m->payload, m->len);
                lws_write(wsi, buf + LWS_PRE, m->len, LWS_WRITE_TEXT);
                free(buf);
            }
            free(m->payload); free(m);
            pthread_mutex_lock(&q_mutex);
            int more = (q_head != NULL);
            pthread_mutex_unlock(&q_mutex);
            if (more) lws_callback_on_writable(wsi);
        }
        break;
    }
    case LWS_CALLBACK_CLOSED:
        LOG("Client disconnected");
        if (g_wsi == wsi) g_wsi = NULL;
        break;
    default: break;
    }
    return 0;
}

static struct lws_protocols protocols[] = {
    { "eeg-protocol", callback_eeg, 0, 8192 },
    { NULL, NULL, 0, 0 }
};

/* ---- Fixed-point helpers ---- */
static float fx_preprocess(float v) {
    if (v >=  512.0f) v -= 512.0f;
    if (v < -512.0f)  v += 512.0f;
    return roundf(v * 64.0f) / 64.0f;
}

static float fx_to_float(U32 fx) {
    int sign = (fx & 0x8000) ? -1 : 1;
    return sign * (float)(fx & 0x7FFF) / (float)SCALE_FACTOR;
}

static U32 fx_from_float(float x) {
    float s = x * (float)SCALE_FACTOR;
    int16_t f = (int16_t)(s >= 0 ? s + 0.5f : s - 0.5f);
    if (f >  32767) f =  32767;
    if (f < -32768) f = -32768;
    return (U32)(f & 0xFFFF);
}

/* ---- FPGA thread ---- */
static void *fpga_thread(void *arg) {
    (void)arg;

    if (!fpga_open()) {
        LOG("ERROR: Cannot open FPGA device");
        send_status("ERROR: Cannot open FPGA device");
        return NULL;
    }
    LOG("FPGA opened"); send_status("FPGA initialized");

    FILE *CRAM_f   = fopen("CRAM_File.txt",   "r");
    FILE *WRAM_f   = fopen("WRAM_File.txt",   "r");
    FILE *BRAM_f   = fopen("BRAM_File.txt",   "r");
    FILE *WRAM2_f  = fopen("WRAM_2_File.txt", "r");
    FILE *BRAM2_f  = fopen("BRAM_2_File.txt", "r");
    if (!CRAM_f || !WRAM_f || !BRAM_f || !WRAM2_f || !BRAM2_f) {
        LOG("ERROR: Cannot open config files");
        send_status("ERROR: Cannot open config files");
        return NULL;
    }

    U32   CRAM[15], WRAM[7068], BRAM[124];
    float weight[48], bias[3];
    U32 v; float vf; int i;

    for (i = 0; fscanf(CRAM_f,  "%8x", &v) == 1 && i < 15;   i++) CRAM[i]   = v;
    fclose(CRAM_f);  LOG("CRAM loaded: %d", i);

    for (i = 0; fscanf(WRAM_f,  "%4x", &v) == 1 && i < 7068; i++) WRAM[i]   = v;
    fclose(WRAM_f);  LOG("WRAM loaded: %d", i);

    for (i = 0; fscanf(BRAM_f,  "%4x", &v) == 1 && i < 124;  i++) BRAM[i]   = v;
    fclose(BRAM_f);  LOG("BRAM loaded: %d", i);

    for (i = 0; fscanf(WRAM2_f, "%f",  &vf)== 1 && i < 48;   i++) weight[i] = vf;
    fclose(WRAM2_f); LOG("Weights loaded: %d", i);

    for (i = 0; fscanf(BRAM2_f, "%f",  &vf)== 1 && i < 3;    i++) bias[i]   = vf;
    fclose(BRAM2_f); LOG("Biases loaded: %d", i);

    for (int j = 0; j < 15;   j++) *(MY_IP_info.reg_mmap + CRAM_INPUT_BASE_PHYS + j) = CRAM[j];
    for (int j = 0; j < 7068; j++) *(MY_IP_info.reg_mmap + WRAM_INPUT_BASE_PHYS + j) = WRAM[j];
    for (int j = 0; j < 124;  j++) *(MY_IP_info.reg_mmap + BRAM_INPUT_BASE_PHYS + j) = BRAM[j];
    send_status("Configuration loaded");

    float *InModel = malloc(N_BEATS * SEG_LEN * sizeof(float));
    float *Label   = malloc(N_BEATS * sizeof(float));
    if (!InModel || !Label) { send_status("ERROR: malloc failed"); return NULL; }

    FILE *fsig = fopen(g_signals_path, "r");
    if (!fsig) { LOG("ERROR: Cannot open %s", g_signals_path); send_status("ERROR: signals"); return NULL; }
    float tmp;
    for (int k = 0; k < N_BEATS * SEG_LEN; k++) { if (fscanf(fsig, "%f", &tmp) != 1) break; InModel[k] = tmp; }
    fclose(fsig); LOG("Signals loaded: %s", g_signals_path);

    FILE *flbl = fopen(g_labels_path, "r");
    if (!flbl) { LOG("ERROR: Cannot open %s", g_labels_path); send_status("ERROR: labels"); return NULL; }
    for (int k = 0; k < N_BEATS; k++) { if (fscanf(flbl, "%f", &tmp) != 1) break; Label[k] = tmp; }
    fclose(flbl); LOG("Labels loaded: %s", g_labels_path);
    send_status("Dataset loaded");

    float    cnn_out[GAP_CH * GAP_LEN], gap[GAP_CH], logits[N_CLASS], img[SEG_LEN];
    U32      pixel[340];
    uint16_t addr[GAP_CH * GAP_LEN];
    for (int j = 0; j < GAP_CH * GAP_LEN; j++) addr[j] = (uint16_t)((j + (j/20)*12) & 0xFFFF);

    int correct = 0;
    struct timespec t0, t1;
    clock_gettime(CLOCK_REALTIME, &t0);
    LOG("Inference start: %d beats", N_BEATS);

    for (int ii = 0; ii < N_BEATS && !force_exit; ii++) {
        for (int k = 0; k < SEG_LEN; k++) img[k] = InModel[ii * SEG_LEN + k];
        int gt = (int)Label[ii];

        for (int k = 0; k < 340; k++)
            pixel[k] = fx_from_float(k < SEG_LEN ? fx_preprocess(img[k]) : 0.0f);
        for (int k = 0; k < 340; k++)
            *(MY_IP_info.reg_mmap + LDM_INPUT_BASE_PHYS + addr[k]) = pixel[k];

        *(MY_IP_info.reg_mmap + START_BASE) = 1;
        while (*(MY_IP_info.reg_mmap + DONE_BASE_PHYS) != 1) usleep(50);

        for (int j = 0; j < GAP_CH * GAP_LEN; j++)
            cnn_out[j] = fx_to_float(*(MY_IP_info.reg_mmap + LDM_OUTPUT_BASE_PHYS + addr[j]));

        for (int j = 0; j < GAP_CH; j++) {
            float s = 0.0f;
            for (int k = 0; k < GAP_LEN; k++) s += cnn_out[GAP_LEN*j + k];
            gap[j] = s / GAP_LEN;
        }
        for (int j = 0; j < N_CLASS; j++) {
            float s = 0.0f;
            for (int k = 0; k < GAP_CH; k++) s += gap[k] * weight[k*N_CLASS + j];
            logits[j] = s + bias[j];
        }

        int pred = 0;
        for (int j = 1; j < N_CLASS; j++) if (logits[j] > logits[pred]) pred = j;

        if (gt == pred) correct++;
        float acc = 100.0f * correct / (ii + 1);
        LOG("Beat %d/%d GT:%d Pred:%d Acc:%.2f%%", ii+1, N_BEATS, gt, pred, acc);

        send_beat_start(ii, ii * SEG_LEN, gt);

        int pred_at = (SEG_LEN * 3) / 4, sent = 0;
        for (int s = 0; s < SEG_LEN && !force_exit; s++) {
            send_sample(ii, ii*SEG_LEN + s, img[s]);
            if (!sent && s == pred_at) { send_beat_pred(ii, pred, acc, ii+1, correct); sent = 1; }
            usleep(SAMPLE_DELAY_US);
        }
        if (!sent) send_beat_pred(ii, pred, acc, ii+1, correct);
    }

    clock_gettime(CLOCK_REALTIME, &t1);
    unsigned long long ns = BILLION*(t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec);
    send_result(N_BEATS, correct, 100.0f*correct/N_BEATS, (float)ns/BILLION);
    free(InModel); free(Label);
    send_status("Processing complete");
    LOG("Done");
    return NULL;
}

/* ---- Signal handler ---- */
static void sighandler(int sig) {
    (void)sig;
    force_exit = 1;
    if (context) lws_cancel_service(context);
}

/* ---- Main ---- */
int main(void) {
    signal(SIGINT, sighandler);

    struct lws_context_creation_info info = {0};
    info.port = 8080; info.protocols = protocols; info.gid = -1; info.uid = -1;

    context = lws_create_context(&info);
    if (!context) { fprintf(stderr, "Failed to create WS context\n"); return 1; }

    LOG("WS server on :8080 | signals=%s labels=%s", g_signals_path, g_labels_path);

    pthread_t tid;
    pthread_create(&tid, NULL, fpga_thread, NULL);
    while (!force_exit) lws_service(context, 1);
    pthread_join(tid, NULL);
    lws_context_destroy(context);

    pthread_mutex_lock(&q_mutex);
    for (struct ws_msg *m = q_head; m; ) {
        struct ws_msg *n = m->next; free(m->payload); free(m); m = n;
    }
    q_head = q_tail = NULL;
    pthread_mutex_unlock(&q_mutex);

    LOG("Server stopped");
    return 0;
}
