#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>  //file_operations structure- which of course allows use to open/close, read/write to device
#include <linux/cdev.h> //this is a char drive; makes cdev available
#include <linux/semaphore.h> //used to acces semaphores
#include <asm/uaccess.h> //copy_to_user and copy_from_user
#include <linux/uaccess.h>
#include <linux/device.h>

#define NAME "cryptctl"

static dev_t major = 0;
static struct cdev *mycdev;
static struct class *myclass = NULL;

// create a stucture for our fake device
struct fake_device{
	char data[100];
	struct semaphore sem;
} virtual_device;

int ret; //will be used to hold return values of functions; this is because the kernel stack is very small

//Encryption: Ei = (Pi + Ki) % 26
char* encrypt(char* key, char c[]){
    /*    int value = (c + key) % 26;
     value += 'a';
     return value; */
    
    int i;
    char currKey = *key;
    for(i = 0; c[i] != '\0'; i++){
        currKey = *key + (i % strlen(key));
        char newChar = c[i] + (currKey - 'a');
        if(newChar > 'z'){
            int shift = newChar - 'z';
            c[i] = 'a' + shift - 1;
        }
        else
            c[i] = newChar;
    }
    return c;
}

//Decryption: Di = (Ei - Ki + 26) % 26
char* decrypt(char* key, char c[]){
    
    int i;
    char currKey = *key;
    for(i =0; c[i] != '\0'; i++){
        currKey = *(key) + (i % strlen(key));
        char newChar = c[i] - (currKey - 'a');
        if(newChar < 'a'){
            int shift = 'a' - newChar;
            c[i] = 'z' - shift +1;
        }
        else{
            c[i] = newChar;
        }
    }
    
    return c;
}

int device_open(struct inode *inode, struct file *filp){
	printk(KERN_INFO "testcode: opened device");
	return 0;
}

ssize_t device_read(struct file* filp, char* bufStoreData, size_t bufCount, loff_t* curOffset){
	//take data from kernel space(device) to user space (process)
	printk(KERN_INFO "testcode: Reading from device");
	//copy_to_user(destination, source, sizeToTransfer);
	ret = copy_to_user(bufStoreData, virtual_device.data, bufCount);
	return ret;
}

ssize_t device_write(struct file* filp, const char* bufSourceData, size_t bufCount, loff_t* curOffset){
	//send data from user to kernel
	printk(KERN_INFO "testcode: writing to device");
	//copy_from_user(dest, source, count)
	ret = copy_from_user(virtual_device.data, bufSourceData, bufCount);
	return ret;
}

int device_close(struct inode *inode, struct file *filp){
	printk(KERN_INFO "testcode: closed device");
	return 0;
}


struct file_operations fops = {
	.owner = THIS_MODULE, //prevent unloading of this module when operations are in use
	.open = device_open, //points to the method to call when opening the device
	.release = device_close, //points to the method to call when closing the device
	.write = device_write, //points to the method to call wehn writing to the device
	.read = device_read //points to the method to call whenreading from the device
};

static void cleanup(int device_created){
	if(device_created){
		device_destroy(myclass, major);
		cdev_del(mycdev);
	}
	if(myclass) class_destroy(myclass);
	if(major != -1) unregister_chrdev_region(major, 1);
}

static int driver_entry(void){

	int device_created = 0;
	ret = alloc_chrdev_region(&major, 0, 1, NAME "_proc");
	if(ret < 0){ //at time kernel functions return negatives, tehre is an erro
		goto error;
	}

	printk(KERN_INFO "Major num: %d", MAJOR(major));

	if((myclass = class_create(THIS_MODULE, NAME "_sys")) == NULL)
		goto error;

	if(device_create(myclass, NULL, major, NULL, NAME) == NULL)
		goto error;
	device_created = 1;	
	mycdev = cdev_alloc();
	cdev_init(mycdev, &fops);
	if(cdev_add(mycdev, major, 1) == -1)
		goto error;

	return 0;

	error:
		printk(KERN_INFO "ERROR");
		cleanup(device_created);
		return -1;

	return 0;
}

static void driver_exit(void){
	cleanup(1);
}


module_init(driver_entry);
module_exit(driver_exit);
MODULE_LICENSE("GPL");
