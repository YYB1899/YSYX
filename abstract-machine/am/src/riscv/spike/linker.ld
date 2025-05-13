ENTRY(_start)

SECTIONS {
  . = 0x80000000;                  /* 基地址设为 0x80000000 */

  /* 代码段 */
  .text : {
    *(entry)
    *(.text*)
  }

  /* 只读数据 */
  .rodata : {
    *(.rodata*)
  }

  /* 显式定义.data段地址（包含数组a） */
  .data 0x80000188 : {             /* 强制.data段从0x80000188开始 */
    *(.data)                       /* 普通.data段内容 */
    *(.sdata)                      /* 小数据段 */
    /* 显式初始化数组（示例值，实际值由编译器填充） */
    LONG(0); LONG(1); LONG(2);     /* 示例初始值，可省略 */
  }

  /* 其他段（保持原样） */
  .htif : {
    PROVIDE(__htif_base = .);
    *(.htif)
  }
  .bss : {
    _bss_start = .;
    *(.bss*)
    *(.sbss*)
    *(.scommon)
  }

  /* 栈和堆空间 */
  _stack_top = ALIGN(0x1000);
  . = _stack_top + 0x8000;
  _stack_pointer = .;
  end = .;
  _end = .;
  _heap_start = ALIGN(0x1000);
}
