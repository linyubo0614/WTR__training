#ifndef PROCESS_TASK_H
#define PROCESS_TASK_H

/* 数据处理任务入口 */
void ProcessTask(void *argument);

//用于切换状态的函数，在buttontask中使用
void Process_RequestModeSwitch(void);

#endif