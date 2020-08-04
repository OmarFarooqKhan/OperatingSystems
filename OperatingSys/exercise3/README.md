This exercise is about developing a device driver for a character device, which implements a simple way of message passing.

Creating the device, which has to be /dev/opsysmem, creates an empty list of messages.
Removing the device deallocates all messages and removes the list of messages.
Reading from the device returns one message, and removes this message from the kernel list. If the list of messages is empty, the reader returns -EAGAIN.
Writing to the device stores the message in kernel space and adds it to the list if the message is below the maximum size, 
and the limit of the size of all messages wouldn't be surpassed with this message. 
