1. Keputusan awal membangun OTA monolitik belum sepenuhnya final
2. Karena OTA monolitik dinilai jadi biang kerok sebua issue
- Isu 1 TWDT reset di tengah proses: `esp_https_ota()` sejauh ini menggunakan monolithic blocking. Masalah utamanya adalah proses ini terlalu lama (30000ms). Selama itu, program eksekusi RTOS akan stuck di fungsi ini. membuat task starvation untuk task lain.
- Karena lama, supervisor yang set waktu 10 detik ketika tidak di feed oleh watchdog, menganggap bahwa device dalam kondisi stuck, jadi supervisor lansung cut down device.
- `esp_https_ota()` mengunci BUS I2C saat menulis data biner baru. selama cache mati, semua fungsi task bakal freeze paksa
