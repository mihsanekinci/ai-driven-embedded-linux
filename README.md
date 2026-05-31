# AI Driven Embedded Linux

Sıfırdan bir ARM Linux sistemi kurma sürecini belgeleyen bu proje; cross-compilation, minimal rootfs oluşturma, QEMU üzerinde bare-metal boot, IPC mekanizmaları, Device Tree düzenleme ve Linux karakter sürücü geliştirme konularını kapsar.

---

## Geliştirme Ortamı

| Araç | Sürüm |
|------|-------|
| Host OS | Ubuntu (VMware VM) |
| Cross-compiler | `arm-linux-gnueabihf-gcc` 15.2.0 |
| Emülatör | QEMU 10.2.1 |
| Userspace | BusyBox 1.36.1 |
| Device Tree Compiler | `dtc` 1.7.2 |
| Hedef Mimari | ARM Cortex-A9 (VExpress-A9) |

---

## Proje Yapısı

```
ai-driven-embedded-linux/
├── source_code/
│   ├── temperature_sensor.c      # ARM C uygulaması
│   ├── temperature_sensor_arm    # ARM binary
│   ├── ipc_demo.c                # IPC demo (fork + pipe)
│   ├── ipc_demo_arm              # ARM binary
│   └── char_driver/
│       ├── temp_driver.c         # Linux kernel modülü
│       ├── Makefile
│       └── analysis.md           # Sürücü analizi dökümü
├── rootfs/                       # Minimal BusyBox RootFS
│   ├── bin/busybox
│   ├── etc/
│   │   ├── inittab
│   │   ├── fstab
│   │   ├── passwd / group
│   │   └── init.d/rcS
│   └── usr/bin/temperature_sensor
├── qemu/
│   ├── zImage                    # ARM kernel (Debian 6.1.0-49 armhf)
│   ├── vexpress-v2p-ca9.dtb      # Varsayılan DTB
│   ├── initramfs.cpio.gz         # cpio initramfs (1.2MB)
│   ├── run_qemu.sh               # Temel boot scripti
│   └── run_qemu_stage5.sh        # Modified DTB ile boot
├── dts/
│   ├── vexpress-original.dts     # Decompile edilmiş orijinal DTS
│   ├── vexpress-modified.dts     # LM75 sensör node eklenmiş
│   └── vexpress-modified.dtb     # Derlenmiş DTB (14KB)
└── screenshots/
    ├── stage3_boot_log.txt       # QEMU boot çıktısı
    ├── stage4_ipc.txt            # IPC demo çıktısı
    ├── stage5_dts.txt            # Device Tree doğrulama
    └── stage6_driver_analysis.txt
```

---

## ARM Cross Compilation

`temperature_sensor.c`, ARM hedef mimarisi için host makinede derlenir ve her saniye sıcaklık ölçümü simüle eder.

```bash
arm-linux-gnueabihf-gcc -static -o temperature_sensor_arm temperature_sensor.c

file temperature_sensor_arm
# → ELF 32-bit LSB executable, ARM
```

---

## Minimal RootFS (BusyBox)

BusyBox 1.36.1 kaynak koddan ARM static binary olarak derlenir. 401 sembolik link ile tam bir userspace oluşturulur; ardından cpio initramfs olarak paketlenir.

```bash
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- defconfig
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-

cd rootfs && find . | cpio -H newc -o | gzip > ../qemu/initramfs.cpio.gz
```

> **Not:** İlk denemede initramfs içindeki binary host makineden (x86-64) kopyalanmıştı; ARM kernel `ENOEXEC` hatasıyla boot edemedi. Debian armhf reposundan `busybox-static_1.35.0-4_armhf.deb` çekilerek ARM ELF32 binary ile yeniden paketlendi.

---

## QEMU Boot

ARM VExpress-A9 platformunda gerçek Linux boot:

```bash
qemu-system-arm \
    -M vexpress-a9 \
    -kernel qemu/zImage \
    -dtb qemu/vexpress-v2p-ca9.dtb \
    -initrd qemu/initramfs.cpio.gz \
    -append "console=ttyAMA0 root=/dev/ram" \
    -serial stdio -display none
```

```
[2.990733] Run /sbin/init as init process
Minimal Linux ARM - Gomulu Sistemler Projesi
Please press Enter to activate this console.
```

---

## IPC: fork() + pipe()

`ipc_demo.c`, `fork()` ve `pipe()` ile iki process arasında veri iletişimini gösterir. Parent process sıcaklık ölçümlerini pipe'a yazar, child process pipe'tan okuyarak ekrana basar.

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

---

## Device Tree Düzenleme

VExpress-A9'un `.dtb` dosyası `dtc` ile decompile edilerek I2C bus'a National LM75 sıcaklık sensörü node'u eklendi ve yeniden derlendi.

```dts
/* dts/vexpress-modified.dts — i2c@16000 altına eklendi */
temperature_sensor: temp-sensor@48 {
    compatible = "national,lm75";
    reg = <0x48>;
    status = "okay";
    label = "CPU Temperature Sensor";
};
```

```bash
dtc -I dts -O dtb -o dts/vexpress-modified.dtb dts/vexpress-modified.dts
```

QEMU içinde `/proc/device-tree` üzerinden doğrulandı:

```
ls /proc/device-tree/.../i2c@16000/
# → dvi-transmitter@39  dvi-transmitter@60  temp-sensor@48

cat .../temp-sensor@48/compatible
# → national,lm75
```

---

## Linux Karakter Sürücü

`source_code/char_driver/temp_driver.c` — `/dev/temp_sensor` aygıt dosyasını oluşturan 114 satırlık kernel modülü.

| Konu | Açıklama |
|------|----------|
| `file_operations` | Kullanıcı ↔ kernel sistem çağrısı köprüsü |
| `alloc_chrdev_region` | Dinamik major/minor number tahsisi |
| `copy_to_user` / `copy_from_user` | Güvenli kullanıcı ↔ kernel bellek kopyalama |
| `printk` + log seviyeleri | `KERN_INFO`, `KERN_ERR` — `dmesg` ile izlenir |
| `class_create` + `device_create` | `/dev/temp_sensor` otomatik oluşumu |
| `module_init` / `module_exit` | `insmod` / `rmmod` yaşam döngüsü |

```
insmod temp_driver.ko
  └─► alloc_chrdev_region → cdev_add → class_create → device_create
        → /dev/temp_sensor oluştu

cat /dev/temp_sensor        → "25.3 C"
echo "test" > /dev/temp_sensor  → dmesg'de görünür

rmmod temp_driver
  └─► device_destroy → class_destroy → cdev_del → unregister_chrdev_region
```

Detaylı analiz için: [source_code/char_driver/analysis.md](source_code/char_driver/analysis.md)

---

## Hızlı Başlangıç

```bash
# QEMU ile boot
bash qemu/run_qemu.sh

# Modified Device Tree ile boot
bash qemu/run_qemu_stage5.sh

# ARM binary'leri host üzerinde test et (qemu-user gerekir)
qemu-arm source_code/temperature_sensor_arm
qemu-arm source_code/ipc_demo_arm
```

---

## Lisans

MIT
