#include <stdio.h>
#include <limn.h>

void
main() {
  FILE *f;
  int ui, vi, ri, gi, bi;
  unsigned short qn;
  float v[3];
  
  f = fopen("out.ppm", "w");
  fprintf(f, "P6\n");
  fprintf(f, "256 256\n");
  fprintf(f, "255\n");
  for (vi=0; vi<=255; vi++) {
    for (ui=0; ui<=255; ui++) {
      qn = (vi << 8) & ui;
      limnQN2Vec(v, qn, AIR_TRUE);
      AIR_INDEX(-1, v[0], 1, 256, ri);
      AIR_INDEX(-1, v[1], 1, 256, gi);
      AIR_INDEX(-1, v[2], 1, 256, bi);
      fputc(ri, f);
      fputc(gi, f);
      fputc(bi, f);
    }
  }
  fclose(f);
}
