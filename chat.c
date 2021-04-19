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
#define MAX_ROOMS_POSSIBLE 256

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

typedef struct my_private_data {
    struct Message_Node *messages_buff;
    unsigned int last_read_index;
} *MyPrivateData;



// this is actually a node of the list
struct Message_Node {
    struct Message_Node *prev;
    struct Message_Node *next;
    struct message_t *message_pointer;
};


// each minor is a chatroom
// this is also the header of the messages linked list
struct Chat_Room {
    unsigned int minor_id;  // the room's ID
    unsigned int participants_number;

    // linked list of messages
    struct Message_Node *messages_node;
    struct Message_Node *head_message;
    struct Message_Node *tail_message;

    struct Message_Node *current_message;  // like iterator
};


// taking advantage of the fact minors are in the range of [0-255]
Chat_Room chat_rooms[MAX_ROOMS_POSSIBLE];

/* globals */
int my_major = 0; /* will hold the major # of my device driver */



int init_module(void) {
    my_major = register_chrdev(my_major, MY_DEVICE, &my_fops);
    // from "my_module.c" hw0
    if (my_major < 0)
    {
        printk(KERN_WARNING
        "can't get dynamic major\n");
        return my_major;
    }
    // null pointer, initialization done on open
    chat_room = NULL;
    printk("Device driver registered - called from insmod\n");

    return 0;
}


void cleanup_module(void) {
    unregister_chrdev(my_major, MY_DEVICE);
    // before leaving free list
    for (i = 0; i < MAX_ROOMS_POSSIBLE; i++)
    {
        if (chat_rooms[i]->minor_id != NULL)
        {
            struct Message_Node *iter = chat_rooms[i]->messages_node->head_message;

            // free all messages
            while (iter != NULL)
            {
                iter = iter->next;
                kfree(iter->message_pointer);  //TODO: need &(iter->message_pointer)?
                kfree(iter->prev);

            }
            // free the list
            kfree(chat_rooms[i]->messages_node)
            // assign room as free
            chat_rooms[i] = NULL
        }
        // release chat rooms list
        kfree(chat_rooms)
    }
    // all allocations been released.
    return;
}

// if room with minor# exist - enter, else- create it
int my_open(struct inode *inode, struct file *filp) {
    // check if this minor number is already exist.
    unsigned int minor = MINOR(inode->i_rdev);

    //unsigned int minor_found = 0;
    //unsigned int i = 0;    // scan list of existing rooms to see if new room is needed
    //unsigned int first_empty_index = 0;    // scan list of existing rooms to see if new room is needed


    // means room exists
    if (chat_rooms[minor] != NULL)
    {
        chat_rooms[minor]->participants_number++;
    }


        // if the room doesn't exist
    else if (chat_rooms[minor] == NULL)
    {
        // if not - create room\chat\messages list
        //new chatroom element
        Chat_Room new_chat_room = (Chat_Room *) kmalloc(sizeof(struct Chat_Room), GFP_KERNEL);
        // kmalloc validation
        if (new_chat_room == NULL)
        {
            return -EFAULT;
        }

        new_chat_room->participants_number = 1;
        new_chat_room->minor_id = minor;

        // create a messages list object
        new_chat_room->messages_node = (Message_Node *) kmalloc(sizeof(struct Message_Node),
                                                                GFP_KERNEL);  // TODO: make sure it's the right casting
        // kmalloc validation
        if (new_chat_room->messages_node == NULL)
        {
            return -ENOMEM;
        }

        // create a messages pointer
        new_chat_room->messages_node->message_pointer = (message_t *) kmalloc(sizeof(struct message_t),
                                                                              GFP_KERNEL);  // TODO: make sure it's the right casting
        // kmalloc validation
        if (new_chat_room->messages_node->message_pointer == NULL)
        {
            return -ENOMEM;
        }

        new_chat_room->current_message = new_chat_room->head_message;
        new_chat_room->tail_message = new_chat_room->head_message;
        messages_node->message_pointer = new_chat_room->head_message;  //  pointer to first message


        // kmalloc validation
        if (new_chat_room->current_message == NULL)
        {
            return -ENOMEM;
        }

        // all kmallocs are successful
        chat_rooms[minor] = new_chat_room;

    }


    return 0;
}


// when one participant only leaves a room
int my_release(struct inode *inode, struct file *filp) {

    unsigned int minor = MINOR(inode->i_rdev);

    // handle file closing

    // scan minor list for the minor number we would like to close


    if (chat_rooms[minor]->minor_id.participants_number > 1)
    {  // leaver isn't the last one
        // else if there still others in the room - participants-=1
        chat_rooms[minor]->minor_id.participants_number -= 1;
    }
    else
    {  // if it has only 1 participant - release it like linked list element and kfree(iter and private data)

        struct Message_Node *iter = chat_rooms[minor]->messages_node->head_message;

        // free all messages
        while (iter != NULL)
        {
            iter = iter->next;
            kfree(iter->message_pointer);  //TODO: need &(iter->message_pointer)?
            kfree(iter->prev);

        }
        // free the list
        kfree(chat_rooms[minor]->messages_node)
        // assign room as free
        chat_rooms[minor] = NULL
    }
    return 0;
}


// file *pointer* has a property of "private data" to indicate if it's open, and where we are on the file
ssize_t my_read(struct file *filp, char *buf, size_t count, loff_t *f_pos) {
    //  our buff is always pointer to message_t
    (struct *message_t)(buf)
    //
    // Do read operation.

    // validate legal message

    // create new message object

    // add null char at the end of the buff

    // place it at the messages list
    // change new_chat_room->tail_message


    // Return number of bytes read.
    return 0;
}


// TODO
/**
 * creates message_t obj and push it to buffer
 * @param filp
 * @param buf
 * @param count
 * @param f_pos
 * @return
 */
ssize_t my_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
    // validate legal message

    // create new message object

    // add null char at the end of the buff

    // place it at the messages list
    // change new_chat_room->tail_message


    unsigned int written = 0, odd = *f_pos & 1;


    if (use_mem)
    {
        while (written < count)
            writeb(0xff * ((++written + odd) & 1), address);
    }
    else
    {
        while (written < count)
            outb(0xff * ((++written + odd) & 1), address);
    }
    *f_pos += count;
    return written;


}


int my_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg) {
    // get the chat_room number from the inode
    unsigned int minor = MINOR(inode->i_rdev);

    switch (cmd)
    {


        case COUNT_UNREAD:
            //
            // scans for the end of the list
            struct Message_Node *iter = chat_rooms[minor]->current_message;
            int steps = 0;
            while (iter != NULL)
            {
                if (iter->next == NULL)
                {
                    return steps;
                }
                steps++;
                iter = iter->next;
            }
            break;

        case SEARCH:

            // scans the messages_list[i]->message_t.sender==arg
            struct Message_Node *iter = chat_rooms[minor]->current_message;
            int steps = 0;
            while (iter != NULL)
            {
                if (iter->message_pointer->pid == arg)
                {
                    return steps;
                }
                steps++;
                iter = iter->next;
            }
            break;

        default:
            return -ENOTTY;
    }

    return 0;
}

/**
 * @param chat_room
 * @param offset in bytes
 * @param type - command (SET, CUR, END)
 * @return for correct command- returns the offset in bytes from the beginning of the file
 */
loff_t my_llseek(struct file *filp, loff_t offset, int type) { //TODO: can we access chat_room and not filp?
//loff_t my_llseek(struct Chat_Room *chat_room, loff_t offset, int type) {
    //
    // Change f_pos field in filp according to offset and type.
    //
    int minor = MINOR(filp->f_dentry->d_inode->i_rdev);
    struct Chat_Room chat_room[minor];  // TODO is this a proper type? by ref?
    int steps = offset / sizeof(struct message_t);

    // assure valid file pointer
    if (flip != NULL);
    switch (type)
    {
        // move Chat_Room.head_message+ offset steps
        case SEEK_SET:
            //

            struct Message_Node *iter = chat_room->head_message;
            int step_count = 0;

            if (steps > 0)
            {
                for (i = 0; i < abs(steps); i++)
                {
                    iter = iter->next;
                    step_count++;
                    if (iter == NULL)
                    { // going out of boundaries - end side
                        chat_room->current_message = iter;
                        break;
                    }
                }
            }
            else if (steps < 0)
            {
                chat_room->current_message = chat_room->head_message;
            }



            // position in file in bytes
            loff_t file_pos = step_count * sizeof(struct message_t);
            filp->f_pos = file_pos;  // TODO is casting right?
            return file_pos;
            //

            // move Chat_Room.current_message+ offset steps
        case SEEK_CUR:
            //
            int step_count = 0;
            struct Message_Node *iter = chat_room->current_message;
            for (i = 0; i < abs(steps); i++)
            {
                if (steps > 0)
                {
                    iter = iter->next;
                    step_count++;
                    if (iter == NULL)
                    { // going out of boundaries - end side
                        chat_room->current_message = iter;
                        break;
                    }
                    else
                    {
                        chat_room->current_message = iter;
                    }
                }
                else if (steps < 0)
                {
                    iter = iter->prev;
                    step_count--;

                    if (iter == chat_room->head_message)
                    { // going out of boundaries - beginning side
                        chat_room->current_message = chat_room->head_message;
                        break;
                    }
                    chat_room->current_message = iter;
                }

            }
            // position in file in bytes
            loff_t file_pos = step_count * sizeof(struct message_t);
            filp->f_pos = file_pos;  // TODO is casting right?
            return file_pos;



            // move Chat_Room.tail_message+ offset steps //TODO note that it will most likely be a negative number
        case SEEK_END:
            //
            int step_count = 0;
            //go to end, then step back
            struct Message_Node *iter = chat_room->current_message;
            while (iter != NULL)
            {
                iter = iter->next;
                step_count++;
            }
            if (steps < 0)
            {

                for (i = 0; i < abs(steps); i++)
                {
                    iter = iter->prev;
                    step_count--;
                    if (iter == chat_room->head_message)
                    { // going out of boundaries - beginning side
                        chat_room->current_message = chat_room->head_message;
                        break;
                    }
                    chat_room->current_message = iter;
                }
            }
            else if (steps > 0)
            {
                chat_room->current_message = chat_room->tail_message->next;
            }

            // position in file in bytes
            loff_t file_pos = step_count * sizeof(struct message_t);
            filp->f_pos = file_pos;  // TODO is casting right?
            return file_pos;


        default:
            return -EINVAL;
    }
}


time_t gettime() {
    do_gettimeofday(&tv);
    return tv.tv_sec;
}

pid_t getpid() {
    return current->pid;
}
