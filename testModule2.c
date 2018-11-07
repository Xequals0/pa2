#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>  //file_operations structure- which of course allows use to open/close, read/write to device
#include <linux/cdev.h> //this is a char drive; makes cdev available
#include <linux/semaphore.h> //used to acces semaphores
#include <asm/uaccess.h> //copy_to_user and copy_from_user
#include <linux/uaccess.h>

// create a stucture for our fake device
struct fake_device{
	char data[100];
	struct semaphore sem;
} virtual_device;

// to later register our device we need a cdev object and some other variables
struct cdev *mcdev; //my char device
int major_number; // will store our major number - extracted from dev_t using macro- mknod /director/file c major minor 
int ret; //will be used to hold return values of functions; this is because the kernel stack is very small
	//so declaring varibales all over the pass in our module functions eats up the stack very fast
dev_t dev_num; //will hold major number that kernel gives us
#define DEVICE_NAME "testDevice"
// name --> appears in /procc/devices




char* encrypt(char* key, char c[]){
   	int i;
	char currKey = *key;
	int shift;
	char newChar;
	for(i = 0; c[i] != '\0'; i++){
		currKey = *key + (i % strlen(key));
		newChar = c[i] + (currKey - 'a');
		if(newChar > 'z'){
			shift = newChar - 'z';
			c[i] = 'a' + shift - 1;
		}
		else
			c[i] = newChar;
	}
	return c;
}



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
//7. called on device_file open
	//inode reference to the file on disk
	//and contains information about that file
	//struct file is represnets an abstract open file
int device_open(struct inode *inode, struct file *filp){	//only allow one process to opent his device by using a semaphore as mutual exlusive lock- mutex
	if(down_interruptible(&virtual_device.sem) != 0){
		printk(KERN_ALERT "testcode: could not lock device during open");
		return -1;
	}
	printk(KERN_INFO "testcode: opened device");
	return 0;
}

//8. called when user wants to get information from the device
ssize_t device_read(struct file* filp, char* bufStoreData, size_t bufCount, loff_t* curOffset){
	//take data from kernel space(device) to user space (process)
	printk(KERN_INFO "testcode: Reading from device");
	//copy_to_user(destination, source, sizeToTransfer);
	ret = copy_to_user(bufStoreData, virtual_device.data, bufCount);
	return ret;
}

//9. called when user wants to send infromation to the device
ssize_t device_write(struct file* filp, const char* bufSourceData, size_t bufCount, loff_t* curOffset){
	//send data from user to kernel
	//copy_from_user(dest, source, count)
	char key[100];
	char * str;
	char str1[100];
	char str2[100];

	strcpy(str2, bufSourceData);
	key[0] = 'b';
	key[1] = '\0';

	str = encrypt(key, str2);
	
	strcpy(str1, str);

	printk(KERN_INFO "testcode: str =%s ", str1);
	//ret = copy_from_user(virtual_device.data, str1, bufCount);
	strcpy(virtual_device.data, str1);

	return  0;
	//return ret;
}

//10. called upon user close
int device_close(struct inode *inode, struct file *filp){
	//by calling up, which is the opposite of down for smeaphore, we release the mutex obtained at device open
	//this has the effect of allowing other process to use the device now
	up(&virtual_device.sem);
	printk(KERN_INFO "testcode: closed device");
	return 0;
}


//6.
struct file_operations fops = {
	.owner = THIS_MODULE, //prevent unloading of this module when operations are in use
	.open = device_open, //points to the method to call when opening the device
	.release = device_close, //points to the method to call when closing the device
	.write = device_write, //points to the method to call wehn writing to the device
	.read = device_read //points to the method to call whenreading from the device
};

static int driver_entry(void){
	//3. register our device wiht the system: a two step process
	//(1) use dynamic allocation to assing our device 
	ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
	if(ret < 0){ //at time kernel functions return negatives, tehre is an erro
		printk(KERN_ALERT "testcode: failed to allcoate a major number");
		return ret; //propogate error
	}

	major_number = MAJOR(dev_num); //extracts the major number and store in our variable
	printk(KERN_INFO "testcode: major number is %d", major_number);
	printk(KERN_INFO "\tuse \"mknod /dev/%s c %d 0\" for device file", DEVICE_NAME, major_number); //dmesg
	
	//(2)
	mcdev = cdev_alloc(); //create our cdev structure, initalized our cdev
	mcdev->ops = &fops;
	mcdev->owner = THIS_MODULE;
	//now that we created cdev, we have to add it to the kernel
	ret = cdev_add(mcdev, dev_num, 1);
	if(ret < 0) { //always check for errors
		printk(KERN_ALERT "testcode: unable to add cdev to kernel");
		return ret;
	}

	//4. initalize our semaphore
	sema_init(&virtual_device.sem, 1); //inital value of one

	return 0;
}

static void driver_exit(void){
	//5. unregister everything in reverse order
	//a.
	cdev_del(mcdev);

	//b.
	unregister_chrdev_region(dev_num, 1);
	printk(KERN_ALERT "testcode: unloaded module");
}


module_init(driver_entry);
module_exit(driver_exit);
