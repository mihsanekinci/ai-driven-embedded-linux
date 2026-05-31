# Dönem Projesi Durum Dosyası
**Ders:** Marmara Üniversitesi Gömülü Sistemler  
**Proje:** AI Driven Embedded Linux  
**Teslim:** 5 Haziran  
**GitHub:** github.com/mihsanekinci/ai-driven-embedded-linux  
**Proje dizini:** ~/gomulu_sistemler/ai-driven-embedded-linux  

## Ortam
- Ubuntu VM (VMware) + VS Code + Claude Code  
- arm-linux-gnueabihf-gcc 15.2.0  
- QEMU 10.2.1  
- dtc 1.7.2  

## Aşamalar ve Puanlar
| Aşama | Görev | Puan | Durum |
|-------|-------|------|-------|
| 1 | ARM C uygulaması cross-compile | 20 | ✅ |
| 2 | BusyBox RootFS | 10 | ✅ |
| 3 | QEMU boot | 10 | ✅ |
| 4 | IPC pipe/fork | 10 | ✅ |
| 5 | Device Tree düzenleme | 10 | ⏳ |
| 6 | Character driver analizi | 10 | ⏳ |
| - | Teknik Rapor | 10 | ⏳ |

## Aşama Detayları

### ✅ Aşama 1 — Cross Compile
- temperature_sensor.c yazıldı ve ARM için derlendi
- qemu-user ile çalıştırıldı, ekran görüntüsü alındı
- Binary: source_code/temperature_sensor

### ✅ Aşama 2 — RootFS
- BusyBox kaynak koddan ARM için derlendi (tc.c hatası çözüldü)
- rootfs/ dizin yapısı oluşturuldu (401 symlink)
- ext2 değil cpio initramfs tercih edildi
- Çıktı: qemu/initramfs.cpio.gz (1.6MB)
- qemu/rootfs.ext2 gitignore'da (64MB)

### ✅ Aşama 3 — QEMU Boot
**Tamamlanma:** 31 Mayıs 2026

**Oluşturulan dosyalar:**
- `qemu/zImage` — Debian 6.1.0-49-armmp ARM kernel (5.2MB)
- `qemu/vexpress-v2p-ca9.dtb` — VExpress-A9 device tree blob (14KB)
- `qemu/run_qemu.sh` — QEMU boot scripti (`-M vexpress-a9 -serial stdio -display none`)
- `qemu/boot.log` — 211 satır boot çıktısı
- `screenshots/stage3_boot_log.txt` — Boot logdan ilk 50 satır

**Karşılaşılan hata:**
- `Failed to execute /sbin/init (error -8: ENOEXEC)` — Aşama 2'de oluşturulan
  initramfs içindeki BusyBox binary'si x86-64 idi (host makineden kopyalanmıştı).
  ARM kernel bu binary'yi çalıştıramadı.
- **Çözüm:** Debian armhf reposundan `busybox-static_1.35.0-4_armhf.deb` indirildi,
  ARM ELF32 binary ile initramfs yeniden paketlendi (1.2MB).

**Boot başarı kanıtı:**
```
[2.990733] Run /sbin/init as init process
Minimal Linux ARM - Gomulu Sistemler Projesi
Please press Enter to activate this console.
```

### ✅ Aşama 4 — IPC
**Tamamlanma:** 31 Mayıs 2026

**Oluşturulan dosyalar:**
- `source_code/ipc_demo.c` — fork() + pipe() kullanan C programı
- `source_code/ipc_demo_arm` — ARM statik binary (486KB)
- `screenshots/stage4_ipc.txt` — QEMU çıktısı

**Çalışma mantığı:**
- Parent (PID=1): pipe açar, fork yapar, 5 sıcaklık ölçümü yazar (1s aralıkla), pipe'ı kapatır
- Child (PID=87): pipe'tan okur, her mesajı ekrana yazar, EOF'ta çıkar

**QEMU çıktısı (kanıt):**
```
IPC Demo - fork() + pipe()
---------------------------
[Parent PID=1] Veri gonderiliyor...
[Child  PID=87] Okuma bekleniyor...
[Child  PID=87] Alindi: Olcum #1: 20.0 C
[Child  PID=87] Alindi: Olcum #2: 23.0 C
[Child  PID=87] Alindi: Olcum #3: 26.0 C
[Child  PID=87] Alindi: Olcum #4: 29.0 C
[Child  PID=87] Alindi: Olcum #5: 32.0 C
[Child  PID=87] Pipe kapandi, cikiliyor.
[Parent PID=1] Tamamlandi.
```

### ⏳ Aşama 5 — Device Tree
- Henüz başlanmadı
- Sensör node yazılacak

### ⏳ Aşama 6 — Character Driver
- Henüz başlanmadı
- file_operations, open/read/write analizi yapılacak

## Teslim Gereklilikleri
Her aşama için raporda:
1. Kullanılan prompt (ne verildi, neden, nasıl geliştirildi)
2. Kodun açıklaması
3. Hata ayıklama süreci

Teslim klasörü: report.pdf, prompts.txt, screenshots/, 
source_code/, rootfs/, dts/, qemu/, README.md
