#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h> /* for copy_to_user */
#include <linux/cdev.h>
#include <asm-generic/errno.h>
#include <linux/init.h>
#include <linux/tty.h>      /* For fg_console */
#include <linux/kd.h>       /* For KDSETLED */
#include <linux/vt_kern.h>
#include <linux/version.h> /* For LINUX_VERSION_CODE */


MODULE_LICENSE("GPL");

int init_module(void);
void cleanup_module(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

#define SUCCESS 0
#define DEVICE_NAME "leds"	
#define BUF_LEN 80

dev_t start;
struct cdev* chardev=NULL;
static int Device_Open = 0;
static char *leidos_Ptr = NULL;


static struct file_operations fops = {
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release
};

//Manejador que controla los leds
struct tty_driver* kbd_driver= NULL;

/* Get driver handler */
struct tty_driver* get_kbd_driver_handler(void)
{
    printk(KERN_INFO "modleds: loading\n");
    printk(KERN_INFO "modleds: fgconsole is %x\n", fg_console);
#if ( LINUX_VERSION_CODE > KERNEL_VERSION(2,6,32) )
    return vc_cons[fg_console].d->port.tty->driver;
#else
    return vc_cons[fg_console].d->vc_tty->driver;
#endif
}

/* Set led state to that specified by mascara */
static inline int set_leds(struct tty_driver* handler, unsigned int mascara)
{
#if ( LINUX_VERSION_CODE > KERNEL_VERSION(2,6,32) )
    return (handler->ops->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED,mascara);
#else
    return (handler->ops->ioctl) (vc_cons[fg_console].d->vc_tty, NULL, KDSETLED, mascara);
#endif
}


int init_module(void)
{
    int major;		/* Major number assigned to our device driver */
    int minor;		/* Minor number assigned to the associated character device */
    int ret;

    /* Get available (major,minor) range */
    if ((ret=alloc_chrdev_region (&start, 0, 1,DEVICE_NAME))) {
        printk(KERN_INFO "Can't allocate chrdev_region()");
        return ret;
    }

    /* Create associated cdev */
    if ((chardev=cdev_alloc())==NULL) {
        printk(KERN_INFO "cdev_alloc() failed ");
        unregister_chrdev_region(start, 1);
        return -ENOMEM;
    }

    cdev_init(chardev,&fops);

    if ((ret=cdev_add(chardev,start,1))) {
        printk(KERN_INFO "cdev_add() failed ");
        kobject_put(&chardev->kobj);
        unregister_chrdev_region(start, 1);
        return ret;
    }

    kbd_driver= get_kbd_driver_handler();

    major=MAJOR(start);
    minor=MINOR(start);

    printk(KERN_INFO "I was assigned major number %d. To talk to\n", major);
    printk(KERN_INFO "the driver, create a dev file with\n");
    printk(KERN_INFO "'sudo mknod -m 666 /dev/%s c %d %d'.\n", DEVICE_NAME, major,minor);
    printk(KERN_INFO "Try to cat and echo to the device file.\n");
    printk(KERN_INFO "Remove the device file and module when done.\n");

    return SUCCESS;
}


void cleanup_module(void)
{
    /* Destroy chardev */
    if (chardev)
        cdev_del(chardev);

    set_leds(kbd_driver,0); //apaga los leds
    /*
     * Unregister the device
     */
    unregister_chrdev_region(start, 1);
}


static int device_open(struct inode *inode, struct file *file)
{
    if (Device_Open){
    	printk(KERN_ALERT "El dispositivo ya está abierto");
        return -EBUSY;
    }
    Device_Open++;

    printk(KERN_INFO "Se abre el dispositivo\n");

    /* Increase the module's reference counter */
    try_module_get(THIS_MODULE);

    return SUCCESS;
}


static int device_release(struct inode *inode, struct file *file)
{
    Device_Open--;		/* We're now ready for our next caller */

    /*
     * Decrement the usage count, or else once you opened the file, you'll
     * never get get rid of the module.
     */
    module_put(THIS_MODULE);
    printk(KERN_INFO "Se cierra el dispositivo\n");

    return 0;
}


static ssize_t device_read(struct file *filp, char *buffer, size_t length, loff_t * offset)
{
    printk(KERN_ALERT "Sorry, this operation isn't supported.\n");
    return -EPERM;
}


static ssize_t device_write(struct file *filp, const char *buffer, size_t len, loff_t * off)
{
    printk(KERN_INFO "-------Write-------\n");

    char* buff  = (char *) vmalloc(len); 

    if(copy_from_user(buff, buffer, len)){
    	return -EFAULT;
    }

    int bytesToWrite = len;

    if (bytesToWrite > strlen(buff)){
        bytesToWrite = strlen(buff);
    }

    unsigned int mascara = 0;
    int numLock = 0;
    int capsLock = 0;
    int scrollLock = 0;

    int i = 0;
    while (i < bytesToWrite){
    	if (*buff == '1'){
    		numLock = 2;
    	}
    	else if (*buff == '2'){
    		capsLock = 4;
    	}
    	else if (*buff == '3'){
    		scrollLock = 1;
    	}

    	printk(KERN_INFO "Encendemos led: %c \n", *buff);

    	++buff;
        ++i;
    }

    mascara = numLock + capsLock + scrollLock; //la máscara es la suma de los tres 
    printk(KERN_INFO "La mascara es: %d \n", mascara); 
    set_leds(kbd_driver, mascara); //enciende los leds segun diga la máscara

    vfree(buff);

    return bytesToWrite;
}