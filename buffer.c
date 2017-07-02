#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "main.h"
#include "buffer.h"

void Buffer_Init(void *buffer_ptr)
{
    buffer_t *buf;
    buf = (buffer_t *)buffer_ptr;
    
    pthread_mutex_init(&buf->Mutex, NULL);
    pthread_cond_init(&buf->Signal, NULL);
    
    pthread_mutex_lock(&buf->Mutex);
    buf->Head = 0;
    buf->Tail = 0;
    buf->Loss = 0;
    pthread_mutex_unlock(&buf->Mutex);
}

uint32_t Buffer_NotEmpty(void *buffer_ptr)
{
    buffer_t *buf;
    buf = (buffer_t *)buffer_ptr;

    uint32_t result;
    
    pthread_mutex_lock(&buf->Mutex);
    result = (buf->Head!=buf->Tail);
    pthread_mutex_unlock(&buf->Mutex);
    
    return result;
}

uint32_t Buffer_Head(void *buffer_ptr)
{
    buffer_t *buf;
    buf = (buffer_t *)buffer_ptr;
    
    uint32_t result;
    
    pthread_mutex_lock(&buf->Mutex);
    result = buf->Head;
    pthread_mutex_unlock(&buf->Mutex);
    
    return result;
}

uint32_t Buffer_Tail(void *buffer_ptr)
{
    buffer_t *buf;
    buf = (buffer_t *)buffer_ptr;
    
    uint32_t result;
    
    pthread_mutex_lock(&buf->Mutex);
    result = buf->Tail;
    pthread_mutex_unlock(&buf->Mutex);
    
    return result;
}

uint32_t Buffer_Loss(void *buffer_ptr)
{
    buffer_t *buf;
    buf = (buffer_t *)buffer_ptr;
    
    uint32_t result;
    
    pthread_mutex_lock(&buf->Mutex);
    result = buf->Loss;
    pthread_mutex_unlock(&buf->Mutex);
    
    return result;
}

/* Lossy when buffer is full */
bool Buffer_Push(void *buffer_ptr, void *buffer_element_ptr)
{
    buffer_t *buf;
    buf = (buffer_t *)buffer_ptr;
    
    pthread_mutex_lock(&buf->Mutex);
    if(buf->Head!=(buf->Tail-1) && !(buf->Head==(BUFFER_LENGTH-1) && buf->Tail==0))
    {
        if(buf->Head==(BUFFER_LENGTH-1))
            buf->Head=0;
        else
            buf->Head++;

        buf->Buffer[buf->Head] = buffer_element_ptr;
        
        pthread_cond_signal(&buf->Signal);

        pthread_mutex_unlock(&buf->Mutex);

        return true;
    }

    buf->Loss++;

    pthread_mutex_unlock(&buf->Mutex);

    return false;
}

/* Lossy when buffer is full */
bool Buffer_BurstPush(void *buffer_ptr, void **buffer_element_ptrs, uint32_t buffer_elements)
{
    uint16_t i;
    buffer_t *buf;
    buf = (buffer_t *)buffer_ptr;
    
    pthread_mutex_lock(&buf->Mutex);
    
    for(i=0;i<buffer_elements;i++)
    {
        if(buf->Head!=(buf->Tail-1) && !(buf->Head==(BUFFER_LENGTH-1) && buf->Tail==0))
        {
            if(buf->Head==(BUFFER_LENGTH-1))
                buf->Head=0;
            else
                buf->Head++;

            buf->Buffer[buf->Head] = buffer_element_ptrs[i];
        
            if(i==0)
            {
                pthread_cond_signal(&buf->Signal);
            }
        }
        else
        {
            buf->Loss++;

            pthread_mutex_unlock(&buf->Mutex);

            return false;
        }
    }

    pthread_mutex_unlock(&buf->Mutex);

    return true;
}

void Buffer_Pop(void *buffer_ptr, void **buffer_element_ptr)
{
    buffer_t *buf;
    buf = (buffer_t *)buffer_ptr;
    
    pthread_mutex_lock(&buf->Mutex);

    if(buf->Head!=buf->Tail)
    {
        if(buf->Tail==(BUFFER_LENGTH-1))
            buf->Tail=0;
        else
            buf->Tail++;

        *buffer_element_ptr = buf->Buffer[buf->Tail];
    
        pthread_mutex_unlock(&buf->Mutex);
    }
    else
    {
        pthread_mutex_unlock(&buf->Mutex);
        
        *buffer_element_ptr = NULL;
    }
}

void Buffer_WaitPop(void *buffer_ptr, void **buffer_element_ptr)
{
    buffer_t *buf;
    buf = (buffer_t *)buffer_ptr;
    
    pthread_mutex_lock(&buf->Mutex);
    
    while(buf->Head==buf->Tail) /* If buffer is empty */
    {
        /* Mutex is atomically unlocked on beginning waiting for signal */
        pthread_cond_wait(&buf->Signal, &buf->Mutex);
        /* and locked again on resumption */
    }
    
    if(buf->Tail==(BUFFER_LENGTH-1))
        buf->Tail=0;
    else
        buf->Tail++;

    *buffer_element_ptr = buf->Buffer[buf->Tail];
    
    pthread_mutex_unlock(&buf->Mutex);
}
