#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...) {
  //panic("Not implemented");
  va_list ap;//声明一个可变参数列表 ap
  va_start(ap,fmt);//初始化 ap，使其指向第一个可变参数fmt
  char buffer[2048];
  unsigned int i = vsprintf(buffer,fmt,ap);
  putstr(buffer);//字符串buffer输出到标准输出设备
  va_end(ap);//清理可变参数列表 ap
  return i;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  //panic("Not implemented");
  int len = 0;
  while(*fmt != '\0'){
  	if(*fmt != '%'){//当前字符不是 %
  		*out = *fmt;
  		out ++;
  		fmt ++;
  		len ++;
  		continue;
  	}//当前字符是 %
  	fmt ++;//跳过 %，并检查下一个字符
  	switch(*fmt){
  		case 's':{
  			char* sp = va_arg(ap, char*);//获取一个指向字符串ap的指针sp
  			while(*sp != '\0'){
				*out = *sp;
				out ++;
				sp ++;
				len ++;
			}
			break;
  		}
  		case 'd':{
  			int num = va_arg(ap,int);
  			if(num < 0){
  				*out = '-';
  				out ++;
  				len ++;
  				num = - num;
  			}
  			if(num == 0){
  				*out = '0';
  				out ++;
  				len ++;
  				break;
  			}
  			char numb[12];
  			int i = 0;
  			while(num != 0){//正向数字转换为字符串
  				numb[i] = '0' + num % 10;//32:numb[0] = 2;numb[1] = 3 i = 2
  				num /= 10;
  				i ++;
  			}
  			while(i > 0){//反向复制字符到输出缓冲区
  				i --;
				*out = numb[i];
				out ++;
				len ++;
			}
			break;
  		}
  	}
  	fmt ++;
  }
  *out = '\0';//添加字符串结束符 \0
  len ++;
  return len;
}

int sprintf(char *out, const char *fmt, ...) {
  //panic("Not implemented");
  va_list ap;//声明一个可变参数列表 ap
  va_start(ap,fmt);//初始化 ap，使其指向第一个可变参数fmt
  unsigned int i = vsprintf(out,fmt,ap);
  va_end(ap);//清理可变参数列表 ap
  return i;
}

//int snprintf(char *out, size_t n, const char *fmt, ...) {
  //panic("Not implemented");
//}

//int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  //panic("Not implemented");
//}

#endif
