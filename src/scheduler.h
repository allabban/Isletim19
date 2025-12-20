#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "FreeRTOS.h"
#include "task.h"

/**
 * @brief Simülasyondaki bir işlemi (task) temsil eden veri yapısı.
 *
 * Bu struct hem "giriş dosyasından okunan iş özelliklerini"
 * hem de FreeRTOS tarafındaki çalışma bilgilerini tutar.
 */
typedef struct {
    int id;               // Görev kimliği (genelde yüklenme sırasına göre atanır)
    int arrivalTime;      // Görevin sisteme gelme zamanı (sn)
    int priority;         // Görevin anlık öncelik seviyesi (0 en yüksek, 5 en düşük)
    int burstTime;        // Görevin toplam CPU ihtiyacı (sn)
    int remainingTime;    // Kalan çalışma süresi (sn)

    int arrivalTimestamp; // Görevin FreeRTOS'ta gerçekten oluşturulduğu anın zamanı (sn)
    int hasStarted;       // Görev ilk defa CPU gördü mü? (log "başladı" için bayrak)

    int deadline;         // Zaman aşımı eşiği: globalTimer bu değere ulaşırsa görev düşer

    char name[16];        // Görev adı (log ve xTaskCreate için)
    TaskHandle_t handle;  // FreeRTOS görev handle'ı (NULL ise oluşturulmamış/silinmiş)
} SimulationTask;

// === Dışarıdan çağrılan fonksiyon prototipleri ===

/**
 * @brief Scheduler başlatılmadan önce gerekli hazırlıkları yapar (örn. görevleri dosyadan yükler).
 */
void vInitScheduler(void);

/**
 * @brief Scheduler/controller görevini oluşturur ve FreeRTOS scheduler'ı başlatır.
 */
void vSchedulerStart(void);

/**
 * @brief Simülasyonda oluşturulan görevlerin kullandığı genel (boş) görev fonksiyonu.
 * Scheduler resume/suspend ile yürütmeyi simüle eder.
 */
void vTaskGenericFunction(void* pvParameters);

/**
 * @brief Bir görevin durumunu konsola (renkli/formatlı) loglar.
 *
 * @param task   Loglanacak görevin adresi
 * @param status Durum metni (örn: "başladı", "yürütülüyor", "askıda", "sonlandı", "zamanaşımı")
 */
void printTaskLog(SimulationTask* task, const char* status);

#endif