#ifndef TASK_SCHEDULER_H
#define TASK_SCHEDULER_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_COMMAND_SIZE 256
#define MAX_TASKS 100

typedef struct Task {
    char command[MAX_COMMAND_SIZE];
    time_t execution_time;
} Task;

void push(Task task);            
Task pop();                      
void execute_task(Task task);    
void task_scheduler();          
void add_task(char* command, time_t exec_time);  
#endif
