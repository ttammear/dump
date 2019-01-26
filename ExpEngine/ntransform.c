
static inline bool seq16_isnewer(uint16_t a, uint16_t b)
{
    return (b > a && b-a < 0xFFFF/2) || (b < a && a-b >= 0xFFFF/2);
}

static inline int seq16_sdif(uint16_t a, uint16_t b)
{
    if(b >= a && b-a < 0xFFFF/2)
        return b - a;
    else if (a > b && a - b < 0xFFFF/2)
        return (int)a - (int)b;
    else if(b > a)
        return -(0xFFFF - b + a);
    return 0xFFFF - a + b;
}

#define ABS(x) ((x)<0?-(x):(x))

void transform_init(NetworkedTransform *t, V3 pos, Quat rot)
{
    t->fromSeq = 65534;
    t->progress = 0.0;
    t->seqs[65534%NTRANS_BUF_SIZE] = 65534;
    t->seqs[65535%NTRANS_BUF_SIZE] = 65535;
    t->seqs[0] = 0;
    t->seqs[1] = 1;
    for(int i = 0; i < ARRAY_COUNT(t->positions); i++)
    {
        t->positions[i] = pos;
        t->rotations[i] = rot;
    }
} 

void transform_update(NetworkedTransform *t, uint16_t seq, uint16_t frameId, V3 position, Quat rotation)
{
    int dif = seq16_sdif(t->fromSeq, seq);
    if(dif < 0)
    {
        printf("Transform update too old! %hu but current %hu\n", seq, t->fromSeq);
        return;
    }
    else if(dif > 2)
    {
        uint16_t nseq = seq - 2;
        printf("Transform update newer than expected, winding forward! %hu to %hu\n", t->fromSeq, nseq);
        t->fromSeq = seq - 2;
        t->progress = 0.5;
    }
    int index = seq%NTRANS_BUF_SIZE;
    t->positions[index] = position;
    t->rotations[index] = rotation;
    t->seqs[index] = seq;
    //printf("receive %hu\n", seq);
}

void transform_advance(NetworkedTransform *t, double dt, uint16_t frameId)
{
    const double rate = 10.0;
    if(t->progress >= 1.0)
    {
        //printf("%u -> %u\n", t->fromSeq, t->fromSeq+1);
        t->fromSeq++;
        t->progress -= 1.0;
    }
    else if(t->progress < 0.0)
    {
        t->fromSeq--;
        t->progress += 1.0;
    }
    double progress = dt*rate;
    t->progress += progress;
}

void transform_get(NetworkedTransform *t, V3 *pos, Quat *rot)
{
    V3 startPos = t->positions[t->fromSeq%NTRANS_BUF_SIZE];
    V3 endPos = t->positions[(t->fromSeq+1)%NTRANS_BUF_SIZE];
    if(t->seqs[t->fromSeq%NTRANS_BUF_SIZE] != t->seqs[(t->fromSeq+1)%NTRANS_BUF_SIZE]-1)
    {
        static int last;
        if(last != t->seqs[(t->fromSeq+1)%NTRANS_BUF_SIZE])
        {
            last = t->seqs[(t->fromSeq+1)%NTRANS_BUF_SIZE];
            t->progress -= 0.1;
            //printf("Seq missing %hu %hu\n", t->seqs[t->fromSeq%NTRANS_BUF_SIZE], t->seqs[(t->fromSeq+1)%NTRANS_BUF_SIZE]);
        }
    }
    *pos = v3_lerp(startPos, endPos, t->progress); 
    quat_identity(rot);
}
