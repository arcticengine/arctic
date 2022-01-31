#include <math.h>


#include "piComplex.h"
/*
void CNB_Set( CNB *z, float x, float y )
{
    z->x = x;
    z->y = y;
}

void CNB_Scale( CNB *z, float s )
{
    z->x *= s;
    z->y *= s;
}

void CNB_IncReal( CNB *z, float x )
{
    z->x += x;
}

void CNB_IncImag( CNB *z, float y )
{
    z->y += y;
}

void CNB_Inc( CNB *z, CNB *b )
{
    z->x += b->x;
    z->y += b->y;
}

void CNB_Dec( CNB *z, CNB *b )
{
    z->x -= b->x;
    z->y -= b->y;
}

void CNB_Sub( CNB *dst, CNB *a, CNB *b )
{
    dst->x = a->x - b->x;
    dst->y = a->y - b->y;
}

void CNB_Add( CNB *dst, CNB *a, CNB *b )
{
    dst->x = a->x + b->x;
    dst->y = a->y + b->y;
}

float CNB_Mod( CNB *z )
{
    return sqrtf( z->x*z->x + z->y*z->y );
}

float CNB_Mod2( CNB *z )
{
    return z->x*z->x + z->y*z->y;
}

float CNB_GetReal( CNB *z )
{
    return z->x;
}

float CNB_GetImag( CNB *z )
{
    return z->y;
}

void CNB_Sqrt( CNB *dst, CNB *z )
{
    float m2 = CNB_Mod( z );
    float x  = CNB_GetReal( z );

    CNB_Set( dst,  sqrtf( .5f*(m2+x) ), 
                  -sqrtf( .5f*(m2-x) ) );


        
}

void CNB_Conj( CNB *dst, CNB *z )
{
    dst->x = z->x;
    dst->y = -z->y;
}

void CNB_Squared( CNB *dst, CNB *z )
{
    dst->x = z->x*z->x - z->y*z->y;
    dst->y = 2.0f*z->x*z->y;
}


void CNB_Mul( CNB *dst, CNB *a, CNB *b )
{
    dst->x = a->x*b->x - a->y*b->y;
    dst->y = a->x*b->y + a->y*b->x;
}

// res = a/b
void CNB_Div( CNB *dst, CNB *a, CNB *b )
{
    float d = b->x*b->x + b->y*b->y;

    d = 1.0f/d;

    dst->x = ( a->x*b->x + a->y*b->y)*d;
    dst->y = (-a->x*b->y + a->y*b->x)*d;
}
*/