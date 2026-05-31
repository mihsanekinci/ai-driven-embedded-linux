# temp_driver.c — Karakter Sürücü Analizi

## 1. `file_operations` struct'ı ne işe yarar?

`file_operations` (satır 65-71), çekirdek ile kullanıcı alanı arasındaki köprüdür.
Kullanıcı bir `/dev/temp_sensor` dosyasını açtığında, okuduğunda veya yazdığında
çekirdek bu struct içindeki fonksiyon işaretçilerine bakarak hangi kodun çalışacağına karar verir.

```c
static const struct file_operations temp_fops = {
    .owner   = THIS_MODULE,
    .open    = temp_open,     // open() sistem çağrısı
    .read    = temp_read,     // read() sistem çağrısı
    .write   = temp_write,    // write() sistem çağrısı
    .release = temp_release,  // close() sistem çağrısı
};
```

Kullanıcı `open("/dev/temp_sensor", O_RDONLY)` yazdığında çekirdek doğrudan
`temp_fops.open` → `temp_open()` fonksiyonunu çağırır.

---

## 2. `open` / `read` / `write` / `release` ne zaman çağrılır?

| Fonksiyon | Kullanıcı çağrısı | Satır |
|-----------|-------------------|-------|
| `temp_open`    | `open("/dev/temp_sensor", ...)` | 21 |
| `temp_read`    | `read(fd, buf, n)`              | 27 |
| `temp_write`   | `write(fd, buf, n)`             | 46 |
| `temp_release` | `close(fd)`                     | 59 |

`temp_read` (satır 27-43): `copy_to_user()` ile çekirdek belleğindeki
`"25.3 C\n"` verisini kullanıcı alanına güvenli biçimde kopyalar.
`loff_t *offset` tekrar okuma yapılmasını engeller (EOF davranışı).

`temp_write` (satır 46-57): `copy_from_user()` ile kullanıcı alanından
çekirdek tamponuna veri çeker ve `printk` ile çekirdek loguna yazar.

---

## 3. `alloc_chrdev_region` ile `register_chrdev` farkı nedir?

| | `register_chrdev` (eski) | `alloc_chrdev_region` (modern) |
|---|---|---|
| Major number | Sabit veya 0 ile istenir | Çekirdek dinamik atar |
| Minor control | Yok | `baseminor` ve `count` ile tam kontrol |
| cdev | Dahili | Ayrı `cdev_init` + `cdev_add` gerekir |

Bu sürücüde (satır 76):
```c
alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
```
Çekirdek boş bir major numarası seçer → çakışma riski sıfır.
`MAJOR(dev_num)` ve `MINOR(dev_num)` makrolarıyla ayrıştırılır (satır 80-81).

---

## 4. `printk` neden `printf` değil?

`printf` C standart kütüphanesine (`libc`) aittir; çekirdek modülleri
kullanıcı alanı kütüphanelerini kullanamaz.

`printk` çekirdeğin kendi mesaj mekanizmasıdır:
- `KERN_INFO`, `KERN_ERR` gibi **log seviyesi** önekleri alır
- Mesajlar `/dev/kmsg` üzerinden akar, `dmesg` komutuyla okunur
- Interrupt bağlamında (kesme içinde) bile güvenli çağrılabilir

```c
printk(KERN_INFO "temp_driver: Driver acildi\n");  // satır 23
```

---

## 5. `module_init` ve `module_exit` makroları ne yapar?

```c
module_init(temp_driver_init);   // satır 113
module_exit(temp_driver_exit);   // satır 114
```

Bu makrolar, `insmod` ve `rmmod` komutlarının hangi fonksiyonu çağıracağını
çekirdeğe bildirir. Doğrudan çağrılmazlar; ELF bölümü etiketleri üretirler.

- `module_init` → `__init` bölümüne işaretçi yazar; modül yüklendikten sonra
  bu bellek serbest bırakılabilir (bu yüzden `__init` niteleyicisi, satır 73).
- `module_exit` → `__exit` bölümüne işaretçi yazar; `rmmod` çağrısında çalışır.

---

## 6. Major/Minor number nedir, `/dev/temp_sensor` nasıl oluşur?

**Major number**: hangi sürücüyü kullanacağını belirtir (örn. major=240 → temp_driver).  
**Minor number**: aynı sürücünün hangi cihaz örneğini kullanacağını belirtir (burada 0).

`/dev/temp_sensor` oluşma sırası (satır 83-98):

```
alloc_chrdev_region()   → major/minor çifti alınır
cdev_init() + cdev_add() → sürücü çekirdeğe kaydedilir
class_create()           → /sys/class/temp/ oluşur
device_create()          → udev/mdev bu sınıfı görür ve
                           /dev/temp_sensor dosyasını otomatik yaratır
```

---

## 7. Akış Diyagramı

```
YUKLEME (insmod temp_driver.ko)
=====================================
insmod
  └─► module_init() → temp_driver_init()
        ├─► alloc_chrdev_region()   → major:minor alındı
        ├─► cdev_init() + cdev_add() → çekirdeğe kayıt
        ├─► class_create()           → /sys/class/temp/
        └─► device_create()          → /dev/temp_sensor oluştu


KULLANICI ERISIMI
=====================================
open("/dev/temp_sensor")
  └─► file_operations.open → temp_open()
        └─► printk("Driver acildi")

read(fd, buf, n)
  └─► file_operations.read → temp_read()
        ├─► copy_to_user("25.3 C\n")
        └─► return: okunan byte sayısı

write(fd, buf, n)
  └─► file_operations.write → temp_write()
        ├─► copy_from_user(kbuf)
        └─► printk("Kullanicidan veri: ...")

close(fd)
  └─► file_operations.release → temp_release()
        └─► printk("Driver kapatildi")


KALDIRMA (rmmod temp_driver)
=====================================
rmmod
  └─► module_exit() → temp_driver_exit()
        ├─► device_destroy()             → /dev/temp_sensor silindi
        ├─► class_destroy()              → /sys/class/temp/ silindi
        ├─► cdev_del()                   → çekirdek kaydı silindi
        └─► unregister_chrdev_region()   → major:minor serbest
```
