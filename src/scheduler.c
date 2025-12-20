#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scheduler.h"

#define MAX_TASKS 100
#define TIMEOUT_WINDOW 20  // Bir görevin (task) çalışır durumdayken en fazla bekleyebileceği süre penceresi (sn)

SimulationTask taskList[MAX_TASKS];   // Simülasyondaki tüm görevlerin tutulduğu dizi
int taskCount = 0;                   // Yüklenen toplam görev sayısı
int globalTimer = 0;                 // Simülasyonun global zamanı (sn)

static int lastRRIndex = -1;         // Priority=3 Round-Robin seçiminde en son seçilen index

// === İstatistik sayaçları (özet rapor için) ===
static int statDroppedTasks = 0;      // Zaman aşımı nedeniyle düşürülen görev sayısı
static int statCompletedTasks = 0;    // Başarıyla tamamlanan görev sayısı
static long statTotalTurnaround = 0;  // Toplam turnaround (tamamlanma) süresi birikimi
static long statTotalWaiting = 0;     // Toplam bekleme süresi birikimi

/**
 * @brief Görevleri dosyadan okuyup taskList dizisine yükler.
 *
 * Dosya formatı: arrivalTime, priority, burstTime
 * - arrivalTime : Görevin sisteme giriş zamanı (sn)
 * - priority    : Öncelik seviyesi (0 en yüksek, 5 en düşük)
 * - burstTime   : Görevin toplam CPU ihtiyacı (sn)
 *
 * Okunan her görev için:
 * - id atanır
 * - remainingTime burstTime olarak başlatılır
 * - handle NULL yapılır (henüz FreeRTOS görevi yaratılmadı)
 * - deadline = arrivalTime + TIMEOUT_WINDOW olarak atanır (ilk zaman aşımı hedefi)
 * - name alanı "proses" yapılır
 */
void loadTasks(const char* filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Hata: %s dosyasi acilamadi.\n", filename);
        exit(1);
    }

    taskCount = 0;

    // Dosyadan satır satır görevleri oku (MAX_TASKS sınırı var)
    while (taskCount < MAX_TASKS &&
        fscanf(file, "%d, %d, %d",
            &taskList[taskCount].arrivalTime,
            &taskList[taskCount].priority,
            &taskList[taskCount].burstTime) == 3) {

        // Temel alanları kur
        taskList[taskCount].id = taskCount;
        taskList[taskCount].remainingTime = taskList[taskCount].burstTime;

        // FreeRTOS görevi henüz oluşturulmadığı için handle NULL
        taskList[taskCount].handle = NULL;

        // arrivalTimestamp: görevin gerçekten sisteme alındığı zaman (oluşturulduğu an)
        taskList[taskCount].arrivalTimestamp = -1;

        // hasStarted: ilk kez CPU görüp görmediğini loglamak için bayrak
        taskList[taskCount].hasStarted = 0;

        // Zaman aşımı hedefi: (ilk kurulumda) arrivalTime + TIMEOUT_WINDOW
        taskList[taskCount].deadline = taskList[taskCount].arrivalTime + TIMEOUT_WINDOW;

        // Görev ismi (FreeRTOS tarafında kullanılacak)
        strcpy(taskList[taskCount].name, "proses");

        taskCount++;
    }

    fclose(file);
}

/**
 * @brief Tüm aktif görevleri dolaşır ve zaman aşımına uğrayanları düşürür.
 *
 * Kural:
 * - Görev "aktif" sayılabilmesi için handle != NULL olmalı
 * - remainingTime > 0 ise tamamlanmamış demektir
 * - globalTimer >= deadline ise zaman aşımı kabul edilir
 *
 * Zaman aşımında:
 * - log basılır ("zamanaşımı")
 * - dropped istatistiği artırılır
 * - FreeRTOS görevi silinir, handle NULL yapılır
 */
static void checkGlobalTimeouts(void) {
    for (int i = 0; i < taskCount; i++) {
        if (taskList[i].handle != NULL && taskList[i].remainingTime > 0) {
            if (globalTimer >= taskList[i].deadline) {
                printTaskLog(&taskList[i], "zamanaşımı");

                statDroppedTasks++;

                vTaskDelete(taskList[i].handle);
                taskList[i].handle = NULL;
            }
        }
    }
}

/**
 * @brief Sıradaki çalıştırılacak görevi seçer.
 *
 * Seçim politikası (koddaki sıraya göre):
 * 1) Priority 0: Bulur bulmaz döner (en yüksek öncelik)
 * 2) Priority 1-2: Önce 1 sonra 2 taranır, ilk uygun görev seçilir
 * 3) Priority 3: Round-Robin mantığıyla (lastRRIndex üzerinden) döngüsel seçim
 * 4) Priority 4-5: Önce 4 sonra 5 taranır
 *
 * Uygunluk koşulları:
 * - handle != NULL (görev oluşturulmuş ve silinmemiş)
 * - remainingTime > 0 (bitmemiş)
 * - priority eşleşiyor
 *
 * @return Seçilen görevin adresi, yoksa NULL
 */
static SimulationTask* selectNextTask(void) {
    // 1) Priority 0 (en kritik)
    for (int i = 0; i < taskCount; i++) {
        if (taskList[i].handle != NULL &&
            taskList[i].remainingTime > 0 &&
            taskList[i].priority == 0) {
            return &taskList[i];
        }
    }

    // 2) Priority 1-2 (sıralı tarama)
    for (int p = 1; p <= 2; p++) {
        for (int i = 0; i < taskCount; i++) {
            if (taskList[i].handle != NULL &&
                taskList[i].remainingTime > 0 &&
                taskList[i].priority == p) {
                return &taskList[i];
            }
        }
    }

    // 3) Priority 3 (Round-Robin)
    // lastRRIndex'ten sonraki elemandan başlayarak döngüsel arar
    for (int k = 0; k < taskCount; k++) {
        int idx = (lastRRIndex + 1 + k) % taskCount;
        if (taskList[idx].handle != NULL &&
            taskList[idx].remainingTime > 0 &&
            taskList[idx].priority == 3) {
            lastRRIndex = idx;   // RR için "en son seçilen" index güncellenir
            return &taskList[idx];
        }
    }

    // 4) Priority 4-5 (en düşük öncelikler)
    for (int p = 4; p <= 5; p++) {
        for (int i = 0; i < taskCount; i++) {
            if (taskList[i].handle != NULL &&
                taskList[i].remainingTime > 0 &&
                taskList[i].priority == p) {
                return &taskList[i];
            }
        }
    }

    // Çalıştırılacak görev yok
    return NULL;
}

/**
 * @brief Ana zamanlayıcı (scheduler/controller) görevi.
 *
 * Bu görev:
 * 1) Her saniye (globalTimer adımında) arrivalTime == globalTimer olan görevleri oluşturur.
 *    - xTaskCreate ile FreeRTOS görevi yaratılır
 *    - hemen suspend edilir (kontrol scheduler’da olsun diye)
 * 2) selectNextTask() ile çalıştırılacak görevi seçer.
 * 3) Seçilen görevi 1 saniye çalıştırır:
 *    - resume -> 1 saniye delay -> suspend
 *    - globalTimer++ ve remainingTime--
 * 4) Görev bittiğinde istatistikleri hesaplar ve görevi siler.
 * 5) Bitmediyse deadline’ı günceller ve (kurala göre) bazı görevlerde priority artırır.
 * 6) Görev yoksa 1 saniye bekleyip globalTimer artırır ve timeout kontrolü yapar.
 * 7) Tüm görevler bittiğinde özet rapor basar ve programı sonlandırır.
 */
void vSchedulerTask(void* pvParameters) {
    (void)pvParameters;

    for (;;) {

        // 1) Yeni gelen görevleri (arrivalTime == globalTimer) oluştur
        for (int i = 0; i < taskCount; i++) {
            if (taskList[i].arrivalTime == globalTimer) {

                // Görevi oluştur (vTaskGenericFunction görevi çalıştıracak genel fonksiyon)
                xTaskCreate(vTaskGenericFunction,
                    taskList[i].name,
                    128,
                    &taskList[i],
                    1,
                    &taskList[i].handle);

                // Scheduler kontrolü ele almak için hemen durdur
                vTaskSuspend(taskList[i].handle);

                // Görevin sisteme alındığı gerçek zaman damgası
                taskList[i].arrivalTimestamp = globalTimer;

                // İlk kez CPU görecek (log için)
                taskList[i].hasStarted = 0;
            }
        }

        // 2) Sıradaki görevi seç
        SimulationTask* current = selectNextTask();

        if (current != NULL) {

            // İlk kez çalışıyorsa "başladı", değilse "yürütülüyor"
            if (current->hasStarted == 0) {
                printTaskLog(current, "başladı");
                current->hasStarted = 1;
            }
            else {
                printTaskLog(current, "yürütülüyor");
            }

            // 3) Çalıştırmadan önce tüm görevlerde timeout kontrolü
            checkGlobalTimeouts();

            // 4) Seçilen görevi 1 saniye çalıştır (quantum = 1 sn)
            vTaskResume(current->handle);
            vTaskDelay(pdMS_TO_TICKS(1000));
            vTaskSuspend(current->handle);

            // Zaman ilerlet ve görev süresini düş
            globalTimer++;
            current->remainingTime--;

            // 5) Görev tamamlandı mı?
            if (current->remainingTime <= 0) {
                printTaskLog(current, "sonlandı");

                // === İstatistikler ===
                statCompletedTasks++;
                int turnaround = globalTimer - current->arrivalTime; // tamamlanma - geliş
                int waiting = turnaround - current->burstTime;       // turnaround - CPU ihtiyacı

                statTotalTurnaround += turnaround;
                statTotalWaiting += waiting;

                // Görevi sistemden kaldır
                vTaskDelete(current->handle);
                current->handle = NULL;
            }
            else {
                // 6) Bitmediyse deadline’ı “şu andan itibaren” tekrar ayarla
                current->deadline = globalTimer + TIMEOUT_WINDOW;

                // 7) Priority artırma kuralı:
                // 0 < priority < 5 ise bir kademe düşür (sayısal olarak artırılıyor)
                if (current->priority > 0 && current->priority < 5) {
                    current->priority++;
                    printTaskLog(current, "askıda"); // preempt edildi / beklemeye alındı
                }
            }

        }
        else {
            // Çalışacak görev yoksa:
            // - timeout kontrolü yap
            // - 1 saniye bekle ve zamanı ilerlet
            checkGlobalTimeouts();
            vTaskDelay(pdMS_TO_TICKS(1000));
            globalTimer++;
        }

        // 8) Simülasyonun bitiş koşulu:
        // - Henüz gelmemiş görev varsa bitmez
        // - Aktif ve remainingTime>0 görev varsa bitmez
        int allDone = 1;
        for (int i = 0; i < taskCount; i++) {
            if (taskList[i].arrivalTime > globalTimer) { allDone = 0; break; }
            if (taskList[i].handle != NULL && taskList[i].remainingTime > 0) { allDone = 0; break; }
        }

        // 9) Bitmişse özet rapor bas ve çık
        if (allDone) {
            printf("\nSimulasyon Tamamlandi.\n");
            printf("--------------------------------------------------\n");
            printf("             SIMULATION SUMMARY                   \n");
            printf("--------------------------------------------------\n");
            printf("Total Simulation Time  : %d seconds\n", globalTimer);
            printf("Total Tasks Processed  : %d\n", taskCount);
            printf("Tasks Completed        : %d\n", statCompletedTasks);
            printf("Tasks Dropped (Timeout): %d\n", statDroppedTasks);

            if (statCompletedTasks > 0) {
                double avgTurnaround = (double)statTotalTurnaround / statCompletedTasks;
                double avgWaiting = (double)statTotalWaiting / statCompletedTasks;

                printf("Avg Turnaround Time    : %.2f sec\n", avgTurnaround);
                printf("Avg Waiting Time       : %.2f sec\n", avgWaiting);
            }
            else {
                printf("Avg Turnaround Time    : N/A\n");
                printf("Avg Waiting Time       : N/A\n");
            }
            printf("--------------------------------------------------\n");

            exit(0);
        }
    }
}

/**
 * @brief Scheduler başlatılmadan önce görev listesini dosyadan yükler.
 *
 * Burada giriş dosyası sabit olarak "giris.txt" seçilmiş.
 */
void vInitScheduler(void) {
    loadTasks("giris.txt");
}

/**
 * @brief FreeRTOS zamanlayıcısını başlatır.
 *
 * - Controller/Scheduler görevi en yüksek önceliklerden biriyle oluşturulur.
 * - Ardından vTaskStartScheduler() çağrısı ile FreeRTOS çalışmaya başlar.
 */
void vSchedulerStart(void) {
    xTaskCreate(vSchedulerTask, "Controller", 1000, NULL, configMAX_PRIORITIES - 1, NULL);
    vTaskStartScheduler();
}