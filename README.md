# "一生一芯"工程项目

这是"一生一芯"的工程项目. 通过运行
```bash
bash init.sh subproject-name
```
进行初始化, 具体请参考[实验讲义][lecture note].

[lecture note]: https://ysyx.oscc.cc/docs/

NEMU和NPC均可运行RT-Thread,NPC总线完成

开启difftest：define DIFTEST宏在npc/include/npc_define.h内。nemu menuconfig打开build target和testing and debugging后，make clean再make run
