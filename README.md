# "一生一芯"工程项目

这是"一生一芯"的工程项目. 通过运行
```bash
bash init.sh subproject-name
```
进行初始化, 具体请参考[实验讲义][lecture note].

[lecture note]: https://ysyx.oscc.cc/docs/

NEMU和NPC均可运行RT-Thread，NPC总线完成。修改环境变量、NPC中NPC宏与SOC系列接口的BUG。make npc时需输入make ARCH=riscv32e-npc all完成（详情见npc/makefile）。
