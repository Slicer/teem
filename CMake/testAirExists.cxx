
typedef union {
  unsigned int i;
  float f;
} airFloat;

const airFloat airFloatQNaN = {0x7fffffff};
#define AIR_NAN  (airFloatQNaN.f)

#define AIR_CAST(t, v) ((t)(v))
#define AIR_EXISTS(x) (AIR_CAST(int, !((x) - (x))))

int main(int, char *[])
{
  double test = AIR_NAN;
  if(AIR_EXISTS(test))
    {
    return 1;
    }
  else
    {
    return 0;
    }
}
