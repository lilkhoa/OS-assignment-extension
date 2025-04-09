/*
 * Copyright (C) 2025 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Sierra release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

 #include "common.h"
 #include "syscall.h"
 #include "stdio.h"
 #include "libmem.h"
 
 #include "queue.h"
 
 int __sys_killall(struct pcb_t *caller, struct sc_regs* regs)
 {
     char proc_name[100];
     uint32_t data;
 
     //hardcode for demo only
     uint32_t memrg = regs->a1;
     
     /* TODO: Get name of the target proc */
     //proc_name = libread..
     int i = 0;
     data = 0;
     int len = sizeof(proc_name) - 1;
     
     /* TODO: Traverse proclist to terminate the proc
      *       stcmp to check the process match proc_name
      */
     while (i < len) {
         if (libread(caller, memrg, i, (uint32_t*)&data) < 0) {
             return -1; // Lỗi đọc bộ nhớ
         }
         proc_name[i] = data;
         if (data == 0) break;
         i++;
     }
     proc_name[i - 1] = '\0'; // Kết thúc mảng với ký tự NULL
 
 
     //caller->running_list
     //caller->mlq_ready_queu
     struct queue_t *running_list = &caller->running_list;
     struct queue_t *ready_queue = &caller->mlq_ready_queue;
     struct pcb_t *target;
     int killed_count = 0;
     
 
     /* TODO Maching and terminating 
      *       all processes with given
      *        name in var proc_name
      */
     int n = running_list->size;
     for (int i = 0; i < n; i++) {
         target = dequeue(running_list);
         if (target == NULL) continue; // Kiểm tra tránh lỗi NULL
 
         if (target->path != NULL && strcmp(target->path, proc_name) == 0) {
             free_pcb(target);
             killed_count++;
         }
         else enqueue(running_list, target); // Trả lại tiến trình vào hàng đợi
     }
 
     for (int q = 0; q < MAX_PRIO; q++) { // Giả sử có nhiều mức ưu tiên
         struct queue_t *queue = &ready_queue[q];
         
         int n = queue->size;
         for (int i = 0; i < n; i++) {
             target = dequeue(queue); // Lấy tiến trình từ mảng
             if (target == NULL) continue;
 
             // Bảo vệ tiến trình hệ thống và chính mình
             if (target->pid == 0 || target == caller) {
                 enqueue(queue, target);
                 continue;
             }
 
             if (target->path != NULL && strcmp(target->path, proc_name) == 0) {
                 free_pcb(target);
                 killed_count++;
             } 
             else enqueue(queue, target); // Trả lại tiến trình vào hàng đợi
         }
     }
 
     printf("Killed %d processes matching '%s'\n", killed_count, proc_name);
     return killed_count; // Trả về số lượng quá trình bị xóa
 }