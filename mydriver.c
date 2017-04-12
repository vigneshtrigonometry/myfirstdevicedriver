#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/file.h>
#include <linux/ioctl.h>

#define MAJOR_NUMBER 61
#define DEVSIZE 4*1024*1024
#define SCULL_IOC_MAGIC 'k'
#define SCULL_HELLO _IO(SCULL_IOC_MAGIC, 1)
#define SET_DEV_MSG _IOW(SCULL_IOC_MAGIC, 2, char*)
#define GET_DEV_MSG _IOR(SCULL_IOC_MAGIC, 3, char*)
#define WR_DEV_MSG _IOWR(SCULL_IOC_MAGIC, 4, char*)
#define SCULL_IOC_MAXNR 4
#define DEV_MSG_SIZE 60

/* forward declaration */
int onebyte_open(struct inode *inode, struct file *filep);
int onebyte_release(struct inode *inode, struct file *filep);
ssize_t onebyte_read(struct file *filep, char *buf, size_t count, loff_t *f_pos);
ssize_t onebyte_write(struct file *filep, const char *buf,size_t count, loff_t *f_pos);
static void onebyte_exit(void);
static loff_t onebyte_llseek(struct file *filp, loff_t offset, int whence);
long onebyte_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

/* definition of file_operation structure */
struct file_operations onebyte_fops = {
read: onebyte_read,
write: onebyte_write,
open: onebyte_open,
release: onebyte_release,
llseek : onebyte_llseek,
unlocked_ioctl : onebyte_ioctl
};

char *onebyte_data = NULL;
char *dev_msg = NULL; //char array to store message in device

int onebyte_open(struct inode *inode, struct file *filep)
{
return 0; // always successful
}

int onebyte_release(struct inode *inode, struct file *filep)
{
return 0; // always successful
}

ssize_t onebyte_read(struct file *filep, char *buf, size_t count, loff_t *f_pos)
{
/*please complete the function on your own*/
//linux doc func to output data to user space
//unsigned long copy_to_user (void __user *to,const void *from,unsigned long n);
 if(*f_pos >= DEVSIZE){
        return 0;
    }
    
    if(count + *f_pos > DEVSIZE){
        count = DEVSIZE - *f_pos;
    }

    if(copy_to_user(buf, onebyte_data + *f_pos, count)){
        return -EFAULT;
    }

    *f_pos = *f_pos + count;
return count;
/*if(onebyte_data != NULL && *f_pos == 0){ //only if 0 we copy data to user space 
copy_to_user(buf, onebyte_data, sizeof(buf));
*f_pos+=sizeof(buf); //without it goes into unlimited loop causing XXXXXXXXXXXXX printed on the screen
return 1; //just read one byte
}
return 0; // if my data is null then don't read anything
 */
}

ssize_t onebyte_write(struct file *filep, const char *buf,size_t count, loff_t *f_pos)
{
/*please complete the function on your own*/
/* loff_t is long offset - a kernel data structure that points to offset provided by user.
This must be updated because it tells us where to write. (Seek pointers) */

    if(*f_pos >= DEVSIZE){
        return -ENOSPC;
    }

    if(count + *f_pos > DEVSIZE){
        count = DEVSIZE - *f_pos;
    }

    if(copy_from_user(onebyte_data + *f_pos, buf, count)){
        return -EFAULT;
    }

    *f_pos += count;
    //print the size written
     printk(KERN_ALERT "The return value is %lld", *f_pos);

return count;

/*
if(*f_pos ==0 ){
copy_from_user(onebyte_data, buf, sizeof(buf));
*f_pos += sizeof(buf);
printk(KERN_ALERT "The return value is %lld",*f_pos);
return 1;
}
else{
return -ENOSPC;
} */
}

static loff_t onebyte_llseek(struct file *filp, loff_t offset, int whence)  
{  
    loff_t newpos;  
  
    switch(whence) {  
      case 0: /* SEEK_SET */  
        newpos = offset;  
        break;  
  
      case 1: /* SEEK_CUR */  
        newpos = filp->f_pos + offset;  
        break;  
  
      case 2: /* SEEK_END */  
        newpos = DEVSIZE -1 + offset;  
        break;  
  
      default: /* can't happen */  
        return -EINVAL;  
    }  
    if ((newpos<0) || (newpos>DEVSIZE))  
        return -EINVAL;  
  
    filp->f_pos = newpos;  
    return newpos;  
}

// ioctl function to set dev_msg using _IOW macro
long onebyte_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
    int err = 0;
    int retval = 0;
    char *tmp = NULL;

    if(_IOC_TYPE(cmd) != SCULL_IOC_MAGIC) return -ENOTTY;
    if(_IOC_NR(cmd) > SCULL_IOC_MAXNR) return -ENOTTY;

    if (_IOC_DIR(cmd) & _IOC_READ)
        err = !access_ok(VERIFY_WRITE, (void *)arg, _IOC_SIZE(cmd));
    else if (_IOC_DIR(cmd) & _IOC_WRITE)
        err = !access_ok(VERIFY_READ, (void *)arg, _IOC_SIZE(cmd));
    if (err) 
        return -EFAULT;

    switch(cmd){
        case SCULL_HELLO:
            printk(KERN_WARNING "hello from ioctl\n");
            break;
        case SET_DEV_MSG:
            if(copy_from_user(dev_msg, (char *)arg, DEV_MSG_SIZE)){
                return -EFAULT;
            }
            printk(KERN_ALERT "setting dev_msg: %s", dev_msg);
            break;
        case GET_DEV_MSG:
            if(copy_to_user((char *)arg, dev_msg, DEV_MSG_SIZE)){
                return -EFAULT;
            }
            printk(KERN_ALERT "getting dev_msg: %s", dev_msg);
            break;
        case WR_DEV_MSG:
            tmp = kmalloc(DEV_MSG_SIZE, GFP_KERNEL);
            if(copy_from_user(tmp, (char *)arg,  DEV_MSG_SIZE)){
                return -EFAULT;
            }
            
            if(copy_to_user((char *)arg, dev_msg, DEV_MSG_SIZE)){
                return -EFAULT;
            }
            strcpy(dev_msg, tmp);
            printk(KERN_ALERT "dev_msg after _IOWR operation: %s\n", dev_msg);
            kfree(tmp);
            break;
        default:
            return -ENOTTY;
    }
    return retval;
}

static int onebyte_init(void)
{
int result;
// register the device
result = register_chrdev(MAJOR_NUMBER, "onebyte",
&onebyte_fops);
if (result < 0) {
return result;
}
// allocate one byte of memory for storage
// kmalloc is just like malloc, the second parameter is
// the type of memory to be allocated.
// To release the memory allocated by kmalloc, use kfree.
onebyte_data = kmalloc(DEVSIZE, GFP_KERNEL);
if (!onebyte_data) {
onebyte_exit();
// cannot allocate memory
// return no memory error, negative signify a failure
return -ENOMEM;
}

//initializing and allocating space to dev_msg
dev_msg = kmalloc(DEV_MSG_SIZE, GFP_KERNEL);
if(!dev_msg){
  onebyte_exit();
  return -ENOMEM;
}
// initialize the value to be X
*onebyte_data = 'X';
printk(KERN_ALERT "This is a onebyte device module\n");
return 0;
}


static void onebyte_exit(void)
{
// if the pointer is pointing to something
if (onebyte_data) {
// free the memory and assign the pointer to NULL
kfree(onebyte_data);
onebyte_data = NULL;
}
// unregister the device
unregister_chrdev(MAJOR_NUMBER, "onebyte");
printk(KERN_ALERT "Onebyte device module is unloaded\n");
}
MODULE_LICENSE("GPL");
module_init(onebyte_init);
module_exit(onebyte_exit);
