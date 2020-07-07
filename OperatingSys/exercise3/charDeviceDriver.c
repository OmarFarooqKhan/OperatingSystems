/*
 *  chardev.c: Creates a read-only char device that says how many times
 *  you've read from the dev file
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include<linux/slab.h>
#include <linux/fs.h>
#include <asm/uaccess.h>    /* for put_user */
#include <charDeviceDriver.h>
#include "ioctl.h"

MODULE_LICENSE("GPL");

/*
 * This function is called whenever a process tries to do an ioctl on our
 * device file. We get two extra parameters (additional to the inode and file
 * structures, which all device functions get): the number of the ioctl called
 * and the parameter given to the ioctl function.
 *
 * If the ioctl is write or read/write (meaning output is returned to the
 * calling process), the ioctl call returns the output of this function.
 *
 */

static int BUFFER_MAX = 2 * 1024 *1024;

DEFINE_MUTEX  (devLock);

/* A linked list node */
static struct Node {
    // Any data type can be stored in this node
    char  *data;
  size_t length;
    struct Node *next;
};

static int currentMem = 0;
static struct Node *head = NULL;
static long device_ioctl(struct file *file,    /* see include/linux/fs.h */
         unsigned int ioctl_num,    /* number and param for ioctl */
         unsigned long ioctl_param)
{

    /*
     * Switch according to the ioctl called
     */
    if (ioctl_num == MSG_LIM) {
	unsigned int size;
	copy_from_user(&size,(void *)ioctl_param,sizeof(unsigned int));

  if(size > currentMem) {
  
        BUFFER_MAX = size;
        /*         return 0; */
        return SUCCESS; /* can pass integer as return value */
  }else{

return -EINVAL;
}
    }

    else {
        /* no operation defined - return failure */
        return -EINVAL;

    }
}


/*
 * This function is called when the module is loaded
 */
int init_module(void)
{
        Major = register_chrdev(0, DEVICE_NAME, &fops);

    if (Major < 0) {
      printk(KERN_ALERT "Registering char device failed with %d\n", Major);
      return Major;
    }

    printk(KERN_INFO "I was assigned major number %d. To talk to\n", Major);
    printk(KERN_INFO "the driver, create a dev file with\n");
    printk(KERN_INFO "'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, Major);
    printk(KERN_INFO "Try various minor numbers. Try to cat and echo to\n");
    printk(KERN_INFO "the device file.\n");
    printk(KERN_INFO "Remove the device file and module when done.\n");

    return SUCCESS;
}

/*
 * This function is called when the module is unloaded
 */
void cleanup_module(void)
{
    struct Node *newHead =NULL;
    while(head != NULL){
	currentMem = currentMem - (head->length);
        kfree(head->data);
        newHead = head->next;
        kfree(head);
        head = newHead;
    }
    /*  Unregister the device */
    unregister_chrdev(Major, DEVICE_NAME);
}

/*
 * Methods
 */


void push(struct Node** head_ref, char *new_data, size_t data_size)
{
    // Allocate memory for node
    struct Node *new_node;
    struct Node *cursor;
    new_node = kmalloc(sizeof(struct Node),GFP_KERNEL);
  
    new_node->data  = kmalloc(data_size,GFP_KERNEL);
    new_node->next = NULL;
  new_node->length = data_size;
    currentMem += data_size;
    // Copy contents of new_data to newly allocated memory.
    // Assumption: char takes 1 byte.
    int i;
    for (i=0; i<data_size; i++)
        *(char *)(new_node->data + i) = *(char *)(new_data + i);
  
    // Change head pointer as new node is added at the beginning
    if(*head_ref == NULL){
        *head_ref = new_node;
    }

    else{
    cursor = *head_ref;
    while(cursor->next != NULL){
        cursor = cursor->next;
    }
    cursor->next = new_node;
    }
}

/*
 * Called when a process tries to open the device file, like
 * "cat /dev/mycharfile"
 */
static int device_open(struct inode *inode, struct file *file)
{
    
    mutex_lock (&devLock);
    
    try_module_get(THIS_MODULE);
    
    return SUCCESS;
}

/* Called when a process closes the device file. */
static int device_release(struct inode *inode, struct file *file)
{
   

    mutex_unlock (&devLock);
    module_put(THIS_MODULE);

    return 0;
}

/*
 * Called when a process, which already opened the dev file, attempts to
 * read from it.
 */
static ssize_t device_read(struct file *filp,    /* see include/linux/fs.h   */
               char *buffer,    /* buffer to fill with data */
               size_t length,    /* length of the buffer     */
               loff_t * offset)
{
    

    /* result of function calls */
    int result;
  
  if (head == NULL){
    return -EAGAIN;
  }
  
  result = copy_to_user(buffer,(head->data),(head->length));
  if(result != 0){
    return -EAGAIN;
  }

   struct Node *newHead = NULL;
     kfree(head->data);
     currentMem = currentMem - (head->length);
     newHead = head->next;
     kfree(head);
     head = newHead;

    return length;
}

/* Called when a process writes to dev file: echo "hi" > /dev/hello  */
static ssize_t
device_write(struct file *filp, const char *buff, size_t len, loff_t * off){

    if(len > BUF_LEN){
        printk(KERN_ALERT "Sorry, this operation isn't supported.\n");
    return -EINVAL;
    }

    if(len + currentMem > BUFFER_MAX ){
    printk(KERN_ALERT "Sorry, this operation isn't supported.\n");
    return -EINVAL;
    }

    else{
        copy_from_user(msg,buff,len);
        push(&head,msg,len);
        memset(msg,0,len);
    return len;
    }
}
