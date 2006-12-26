#include <Afx.h>

#include  <math.h>
#include  <stdlib.h>
#include  <stdio.h>

//typedef struct  {float x,y;}   complex;

#include  "../include/proto.h"
//#include  "fft.h"

static  int     N = -1;
static  complex *zeta;

//#define TWOPI   (2.0 * M_PI)

void fft(complex c[],int n)
   {
   int          i,l,j,p,q,n1,km,k,k1,k2,m;
   complex      swap,eta;
   double       scale;

   m = n>>1;

   /* calculate sine look-up table */

   if(n != N)           /* if sine table never before calculated */
      dolookup(n);

   n1 = n - 1;
   p = int((log((double)n) / log(2.0)) + 0.5);
   q = p - 1;

   for(l=0; l<p; l++)
      {
      for(k=0; k<n1; k+=m)
	for(j=1; j<=m; j++,k++)
	   {
	   for(i=1,k2=(k1=k>>q)&1; i<p; i++)    k2 = (k2<<1) + ((k1>>=1) & 1);
	   km = k + m;
	   eta.x = c[km].x * zeta[k2].x + c[km].y * zeta[k2].y;
	   eta.y =-c[km].x * zeta[k2].y + c[km].y * zeta[k2].x;
	   c[km].x = c[k].x - eta.x;
	   c[km].y = c[k].y - eta.y;
	   c[k].x += eta.x;
	   c[k].y += eta.y;
	   }
      m >>= 1;
      q--;
      }

   for(k=0; k<n1; k++)
      {
      for(i=1,j=(k1=k)&1; i<p; i++)     j = (j<<1) + ((k1>>=1) & 1);
      if (j>k)
	{
	swap.x = c[j].x;        swap.y = c[j].y;
	c[j].x = c[k].x;        c[j].y = c[k].y;
	c[k].x = swap.x;        c[k].y = swap.y;
	}
      }
   
   scale = 1.0 / (double)n;
   n >>= 1;
   for(i=0; i<n; i++)
      {
      swap.x = float(c[i].x * scale);
      swap.y = float(c[i].y * scale);
      c[i].x = float(c[i+n].x * scale);
      c[i].y = float(c[i+n].y * scale);
      c[i+n].x = swap.x;
      c[i+n].y = swap.y;
      }
   
   }

void arcfft(complex c[], int n)
   {
   int          i,l,j,p,q,m,n1,km,k,k1,k2;
   double       scale;
   complex      swap,eta;

   m = n>>1;

   /* calculate sine look-up table */

   if(n != N)           /* if sine table never before calculated */
      dolookup(n);

   n1 = n - 1;
   p = int((log((double)n) / log(2.0)) + 0.5);
   q = p - 1;

   for(l=0; l<p; l++)
      {
      for(k=0; k<n1; k+=m)
	for(j=1; j<=m; j++,k++)
	   {
	   for(i=1,k2=(k1=k>>q)&1; i<p; i++)    k2 = (k2<<1) + ((k1>>=1) & 1);
	   km = k + m;
	   eta.x = c[km].x * zeta[k2].x - c[km].y * zeta[k2].y;
	   eta.y = c[km].x * zeta[k2].y + c[km].y * zeta[k2].x;
	   c[km].x = c[k].x - eta.x;
	   c[km].y = c[k].y - eta.y;
	   c[k].x += eta.x;
	   c[k].y += eta.y;
	   }
      m >>= 1;
      q--;
      }

   for(k=0; k<n1; k++)
      {
      for(i=1,j=(k1=k)&1; i<p; i++)     j = (j<<1) + ((k1>>=1) & 1);
      if (j>k)
	{
	swap.x = c[j].x;        swap.y = c[j].y;
	c[j].x = c[k].x;        c[j].y = c[k].y;
	c[k].x = swap.x;        c[k].y = swap.y;
	}
      }
   
   scale = 1.0 / (double)n;
   for(i=0; i<n; i++)
      {
		c[i].x *= float(scale);
		c[i].y *= float(scale);
      }
   }

void hann(complex a[], int n)
   {
   int          i;
   double       scale;

   if(n != N)           /* if sine table never before calculated */
      dolookup(n);

   for(i=0; i<n; i++)
      {
      a[i].x *= float((scale = 1.0 - zeta[i].x));
      a[i].y *= float(scale);
      }
   }

void hamm(complex a[], int n)
   {
   int          i;
   double       scale;

   if(n != N)           /* if sine table never before calculated */
      dolookup(n);
   
   for(i=0; i<n; i++)
      {
      a[i].x *= float((scale = 1.08 - 0.92 * zeta[i].x));
      a[i].y *= float(scale);
      }
   }
      
void dolookup(int    n)      
   {
   int          i,j,k,l,t,u,r,v,w;
   double       arg,scale;

   if(N != -1)       free(zeta);     /* if not first table, release mem */
   if(!(zeta = (complex *)malloc((n + 1) * sizeof(complex))))
     {printf("not enough memory for sine table\n");  exit(0);}
   N = n;
   scale = 1.0 / (double)n;
   arg = TWOPI * scale;
   r = n>>2;
      
   /* compute the lookup table */
   /* note: lookup table is scaled by TWOPI / n so result is scaled */
   for(i=0,j=k=n>>1,l=n,t=u=r,v=w=r*3; i<=r; i++,j--,k++,l--,t--,u++,v--,w++)
      {
       zeta[u].x = zeta[v].x = zeta[l].y = zeta[k].y =
     -(zeta[t].x = zeta[w].x = zeta[j].y = zeta[i].y = 
      float(sin(arg*i)));
      }
   }
