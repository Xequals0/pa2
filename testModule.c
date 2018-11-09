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
	int changed;
} keyStruct;

typedef struct{
    int pair;
    int deleted;
} delStruct;

typedef struct
{
	char key[256];
	char data[1000];
	int encminor;
	int decminor;
	struct cdev *enccdev;
	struct cdev *deccdev;
} devpair;

devpair devices[100];

static int minor = 1;
int count = 0;

int ret;
char* encrypt(char* key, char c[]){
   	int i;
	char currKey = *key;
	for(i = 0; c[i] != '\0'; i++){
		if(c[i] == ' ') continue;
		currKey = *(key +(i % strlen(key)));
		printk(KERN_INFO "here: %c",c[i] + (currKey - 'a'));
		if(c[i] + (currKey - 'a') >  'z'){
			c[i] = c[i] + currKey - 'z' - 1;
		}
		else{
			c[i] = c[i] + (currKey -'a');
		}
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
		currKey = *(key + (i % strlen(key)));
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

	printk(KERN_INFO "testcode: reading - in data= %s", devices[i].data);
	ret = copy_to_user(bufStoreData, devices[i].data, bufCount);
	return ret;
}

ssize_t device_write(struct file* filp, const char* bufSourceData, size_t bufCount, loff_t* curOffset){
	int devminor = iminor(filp->f_path.dentry->d_inode);
	int i;
	int bytesRead;
	int encDev = 0;
	char * str;
	char str1[1000];
	char str2[1000];

	for(i = 0; i < 100; i++)
	{
		if(devices[i].decminor == devminor) break;
		else if(devices[i].encminor == devminor)
		{
			encDev = 1;
			break;
		}
	}

	printk(KERN_INFO "!!-- %d",encDev);
	bytesRead = copy_from_user(str2, bufSourceData, bufCount);
	printk(KERN_INFO "input text: %s", str2);
	if(encDev)
	{
		str = encrypt(devices[i].key, str2);
		strcpy(str1, str);
		strcpy(devices[i].data, str1);
		printk(KERN_INFO "encrypted text: %s", str1);
	}
	else
	{
		str = decrypt(devices[i].key, str2);
		strcpy(str1, str);
		strcpy(devices[i].data, str1);
		printk(KERN_INFO "decrypted text: %s", str1);
	}
	return  bytesRead;
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
			ret = delete_dev_pair(((delStruct*) arg)->pair);
			copy_to_user(&(((delStruct*) arg)->deleted), &ret, sizeof(int));						
			break;
		case CHANGE_IOCTL:
			copy_from_user(key, ((keyStruct*) arg)->key, strlen(((keyStruct*) arg)->key) + 1);
			ret = change_key(key, ((keyStruct*) arg)->pair);
			copy_to_user(&(((keyStruct*) arg)->changed), &ret, sizeof(int));									
			break;
		default:
			break;
	}

	return 0;
}

struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = device_open,
	.release = device_close,
	.write = device_write,
	.read = device_read,
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
		return 0;
	}
	printk(KERN_INFO "invalid pair");
	return -1;
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

		return 0;
	}

	return -1;
}

int create_dev_pair(char* key)
{

	char nameBuf[10];
	sprintf(nameBuf, ENCDEV"%d", count);
	printk(KERN_INFO "1- Major num: %d", minor);

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
	if(ret < 0){
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
