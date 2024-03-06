#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#define NANOSECOND_CONVERSION 1e9

pthread_mutex_t loading_mutex, queue_mutex, track_mutex;                  // mutexes
pthread_cond_t start_loading, done_loading, ready_to_load, done_crossing; // condition variables
int train_count = 0;
struct timespec global_start_time;
int start_loading_condition, train_in_queue, train_on_track = 0;
struct Node *westbound_head = NULL; // head of the westbound linked list
struct Node *eastbound_head = NULL; // head of the westbound linked list
enum status *status_flags;          // this is an array flags for each train to confirm when it is time to cross

enum priority // the priority of each train
{
  low,
  high
} priority;

enum status // whether a train has beem granted permission to cross the main track
{
  READY,
  GRANTED
} status;

// Struct for each train object 🚂
struct Train
{
  int train_num;
  enum priority prio;
  pthread_t threadID;
  char direction[5]; // direction the train is bound for
  int load_time;
  int cross_time;
  pthread_cond_t cross_condition; // the trains condition variable that is unique to each train
};

// struct for the linked list node
struct Node
{
  struct Train *train;
  struct Node *next;
  enum priority prio;
};

// This function is from the starter code file loading_train.c
// Convert timespec to seconds
double timespec_to_seconds(struct timespec *ts)
{
  return ((double)ts->tv_sec) + (((double)ts->tv_nsec) / NANOSECOND_CONVERSION);
}

// This function calls the get time function updating the timespec function
void read_time(struct timespec *ts)
{
  if (clock_gettime(CLOCK_MONOTONIC, ts) == -1)
  {
    perror("clock_gettime");
    exit(1);
  }
}

// This function takes two time spec structures and prints out the time for the simulation
void print_time(struct timespec *start_time, struct timespec *end_time, char *message)
{
  double time = timespec_to_seconds(end_time) - timespec_to_seconds(start_time);
  printf("%02.0f:%02.0f:%04.1f %s", (time / 3600), (time / 60), time, message);
}

// returns 1 if t1 is higher priority than t2 and 0 if t1 is less than t2 or if they are equal
int compare_trains(struct Train *t1, struct Train *t2)
{
  if (t1->prio > t2->prio)
  {
    return 1;
  }
  else if (t1->prio == t2->prio)
  {
    if (t1->load_time == t2->load_time)
    {
      if (t1->train_num < t2->train_num) // should be <
      {
        return 1;
      }
    }
  }
  return 0;
}

void enqueue(struct Train *train_ptr, struct Node **head)
{
  // make node
  struct Node *new_node = malloc(sizeof(struct Node));
  if (new_node == NULL)
  {
    printf("ERROR: could not allocate memory\n");
    exit(0);
  }
  new_node->train = train_ptr;
  new_node->next = NULL;
  new_node->prio = train_ptr->prio;

  // adding node to correct spot in list
  struct Node *node_ptr = *head;
  if ((*head) == NULL)
  {
    *head = new_node;
  }
  else if (compare_trains(new_node->train, (*head)->train) == 1)
  {
    new_node->next = *head;
    *head = new_node;
  }
  else
  {
    while (node_ptr->next != NULL && compare_trains(new_node->train, node_ptr->next->train) == 0)
    {
      node_ptr = node_ptr->next;
    }
    new_node->next = node_ptr->next;
    node_ptr->next = new_node;
  }
}

// this function returns 1 if the queue is empty and 0 otherwise
int isEmpty(struct Node **head)
{
  return (*head) == NULL;
}

// this function returns the train from the first element of the queue
struct Train *peek(struct Node **head)
{
  return (*head)->train;
}

// This function dequeues the first node from the linked list and frees the memory while returning the train in that node
struct Train *dequeue(struct Node **head)
{
  struct Node *temp = *head;
  struct Train *train = (*head)->train;
  (*head) = (*head)->next;
  free(temp);
  return train;
}

// this is the function each thread runs once created
void *ThreadFunction(void *void_train_pointer)
{
  struct Train *train_ptr = void_train_pointer;
  struct timespec thread_time; // creates a thread specific timespec

  pthread_mutex_lock(&loading_mutex);
  train_count++; // increments the count of the number of trains that are ready
  pthread_cond_signal(&ready_to_load);
  while (!start_loading_condition) // checks that the flag to start loading is true
  {
    pthread_cond_wait(&start_loading, &loading_mutex); // waits for a signal from main to start
  }
  pthread_mutex_unlock(&loading_mutex);
  usleep((train_ptr->load_time) * 100000.0); // Sleeps to simulate loading
  char message[100];                         // this holds the messages that the thread creates before it prints them
  sprintf(message, "Train %2d is ready to go %4s\n", train_ptr->train_num, train_ptr->direction);
  read_time(&thread_time);
  print_time(&global_start_time, &thread_time, message);

  // adds train to queue
  pthread_mutex_lock(&queue_mutex);
  if (strcmp(train_ptr->direction, "West") == 0)
  {
    enqueue(train_ptr, &westbound_head);
  }
  else
  {
    enqueue(train_ptr, &eastbound_head);
  }
  train_in_queue++;
  pthread_cond_signal(&done_loading);
  status_flags[train_ptr->train_num] = READY;
  pthread_mutex_unlock(&queue_mutex);

  pthread_mutex_lock(&track_mutex);
  while (status_flags[train_ptr->train_num] != GRANTED) // waits for flag to change giving it permission to cross
  {
    pthread_cond_wait(&train_ptr->cross_condition, &track_mutex); // waits for signal to start crossing
  }

  sprintf(message, "Train %2d is ON the main track going %4s\n", train_ptr->train_num, train_ptr->direction);
  read_time(&thread_time);
  print_time(&global_start_time, &thread_time, message);
  usleep((train_ptr->cross_time) * 100000.0); // Sleeps to simulate loading
  sprintf(message, "Train %2d is OFF the main track after going %4s\n", train_ptr->train_num, train_ptr->direction);
  read_time(&thread_time);
  print_time(&global_start_time, &thread_time, message);

  train_on_track = 0;                  // resets global variable so main thread knows the train is finished crossing
  pthread_cond_signal(&done_crossing); // signal main that train is done crossing
  pthread_mutex_unlock(&track_mutex);
  pthread_cond_destroy(&train_ptr->cross_condition); // destroys the trains individual condition variable
  free(train_ptr);                                   // frees train memory
  pthread_exit(NULL);                                // returns from the thread function
}

// this function reads from the given input file and creates all the corresponding trains
int Read_Input(char *file)
{
  FILE *file_ptr = fopen(file, "r");
  if (file_ptr == NULL)
  {
    printf("ERROR: no such file.\n");
    exit(1);
  }
  struct Node *root = NULL;
  struct Train *train_ptr;
  char dir_and_prio;
  int load_t, cross_t, error_check;
  int train_num = 0;
  enum priority prio;
  char *direction;
  while (fscanf(file_ptr, "%c %d %d\n", &dir_and_prio, &load_t, &cross_t) != EOF)
  {
    prio = isupper(dir_and_prio) ? high : low;
    if (toupper(dir_and_prio) == 'E')
    {
      direction = "East";
    }
    else
    {
      direction = "West";
    }
    train_ptr = malloc(sizeof(struct Train));
    train_ptr->train_num = train_num;
    train_ptr->prio = prio;
    strcpy(train_ptr->direction, direction);
    train_ptr->load_time = load_t;
    train_ptr->cross_time = cross_t;
    pthread_cond_init(&(train_ptr->cross_condition), NULL);

    train_num++;
    error_check = pthread_create(&(train_ptr->threadID), NULL, ThreadFunction, (void *)train_ptr);
    if (error_check)
    {
      printf("ERROR: return code from pthread_create() is %d\n", error_check);
      exit(1);
    }
  }
  status_flags = malloc(sizeof(enum status) * train_num);
  return train_num;
}

// dequeues a train from the westbound station and updates the appropriate variables
struct Train *dequeue_westbound(char *last_station, int *in_a_row)
{
  *in_a_row = *last_station == 'w' ? (*in_a_row) + 1 : 1;
  *last_station = 'w';
  return dequeue(&westbound_head);
}

// dequeues a train from the eastbound station and updates the appropriate variables
struct Train *dequeue_eastbound(char *last_station, int *in_a_row)
{
  *in_a_row = *last_station == 'e' ? (*in_a_row) + 1 : 1;
  *last_station = 'e';
  return dequeue(&eastbound_head);
}

// This function chooses the next train to dequeue based on the crossing criteria
struct Train *choose_next_train(char *last_station, int *in_a_row)
{
  if (!isEmpty(&westbound_head) && isEmpty(&eastbound_head))
  {
    return dequeue_westbound(last_station, in_a_row);
  }
  else if (!isEmpty(&eastbound_head) && isEmpty(&westbound_head))
  {
    return dequeue_eastbound(last_station, in_a_row);
  }
  else if (!isEmpty(&westbound_head) && !isEmpty(&eastbound_head))
  {
    if (*in_a_row < 3)
    {
      if (peek(&westbound_head)->prio == high && peek(&eastbound_head)->prio == low)
      {
        return dequeue_westbound(last_station, in_a_row);
      }
      else if (peek(&westbound_head)->prio == low && peek(&eastbound_head)->prio == high)
      {
        return dequeue_eastbound(last_station, in_a_row);
      }
      else
      {
        if (*last_station == 'w')
        {
          return dequeue_eastbound(last_station, in_a_row);
        }
        else
        {
          return dequeue_westbound(last_station, in_a_row);
        }
      }
    }
    else
    {
      if (*last_station == 'w')
      {
        return dequeue_eastbound(last_station, in_a_row);
      }
      else
      {
        return dequeue_westbound(last_station, in_a_row);
      }
    }
  }
  else
  {
    printf("ERROR: no trains in queue\n");
    return NULL;
  }
}

int main(int argc, char *argv[])
{
  // initializes all mutexes and convars
  pthread_mutex_init(&loading_mutex, NULL);
  pthread_mutex_init(&queue_mutex, NULL);
  pthread_mutex_init(&track_mutex, NULL);
  pthread_cond_init(&done_loading, NULL);
  pthread_cond_init(&start_loading, NULL);
  pthread_cond_init(&ready_to_load, NULL);
  pthread_cond_init(&done_crossing, NULL);

  int max_trains = Read_Input(argv[1]); // read file and create all threads

  pthread_mutex_lock(&loading_mutex);

  while (train_count < max_trains) // waits for all trains to finish loading before signaling
  {
    pthread_cond_wait(&ready_to_load, &loading_mutex);
  }

  start_loading_condition = 1;            // this line lets the threads exit their while loop and start loading
  pthread_cond_broadcast(&start_loading); // sends signal to start loading thus waking up all the threads
  read_time(&global_start_time);          // stars the global timer
  pthread_mutex_unlock(&loading_mutex);   // unlocks the loading mutex so other threads can start loading

  // Done loading, now crossing section
  char last_station = 'n';         // starts as neither
  int in_a_row = 0;                // counts the amount of trains that have gone in a row
  int trains_left = max_trains;    // counts the number of trains that have left to cross
  pthread_t threadIDs[max_trains]; // keeps track of the thread id as it processes the trains

  while (trains_left > 0)
  {
    pthread_mutex_lock(&queue_mutex);
    while (train_in_queue <= 0)
    {
      pthread_cond_wait(&done_loading, &queue_mutex);
    }
    struct Train *cur_train = choose_next_train(&last_station, &in_a_row);
    threadIDs[cur_train->train_num] = cur_train->threadID;
    train_in_queue--;
    trains_left--;
    train_on_track = 1;
    pthread_mutex_unlock(&queue_mutex);

    pthread_mutex_lock(&track_mutex);
    status_flags[cur_train->train_num] = GRANTED;
    pthread_mutex_unlock(&track_mutex);
    pthread_cond_signal(&(cur_train->cross_condition));

    pthread_mutex_lock(&track_mutex);
    while (train_on_track)
    {
      pthread_cond_wait(&done_crossing, &track_mutex);
    }
    pthread_mutex_unlock(&track_mutex);
  }

  for (int i = 0; i < max_trains; i++)
  {
    pthread_join(threadIDs[i], NULL); // this is like waiting for the mutex
  }

  // frees all memory and variables that were used
  free(status_flags);

  pthread_mutex_destroy(&loading_mutex);
  pthread_mutex_destroy(&queue_mutex);
  pthread_mutex_destroy(&track_mutex);
  pthread_cond_destroy(&done_loading);
  pthread_cond_destroy(&start_loading);
  pthread_cond_destroy(&ready_to_load);
  pthread_cond_destroy(&done_crossing);

  pthread_exit(NULL);
}
