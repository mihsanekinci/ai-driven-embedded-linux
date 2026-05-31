#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Gomulu Sistemler Projesi");
MODULE_DESCRIPTION("Ornek Sicaklik Sensor Character Driver");

#define DEVICE_NAME "temp_sensor"
#define CLASS_NAME  "temp"
#define FAKE_TEMP   "25.3 C\n"

static dev_t dev_num;
static struct cdev temp_cdev;
static struct class *temp_class;
static struct device *temp_device;

static int temp_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "temp_driver: Driver acildi\n");
    return 0;
}

static ssize_t temp_read(struct file *file, char __user *buf,
                         size_t count, loff_t *offset)
{
    const char *data = FAKE_TEMP;
    size_t len = strlen(data);

    if (*offset >= len)
        return 0;

    if (count > len - *offset)
        count = len - *offset;

    if (copy_to_user(buf, data + *offset, count))
        return -EFAULT;

    *offset += count;
    printk(KERN_INFO "temp_driver: %zu byte okundu -> %s", count, data);
    return count;
}

static ssize_t temp_write(struct file *file, const char __user *buf,
                          size_t count, loff_t *offset)
{
    char kbuf[64] = {0};
    size_t to_copy = min(count, sizeof(kbuf) - 1);

    if (copy_from_user(kbuf, buf, to_copy))
        return -EFAULT;

    printk(KERN_INFO "temp_driver: Kullanicidan veri alindi: %s\n", kbuf);
    return count;
}

static int temp_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "temp_driver: Driver kapatildi\n");
    return 0;
}

static const struct file_operations temp_fops = {
    .owner   = THIS_MODULE,
    .open    = temp_open,
    .read    = temp_read,
    .write   = temp_write,
    .release = temp_release,
};

static int __init temp_driver_init(void)
{
    int ret;

    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ERR "temp_driver: Major number alinamadi\n");
        return ret;
    }
    printk(KERN_INFO "temp_driver: Major=%d, Minor=%d\n",
           MAJOR(dev_num), MINOR(dev_num));

    cdev_init(&temp_cdev, &temp_fops);
    temp_cdev.owner = THIS_MODULE;
    ret = cdev_add(&temp_cdev, dev_num, 1);
    if (ret < 0) {
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }

    temp_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(temp_class)) {
        cdev_del(&temp_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(temp_class);
    }

    temp_device = device_create(temp_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(temp_device)) {
        class_destroy(temp_class);
        cdev_del(&temp_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(temp_device);
    }

    printk(KERN_INFO "temp_driver: /dev/%s olusturuldu\n", DEVICE_NAME);
    return 0;
}

static void __exit temp_driver_exit(void)
{
    device_destroy(temp_class, dev_num);
    class_destroy(temp_class);
    cdev_del(&temp_cdev);
    unregister_chrdev_region(dev_num, 1);
    printk(KERN_INFO "temp_driver: Kaldirildi\n");
}

module_init(temp_driver_init);
module_exit(temp_driver_exit);
