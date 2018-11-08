#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/uaccess.h>
#include <linux/device.h>

#define NAME "cryptctl"
#define ENCDEV "encrypt"
#define DECDEV "decrypt"

#define IOCTLNUM 'k'
#define CREATE_IOCTL _IOW(IOCTLNUM, 1, int)
#define DELETE_IOCTL _IOW(IOCTLNUM, 2, int)
#define CHANGE_IOCTL _IOW(IOCTLNUM, 3, int)

static dev_t major = 0;
static struct cdev *mycdev;
static struct class *myclass = NULL;

typedef struct{
	int pair;
	char key[256];
} keyStruct;

typedef struct
{
	char key[256];
	char data[100];
	int encminor;
	int decminor;
	struct cdev *enccdev;
	struct cdev *deccdev;
} devpair;

devpair devices[100];

static int minor = 1;
int count = 0;

int ret; //will be used to hold return values of functions; this is because the kernel stack is very small

char* encrypt(char* key, char c[]){
   	int i;
	char currKey = *key;
	int shift;
	char newChar;
	for(i = 0; c[i] != '\0'; i++){
		if(c[i] == ' ') continue;
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
	char newChar;
	int shift;
	for(i =0; c[i] != '\0'; i++){
		if(c[i] == ' ')continue;
		currKey = *(key) + (i % strlen(key));
		newChar = c[i] - (currKey - 'a');
		if(newChar < 'a'){
			shift = 'a' - newChar;
			c[i] = 'z' - shift +1;
		}
		else{
			c[i] = newChar;
		}
	}
    return c;
}

int device_open(struct inode *inode, struct file *filp){
	printk(KERN_INFO "cryptcl: opened device");
	return 0;
}

ssize_t device_read(struct file* filp, char* bufStoreData, size_t bufCount, loff_t* curOffset){
	int devminor = iminor(filp->f_path.dentry->d_inode);
	int i;
	int encDev = 0;

	for(i = 0; i < 100; i++)
	{
		if(devices[i].decminor == devminor) break;
		else if(devices[i].encminor == devminor)
		{
			encDev = 1;
			break;
		}
	}

	char *str;
	char str1[100];
	printk(KERN_INFO "testcode: reading - in data= %s", devices[i].data);
	str = decrypt(devices[i].key, devices[i].data);
	strcpy(str1, str);
	ret = copy_to_user(bufStoreData, str1, bufCount);
	return ret;
}

//9. called when user wants to send infromation to the device
ssize_t device_write(struct file* filp, const char* bufSourceData, size_t bufCount, loff_t* curOffset){
	int devminor = iminor(filp->f_path.dentry->d_inode);
	int i;
	int encDev = 0;

	for(i = 0; i < 100; i++)
	{
		if(devices[i].decminor == devminor) break;
		else if(devices[i].encminor == devminor)
		{
			encDev = 1;
			break;
		}
	}

	char * str;
	char str1[100];
	char str2[100];

	strcpy(str2, bufSourceData);

	str = encrypt(devices[i].key, str2);

	strcpy(str1, str);

	printk(KERN_INFO "testcode: str =%s ", str1);
	strcpy(devices[i].data, str1);

	return  0;
	//return ret;
}

int device_close(struct inode *inode, struct file *filp){
	printk(KERN_INFO "crypt: closed device");
	return 0;
}

int create_dev_pair(char*);
int delete_dev_pair(int);
int change_key(char*, int);

static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	char key[256];
	int pair;

	printk(KERN_INFO "IOCTL CALLED");	
	switch(cmd){
		case CREATE_IOCTL:
			copy_from_user(key, ((keyStruct*) arg)->key, strlen(((keyStruct*) arg)->key) + 1);
			pair = create_dev_pair(key);
			copy_to_user(&(((keyStruct*) arg)->pair), &pair, sizeof(int));			
			break;
		case DELETE_IOCTL:
			printk(KERN_INFO "%ld", arg);
			delete_dev_pair(arg);
			break;
		case CHANGE_IOCTL:
			copy_from_user(key, ((keyStruct*) arg)->key, strlen(((keyStruct*) arg)->key) + 1);
			change_key(key, ((keyStruct*) arg)->pair);
			break;
		default:
			break;
	}

	return 0;
}

struct file_operations fops = {
	.owner = THIS_MODULE, //prevent unloading of this module when operations are in use
	.open = device_open, //points to the method to call when opening the device
	.release = device_close, //points to the method to call when closing the device
	.write = device_write, //points to the method to call wehn writing to the device
	.read = device_read, //points to the method to call whenreading from the device
	.unlocked_ioctl = device_ioctl
};

static void cleanup(int device_created){
	if(device_created){
		int i;
		for(i = 0; i < 100; i++)
		{
			if(devices[i].encminor != -1)
			{
				device_destroy(myclass, MKDEV(MAJOR(major),devices[i].encminor));
				if(devices[i].enccdev)cdev_del(devices[i].enccdev);
				device_destroy(myclass, MKDEV(MAJOR(major),devices[i].decminor));
				if(devices[i].deccdev)cdev_del(devices[i].deccdev);
			}
		}

		device_destroy(myclass, major);
		class_destroy(myclass);
		cdev_del(mycdev);		
	}
	if(myclass) class_destroy(myclass);
	if(major != -1) unregister_chrdev_region(major, 201);
}

int change_key(char* key, int pair)
{
	if(devices[pair].encminor != -1)
	{
		strcpy(devices[pair].key, key);
		printk(KERN_INFO "-- %s -- %d --", devices[pair].key, pair);
	}
	else printk(KERN_INFO "invalid pair");
	return 0;
}

int delete_dev_pair(int pair)
{
	if(devices[pair].encminor != -1)
	{
		device_destroy(myclass, MKDEV(MAJOR(major),devices[pair].encminor));
		if(devices[pair].enccdev)cdev_del(devices[pair].enccdev);
		device_destroy(myclass, MKDEV(MAJOR(major),devices[pair].decminor));
		if(devices[pair].deccdev)cdev_del(devices[pair].deccdev);

		devices[pair].encminor = -1;
	}

	return 0;
}

int create_dev_pair(char* key)
{

	char nameBuf[10];
	sprintf(nameBuf, ENCDEV"%d", count);
	printk(KERN_INFO "1- Major num: %d", minor);

	//if((devices[count].encclass = class_create(THIS_MODULE, nameBuf)) == NULL)
	//	goto error;

	if(device_create(myclass, NULL, MKDEV(MAJOR(major), minor), NULL, nameBuf) == NULL)
		goto error;

	devices[count].enccdev = cdev_alloc();
	cdev_init(devices[count].enccdev, &fops);
	if(cdev_add(devices[count].enccdev, MKDEV(MAJOR(major), minor), 1) == -1)
		goto error;

	devices[count].encminor = minor;
	minor++;

	memset(nameBuf, '\0', strlen(nameBuf));
	sprintf(nameBuf, DECDEV"%d", count);
	printk(KERN_INFO "2- Major num: %d", minor);

	//if((devices[count].decclass = class_create(THIS_MODULE, nameBuf)) == NULL)
	//	goto error;

	if(device_create(myclass, NULL, MKDEV(MAJOR(major), minor), NULL, nameBuf) == NULL)
		goto error;

	devices[count].deccdev = cdev_alloc();
	cdev_init(devices[count].deccdev, &fops);
	if(cdev_add(devices[count].deccdev, MKDEV(MAJOR(major), minor), 1) == -1)
		goto error;	

	devices[count].decminor = minor;
	strcpy(devices[count].key, key);

	printk(KERN_INFO "--%s",devices[count].key);

	minor++;
	count++;
	return count-1;;
	error:
		printk(KERN_INFO "ERROR");
		cleanup(1);
		return -1;
}

static int driver_entry(void){

	int device_created = 0;
	int i;
	ret = alloc_chrdev_region(&major, 0, 201, NAME "_proc");
	if(ret < 0){ //at time kernel functions return negatives, tehre is an erro
		goto error;
	}

	printk(KERN_INFO "Major num: %d", MAJOR(major));

	if((myclass = class_create(THIS_MODULE, NAME "_sys")) == NULL)
		goto error;

	if(device_create(myclass, NULL, MKDEV(MAJOR(major), 0), NULL, NAME) == NULL)
		goto error;
	device_created = 1;	
	mycdev = cdev_alloc();
	cdev_init(mycdev, &fops);
	if(cdev_add(mycdev, MKDEV(MAJOR(major), 0), 1) == -1)
		goto error;

	for(i = 0; i < 100; i++)
	{
		devices[i].encminor = -1;
	}

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
