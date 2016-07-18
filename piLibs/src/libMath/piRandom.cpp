#define IM1 2147483563
#define IM2 2147483399
#define IMM1 (IM1-1)
#define IA1 40014
#define IA2 40692
#define IQ1 53668
#define IQ2 52774
#define IR1 12211
#define IR2 3791
#define NTAB 32
#define NDIV (1+IMM1/NTAB)

//=========================================================================

/*
Long period (> 2 × 1018) random number generator of L’Ecuyer with Bays-Durham shuffle
and added safeguards. Returns a uniform random deviate between 0.0 and 1.0 (exclusive of
the endpoint values). Call with idum a negative integer to initialize; thereafter, do not alter
idum between successive deviates in a sequence. RNMX should approximate the largest floating
value that is less than 1.
*/

static long idum2=123456789;
static long iy=0;
static long iv[NTAB];

void nrcp_rand2_init( long *idum )
{
    int j;
    long    k;

    if (-(*idum) < 1) 
        *idum=1;       //             Be sure to prevent idum = 0.
    else
        *idum = -(*idum);
    idum2=(*idum);
    for (j=NTAB+7;j>=0;j--)
        {        //             Load the shuffle table (after 8 warm-ups).
        k=(*idum)/IQ1;
        *idum=IA1*(*idum-k*IQ1)-k*IR1;
        if (*idum < 0) *idum += IM1;
        if (j < NTAB) iv[j] = *idum;
        }
    iy=iv[0];
}

long nrcp_rand2_int31( long *idum )
{
    int j;
    long k;


    k=(*idum)/IQ1;                   // Start here when not initializing.
    *idum=IA1*(*idum-k*IQ1)-k*IR1;   // Compute idum=(IA1*idum) % IM1 without
                                    // overflows by Schrage’s method. if (*idum < 0) *idum += IM1;
    k=idum2/IQ2;
    idum2=IA2*(idum2-k*IQ2)-k*IR2;    //     Compute idum2=(IA2*idum) % IM2 likewise.
    if (idum2 < 0) 
        idum2 += IM2;
    j=iy/NDIV;                        //     Will be in the range 0..NTAB-1.
    iy=iv[j]-idum2;                   //     Here idum is shuffled, idum and idum2 are
                                       //combined to generate output.
    iv[j] = *idum;
    if (iy < 1) 
        iy += IMM1;

    return iy;
}


//=========================================================================

/*
   A C-program for MT19937, with initialization improved 2002/1/26.
   Coded by Takuji Nishimura and Makoto Matsumoto.

   Before using, initialize the state by using init_genrand(seed)
   or init_by_array(init_key, key_length).

   Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

     3. The names of its contributors may not be used to endorse or promote
        products derived from this software without specific prior written
        permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


   Any feedback is very welcome.
   http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html
   email: m-mat @ math.sci.hiroshima-u.ac.jp (remove space)
*/

/* Period parameters */
#define N 624
#define M 397
#define MATRIX_A 0x9908b0dfUL   /* constant vector a */
#define UPPER_MASK 0x80000000UL /* most significant w-r bits */
#define LOWER_MASK 0x7fffffffUL /* least significant r bits */

static unsigned long mt[N]; /* the array for the state vector  */
static int mti=N+1; /* mti==N+1 means mt[N] is not initialized */

/* initializes mt[N] with a seed */
void mt19937_init(unsigned long s)
{
    mt[0]= s & 0xffffffffUL;
    for (mti=1; mti<N; mti++) {
        mt[mti] = 
	    (1812433253UL * (mt[mti-1] ^ (mt[mti-1] >> 30)) + mti); 
        /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
        /* In the previous versions, MSBs of the seed affect   */
        /* only MSBs of the array mt[].                        */
        /* 2002/01/09 modified by Makoto Matsumoto             */
        mt[mti] &= 0xffffffffUL;
        /* for >32 bit machines */
    }
}

/* initialize by an array with array-length */
/* init_key is the array for initializing keys */
/* key_length is its length */
/* slight change for C++, 2004/2/26 */
void mt19937_init_by_array(unsigned long init_key[], int key_length)
{
    int i, j, k;
    mt19937_init(19650218UL);
    i=1; j=0;
    k = (N>key_length ? N : key_length);
    for (; k; k--) {
        mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1664525UL))
          + init_key[j] + j; /* non linear */
        mt[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
        i++; j++;
        if (i>=N) { mt[0] = mt[N-1]; i=1; }
        if (j>=key_length) j=0;
    }
    for (k=N-1; k; k--) {
        mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1566083941UL))
          - i; /* non linear */
        mt[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
        i++;
        if (i>=N) { mt[0] = mt[N-1]; i=1; }
    }

    mt[0] = 0x80000000UL; /* MSB is 1; assuring non-zero initial array */ 
}

/* generates a random number on [0,0xffffffff]-interval */
unsigned long mt19937_int32(void)
{
    unsigned long y;
    static unsigned long mag01[2]={0x0UL, MATRIX_A};
    /* mag01[x] = x * MATRIX_A  for x=0,1 */

    if (mti >= N) { /* generate N words at one time */
        int kk;

        if (mti == N+1)   /* if init_genrand() has not been called, */
            mt19937_init(5489UL); /* a default initial seed is used */

        for (kk=0;kk<N-M;kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        for (;kk<N-1;kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        y = (mt[N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
        mt[N-1] = mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1UL];

        mti = 0;
    }
  
    y = mt[mti++];

    /* Tempering */
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);

    return y;
}
/*
// generates a random number on [0,0x7fffffff]-interval 
long mt19937_int31(void)
{
    return (long)(mt19937_int32()>>1);
}

// generates a random number on [0,1]-real-interval 
double genrand_real1(void)
{
    return mt19937_int32()*(1.0/4294967295.0);      // divided by 2^32-1 
}

// generates a random number on [0,1)-real-interval 
double genrand_real2(void)
{
    return mt19937_int32()*(1.0/4294967296.0);      // divided by 2^32
}

// generates a random number on (0,1)-real-interval 
double genrand_real3(void)
{
    return (((double)mt19937_int32()) + 0.5)*(1.0/4294967296.0);      // divided by 2^32
}
*/
// generates a random number on [0,1) with 53-bit resolution
double mt19937_dou53(void) 
{ 
    const unsigned long a = mt19937_int32()>>5;
    const unsigned long b = mt19937_int32()>>6;
    return(a*67108864.0+b)*(1.0/9007199254740992.0); 
} 


//=========================================================================

unsigned int pirand_int15b( int *mirand )
{
    mirand[0] = mirand[0]*0x343fd+0x269ec3;
    return( (mirand[0]>>16)&32767 );
}


float pirand_flo23b( int *mirand )
{
    mirand[0] = mirand[0]*0x343fd+0x269ec3;

    unsigned int a = (unsigned int)mirand[0];
    a = (a>>9) | 0x3f800000;
    float res = *((float*)&a);
    res -= 1.0f;
    return( res );
}



float pirand_sflo23b( int *mirand )
{
    mirand[0] = mirand[0]*0x343fd+0x269ec3;

    unsigned int a = (unsigned int)mirand[0];
    a = (a>>9) | 0x40000000;
    float res = *((float*)&a);
    res -= 3.0f;
    return( res );
}