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

//typedef struct my_private_data {
//    struct Message_Node *messages_buff;
//    unsigned int last_read_index;
//} *MyPrivateData;
//


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

    return 0;
}


void cleanup_module(void) {
    unregister_chrdev(my_major, MY_DEVICE);
    // before leaving free list
    int i = 0;
    for (i = 0; i < MAX_ROOMS_POSSIBLE; i++)
    {

        struct Message_Node *iter = chat_rooms[i].head_message;

        // free all messages
        while (iter != NULL)
        {
            iter = iter->next;
            kfree(iter->message_pointer);  //TODO: need &(iter->message_pointer)?
            kfree(iter->prev);

        }
        // free the list
        kfree(chat_rooms[i].head_message);
        // assign room as free
        chat_rooms[i].participants_number = 0;



        // release chat rooms list
        //kfree(chat_rooms);
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
    if (chat_rooms[minor].participants_number != 0)
    {
        chat_rooms[minor].participants_number++;
    }


        // if the room doesn't exist
    else if (chat_rooms[minor].participants_number == 0)
    {
        // if not - create room\chat\messages list

        //new chatroom element
        //Chat_Room new_chat_room = (Chat_Room *) kmalloc(sizeof(struct Chat_Room), GFP_KERNEL);
        struct Chat_Room new_chat_room = chat_rooms[minor];

        new_chat_room.participants_number = 1;
        new_chat_room.minor_id = minor;

        // create a messages list object
        new_chat_room.head_message = (struct Message_Node *) kmalloc(sizeof(struct Message_Node),
                                                                     GFP_KERNEL);  // TODO: make sure it's the right casting
        // kmalloc validation
        if (new_chat_room.head_message == NULL)
        {
            return -ENOMEM;
        }

        // create a messages pointer
        new_chat_room.head_message->message_pointer = (struct message_t *) kmalloc(sizeof(struct message_t),
                                                                                   GFP_KERNEL);  // TODO: make sure it's the right casting
        // kmalloc validation
        if (new_chat_room.head_message->message_pointer == NULL)
        {
            return -ENOMEM;
        }

        new_chat_room.tail_message = new_chat_room.head_message;

        // all kmallocs are successful
        chat_rooms[minor] = new_chat_room;

    }


    return 0;
}


// when one participant only leaves a room
int my_release(struct inode *inode, struct file *filp) {

    unsigned int minor = MINOR(inode->i_rdev);
    // handle file closing
    if (chat_rooms[minor].participants_number > 1)
    {  // leaver isn't the last one
        // else if there still others in the room - participants-=1
        chat_rooms[minor].participants_number -= 1;
    }
    else
    {  // if it has only 1 participant - release it like linked list element and kfree(iter and private data)

        struct Message_Node *iter = chat_rooms[minor].head_message;

        // free all messages
        while (iter != NULL)
        {
            iter = iter->next;
            kfree(iter->message_pointer);  //TODO: need &(iter->message_pointer)?
            kfree(iter->prev);

        }
        // free the list
        kfree(chat_rooms[minor].head_message);
        // assign room as free
        chat_rooms[minor].participants_number = 0;
    }
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
    //  our buff is always pointer to message_t

    // go to the right chat room
    // assuming room number is fine and room exists from my_open
    int minor = MINOR(filp->f_dentry->d_inode->i_rdev); // which is the chat_room

    // go to the last read message according to f_pos by bytes
    struct Message_Node *iter = chat_rooms[minor].head_message;
    int steps = 0;
    while (iter != NULL)
    {

        steps++;
        iter = iter->next;
    }

    int num_read_messages = (*f_pos / sizeof(struct message_t));
    // check how many unread messages are there
    size_t num_unread_messages = steps - num_read_messages;
    // check how many messages fit into count
    size_t num_wanted_messages = (count / sizeof(struct message_t));

    int diff = num_wanted_messages - num_unread_messages; // diff is number of meesage_t


    if (diff < 0) diff = num_wanted_messages;  //we can provide that number of messages
    if (diff > 0) diff = num_unread_messages;  // not enough messages as required, read all unread

    size_t diff_bytes = diff * sizeof(struct message_t);

    // copy to user
    if (copy_to_user((void *) buf, &f_pos, diff_bytes))  // return 0 is error
    {
        printk("Read: Failed to write map to user space.\n");
        return -EBADF;
    }
    // update f_pos
    // TODO: make sure that is how updating f_pos is done, it's also loff_t+size_t types
    f_pos = f_pos + diff_bytes;

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

    unsigned int msg_len = strnlen_user(buf, MAX_MESSAGE_LENGTH);

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

    // DONE

    return msg_len;  // number of bytes written


}

// TODO: need to fix! count should be done from f_pos, not from current message!
int my_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg) {
    // get the chat_room number from the inode
    unsigned int minor = MINOR(inode->i_rdev);
    loff_t curr_fpos = filp->f_pos;

    if (cmd == COUNT_UNREAD)
    {

        //
        // scans for the end of the list
        // TODO: use pointer arithmetics to start from head+f_pos
        struct Message_Node *iter = *((&(chat_rooms[minor].head_message)) +
                                      curr_fpos);  // TODO: verify proper use of pointers
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
    }

    else if (cmd == SEARCH)
    {

        // scans the messages_list[i]->message_t.sender==arg
        struct Message_Node *iter = *((&(chat_rooms[minor].head_message)) + curr_fpos);;
        size_t num_read_messages = (curr_fpos / sizeof(struct message_t));

        int steps = 0;
        while (iter != NULL)
        {
            if (iter->message_pointer->pid == arg)
            {
                return steps + num_read_messages;
            }
            steps++;
            iter = iter->next;
        }
        return -ENOENT;
    }
    else
        return -ENOTTY;


    return 0;
}

/**
 * @param chat_room
 * @param offset in bytes
 * @param type - command (SET, CUR, END)
 * @return for correct command- returns the offset in bytes from the beginning of the file
 */
loff_t my_llseek(struct file *filp, loff_t offset, int type) {
    //
    // Change f_pos field in filp according to offset and type.
    //

    loff_t curr_fpos = filp->f_pos;


    int minor = MINOR(filp->f_dentry->d_inode->i_rdev);
    int steps = offset / sizeof(struct message_t);

    struct Message_Node *iter;

    // assure valid file pointer
    if (filp != NULL);

        // move Chat_Room.head_message+ offset steps
    if (type == SEEK_SET)
    {

        iter = chat_rooms[minor].head_message;
            int step_count = 0;

            if (steps > 0)
            {
                int i;
                for (i = 0; i < abs(steps); i++)
                {
                    iter = iter->next;
                    step_count++;
                    if (iter == NULL)
                    { // going out of boundaries - end side
                        curr_fpos = step_count * sizeof(struct message_t);
//                        chat_room->current_message = iter;
                        break;
                    }
                }
            }
            else if (steps < 0)
            {
                curr_fpos = 0;
//                chat_room->current_message = chat_room->head_message;
            }



            // position in file in bytes
            filp->f_pos = curr_fpos;  // TODO is casting right?
            return curr_fpos;
            //
    }
            // move Chat_Room.current_message+ offset steps
    else if (type == SEEK_CUR)
    {

            int step_count = 0;
        //int current_message = curr_fpos / sizeof(struct message_t);

            // get to current message
        iter = *((&(chat_rooms[minor].head_message)) + curr_fpos);

        int i = 0;
            for (i = 0; i < abs(steps); i++)
            {
                if (steps > 0)
                {
                    iter = iter->next;
                    step_count++;
                    if (iter == NULL)
                    { // going out of boundaries - end side
                        //current_message = iter;
                        curr_fpos = curr_fpos + sizeof(struct message_t);
                        break;
                    }
                    else
                    {
                        curr_fpos = curr_fpos + sizeof(struct message_t);

                    }
                }
                else if (steps < 0)
                {
                    iter = iter->prev;
                    step_count--;

                    if (iter == chat_rooms[minor].head_message)
                    { // going out of boundaries - beginning side
                        curr_fpos = 0;
//                        chat_room->current_message = chat_room->head_message;
                        break;
                    }
                    curr_fpos = curr_fpos - sizeof(struct message_t);
                }

            }
            // position in file in bytes
//            loff_t file_pos = step_count * sizeof(struct message_t);
            filp->f_pos = curr_fpos;
        return filp->f_pos;
    }


            // move Chat_Room.tail_message+ offset steps //TODO note that it will most likely be a negative number
    else if (type == SEEK_END)
    {
            //
            int step_count = 0;
            //go to end, then step back
        iter = chat_rooms[minor].tail_message;
//            while (iter != NULL)
//            {
//                iter = iter->next;
//                step_count++;
//            }
            if (steps < 0)
            {
                int i = 0;
                for (i = 0; i < abs(steps); i++)
                {
                    iter = iter->prev;
                    step_count--;
                    if (iter == chat_rooms[minor].head_message)
                    { // going out of boundaries - beginning side
//                        chat_room->current_message = chat_room->head_message;
                        curr_fpos = 0;
                        break;
                    }
                    curr_fpos = curr_fpos - sizeof(struct message_t);
                }
            }
            else if (steps > 0)
            {
                curr_fpos = curr_fpos + sizeof(struct message_t);
            }

            // position in file in bytes
            filp->f_pos = curr_fpos;
            return curr_fpos;
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
