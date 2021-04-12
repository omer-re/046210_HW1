/* chat.c: Example char device module.
 *
 */
/* Kernel Programming */
#define MODULE
#define LINUX
#define __KERNEL__

#include <linux/kernel.h>  	
#include <linux/module.h>
#include <linux/time.h>
#include <linux/fs.h>       		
#include <asm/uaccess.h>
#include <linux/errno.h>  
#include <asm/segment.h>
#include <asm/current.h>

#include "chat.h"

#define MY_DEVICE "chat"

MODULE_AUTHOR("Anonymous");
MODULE_LICENSE("GPL");

/* globals */
int my_major = 0; /* will hold the major # of my device driver */
struct timeval tv; /* Used to get the current time */

struct file_operations my_fops = {
    .open = my_open,
    .release = my_release,
    .read = my_read,
    .ioctl = my_ioctl,
    .llseek = my_llseek
};

typedef struct message_t *MyPrivateData;

// each minor is a chatroom
typedef struct minor_list{
    int minor;  // in range of [0-255]
    int participants_number;

    //MyPrivateData private_data;
    struct message_t* message_p;

    struct minor_list* next;
    struct minor_list* prev;
}*Chat_room;


/* globals */
int my_major = 0; /* will hold the major # of my device driver */
Chat_room chat_room;



int init_module(void)
{
    my_major = register_chrdev(my_major, MY_DEVICE, &my_fops);
    // from "my_module.c" hw0
    if (my_major < 0)
    {
        printk(KERN_WARNING "can't get dynamic major\n");
        return my_major;
    }
    // null pointer, initialization done on open
    chat_room=NULL;
    printk("Device driver registered - called from insmod\n");

    return 0;
}


void cleanup_module(void)
{
    unregister_chrdev(my_major, MY_DEVICE);
    // before leaving free list
    while(messages_list.message_p!=NULL){
        while(messages_list.next!=NULL){
            kfree(messages_list->message_p->message);
            messages_list.message_p=messages_list.next;
            kfree(messages_list.message_p.prev);
        }

    }

    // all allocations been released.
    return;
}

// if room with minor# exist - enter, else- create it
int my_open(struct inode *inode, struct file *filp)
{
    // check if this minor number is already exist.
    unsigned int minor = MINOR(inode->i_rdev);
    //add minor to the list:
    Chat_room iter = chat_room;
    // scan list of existing rooms to see if new room is needed
    while(iter != NULL)
    {
        // means end of list and no match
        if(iter->next == NULL)
        {
            break;
        }

        if(iter->minor == minor)
        {
            // minor already has an open file so we use it's private data with buffer
            iter->participants_number+=1;

            // TODO: what should be in our private_data? message_t? seek pointer? both?
            filp->private_data = iter->private_data;
            return 0;
        }


        iter = iter->next;
    }
    // if it does - join the room


    // if not - create room\chat\messages list
    //new chatroom element
    Chat_room chat_r = (MyPrivateData)kmalloc(sizeof(struct message_t),GFP_KERNEL);
    if(chat_r == NULL) {
        return -ENOMEM;
    }

    mpd->points = 0;
    filp->private_data = mpd;

    //new chatroom element
    MinorList new_minor = (MinorList)kmalloc(sizeof(struct minor_list),GFP_KERNEL);
    // verify allocation
    if(new_minor == NULL) {
        return -EFAULT;
    }
    new_minor->private_data = mpd;
    new_minor->minor = minor;
    // initializing list's first node
    chat_room.prev=NULL;
    chat_room.next=NULL;
    chat_room.message_p=NULL;
    chat_room->participants_number=1;


    if(iter == NULL)
    {
        m_list = new_minor;
    }
    else
    {
        iter->next = new_minor;
    }

    // handle open

    return 0;
}

// when one participant only leaves a room
int my_release(struct inode *inode, struct file *filp)
{
    // handle file closing

    // scan minor list for the minor number we would like to close
    // if it exist
    // if it has only 1 participant - release it like linked list element and kfree(iter and private data)
    // else if there still others in the room - participants-=1

    return 0;
}

// file *pointer* has a property of "private data" to indicate if it's open, and where we are on the file
ssize_t my_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
    //  our buff is always pointer to message_t
    (struct *message_t)(buf)
    //
    // Do read operation.



    // Return number of bytes read.
    return 0; 
}

int my_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
    switch(cmd)
    {
    case COUNT_UNREAD:
	//
	// handle 
	//
	break;
    case SEARCH:
	//
	// handle 
	//
	break;

    default:
	return -ENOTTY;
    }

    return 0;
}

loff_t my_llseek(struct file *filp, loff_t offset, int type)
{
    //
    // Change f_pos field in filp according to offset and type.
    //
    return -EINVAL;
}

time_t gettime() {
    do_gettimeofday(&tv);
    return tv.tv_sec;
}

pid_t getpid() {
    return current->pid;
}
