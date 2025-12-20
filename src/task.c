#include "scheduler.h"
#include <stdio.h>
#include <string.h>

// ANSI renk kodları (terminal çıktısını renklendirmek için)
#define COLOR_RESET   "\033[0m"
#define COLOR_BLUE    "\033[1;34m"
#define COLOR_RED     "\033[1;31m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_YELLOW  "\033[1;33m"
#define COLOR_MAGENTA "\033[1;35m"
#define COLOR_CYAN    "\033[1;36m"
#define COLOR_BEIGE   "\033[38;5;230m"

// Görev ID'sine göre kullanılacak renk dizisi (id % 6 ile seçiliyor)
const char* TASK_COLORS[] = {
    COLOR_BLUE, COLOR_RED, COLOR_GREEN,
    COLOR_YELLOW, COLOR_MAGENTA, COLOR_CYAN
};

// Scheduler tarafında tutulan global simülasyon zamanını burada kullanmak için extern
extern int globalTimer;

/**
 * @brief Bir görevin (task) durumunu, simülasyon zamanıyla birlikte renklendirerek konsola yazdırır.
 *
 * Amaç:
 * - Her görevi farklı renkte göstererek logları okunabilir hale getirmek.
 * - Zaman, isim, durum, id, öncelik ve kalan süre bilgilerini tek satırda raporlamak.
 *
 * Renk seçimi:
 * - colorIndex = task->id % 6 ile TASK_COLORS dizisinden seçilir.
 *
 * Çıktı formatı:
 * - "<zaman>.0000 sn <isim> <durum> (id:xxxx öncelik:x kalan süre:x sn)"
 *
 * Not:
 * - ffush(stdout) ile çıktının anında görünmesi sağlanır (buffer gecikmesini engeller).
 *
 * @param task   Log basılacak görevin adresi
 * @param status Görevin durumu (örn: "başladı", "yürütülüyor", "askıda", "sonlandı", "zamanaşımı")
 */
void printTaskLog(SimulationTask* task, const char* status) {
    int colorIndex = task->id % 6;
    const char* color = TASK_COLORS[colorIndex];

    // \t kaldırıldı (bazı terminallerde satır kayması/kesilme sorununu azaltmak için)
    // Hizalama: %-6s (isim) ve %-13s (durum) alanları sabit genişlikte yazılır
    printf("%s%d.0000 sn %-6s %-13s (id:%04d  öncelik:%d  kalan süre:%d sn)%s\n",
        color,
        globalTimer,
        task->name,
        status,
        task->id,
        task->priority,
        task->remainingTime,
        COLOR_RESET);
    fflush(stdout);
}

/**
 * @brief Simülasyonda oluşturulan FreeRTOS görevleri için "placeholder" görev fonksiyonu.
 *
 * Bu fonksiyon:
 * - Gerçek iş yapmaz.
 * - Sonsuz döngüde kısa aralıklarla (100 ms) delay atarak görev "yaşıyormuş" gibi davranır.
 *
 * Not:
 * - Asıl çalışma/CPU tüketimi, scheduler'ın resume/suspend ve remainingTime mantığıyla simüle edilir.
 *
 * @param pvParameters Göreve aktarılabilecek parametre (burada kullanılmıyor)
 */
void vTaskGenericFunction(void* pvParameters) {
    (void)pvParameters;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}