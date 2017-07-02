#ifndef __RX_BUFFER_H_
#define __RX_BUFFER_H_

#define BUFFER_LENGTH    32768

typedef struct buffer_t
{
    /* Buffer Access Lock */
    pthread_mutex_t Mutex;
    /* New Data Signal */
    pthread_cond_t Signal;
    /* Head and Tail Indexes */
    uint32_t Head, Tail;
    /* Data Loss Counter */
    uint32_t Loss;
    /* Data */
    void *Buffer[BUFFER_LENGTH];
} buffer_t;

/** Common functions **/
void Buffer_Init(void *buffer_ptr);
uint32_t Buffer_NotEmpty(void *buffer_ptr);
uint32_t Buffer_Head(void *buffer_ptr);
uint32_t Buffer_Tail(void *buffer_ptr);
uint32_t Buffer_Loss(void *buffer_ptr);
bool Buffer_Push(void *buffer_ptr, void *buffer_element_ptr);
bool Buffer_BurstPush(void *buffer_ptr, void **buffer_element_ptrs, uint32_t buffer_elements);
void Buffer_Pop(void *buffer_ptr, void **buffer_element_ptr);
void Buffer_WaitPop(void *buffer_ptr, void **buffer_element_ptr);

#endif