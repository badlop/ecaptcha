// http://github.com/ITikhonov/captcha
// https://github.com/huacnlee/rucaptcha
#include <stdint.h>
#include <string.h>
#include "erl_nif.h"
#include "font.h"

static int8_t *lt[];

static const int8_t sw[200]=
    {
     0, 4, 8, 12, 16, 20, 23, 27, 31, 35, 39, 43, 47, 50, 54, 58, 61, 65, 68, 71, 75, 78, 81, 84,
     87, 90, 93, 96, 98, 101, 103, 105, 108, 110, 112, 114, 115, 117, 119, 120, 121, 122, 123, 124,
     125, 126, 126, 127, 127, 127, 127, 127, 127, 127, 126, 126, 125, 124, 123, 122, 121, 120, 119,
     117, 115, 114, 112, 110, 108, 105, 103, 101, 98, 96, 93, 90, 87, 84, 81, 78, 75, 71, 68, 65,
     61, 58, 54, 50, 47, 43, 39, 35, 31, 27, 23, 20, 16, 12, 8, 4, 0, -4, -8, -12, -16, -20, -23,
     -27, -31, -35, -39, -43, -47, -50, -54, -58, -61, -65, -68, -71, -75, -78, -81, -84, -87, -90,
     -93, -96, -98, -101, -103, -105, -108, -110, -112, -114, -115, -117, -119, -120, -121, -122,
     -123, -124, -125, -126, -126, -127, -127, -127, -127, -127, -127, -127, -126, -126, -125,
     -124, -123, -122, -121, -120, -119, -117, -115, -114, -112, -110, -108, -105, -103, -101, -98,
     -96, -93, -90, -87, -84, -81, -78, -75, -71, -68, -65, -61, -58, -54, -50, -47, -43, -39, -35,
     -31, -27, -23, -20, -16, -12, -8, -4};


#define MAX(x,y) ((x>y)?(x):(y))

static int letter(unsigned char n, int pos, unsigned char im[70*200], unsigned char swr[200],
                  uint8_t s1, uint8_t s2) {
  int8_t *p=lt[n];
  unsigned char *r=im+200*16+pos;
  unsigned char *i=r;
  int sk1=s1+pos;
  int sk2=s2+pos;
  int mpos=pos;
  int row=0;
  for(;*p!=-101;p++) {
    if(*p<0) {
      if(*p==-100) { r+=200; i=r; sk1=s1+pos; row++; continue; }
      i+=-*p;
      continue;
    }

    if(sk1>=200) sk1=sk1%200;
    int skew=sw[sk1]/16;
    sk1+=(swr[pos+i-r]&0x1)+1;

    if(sk2>=200) sk2=sk2%200;
    int skewh=sw[sk2]/70;
    sk2+=(swr[row]&0x1);

    unsigned char *x=i+skew*200+skewh;
    mpos=MAX(mpos,pos+i-r);

    if((x-im)<70*200) *x=(*p)<<4;
    i++;
  }
  return mpos + 3;
}

static void line(unsigned char im[70*200], unsigned char swr[200], uint8_t s1) {
  int x;
  int sk1=s1;
  for(x=0;x<199;x++) {
    if(sk1>=200) sk1=sk1%200;
    int skew=sw[sk1]/20;
    sk1+=swr[x]&(0x3+1);
    unsigned char *i= im+(200*(45+skew)+x);
    i[0]=0; i[1]=0; i[200]=0; i[201]=0;
  }
}

#define NDOTS 200

static void dots(unsigned char im[70*200], uint32_t* dr) {
  int n;
  for(n=0;n<NDOTS;n++) {
    uint32_t v=dr[n];
    unsigned char *i=im+v%(200*67);

    i[0]=0xff;
    i[1]=0xff;
    i[2]=0xff;
    i[200]=0xff;
    i[201]=0xff;
    i[202]=0xff;
  }
}

static void blur(unsigned char im[70*200]) {
  unsigned char *i=im;
  int x,y;
  for(y=0;y<68;y++) {
    for(x=0;x<198;x++) {
      unsigned int c11=*i,c12=i[1],c21=i[200],c22=i[201];
      *i++=((c11+c12+c21+c22)/4);
    }
  }
}

static void filter(unsigned char im[70*200]) {
  unsigned char om[70*200];
  unsigned char *i=im;
  unsigned char *o=om;

  memset(om,0xff,sizeof(om));

  int x,y;
  for(y=0;y<70;y++) {
    for(x=4;x<200-4;x++) {
      if(i[0]>0xf0 && i[1]<0xf0) { o[0]=0; o[1]=0; }
      else if(i[0]<0xf0 && i[1]>0xf0) { o[0]=0; o[1]=0; }

      i++;
      o++;
    }
  }

  memmove(im,om,sizeof(om));
}

static void captcha(const unsigned char* rand, unsigned char im[70*200], const unsigned char* l,
                    int length, int i_line, int i_blur, int i_filter, int i_dots) {
  unsigned char swr[200];
  uint8_t s1,s2;
  uint32_t dr[NDOTS];

  memcpy(swr, rand, 200);
  memcpy(dr, rand+=200, sizeof(dr));
  memcpy(&s1, rand+=sizeof(dr), 1);
  memcpy(&s2, rand+=1, 1);
  memset(im,0xff,200*70);
  s1=s1&0x7f;
  s2=s2&0x3f;

  int x;
  int p=30;
  for(x=0;x<length;x++){
    p=letter(l[x]-'a',p,im,swr,s1,s2);
  }

  if (i_line == 1) {
    line(im,swr,s1);
  }
  if (i_dots == 1) {
    dots(im, dr);
  }
  if (i_blur == 1) {
    blur(im);
  }
  if (i_filter == 1) {
    filter(im);
  }
}


/* ERLANG */


static ERL_NIF_TERM
mk_atom(ErlNifEnv* env, const char* atom)
{
    ERL_NIF_TERM ret;

    if(!enif_make_existing_atom(env, atom, &ret, ERL_NIF_LATIN1))
    {
        return enif_make_atom(env, atom);
    }

    return ret;
}

static ERL_NIF_TERM
mk_error(ErlNifEnv* env, const char* mesg)
{
    return enif_make_tuple2(env, mk_atom(env, "error"), mk_atom(env, mesg));
}

static ERL_NIF_TERM
mk_pixels(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    ERL_NIF_TERM opts_head, opts_tail, img_data_bin;
    ErlNifBinary chars_bin, rand_bin;
    int i_line = 0, i_blur = 0, i_filter = 0, i_dots = 0;
    char opt_name[8];

    unsigned char* img;

    if( argc != 3 ) {
        return enif_make_badarg(env);
    }

    if(!enif_inspect_binary(env, argv[0], &chars_bin)) {
        return mk_error(env, "chars_not_binary");
    }
    if(chars_bin.size < 1 || chars_bin.size > 7) {
        return mk_error(env, "wrong_chars_length");
    }
    for(int i = 0; i < chars_bin.size; i++) {
        if (chars_bin.data[i] > 'z' || chars_bin.data[i] < 'a') {
            return mk_error(env, "invalid_character");
        }
    }

    if(!enif_inspect_binary(env, argv[1], &rand_bin)) {
        return mk_error(env, "bad_random");
    }
    if(rand_bin.size < (200 + NDOTS * sizeof(uint32_t) + 2)) {
        return mk_error(env, "small_rand_binary");
    }

    if(!enif_is_list(env, argv[2])) {
        return mk_error(env, "opts_not_list");
    }

    opts_tail = argv[2];
    while (enif_get_list_cell(env, opts_tail, &opts_head, &opts_tail)) {
        if(!enif_get_atom(env, opts_head, opt_name, sizeof(opt_name), ERL_NIF_LATIN1)) {
            return mk_error(env, "non_atom_option");
        }
        if (!strcmp(opt_name, "line")) {
            i_line = 1;
        } else if (!strcmp(opt_name, "blur")) {
            i_blur = 1;
        } else if (!strcmp(opt_name, "filter")) {
            i_filter = 1;
        } else if (!strcmp(opt_name, "dots")) {
            i_dots = 1;
        } else {
            return mk_error(env, "unknown_option");
        }
    }
    img = enif_make_new_binary(env, 70*200, &img_data_bin);

    captcha(rand_bin.data, img, chars_bin.data, chars_bin.size, i_line, i_blur, i_filter, i_dots);
    return img_data_bin;
}

static ErlNifFunc nif_funcs[] = {
    {"pixels", 3, mk_pixels}
};

ERL_NIF_INIT(ecaptcha_nif, nif_funcs, NULL, NULL, NULL, NULL);
