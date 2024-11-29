#include "task_scheduler.h"

int task_size = 0;
Task tasks[MAX_TASKS];

void swap(Task *t1, Task *t2) {
    Task temp = *t1;
    *t1 = *t2;
    *t2 = temp;
}

void min_heapify(int i) {
    int left = 2 * i + 1;
    int right = 2 * i + 2;
    int smallest = i;

    if (left < task_size && tasks[left].execution_time < tasks[smallest].execution_time)
        smallest = left;

    if (right < task_size && tasks[right].execution_time < tasks[smallest].execution_time)
        smallest = right;

    if (smallest != i) {
        swap(&tasks[i], &tasks[smallest]);
        min_heapify(smallest);
    }
}

void push(Task task) {
    if (task_size >= MAX_TASKS) {
        printf("Task limit reached.\n");
        return;
    }

    tasks[task_size] = task;
    int i = task_size++;
    
    while (i > 0 && tasks[(i - 1) / 2].execution_time > tasks[i].execution_time) {
        swap(&tasks[i], &tasks[(i - 1) / 2]);
        i = (i - 1) / 2;
    }
}

Task pop() {
    if (task_size == 0) {
        Task empty_task = {"", 0};
        return empty_task;
    }

    Task root = tasks[0];
    tasks[0] = tasks[--task_size];
    min_heapify(0);

    return root;
}

void print_tasks() {
    for (int i = 0; i < task_size; i++) {
        printf("Task: %s, Execution Time: %ld\n", tasks[i].command, tasks[i].execution_time - time(NULL));
    }
}

void execute_task(Task task) {
    pid_t pid = fork();
    if (pid == 0) {
        printf("Executing: %s\n", task.command);
        system(task.command);
        return;
    }
}

void task_scheduler() {
    while (task_size > 0) {
        Task task = pop();
        time_t current_time = time(NULL);

        if (current_time < task.execution_time) {
            sleep(task.execution_time - current_time);
        }

        execute_task(task);
    }
}

void add_task(char* command, time_t exec_time) {
    Task new_task;
    strncpy(new_task.command, command, MAX_COMMAND_SIZE);
    new_task.execution_time = exec_time;
    push(new_task);
    printf("Task added successfully.\n");
    print_tasks();
    task_scheduler();
}