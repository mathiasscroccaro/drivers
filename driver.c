#include <linux/module.h>    	// included for all kernel modules
#include <linux/kernel.h>    	// included for KERN_INFO
#include <linux/init.h>      	// included for __init and __exit macroso
#include <linux/fs.h>			// included for file system operations
#include <asm/uaccess.h>		// included for coping data from and to user-space
#include <asm/io.h>				// included for physical memory acess

#define REG_BASE 			0x01C20800
#define PA_CFG2_REG_OFFSET		0x08
#define PA_DATA_REG_OFFSET		0x10

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathias Scroccaro Costa");
MODULE_DESCRIPTION("A Simple Hello World module");

static int device_open(struct inode *, struct file *);
static int device_release (struct inode *, struct file *);
static ssize_t device_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char __user *, size_t, loff_t *);

int major;
unsigned char counter = 0;
unsigned char fReadReady = 0;

static struct kobject *example_kobject;

static int register1;
static int register2;

static int baz,bar;

unsigned int * PA_CFG2_REG;
unsigned int * PA_DATA_REG;

struct file_operations hello_fo = {
	.owner 		= THIS_MODULE, 
	.open 		= device_open,
	.release	= device_release,
	.read 		= device_read,
	.write		= device_write
};

static ssize_t b_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int var;

	if (strcmp(attr->attr.name, "baz") == 0)
		var = baz;
	else
		var = bar;
	return sprintf(buf, "%d\n", var);
}

static ssize_t b_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	int var;

	sscanf(buf, "%du", &var);
	if (strcmp(attr->attr.name, "baz") == 0)
		baz = var;
	else
		bar = var;
	return count;
}

static struct kobj_attribute baz_attribute = __ATTR(baz, 0666, b_show, b_store);
static struct kobj_attribute bar_attribute = __ATTR(bar, 0666, b_show, b_store);

static struct attribute *attrs[] = {
	&baz_attribute.attr,
	&bar_attribute.attr,
	NULL,	/* need to NULL terminate the list of attributes */
};

static struct attribute_group attr_group = {
	.attrs = attrs,
};

static int device_init(void)
{	
	unsigned int temp;

	PA_DATA_REG = ioremap(REG_BASE+PA_DATA_REG_OFFSET,sizeof(unsigned int));
	PA_CFG2_REG = ioremap(REG_BASE+PA_CFG2_REG_OFFSET,sizeof(unsigned int));

	/* Changing PA20 to output */
	temp = ioread32(PA_CFG2_REG);
	temp &= ~(0x7<<16);
	temp |= (1<<16);    
	iowrite32(temp,PA_CFG2_REG);
	
	example_kobject = kobject_create_and_add("mydriver",kernel_kobj);
	sysfs_create_group(example_kobject, &attr_group);

	/* Getting major number */
	major = register_chrdev(	0,							
					"Hello World Driver",		
					& hello_fo);	
	printk(KERN_INFO "Hello World Driver major number: %d",major);				

    return 0;	

}

		void device_exit(void)
{
    printk(KERN_INFO "Exiting driver.\n");

	iounmap(PA_DATA_REG);
	iounmap(PA_CFG2_REG);

	unregister_chrdev(major,"Hello World Driver");
}

static int device_open(struct inode *inode, struct file *f)
{
	printk(KERN_ALERT "Inside function %s",__FUNCTION__);
	counter++;
	return 0;
}

static int device_release (struct inode *inode, struct file *f)
{
	printk(KERN_ALERT "Inside function %s",__FUNCTION__);	
	return 0;
}

static ssize_t device_read(struct file *f, char __user * buffer, size_t length, loff_t *offset)
{
	unsigned int value = ioread32(PA_DATA_REG);

	memset(buffer,0,10);
	
	if (fReadReady)
	{
		if (value & (1<<20))
			sprintf(buffer,"Led On\n");
		else
			sprintf(buffer,"Led Off\n");
		
		fReadReady = 0;
		return strlen(buffer);
	}
	else
	{
		return 0;
	}	
}

static ssize_t device_write(struct file *f, const char __user * buffer, size_t length, loff_t * offset)
{	
	unsigned char count = 0;
	unsigned char len = 0;
	unsigned char msg[20];
	unsigned int temp = 0;

	memset(msg,0,20);

	while(len < length)
	{
		msg[len] = buffer[len++];
		count++;
	}

	fReadReady = 1;

	if (strcmp(msg,"liga\n")==0)
	{
		printk(KERN_INFO "PA20 1");
		
		temp = ioread32(PA_DATA_REG);
		temp |= (1<<20);

		iowrite32(temp,PA_DATA_REG);
	}
		
	else if (strcmp(msg,"desliga\n")==0)
	{
		printk(KERN_INFO "PA20 0");

		temp = ioread32(PA_DATA_REG);
		temp &= ~(1<<20);

		iowrite32(temp,PA_DATA_REG);
	}
	else
	{
		fReadReady = 0;
		printk(KERN_INFO "Command \"%s\" not valid\nPlease select: 'liga' or 'desliga'",msg);
	}
		

	return length;
}



module_init(device_init);
module_exit(device_exit);
