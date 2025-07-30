/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <common.h>
#include <device/map.h>
#include <SDL2/SDL.h>

enum {
  reg_freq,
  reg_channels,
  reg_samples,
  reg_sbuf_size,
  reg_init,
  reg_count,
  nr_reg
};

static uint8_t *sbuf = NULL; // 流缓冲区指针
static uint32_t *audio_base = NULL; // 寄存器基址指针
static uint32_t sbuf_pos = 0; // 当前写入位置

void sdl_audio_callback(void *userdata, uint8_t *stream, int len){ //音频系统需要更多数据时调用此函数
  SDL_memset(stream, 0, len); //清空输出缓冲区
  uint32_t used_cnt = audio_base[reg_count]; //确定可用的数据量
  len = len > used_cnt ? used_cnt : len;
  uint32_t sbuf_size = audio_base[reg_sbuf_size];
  if( (sbuf_pos + len) > sbuf_size ){ //当前读取位置加上要读的数据长度是否会超出缓冲区大小
    SDL_MixAudio(stream, sbuf + sbuf_pos, sbuf_size - sbuf_pos , SDL_MIX_MAXVOLUME);//第一次从当前位置读到缓冲区末尾
    SDL_MixAudio(stream +  (sbuf_size - sbuf_pos), sbuf +  (sbuf_size - sbuf_pos), len - (sbuf_size - sbuf_pos), SDL_MIX_MAXVOLUME);//第二次从缓冲区开头继续读剩余部分
  }
  else SDL_MixAudio(stream, sbuf + sbuf_pos, len , SDL_MIX_MAXVOLUME);
  sbuf_pos = (sbuf_pos + len) % sbuf_size; //更新下次读取的位置
  audio_base[reg_count] -= len;
}

void init_sound() { //初始化 SDL音频子系统并开始播放音频
  SDL_AudioSpec s = {};
  s.format = AUDIO_S16SYS;  // 假设系统中音频数据的格式总是使用16位有符号数来表示
  s.userdata = NULL;        // 不使用
  s.freq = audio_base[reg_freq];
  s.channels = audio_base[reg_channels];
  s.samples = audio_base[reg_samples];
  s.callback = sdl_audio_callback;
  SDL_InitSubSystem(SDL_INIT_AUDIO);
  SDL_OpenAudio(&s, NULL);
  SDL_PauseAudio(0);       //播放，可以执行音频子系统的回调函数
}

static void audio_io_handler(uint32_t offset, int len, bool is_write) { //对音频控制寄存器的读写请求时被调用
  if(audio_base[reg_init]==1){
    init_sound();
    audio_base[reg_init] = 0;
  }
}

void init_audio() { //初始化音频设备
  uint32_t space_size = sizeof(uint32_t) * nr_reg;
  audio_base = (uint32_t *)new_space(space_size);
#ifdef CONFIG_HAS_PORT_IO
  add_pio_map ("audio", CONFIG_AUDIO_CTL_PORT, audio_base, space_size, audio_io_handler);
#else
  add_mmio_map("audio", CONFIG_AUDIO_CTL_MMIO, audio_base, space_size, audio_io_handler);
#endif

  sbuf = (uint8_t *)new_space(CONFIG_SB_SIZE);
  add_mmio_map("audio-sbuf", CONFIG_SB_ADDR, sbuf, CONFIG_SB_SIZE, NULL);
  audio_base[reg_sbuf_size] = CONFIG_SB_SIZE;
}
