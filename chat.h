#ifndef _CHAT_H_
#define _CHAT_H_

#include <linux/ioctl.h>
#include <linux/types.h>

#define MY_MAGIC 'r'
#define COUNT_UNREAD _IO(MY_MAGIC, 0)
#define SEARCH _IO(MY_MAGIC, 1)

#define MAX_MESSAGE_LENGTH 100
#define MAX_ROOMS_POSSIBLE 256

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2




//
// Function prototypes
//
int my_open(struct inode *, struct file *);

int my_release(struct inode *, struct file *);

ssize_t my_read(struct file *, char *, size_t, loff_t *);

ssize_t my_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

int my_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);

loff_t my_llseek(struct file *, loff_t, int);

pid_t getpid();

time_t gettime();

struct message_t {
    pid_t pid;  // the sender
    time_t timestamp;
    char message[MAX_MESSAGE_LENGTH];

};

// this is actually a node of the list
struct Message_Node {
    struct Message_Node *prev;
    struct Message_Node *next;
    //pid_t pid;  // the sender
    //time_t timestamp;
    struct message_t *message_pointer;
};

// each minor is a chatroom
// this is also the header of the messages linked list
struct Chat_Room {
    unsigned int minor_id;  // the room's ID
    unsigned int participants_number;
    unsigned int num_of_messages;

    // linked list of messages
    struct Message_Node *head_message;
    //struct Message_Node *tail_message;
};

// taking advantage of the fact minors are in the range of [0-255]
struct Chat_Room chat_rooms[MAX_ROOMS_POSSIBLE];

#endif
