#include <am.h>
#include <nemu.h>

#define KEYDOWN_MASK 0x8000

void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
  uint32_t code = inl(KBD_ADDR);
  kbd->keydown = (code & KEYDOWN_MASK ? true : false);//提取code[31]按下位
  kbd->keycode = code & ~KEYDOWN_MASK;//提取code[30]-code[0]信息位
}
