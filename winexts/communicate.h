#ifndef COMMUNICATE_H
#define COMMUNICATE_H

communicate_error_t communicate_read__read_struct(
  task_t* task,
  uintptr_t address,
  communicate_read_t* communicate_read);

communicate_error_t communicate_read__write_struct(
  task_t* task,
  uintptr_t address,
  communicate_write_t* communicate_write);

communicate_error_t communicate_read__remote_mmap_struct(
  task_t* task,
  uintptr_t address,
  communicate_remote_mmap_t* communicate_remote_mmap);

communicate_error_t communicate_read__remote_munmap_struct(
  task_t* task,
  uintptr_t address,
  communicate_remote_munmap_t* communicate_remote_munmap);

communicate_error_t communicate_read__remote_clone_struct(
  task_t* task,
  uintptr_t address,
  communicate_remote_clone_t* communicate_remote_clone);

communicate_error_t communicate_read__list_vmas_struct(
  task_t* task,
  uintptr_t address,
  communicate_list_vmas_t* communicate_list_vmas);

communicate_error_t communicate_read__remote_mprotect_struct(
  task_t* task,
  uintptr_t address,
  communicate_remote_mprotect_t* communicate_remote_mprotect);

communicate_error_t communicate_process_cmd_read(uintptr_t address);

communicate_error_t communicate_process_cmd_write(uintptr_t address);

communicate_error_t communicate_process_cmd_remote_mmap(uintptr_t address);

communicate_error_t communicate_process_cmd_remote_munmap(
  uintptr_t address);

communicate_error_t communicate_process_cmd_remote_clone(
  uintptr_t address);

communicate_error_t communicate_process_cmd_list_vmas(uintptr_t address);

communicate_error_t communicate_process_cmd_remote_mprotect(
  uintptr_t address);

#endif
