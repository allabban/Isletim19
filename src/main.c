#include <stdio.h>
#include <stdlib.h>
#include "scheduler.h"

/**
 * @brief Programın giriş noktası.
 *
 * - stdout buffering kapatılır: Log/printf çıktıları anlık (gecikmesiz) görünür.
 * - Scheduler için gerekli başlangıç işlemleri yapılır (görevlerin yüklenmesi vb.).
 * - FreeRTOS scheduler başlatılır; bu noktadan sonra kontrol RTOS'a geçer.
 *
 * @return Normal şartlarda scheduler başladığı için buraya dönmez.
 */
int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0); // stdout tamponlamasını kapat (printf çıktıları anında gelsin)

    vInitScheduler();   // Görevleri/parametreleri hazırla (ör. giris.txt yükleme)
    vSchedulerStart();  // Scheduler görevini oluştur ve FreeRTOS'u başlat

    return 0; // Teoride RTOS çalışırken buraya dönülmez; güvenlik için mevcut.
}
