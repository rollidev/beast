/* GIMP RGBA C-Source image dump 1-byte-run-length-encoded (inport.c) */

#define INPORT_IMAGE_WIDTH (64)
#define INPORT_IMAGE_HEIGHT (64)
#define INPORT_IMAGE_BYTES_PER_PIXEL (4) /* 3:RGB, 4:RGBA */
#define INPORT_IMAGE_RLE_PIXEL_DATA ((guint8*) INPORT_IMAGE_rle_pixel_data)
#define INPORT_IMAGE_RUN_LENGTH_DECODE(image_buf, rle_data, size, bpp) do \
{ guint __bpp; guint8 *__ip; const guint8 *__il, *__rd; \
  __bpp = (bpp); __ip = (image_buf); __il = __ip + (size) * __bpp; \
  __rd = (rle_data); if (__bpp > 3) { /* RGBA */ \
    while (__ip < __il) { guint __l = *(__rd++); \
      if (__l & 128) { __l = __l - 128; \
        do { memcpy (__ip, __rd, 4); __ip += 4; } while (--__l); __rd += 4; \
      } else { __l *= 4; memcpy (__ip, __rd, __l); \
               __ip += __l; __rd += __l; } } \
  } else { /* RGB */ \
    while (__ip < __il) { guint __l = *(__rd++); \
      if (__l & 128) { __l = __l - 128; \
        do { memcpy (__ip, __rd, 3); __ip += 3; } while (--__l); __rd += 3; \
      } else { __l *= 3; memcpy (__ip, __rd, __l); \
               __ip += __l; __rd += __l; } } \
  } } while (0)
static const guint8 INPORT_IMAGE_rle_pixel_data[2778] =
("\377\0\0\0\0\377\0\0\0\0\377\0\0\0\0\202\0\0\0\0\1\34\34\377\225\275\0\0\0"
 "\0\3\32\32\377\5''\377\272%%\377\377\274\0\0\0\0\4''\377,11\377\333..\377"
 "\377&&\377\377\273\0\0\0\0\5""55\377b::\377\35488\377\377..\377\377&&\377"
 "\377\271\0\0\0\0\7,,\377\5@@\377\232HH\377\377@@\377\37788\377\377//\377\377"
 "&&\377\377\270\0\0\0\0\10""77\377\35QQ\377\301RR\377\377II\377\377@@\377\377"
 "88\377\377..\377\377\37\37\377\261\267\0\0\0\0\11@@\377-ZZ\377\350[[\377\377"
 "RR\377\377JJ\377\377@@\377\37766\377\375$$\377~\24\24\377\5\266\0\0\0\0\10"
 "YY\377\216gg\377\374dd\377\377[[\377\377RR\377\377JJ\377\377==\377\366++\377"
 "K\266\0\0\0\0\11GG\377\20ii\377\261uu\377\377mm\377\377dd\377\377[[\377\377"
 "RR\377\377@@\377\277&&\377'\266\0\0\0\0\10^^\3771zz\377\341~~\377\377vv\377"
 "\377mm\377\377dd\377\377WW\377\373>>\377v\267\0\0\0\0\10oo\377a\205\205\377"
 "\356\210\210\377\377\177\177\377\377vv\377\377mm\377\377__\377\372DD\377b"
 "\242\0\0\0\0\3\0\211\0\32\0\211\0K\0\211\0l\202\0\211\0|\3\0\211\0l\0\211"
 "\0K\0\211\0\32\214\0\0\0\0\11XX\377\6yy\377\211\226\226\377\376\220\220\377"
 "\377\210\210\377\377\177\177\377\377vv\377\377cc\377\327AA\3774\240\0\0\0"
 "\0\3\0\211\0\30\0\211\0|\0\211\0\322\210\0\211\0\377\3\0\211\0\322\0\211\0"
 "|\0\211\0\30\210\0\0\0\0\11dd\377\31\221\221\377\304\241\241\377\377\232\232"
 "\377\377\220\220\377\377\210\210\377\377~~\377\377aa\377\261::\377\12\237"
 "\0\0\0\0\3\0\211\0\22\0\211\0\214\0\211\0\366\214\0\211\0\377\3\0\211\0\366"
 "\0\211\0\214\0\211\0\22\205\0\0\0\0\11\222\222\377G\247\247\377\363\253\253"
 "\377\377\242\242\377\377\232\232\377\377\221\221\377\377\177\177\377\370Z"
 "Z\377l<<\377\5\237\0\0\0\0\2\0\211\0>\0\211\0\344\220\0\211\0\377\2\0\211"
 "\0\344\0\211\0>\203\0\0\0\0\10\227\227\377\210\270\270\377\376\264\264\377"
 "\377\253\253\377\377\242\242\377\377\232\232\377\377\201\201\377\344VV\377"
 "(\240\0\0\0\0\2\0\211\0l\0\211\0\374\222\0\211\0\377\13\0\211\0\374\0\211"
 "\0l{{\377\36\252\252\377\263\305\305\377\377\275\275\377\377\264\264\377\377"
 "\254\254\377\377\242\242\377\377\204\204\377\273OO\377\22\240\0\0\0\0\1\0"
 "\211\0l\226\0\211\0\377\10k\251\215\362\317\317\377\377\306\306\377\377\275"
 "\275\377\377\264\264\377\377\247\247\377\376\203\203\377\236RR\377\12\213"
 "\0\0\0\0\2\34\34\3776\"\"\377{\223\0\0\0\0\2\0\211\0>\0\211\0\374\226\0\211"
 "\0\377\6\2\211\3\377\234\275\301\377\306\306\377\377\276\276\377\377\246\246"
 "\377\354tt\377?\211\0\0\0\0\6..\377'99\377o99\377\32166\377\37400\377\377"
 "((\377\377\222\0\0\0\0\2\0\211\0\22\0\211\0\344\230\0\211\0\377\4\25\220\33"
 "\377\270\301\355\377\244\244\377\317pp\377(\207\0\0\0\0\11HH\377LWW\377\234"
 "QQ\377\332TT\377\377KK\377\377BB\377\377::\377\37711\377\377((\377\377\222"
 "\0\0\0\0\1\0\211\0\214\232\0\211\0\377\2=\223_\340ee\377\12\203\0\0\0\0\16"
 "LL\377\5YY\377Fss\377\232rr\377\362tt\377\376nn\377\377ff\377\377]]\377\377"
 "TT\377\377LL\377\377BB\377\377::\377\37711\377\377((\377\377\221\0\0\0\0\2"
 "\0\211\0\30\0\211\0\366\232\0\211\0\377\23\0\211\0\366\0\211\0\30cc\377\26"
 "\207\207\377|\217\217\377\263\214\214\377\365\221\221\377\377\211\211\377"
 "\377\200\200\377\377xx\377\377oo\377\377ff\377\377]]\377\377TT\377\377LL\377"
 "\377CC\377\37799\377\377++\377\342$$\377\216\221\0\0\0\0\1\0\211\0|\234\0"
 "\211\0\377\20[\236\202\375\264\264\377\377\254\254\377\377\244\244\377\377"
 "\233\233\377\377\222\222\377\377\211\211\377\377\200\200\377\377xx\377\377"
 "oo\377\377ff\377\377]]\377\377SS\377\377DD\377\275<<\377|##\377,\223\0\0\0"
 "\0\1\0\211\0\322\234\0\211\0\377\15!\222-\377\265\265\377\377\254\254\377"
 "\377\244\244\377\377\233\233\377\377\222\222\377\377\212\212\377\377\200\200"
 "\377\377rr\377\375``\377\352UU\377\300;;\3778++\377\1\225\0\0\0\0\1\0\211"
 "\0\32\236\0\211\0\377\10\243\261\345\377\255\255\377\377\244\244\377\377\232"
 "\232\377\377\201\201\377\341\201\201\377\217bb\377e>>\377\10\231\0\0\0\0\1"
 "\0\211\0K\236\0\211\0\377\4q\233\261\367\224\224\377\336||\377\202QQ\377\14"
 "\235\0\0\0\0\1\0\211\0l\236\0\211\0\377\1\0\211\0l\240\0\0\0\0\1\0\211\0|"
 "\236\0\211\0\377\1\0\211\0|\240\0\0\0\0\1\0\211\0|\236\0\211\0\377\1\0\211"
 "\0|\240\0\0\0\0\1\0\211\0l\236\0\211\0\377\1\0\211\0l\240\0\0\0\0\1\0\211"
 "\0K\236\0\211\0\377\4r\233\261\367\224\224\377\336||\377\202RR\377\14\235"
 "\0\0\0\0\1\0\211\0\32\236\0\211\0\377\10\243\261\345\377\256\256\377\377\245"
 "\245\377\377\232\232\377\377\202\202\377\341\202\202\377\217cc\377e??\377"
 "\10\232\0\0\0\0\1\0\211\0\322\234\0\211\0\377\15!\222-\377\267\267\377\377"
 "\256\256\377\377\245\245\377\377\234\234\377\377\224\224\377\377\213\213\377"
 "\377\202\202\377\377ss\377\375bb\377\352VV\377\300<<\3778,,\377\1\226\0\0"
 "\0\0\1\0\211\0|\234\0\211\0\377\20[\236\202\375\266\266\377\377\256\256\377"
 "\377\245\245\377\377\234\234\377\377\224\224\377\377\213\213\377\377\202\202"
 "\377\377yy\377\377pp\377\377hh\377\377^^\377\377UU\377\377FF\377\275>>\377"
 "|$$\377,\223\0\0\0\0\2\0\211\0\30\0\211\0\366\232\0\211\0\377\23\0\211\0\366"
 "\0\211\0\30dd\377\26\211\211\377|\220\220\377\263\216\216\377\365\223\223"
 "\377\377\213\213\377\377\202\202\377\377zz\377\377qq\377\377hh\377\377__\377"
 "\377VV\377\377NN\377\377EE\377\377;;\377\377--\377\342&&\377\216\222\0\0\0"
 "\0\1\0\211\0\214\232\0\211\0\377\2>\223_\340ff\377\12\203\0\0\0\0\16MM\377"
 "\5[[\377Fuu\377\232uu\377\362vv\377\376qq\377\377hh\377\377__\377\377VV\377"
 "\377NN\377\377EE\377\377<<\377\37733\377\377**\377\377\222\0\0\0\0\2\0\211"
 "\0\22\0\211\0\344\230\0\211\0\377\4\26\220\33\377\272\304\355\377\245\245"
 "\377\317qq\377(\207\0\0\0\0\11JJ\377LZZ\377\234TT\377\332VV\377\377NN\377"
 "\377EE\377\377<<\377\37744\377\377**\377\377\223\0\0\0\0\2\0\211\0>\0\211"
 "\0\374\226\0\211\0\377\6\2\211\3\377\236\300\301\377\311\311\377\377\300\300"
 "\377\377\251\251\377\354vv\377?\211\0\0\0\0\6""00\377';;\377o;;\377\32199"
 "\377\37433\377\377++\377\377\224\0\0\0\0\1\0\211\0l\226\0\211\0\377\10m\252"
 "\215\362\322\322\377\377\311\311\377\377\300\300\377\377\270\270\377\377\253"
 "\253\377\376\205\205\377\236SS\377\12\213\0\0\0\0\2\36\36\3776%%\377{\225"
 "\0\0\0\0\2\0\211\0l\0\211\0\374\222\0\211\0\377\13\0\211\0\374\0\211\0l||"
 "\377\36\255\255\377\263\310\310\377\377\300\300\377\377\270\270\377\377\257"
 "\257\377\377\246\246\377\377\207\207\377\273QQ\377\22\242\0\0\0\0\2\0\211"
 "\0>\0\211\0\344\220\0\211\0\377\2\0\211\0\344\0\211\0>\203\0\0\0\0\10\232"
 "\232\377\210\274\274\377\376\270\270\377\377\257\257\377\377\246\246\377\377"
 "\236\236\377\377\205\205\377\344XX\377(\242\0\0\0\0\3\0\211\0\22\0\211\0\214"
 "\0\211\0\366\214\0\211\0\377\3\0\211\0\366\0\211\0\214\0\211\0\22\205\0\0"
 "\0\0\11\226\226\377G\252\252\377\363\257\257\377\377\246\246\377\377\236\236"
 "\377\377\225\225\377\377\203\203\377\370]]\377l>>\377\5\242\0\0\0\0\3\0\211"
 "\0\30\0\211\0|\0\211\0\322\210\0\211\0\377\3\0\211\0\322\0\211\0|\0\211\0"
 "\30\210\0\0\0\0\11ff\377\31\225\225\377\304\246\246\377\377\236\236\377\377"
 "\225\225\377\377\214\214\377\377\202\202\377\377ee\377\261==\377\12\244\0"
 "\0\0\0\3\0\211\0\32\0\211\0K\0\211\0l\202\0\211\0|\3\0\211\0l\0\211\0K\0\211"
 "\0\32\214\0\0\0\0\11ZZ\377\6}}\377\211\232\232\377\376\225\225\377\377\214"
 "\214\377\377\204\204\377\377{{\377\377gg\377\327DD\3774\271\0\0\0\0\10rr\377"
 "a\212\212\377\356\214\214\377\377\204\204\377\377{{\377\377rr\377\377dd\377"
 "\372HH\377b\271\0\0\0\0\10aa\3771\177\177\377\341\204\204\377\377{{\377\377"
 "rr\377\377jj\377\377]]\377\373BB\377v\271\0\0\0\0\11JJ\377\20mm\377\261{{"
 "\377\377rr\377\377jj\377\377aa\377\377XX\377\377EE\377\277**\377'\271\0\0"
 "\0\0\10]]\377\216mm\377\374jj\377\377aa\377\377XX\377\377PP\377\377BB\377"
 "\366//\377K\271\0\0\0\0\11DD\377-__\377\350aa\377\377XX\377\377PP\377\377"
 "GG\377\377<<\377\375**\377~\27\27\377\5\270\0\0\0\0\10;;\377\35WW\377\301"
 "XX\377\377PP\377\377GG\377\377>>\377\37755\377\377$$\377\261\271\0\0\0\0\7"
 "//\377\5EE\377\232OO\377\377GG\377\377>>\377\37766\377\377--\377\377\273\0"
 "\0\0\0\5::\377bAA\377\354>>\377\37766\377\377--\377\377\274\0\0\0\0\4++\377"
 ",88\377\33366\377\377--\377\377\275\0\0\0\0\3\36\36\377\5--\377\272--\377"
 "\377\277\0\0\0\0\1\"\"\377\225\377\0\0\0\0\377\0\0\0\0\302\0\0\0\0");

