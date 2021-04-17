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


// TODO: linked list of message_t
// this is actually a node of the list
struct Messages_List {
    struct Messages_List *prev;
    struct Messages_List *next;
    struct message_t *message_pointer;
};


// each minor is a chatroom
// this is also the header of the messages linked list
struct Chat_Room {
    unsigned int minor_id;  // the room's ID
    unsigned int participants_number;

    // linked list of messages
    struct Messages_List *messages_list;
    struct message_t *head_message;
    struct message_t *tail_message;

    struct message_t *current_message;  // like iterator
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
        if (chat_rooms[i]->minor_id == NULL)
        {
            struct Messages_List *iter = chat_rooms[i]->messages_list->head_message;

            // free all messages
            while (iter != NULL)
            {
                iter = iter->next;
                kfree(iter->message_pointer);  //TODO: need &(iter->message_pointer)?
                kfree(iter->prev);

            }
            // free the list
            kfree(chat_rooms[i]->messages_list)
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
    unsigned int minor_found = 0;
    unsigned int i = 0;    // scan list of existing rooms to see if new room is needed
    unsigned int first_empty_index = 0;    // scan list of existing rooms to see if new room is needed

    for (i = 0; i < MAX_ROOMS_POSSIBLE; i++)
    {

        // save the first available minor that's free
        if (chat_rooms[i]->minor_id == NULL)
        {  // first empty index
            first_empty_index = i;
            return 0;

        }

    }

    // means end of list and no match
    if (i == 255 && minor_found != 1)
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
        new_chat_room->messages_list = (Messages_List *) kmalloc(sizeof(struct Messages_List),
                                                                 GFP_KERNEL);  // TODO: make sure it's the right casting
        // kmalloc validation
        if (new_chat_room->messages_list == NULL)
        {
            return -ENOMEM;
        }

        // create a messages pointer
        new_chat_room->messages_list->message_pointer = (message_t *) kmalloc(sizeof(struct message_t),
                                                                              GFP_KERNEL);  // TODO: make sure it's the right casting
        // kmalloc validation
        if (new_chat_room->messages_list->message_pointer == NULL)
        {
            return -ENOMEM;
        }

        new_chat_room->current_message = new_chat_room->head_message;
        new_chat_room->tail_message = new_chat_room->head_message;
        messages_list->message_pointer = new_chat_room->head_message;  //  pointer to first message


        // kmalloc validation
        if (new_chat_room->current_message == NULL)
        {
            return -ENOMEM;
        }

        // all kmallocs are successful
        chat_rooms[first_empty_index] = new_chat_room;

    }

    return 0;
}


// when one participant only leaves a room
int my_release(struct inode *inode, struct file *filp) {
    // handle file closing

    // scan minor list for the minor number we would like to close
    for (i = 0; i < MAX_ROOMS_POSSIBLE; i++)
    {
        if (chat_rooms[i]->minor_id == minor)
        {  // if it does - join the room
            if (chat_rooms[i]->minor_id.participants_number > 1)
            {  // leaver isn't the last one
                // else if there still others in the room - participants-=1
                chat_rooms[i]->minor_id.participants_number -= 1;
            }
            else
            {  // if it has only 1 participant - release it like linked list element and kfree(iter and private data)

                struct Messages_List *iter = chat_rooms[i]->messages_list->head_message;

                // free all messages
                while (iter != NULL)
                {
                    iter = iter->next;
                    kfree(iter->message_pointer);  //TODO: need &(iter->message_pointer)?
                    kfree(iter->prev);

                }
                // free the list
                kfree(chat_rooms[i]->messages_list)
                // assign room as free
                chat_rooms[i] = NULL
            }

        }
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

int my_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg) {
    switch (cmd)
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


//loff_t my_llseek(struct file *filp, loff_t offset, int type) { //TODO: can we access chat_room and not filp?
loff_t my_llseek(struct Chat_Room *chat_room, loff_t offset, int type) {
    //
    // Change f_pos field in filp according to offset and type.
    //

    int steps = offset / sizeof(struct message_t);

    // assure valid file pointer
    if (flip != NULL);
    switch (type)
    {
        // move Chat_Room.head_message+ offset steps
        case SEEK_SET:
            //

            struct Messages_List *iter = chat_room->head_message;
            for (i = 0; i < abs(steps); i++)
            {


                if (steps > 0)
                {
                    iter = iter->next;

                    if (iter == NULL)
                    { // going out of boundaries - end side
                        chat_room->current_message = iter;
                        return -EINVAL
                    }
                }
                else
                {
                    printf("Negative offset on SEEK_SET"); //todo: remove before handing
                }

            }
            //
            break;

            // move Chat_Room.current_message+ offset steps
        case SEEK_CUR:
            //
            struct Messages_List *iter = chat_room->current_message;
            for (i = 0; i < abs(steps); i++)
            {
                if (steps > 0)
                {
                    iter = iter->next;

                    if (iter == NULL)
                    { // going out of boundaries - end side
                        chat_room->current_message = iter;
                        return -EINVAL
                    }
                }
                else if (steps < 0)
                {
                    iter = iter->prev;

                    if (iter == chat_room->head_message)
                    { // going out of boundaries - beginning side
                        chat_room->current_message = chat_room->head_message;
                        return -EINVAL
                    }
                }
            }            //
            break;

            // move Chat_Room.tail_message+ offset steps //TODO note that it will most likely be a negative number
        case SEEK_END:
            //

            //go to end, then step back
            struct Messages_List *iter = chat_room->current_message;
            while (iter != NULL)
            {
                iter = iter->next;
            }
            for (i = 0; i < abs(steps); i++)
            {

                if (steps < 0)
                {
                    iter = iter->prev;

                    if (iter == chat_room->head_message)
                    { // going out of boundaries - beginning side
                        chat_room->current_message = chat_room->head_message;
                        return -EINVAL
                    }
                }
                else
                {
                    printf("Positive offset on SEEK_END"); //todo: remove before handing
                }
            }
            //
            break;

        default:
            return -ENOTTY;
    }

    return -EINVAL;
}

time_t gettime() {
    do_gettimeofday(&tv);
    return tv.tv_sec;
}

pid_t getpid() {
    return current->pid;
}
