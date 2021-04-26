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
#include <linux/slab.h>
#include <linux/limits.h>
#include <stdbool.h>

#include "chat.h"

#define MY_DEVICE "chat"

#define DEBUGEH



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

int init_module(void) {
    my_major = register_chrdev(my_major, MY_DEVICE, &my_fops);
    // from "my_module.c" hw0
    if (my_major < 0)
    {
        printk(KERN_WARNING
        "can't get dynamic major\n");
        return my_major;
    }
    int i = 0;
    for (i = 0; i < MAX_ROOMS_POSSIBLE; i++)
    {
        chat_rooms[i].minor_id = i;
        chat_rooms[i].participants_number = 0;
        chat_rooms[i].head_message = NULL;
        chat_rooms[i].tail_message = NULL;

    }
    printk("Device driver registered - called from insmod\n");
#ifdef DEBUGEH
    printk("\nDEBUGEH: initialzing module\n");
#endif
    return 0;
}


void cleanup_module(void) {

#ifdef DEBUGEH
    printk("\nDEBUGEH: Cleanup\n");
#endif
    unregister_chrdev(my_major, MY_DEVICE);
    // before leaving free list
    int i = 0;
    for (i = 0; i < MAX_ROOMS_POSSIBLE; i++)
    {
        struct Message_Node *iter_current = chat_rooms[i].head_message;
        struct Message_Node *iter_next = iter_current->next;

        if (iter_current != NULL)
        {
            if (iter_next != NULL)
            {
                // free all messages
                while (iter_next != NULL)  // means we have at least 2 nodes alive
                {
                    kfree(iter_current->message_pointer);
                    iter_current = iter_next;
                    iter_next = iter_current->next;
                }
            }
            kfree(iter_current);
        }
        // assign room as free
        chat_rooms[i].participants_number = 0;



        // release chat rooms list
        //kfree(chat_rooms);
    }
    // all allocations been released.

#ifdef DEBUGEH
    printk("\nDEBUGEH: Cleanup done\n");
#endif
    return;
}

// if room with minor# exist - enter, else- create it
int my_open(struct inode *inode, struct file *filp) {
    // check if this minor number is already exist.
    unsigned int minor = MINOR(inode->i_rdev);
#ifdef DEBUGEH
    printk("\nDEBUGEH: my_open, minor %d, participants num: %d\n", minor, chat_rooms[minor].participants_number);
#endif

    // means room exists
    if (chat_rooms[minor].participants_number != 0)
    {
        chat_rooms[minor].participants_number++;
    }


        // if the room doesn't exist
    else if (chat_rooms[minor].participants_number == 0)
    {
        // if not - create room\chat\messages list
        chat_rooms[minor].participants_number = 1;
        chat_rooms[minor].minor_id = minor;

        // create a messages list object
        chat_rooms[minor].head_message = (struct Message_Node *) kmalloc(sizeof(struct Message_Node),
                                                                         GFP_KERNEL);  // TODO: make sure it's the right casting
        // kmalloc validation
        if (chat_rooms[minor].head_message == NULL)
        {
            return -ENOMEM;
        }

        // create a messages pointer
        chat_rooms[minor].head_message->message_pointer = (struct message_t *) kmalloc(sizeof(struct message_t),
                                                                                       GFP_KERNEL);  // TODO: make sure it's the right casting
        // kmalloc validation
        if (chat_rooms[minor].head_message->message_pointer == NULL)
        {
            return -ENOMEM;
        }

        chat_rooms[minor].tail_message = chat_rooms[minor].head_message;

        // all kmallocs are successful
        //chat_rooms[minor] = chat_rooms[minor];
#ifdef DEBUGEH
        printk("\nDEBUGEH: my_open, is done\n");
#endif

    }


    return 0;
}


// when one participant only leaves a room
int my_release(struct inode *inode, struct file *filp) {
#ifdef DEBUGEH
    printk("\nDEBUGEH: my_release started\n");
#endif
    unsigned int minor = MINOR(inode->i_rdev);
#ifdef DEBUGEH
    printk("\nDEBUGEH: minor:");
    printk(minor);
    printk("\n");
#endif
    // handle file closing
#ifdef DEBUGEH
    printk("\nDEBUGEH: my_release 183\n");
#endif
    if (chat_rooms[minor].participants_number > 1)
    {  // leaver isn't the last one
#ifdef DEBUGEH
        printk("\nDEBUGEH: my_release 188 if\n");
#endif
        // else if there still others in the room - participants-=1
        chat_rooms[minor].participants_number -= 1;
    }
    else
    {  // if it has only 1 participant - release it like linked list element and kfree(iter and private data)
#ifdef DEBUGEH
        printk("\nDEBUGEH: my_release 196 else\n");
#endif

        /////  rolling in linked list   ////////
        struct Message_Node *iter_current = chat_rooms[minor].head_message;
        struct Message_Node *iter_next = iter_current->next;

        if (iter_current != NULL)
        {
            if (iter_next != NULL)
            {
                // free all messages
                while (iter_next != NULL)  // means we have at least 2 nodes alive
                {
                    kfree(iter_current->message_pointer);
                    iter_current = iter_next;
                    iter_next = iter_current->next;
                }
            }
            kfree(iter_current);
        }

        /////  rolling in linked list   ////////

        // assign room as free
        chat_rooms[minor].participants_number = 0;
    }
#ifdef DEBUGEH
    printk("\nDEBUGEH: done my_release 221 if\n");
#endif
    return 0;
}


// file *pointer* has a property of "private data" to indicate if it's open, and where we are on the file
/**
 *
 * @param filp
 * @param buf  - buffer for "copy to user"
 * @param count number of bytes we'd like to read
 * @param f_pos
 * @return
 **/
ssize_t my_read(struct file *filp, char *buf, size_t count, loff_t *f_pos) {

#ifdef DEBUGEH
    printk("\nDEBUGEH: my_read started\n");
#endif

    //  our buff is always pointer to message_t

    // go to the right chat room
    // assuming room number is fine and room exists from my_open
    int minor = MINOR(filp->f_dentry->d_inode->i_rdev); // which is the chat_room

    //int num_read_messages = ((long) *f_pos / sizeof(struct message_t));
    int num_read_messages = f_pos;
    // check how many unread messages are there
    int num_unread_messages = chat_rooms[minor].num_of_messages - num_read_messages;

    // check how many messages fit into count
    int num_wanted_messages = (count / sizeof(struct message_t));

    int diff = num_wanted_messages - num_unread_messages; // diff is number of meesage_t

    if (diff < 0) diff = num_wanted_messages;  //we can provide that number of messages
    if (diff >= 0) diff = num_unread_messages;  // not enough messages as required, read all unread

    int diff_bytes = diff * sizeof(struct message_t);

    // pointer to f_pos = starting point for copy_to_user
    int i = 0;
    struct Message_Node *iter_current = chat_rooms[minor].head_message;
    // promised to be in boundaries, therefore no need checking for NULL
    for (i = 0; i < num_read_messages; i++)
    {
        iter_current = iter_current->next;
    }

    // copy to user
    if (copy_to_user((void *) buf, &iter_current, diff_bytes))  // return 0 is error
    {
        printk("Read: Failed to write map to user space.\n");
        return -EBADF;
    }

    // update f_pos
    f_pos += diff;

#ifdef DEBUGEH
    printk("\nDEBUGEH: my_read is done\n");
#endif
    return diff_bytes;
}

/**
 * For each message, gets buffer, validates it, copy from user into messaget obj
 * After returning with 0 from this function the buffer was copied and it's in the message_t obj
 * @param pointer to new_message message_t obj which was already created
 * @param buffer- the string that is the message
 * @return 0 for proper handling;
 */
unsigned int StringHandler(struct message_t *new_message, const char *buffer) {

    // validate proper size
    unsigned int msg_len = strnlen_user(buffer, MAX_MESSAGE_LENGTH);

    if (msg_len == 0)
    {
        printk("problem with strnlen_user\n");

    }

    int required_buf = (msg_len); // number of chars needed
    if (required_buf > MAX_MESSAGE_LENGTH)
    {
        return -ENOSPC;
    }

    if (buffer == NULL)
    {
        return -EFAULT;
    }

    // validate allocation
    if (new_message->message == NULL)
    {
        return -EFAULT;
    }

    // copy the string message from the user space buffer to kernel's message_t.message[]
    if (copy_from_user(buffer, new_message->message, MAX_MESSAGE_LENGTH) != 0)
    {
        printk("write: Error on copying from user space.\n");
        return -EBADF;
    }

    // if message is shorter than max and has no '/0' - add it.
    if (msg_len < MAX_MESSAGE_LENGTH && buffer[msg_len - 1] != "\0")
    {  // needs to add it
        new_message->message[msg_len - 1] = "\0";

    }

    return 0;
}


/**
 * creates message_t obj and push it to buffer
 * @param filp
 * @param buf is in size of "count" bytes
 * @param count
 * @param f_pos
 * @return
 */
ssize_t my_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
#ifdef DEBUGEH
    printk("\nDEBUGEH: My_write started\n");
#endif
    unsigned int msg_len = strnlen_user(buf, MAX_MESSAGE_LENGTH);
    printk("write: Couldn't allocate mem for input string.\n");

    // create a messages pointer
    struct message_t *new_message = (struct message_t *) kmalloc(sizeof(struct message_t),
                                                                 GFP_KERNEL);  // TODO: make sure it's the right casting
    // kmalloc validation
    if (new_message == NULL)
    {
        printk("write: Couldn't allocate mem for input string.\n");
        return -EFAULT;
    }

    int str_hndlr_res = StringHandler(new_message, buf);
    if (str_hndlr_res != 0)
    {  // if failed, free new_message
        kfree(new_message);
        return str_hndlr_res; // let the original error to bubble up
    }

    int minor = MINOR(filp->f_dentry->d_inode->i_rdev); // which is the chat_room

    // TODO: who opens the room? where is "my_open" called?
    // if no where else- we need to create the room here.
    if (chat_rooms[minor].participants_number == 0)
    {
        printk("my_open to initiate room is missing!.\n");
    }

    new_message->pid = getpid();
    new_message->timestamp = gettime();

    // create a messages list object
    struct Message_Node *new_node = (struct Message_Node *) kmalloc(sizeof(struct Message_Node),
                                                                    GFP_KERNEL);  // TODO: make sure it's the right casting
    // kmalloc validation
    if (new_node == NULL)
    {
        return -ENOMEM;
    }

    // put new message in message node
    new_node->message_pointer = new_message;
    // append new message node to the messages list, and fix the pointers of the list.
    new_node->prev = chat_rooms[minor].tail_message;
    new_node->next = NULL;
    chat_rooms[minor].tail_message->next = new_node;
    chat_rooms[minor].num_of_messages++;


    // DONE

    return msg_len;  // number of bytes written


}

int my_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg) {
    // get the chat_room number from the inode
    unsigned int minor = MINOR(inode->i_rdev);
    //loff_t curr_fpos = filp->f_pos;
    int curr_fpos = filp->f_pos;  // current index of last read message

    if (cmd == COUNT_UNREAD)
    {
        int diff = chat_rooms[minor].num_of_messages - curr_fpos;
        return diff;
    }

    else if (cmd == SEARCH)
    {
        if (curr_fpos == chat_rooms[minor].num_of_messages)
        {  // end of the list, no further messages with that sender
            // ended with no result
            return -ENOENT;
        }
        /////  rolling in linked list   ////////
        struct Message_Node *iter_current = chat_rooms[minor].head_message;
        struct Message_Node *iter_next = iter_current->next;
        int steps = 0;

        if (iter_current != NULL)
        {
            if (iter_next != NULL)
            {
                while (iter_next != NULL)  // means we have at least 2 nodes alive
                {
                    if (iter_current->message_pointer->pid == arg)
                    {
                        if (steps > curr_fpos)  // found next message from sender
                        {
                            return (curr_fpos + steps) * sizeof(struct message_t);
                        }
                        // continue
                    }
                    steps++;
                    iter_current = iter_next;
                    iter_next = iter_current->next;
                }
                // if ended with no result
                return -ENOENT;
                /////  rolling in linked list   ////////
            }
        }
        else  // case wrong command
            return -ENOTTY;

    }
}

/**
 * F_POS IS THE INDEX OF THE MESSAGE IN THE LIST
 *
 * @param chat_room
 * @param offset in bytes
 * @param type - command (SET, CUR, END)
 * @return for correct command- returns the offset in bytes from the beginning of the file
 */
loff_t my_llseek(struct file *filp, loff_t offset, int type) {

    int minor = MINOR(filp->f_dentry->d_inode->i_rdev);

    //loff_t curr_fpos = filp->f_pos;
    loff_t curr_fpos = filp->f_pos;
    loff_t messages_offset =
            (long) offset / (long) sizeof(struct message_t);  // promised to be int, mult of sizeof(message_t)
    int destination_msg = curr_fpos + messages_offset;  // the index of the message we will arrive after offset

    // assure valid file pointer
    if (filp != NULL);

    // check list isn't empty
    if (chat_rooms[minor].num_of_messages == 0)
    {
        return 0;
    }
    // move Chat_Room.head_message+ offset steps
    ///// SEEK_SET  /////////
    if (type == SEEK_SET)
    {

        if (messages_offset <= 0)
        {
            curr_fpos = 0;
            return 0;
        }
        else if (destination_msg > chat_rooms[minor].num_of_messages)
        { // out of boundaries
            curr_fpos = chat_rooms[minor].num_of_messages + 1; // TODO: +1 even though there's no message there
            return (chat_rooms[minor].num_of_messages + 1) * sizeof(struct message_t);
        }

        else if (destination_msg > 0)
        {
            curr_fpos = destination_msg;
            return (destination_msg) * sizeof(struct message_t);

        }

    }

        // move Chat_Room.tail_message+ offset steps
        ///// SEEK_END  /////////
    else if (type == SEEK_END)
    {
        // case out of boundaries end side
        if (destination_msg >= chat_rooms[minor].num_of_messages)
        {
            curr_fpos = chat_rooms[minor].num_of_messages + 1; // TODO: +1 even though there's no message there
            return (chat_rooms[minor].num_of_messages + 1) * sizeof(struct message_t);
        }
            // case out of boundaries beginning side
        else if (destination_msg < 0)
        { // out of boundaries
            curr_fpos = 0;
            return 0;
        }

            // case in boundaries
        else if (destination_msg > 0)
        {
            curr_fpos = destination_msg;
            return (destination_msg) * sizeof(struct message_t);

        }

    }



        // move Chat_Room.current_message+ offset steps
        ///// SEEK_CUR /////////
    else if (type == SEEK_CUR)
    {


        if (destination_msg >= chat_rooms[minor].num_of_messages)
        {
            curr_fpos = chat_rooms[minor].num_of_messages + 1; // TODO: +1 even though there's no message there
            return (chat_rooms[minor].num_of_messages + 1) * sizeof(struct message_t);
        }

        else if (destination_msg < 0)
        { // out of boundaries
            curr_fpos = 0;
            return (chat_rooms[minor].num_of_messages + 1) * sizeof(struct message_t);
        }

        else if (messages_offset > 0)
        {
            curr_fpos = destination_msg;
            return (destination_msg) * sizeof(struct message_t);

        }
    }

    else
            return -EINVAL;


}


time_t gettime() {
    do_gettimeofday(&tv);
    return tv.tv_sec;
}

pid_t getpid() {
    return current->pid;
}