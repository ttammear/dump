
#define FLAGSET(var, flag) ((var & flag) != 0)
#define ARRAY_COUNT(x) (sizeof(x) / sizeof(x[0]))

// floor without overflow protection
#define USFLOOR(x) (((int)(x)) - (((int)(x)) > (x)))
#define MIN(x, y) ((x) < (y) ? (x) : (y))
