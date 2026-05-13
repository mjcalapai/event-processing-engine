// DAVE CODE NOT OURS

#include <boundedBuffer.h>
#include "log_entry.h"


template <typename T>
BoundedBuffer<T>::BoundedBuffer(int N) {

  buffer = new T[N];
  buffer_size = N;
  buffer_first = 0;
  buffer_last = 0;
  buffer_cnt = 0;

  pthread_mutex_init(&buffer_lock, NULL);
  pthread_cond_init(&buffer_not_full, NULL);
  pthread_cond_init(&buffer_not_empty, NULL);
}


template <typename T>
BoundedBuffer<T>::~BoundedBuffer() {
  // TODO: destructor to clean up anything necessary
  delete[] buffer;
  pthread_mutex_destroy(&buffer_lock);
  pthread_cond_destroy(&buffer_not_full);
  pthread_cond_destroy(&buffer_not_empty);
}


template <typename T>
void BoundedBuffer<T>::append(T data) {
  // TODO: append a data item to the circular buffer
  pthread_mutex_lock(&buffer_lock);
  while (buffer_cnt == buffer_size) {
    pthread_cond_wait(&buffer_not_full, &buffer_lock);
  }
  buffer[buffer_last] = data;
  buffer_last = (buffer_last + 1) % buffer_size;
  buffer_cnt++;
  pthread_cond_signal(&buffer_not_empty);
  pthread_mutex_unlock(&buffer_lock);
}


template <typename T>
T BoundedBuffer<T>::remove() {
  pthread_mutex_lock(&buffer_lock);
  while(buffer_cnt == 0){
    pthread_cond_wait(&buffer_not_empty, &buffer_lock);
  }
  T data = buffer[buffer_first];
  buffer_first = (buffer_first + 1) % buffer_size;
  buffer_cnt--;
  pthread_cond_signal(&buffer_not_full);
  pthread_mutex_unlock(&buffer_lock);
  return data;
}


template <typename T>
bool BoundedBuffer<T>::isEmpty() {
  pthread_mutex_lock(&buffer_lock);
  bool cond = (buffer_cnt == 0);
  pthread_mutex_unlock(&buffer_lock);
  return cond;
}

template class BoundedBuffer<int>;
template class BoundedBuffer<LogEntry*>;