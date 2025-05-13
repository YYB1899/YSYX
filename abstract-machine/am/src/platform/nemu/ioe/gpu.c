#include <am.h>
#include <nemu.h>

#define SYNC_ADDR (VGACTL_ADDR + 4)

void __am_gpu_init() {
  int i;
  int w = inl(VGACTL_ADDR) >> 16;;  // TODO: get the correct width
  int h = inl(VGACTL_ADDR) & 0x0000ffff;;  // TODO: get the correct height
  uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
  for (i = 0; i < w * h; i ++) fb[i] = i;
  outl(SYNC_ADDR, 1);
 }

void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
  int W = inl(VGACTL_ADDR) >> 16;
  int H = inl(VGACTL_ADDR) & 0x0000ffff;
  *cfg = (AM_GPU_CONFIG_T) {
    .present = true, .has_accel = false,
    .width = W, .height = H,
    .vmemsz = W * H * sizeof(uint32_t)
  };
}

void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
  int x = ctl->x, y = ctl->y, w = ctl->w, h = ctl->h;
  if (!ctl->sync && (w == 0 || h == 0)) return;
  uint32_t *pixels = ctl->pixels; //像素数据指针
  uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;//帧缓冲区的起始地址
  uint32_t screen_w = inl(VGACTL_ADDR) >> 16;
  for (int i = y; i < y+h; i++) {
    for (int j = x; j < x+w; j++) {
      fb[screen_w*i+j]  //目标屏幕
      = pixels[w*(i-y)+(j-x)]; //源像素数据
    }
  }//将指定矩形区域的像素数据写入帧缓冲区对应位置
  if (ctl->sync) {
    outl(SYNC_ADDR, 1);
  }
}

void __am_gpu_status(AM_GPU_STATUS_T *status) {
  status->ready = true;
}
