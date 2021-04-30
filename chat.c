//VERSION OF 30.4 16:14

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
        .write = my_write,
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
        chat_rooms[minor].head_message = (struct Message_Node *) kmalloc(sizeof(struct Message_Node), GFP_KERNEL);
        // kmalloc validation
        if (chat_rooms[minor].head_message == NULL)
        {
            return -ENOMEM;
        }

        // create a messages pointer
        chat_rooms[minor].head_message->message_pointer = (struct message_t *) kmalloc(sizeof(struct message_t),
                                                                                       GFP_KERNEL);
        // kmalloc validation
        if (chat_rooms[minor].head_message->message_pointer == NULL)
        {
            return -ENOMEM;
        }


#ifdef DEBUGEH
        printk("\nDEBUGEH: my_open, init f_pos value is %d\n", (int) (filp->f_pos));
#endif
        //chat_rooms[minor].tail_message = chat_rooms[minor].head_message;

        // all kmallocs are successful
#ifdef DEBUGEH
        printk("\nDEBUGEH: my_open, is done\n");
#endif

    }


    return 0;
}


// when one participant only leaves a room
// if room has 0 participants - reset room (remove messages list)
int my_release(struct inode *inode, struct file *filp) {
#ifdef DEBUGEH
    printk("\nDEBUGEH: my_release started\n");
#endif
    unsigned int minor = MINOR(inode->i_rdev);
#ifdef DEBUGEH
    printk("\nDEBUGEH: minor: %d\n", minor);
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
        printk("\nDEBUGEH: my_release 199 else\n");
#endif
        if (chat_rooms[minor].num_of_messages > 0)
        {
            /////  rolling in linked list   ////////
            struct Message_Node *iter_current = chat_rooms[minor].head_message;
            //struct Message_Node *iter_next = iter_current->next;

            if (iter_current != NULL)
            {
#ifdef DEBUGEH
                printk("\nDEBUGEH: my_release 210 if\n");
#endif


                // free all messages
                int m;
                for (m = 0; m < chat_rooms[minor].num_of_messages - 1; m++)
                    // means we have at least 2 nodes alive
                {
                    kfree(iter_current->message_pointer);
                    iter_current = iter_current->next;
                }
#ifdef DEBUGEH
                printk("\nDEBUGEH: my_release 224 done releasing in for\n");
#endif

                kfree(iter_current);
                chat_rooms[minor].num_of_messages = 0;
            }

            /////  rolling in linked list   ////////

            // assign room as free
            chat_rooms[minor].participants_number = 0;
        }
        else
        {
#ifdef DEBUGEH
            printk("\nDEBUGEH: my_release: No messages to release\n");
#endif
        }
    }
#ifdef DEBUGEH
    printk("\nDEBUGEH: done my_release 243\n");
#endif
    return 0;
}


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
    // message loop

    printk("\nDEBUGEH: my_read started empty user buff\n");
#endif
    //init buff
    int c = 0;
    for (c = 0; c < MAX_MESSAGE_LENGTH; c++)
    {
        buf[c] = '\0';
    }
#ifdef DEBUGEH
    printk("\nDEBUGEH: msg_len %d\n", count);
#endif

    //  our buff is always pointer to message_t

    // go to the right chat room
    // assuming room number is fine and room exists from my_open
    int minor = MINOR(filp->f_dentry->d_inode->i_rdev); // which is the chat_room

    int num_read_messages = *f_pos;
    // check how many unread messages are there
#ifdef DEBUGEH
    printk("\nDEBUGEH: my_read messages in room:  %d  num of read messages: %d  \n", chat_rooms[minor].num_of_messages,
           num_read_messages);
#endif

    int num_unread_messages = chat_rooms[minor].num_of_messages - num_read_messages;

    // check how many messages fit into count
    int num_wanted_messages = (count / sizeof(struct message_t));

    int diff = num_wanted_messages - num_unread_messages; // diff is number of meesage_t
#ifdef DEBUGEH
    printk("\nDEBUGEH: my_read wanted is: %d   diff is: %d  \n", num_wanted_messages, diff);
#endif
    if (diff < 0) diff = num_wanted_messages;  //we can provide that number of messages
    else if (diff >= 0) diff = num_unread_messages;  // not enough messages as required, read all unread

    int diff_bytes = diff * sizeof(struct message_t);

    // pointer to f_pos = starting point for copy_to_user
    int i = 0;
    struct Message_Node *iter_current = chat_rooms[minor].head_message;
    // promised to be in boundaries, therefore no need checking for NULL
    for (i = 0; i < num_read_messages; i++)
    {
        iter_current = iter_current->next;
    }

    // in case there are few messages (diff is the num of messages needs to read), read one by one from f_pos
    for (i = 0; i < diff; i++)
    {
        // copy to user
        if (copy_to_user((void *) buf, iter_current->message_pointer, diff_bytes))  // return 0 is error
        {
            printk("Read: Failed to write map to user space.\n");
            return -EBADF;
        }
        iter_current = iter_current->next;

    }
    // update f_pos
    *f_pos += diff;
#ifdef DEBUGEH
    printk("\nDEBUGEH: my_read f_pos is: fpos=%d *fpos=%d\n", (int) f_pos, (int) *f_pos);
#endif

#ifdef DEBUGEH
    // message loop
    printk("\nDEBUGEH: my_read check user buf (should contain message)\n");

    c = 0;
    for (c = 0; c < count / (2 * sizeof(char)); c++)
    {
        printk("%c", buf[c]);
    }
    printk("\nDEBUGEH: msg_len %d\n", count);


#endif


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
unsigned int StringHandler(struct message_t *new_message, const char *buffer, size_t count) {
    unsigned int msg_len = strnlen_user(buffer, MAX_MESSAGE_LENGTH);

#ifdef DEBUGEH
    // message loop
    // init message
    int c = 0;
    printk("\nDEBUGEH: StringHandler beginning read user buffer\t");
    for (c = 0; c < msg_len; c++)
    {
        printk("%c", buffer[c]);
    }
    printk("\nDEBUGEH: msg_len %d\n", msg_len);


#endif
    // validate proper size

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

    if (memset(new_message->message, 0, MAX_MESSAGE_LENGTH) == NULL)
    {
#ifdef DEBUGEH
        printk("\nDEBUGEH: memset failed line 362\n");
#endif
    }




    // copy the string message from the user space buffer to kernel's message_t.message[]
    if (copy_from_user(new_message->message, buffer, count) != 0)
    {
        printk("write: Error on copying from user space.\n");
        return -EBADF;
    }


#ifdef DEBUGEH
    // message loop
    // init message
    printk("\nDEBUGEH:string handler new_message->message result:\t");
    for (c = 0; c < MAX_MESSAGE_LENGTH; c++)
    {
        printk("%c", new_message->message[c]);
    }
    printk("\nDEBUGEH: end string handler\n");


#endif

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

    // create a messages pointer
    struct message_t *new_message = (struct message_t *) kmalloc(sizeof(struct message_t), GFP_KERNEL);
    // kmalloc validation
    if (new_message == NULL)
    {
        printk("write: Couldn't allocate mem for input string.\n");
        return -EFAULT;
    }

    int str_hndlr_res = StringHandler(new_message, buf, count);
    if (str_hndlr_res != 0)
    {  // if failed, free new_message
        kfree(new_message);
#ifdef DEBUGEH
        printk("\nDEBUGEH: My_write 442 return res: %d\n", str_hndlr_res);
#endif
        return str_hndlr_res; // let the original error to bubble up
    }

    int minor = MINOR(filp->f_dentry->d_inode->i_rdev); // which is the chat_room

#ifdef DEBUGEH
    // message loop
    // init message
    int c = 0;
    printk("\nDEBUGEH: MY_WRITE 453 new_message->message result:");
    for (c = 0; c < MAX_MESSAGE_LENGTH && new_message->message[c] != '\0'; c++)
    {
        printk("%c", new_message->message[c]);
    }

#endif


    // if no where else- we need to create the room here.
    if (chat_rooms[minor].participants_number == 0)
    {
        printk("my_open to initiate room is missing!.\n");
    }

    new_message->pid = getpid();
    new_message->timestamp = gettime();

    // create a messages list object
    struct Message_Node *new_node = (struct Message_Node *) kmalloc(sizeof(struct Message_Node), GFP_KERNEL);
    // kmalloc validation
    if (new_node == NULL)
    {
#ifdef DEBUGEH
        printk("\nDEBUGEH: My_write return ENOMEM 403\n");
#endif
        return -ENOMEM;
    }
#ifdef DEBUGEH
    printk("\nDEBUGEH: My_write return get pid %d\n", getpid());
    printk("\nDEBUGEH:  new_node->message_pointer pid1 %d\n", new_message->pid);
#endif
    // put new message in message node
    new_node->pid = getpid();
    new_node->timestamp = gettime();
    new_node->message_pointer = new_message;
    // append new message node to the messages list, and fix the pointers of the list.
    if (chat_rooms[minor].num_of_messages == 0)
    {
        chat_rooms[minor].head_message = new_node;
    }

    else
    {

        int i = 0;
        struct Message_Node *curr = chat_rooms[minor].head_message;

        for (i = 0; i < (chat_rooms[minor].num_of_messages - 1); i++)
        {
            curr = curr->next;
        }
        curr->next = new_node;
        new_node->prev = curr;

    }


    new_node->next = NULL;
    chat_rooms[minor].num_of_messages++;


    // DONE
#ifdef DEBUGEH
    printk("\nDEBUGEH: My_write ended, msg_len return is  %d\n", msg_len - 1);
#endif
    if (msg_len >= MAX_MESSAGE_LENGTH) return msg_len;  // number of bytes written
    else return msg_len - 1;
}

int my_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg) {
    // get the chat_room number from the inode
    unsigned int minor = MINOR(inode->i_rdev);
    int curr_fpos = filp->f_pos;  // current index of last read message

    if (cmd == COUNT_UNREAD)
    {
        int diff = chat_rooms[minor].num_of_messages - curr_fpos;
        return diff;
    }

    else if (cmd == SEARCH)
    {
#ifdef DEBUGEH
        printk("\nDEBUGEH: My_ioctl , SEARCH. curr_pos is %d\n", curr_fpos);
#endif
#ifdef DEBUGEH
        printk("\nDEBUGEH: My_ioctl , SEARCH. arg  is %d\n", (int) arg);
#endif


        /////  rolling in linked list   ////////
        struct Message_Node *iter_current = chat_rooms[minor].head_message;
        int steps;
        steps = 0;

        if (iter_current->next == NULL)
        {
            if ((iter_current->pid == arg) && (curr_fpos == 0))
            {
                return 0;
            }
        }


        else if (iter_current->next != NULL)
        {
            while (iter_current->next != NULL)  // means we have at least 2 nodes alive
            {
#ifdef DEBUGEH
                printk("\nDEBUGEH: My_ioctl , SEARCH WHILE, steps : %d", steps);
#endif

                if (iter_current->pid == arg)
                {
#ifdef DEBUGEH
                    printk("\nDEBUGEH: My_ioctl , SEARCH EQ");
#endif
                    if (steps >= curr_fpos)  // found next message from sender
                    {
                        return (steps) * sizeof(struct message_t);
                    }
                    // continue
                }
                steps++;
                iter_current = iter_current->next;
            }

            //if ended with no result
            if ((iter_current->pid == arg) && (steps == curr_fpos))
            {

#ifdef DEBUGEH
                printk("\nDEBUGEH: My_ioctl , SEARCH.ended 602");
#endif
                return (steps) * sizeof(struct message_t);
            }

            if (curr_fpos == chat_rooms[minor].num_of_messages)
            {  // end of the list, no further messages with that sender
                // ended with no result
                return -ENOENT;
            }
            // if ended with no result
            return -ENOENT;
            /////  rolling in linked list   ////////
        }
    }

    else  // case wrong command
        return -ENOTTY;

    return -EFAULT;

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

    int curr_fpos = filp->f_pos;
#ifdef DEBUGEH
    printk("\nDEBUGEH: my_llseek f_pos: %d \t out of %d messages\n", curr_fpos, chat_rooms[minor].num_of_messages);
#endif
    long messages_offset =
            (long) offset / (long) sizeof(struct message_t);  // promised to be int, mult of sizeof(message_t)
    int destination_msg = filp->f_pos + messages_offset;  // the index of the message we will arrive after offset
#ifdef DEBUGEH
    printk("\nDEBUGEH: my_llseek message_offset: %d\n", (int) messages_offset);
#endif
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
            filp->f_pos = 0;
            return 0;
        }
        else if (messages_offset > chat_rooms[minor].num_of_messages)
        { // out of boundaries
            filp->f_pos = chat_rooms[minor].num_of_messages;
            return (chat_rooms[minor].num_of_messages + 1) * sizeof(struct message_t);
        }

        else
        {
            filp->f_pos = messages_offset;
            return (messages_offset) * sizeof(struct message_t);

        }

    }

        // move Chat_Room.tail_message+ offset steps
        ///// SEEK_END  /////////
    else if (type == SEEK_END)
    {
        // case out of boundaries end side
        if (messages_offset >= 0)
        {
#ifdef DEBUGEH
            int curr_fpos2 = filp->f_pos;

            printk("\nDEBUGEH: my_llseek SEEK_END line 710: %d\n", curr_fpos2);
#endif
            filp->f_pos = chat_rooms[minor].num_of_messages; // TODO: +1 even though there's no message there
            return (chat_rooms[minor].num_of_messages + 1) * sizeof(struct message_t);
        }
            // case out of boundaries beginning side
        else if ((int) (-messages_offset) >= chat_rooms[minor].num_of_messages)
        { // out of boundaries
#ifdef DEBUGEH
            printk("\nDEBUGEH: my_llseek SEEK_END line 720: messages_offset = %d \t chat_rooms[minor].num_of_messages= %d\n",
                   (int) messages_offset, (int) (chat_rooms[minor].num_of_messages));
#endif
            filp->f_pos = 0;
            return 0;
        }

            // case in boundaries
        else
        {
#ifdef DEBUGEH
            printk("\nDEBUGEH: my_llseek SEEK_END line 730: new f_pos is %d\n",
                   (int) chat_rooms[minor].num_of_messages + (int) messages_offset);
#endif
            filp->f_pos = chat_rooms[minor].num_of_messages + messages_offset;
            return (filp->f_pos) * sizeof(struct message_t);

        }

    }



        // move Chat_Room.current_message+ offset steps
        ///// SEEK_CUR /////////
    else if (type == SEEK_CUR)
    {
#ifdef DEBUGEH
        printk("\nDEBUGEH: my_llseek SEEK_CURR line 751:  %d\n", (int) filp->f_pos);

        printk("\nDEBUGEH: my_llseek SEEK_CURR line 753: messages_offset = %d \t chat_rooms[minor].num_of_messages= %d\n",
               (int) messages_offset, (int) (chat_rooms[minor].num_of_messages));
#endif

        if ((filp->f_pos + messages_offset) >= (chat_rooms[minor].num_of_messages))
        {
            filp->f_pos = chat_rooms[minor].num_of_messages; // TODO: +1 even though there's no message there
#ifdef DEBUGEH
            printk("\nDEBUGEH: my_llseek SEEK_CURR line 759: dest = %d \t chat_rooms[minor].num_of_messages= %d\n",
                   destination_msg, (int) (chat_rooms[minor].num_of_messages));
#endif

            return (chat_rooms[minor].num_of_messages + 1) * sizeof(struct message_t);
        }

        else if ((filp->f_pos + messages_offset) <= 0)
        { // out of boundaries
#ifdef DEBUGEH
            printk("\nDEBUGEH: my_llseek SEEK_CURR line 773: messages_offset = %d \t chat_rooms[minor].num_of_messages= %d\n",
                   (int) messages_offset, (int) (chat_rooms[minor].num_of_messages));
#endif
            filp->f_pos = 0;
            return 0;
        }

        else
        {
            filp->f_pos = (filp->f_pos + messages_offset);
            return (filp->f_pos) * sizeof(struct message_t);

        }
    }

//    else
//            return -EINVAL;
#ifdef DEBUGEH
    printk("\nDEBUGEH: my_llseek returnning  -EINVAL\n");
#endif
    return -EINVAL;


}

time_t gettime() {
    do_gettimeofday(&tv);
    return tv.tv_sec;
}

pid_t getpid() {
    return current->pid;
}
