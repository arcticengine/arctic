#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "../../kernel/file.h"
#include "image.h"

#define SIGNATURE   (('F'<<24)|('W'<<16)|('A'<<8)|('R'))

int RAW_Load( IMAGE *bmp, const char *name )
{
	FILE	*fp;
	int		amount, magic;

	fp = FILE_Open( name, "rb" );
	if( !fp )
		return( 0 );

	magic = read_int32(fp);
	if( magic!=SIGNATURE )
		{fclose( fp );return( 0 );}

	bmp->xres   = read_int32(fp);
	bmp->yres   = read_int32(fp);
	bmp->zres   = read_int32(fp);
	bmp->format = read_int32(fp);

	if( !IMAGE_Make( bmp, bmp->xres, bmp->yres, bmp->zres, bmp->format ) )
		{fclose( fp );return( 0 );}

	amount = bmp->xres*bmp->yres*bmp->zres;

	switch( bmp->format )
		{
		case IMAGE_FORMAT_I_GREY:
			read_int8array( fp, (unsigned char*)bmp->data, amount );
			break;

		case IMAGE_FORMAT_I_RGBA:
			read_int32array( fp, (unsigned int*)bmp->data, amount );
			break;
		case IMAGE_FORMAT_F_RGBA:
			read_int32array( fp, (unsigned int*)bmp->data, 4*amount );
			break;

		case IMAGE_FORMAT_F_RGB:
			read_int32array( fp, (unsigned int*)bmp->data, 3*amount );
			break;

		default:
			fclose(fp);
			return(0);
		}

	fclose( fp );
	return( 1 );
}

int RAW_Save( IMAGE *bmp, const char *name )
{
	FILE *fp;
	int	amount;

	fp = FILE_Open( name, "wb" );
	if( !fp )
		return( 0 );

	write_int32( fp, SIGNATURE );
	write_int32( fp, bmp->xres );
	write_int32( fp, bmp->yres );
	write_int32( fp, bmp->zres );
	write_int32( fp, bmp->format );

	amount = bmp->xres*bmp->yres*bmp->zres;

	switch( bmp->format )
		{
		case IMAGE_FORMAT_I_GREY:
			write_int8array( fp, (uint8*)bmp->data, amount );
			break;

		case IMAGE_FORMAT_I_RGBA:
			write_int32array( fp, (unsigned int*)bmp->data, 1*amount );
			break;

		case IMAGE_FORMAT_F_RGBA:
			write_int32array( fp, (unsigned int*)bmp->data, 4*amount );
			break;

		case IMAGE_FORMAT_F_RGB:
			write_int32array( fp, (unsigned int*)bmp->data, 3*amount );
			break;

		case IMAGE_FORMAT_F_GREY:
			write_int32array( fp, (unsigned int*)bmp->data, 1*amount );
			break;

		default:
			fclose(fp);
			return(0);
		}

	fclose( fp );
	return( 1 );
}