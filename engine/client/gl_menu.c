/*
gl_draw.c - draw functions for menu
Copyright (C) 2022 Mr0maks

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef XASH_DEDICATED

#include "common.h"
#include "client.h"
#include "gl_local.h"

/*
=============
R_DrawLine
=============
*/
void R_DrawLine( float x, float y, float x2, float y2 )
{
	pglBegin( GL_LINES );
		pglVertex2f( x, y );
		pglVertex2f( x2, y2 );
	pglEnd();
}

/*
=============
R_DrawTriangle
=============
*/
void R_DrawTriangle( float x, float y, float x2, float y2, float x3, float y3 )
{
	pglBegin( GL_TRIANGLES );
		pglVertex2f( x, y );
		pglVertex2f( x2, y2 );
		pglVertex2f( x3, y3 );
	pglEnd();
}

/*
=============
R_DrawFillRectangle
=============
*/
void R_DrawFillTriangle( float x, float y, float x2, float y2, float x3, float y3, float s1, float t1, float s2, float t2, float s3, float t3, HIMAGE texnum )
{
	GL_Bind(XASH_TEXTURE0, texnum);

	pglBegin( GL_TRIANGLES );
		pglTexCoord2f( s1, t1 );
		pglVertex2f( x, y );
		pglTexCoord2f( s2, t2 );
		pglVertex2f( x2, y2 );
		pglTexCoord2f( s3, t3 );
		pglVertex2f( x3, y3 );
	pglEnd();
}

/*
=============
R_DrawFillGadientRectangle
=============
*/
void R_DrawFillGadientTriangle( float x, float y, float x2, float y2, float x3, float y3, byte colors[12] )
{
	pglShadeModel( GL_SMOOTH );

	pglBegin( GL_TRIANGLES );
		pglColor4ub( colors[0], colors[1], colors[2], colors[3]);
		pglVertex2f( x, y );
		pglColor4ub( colors[4], colors[5], colors[6], colors[7]);
		pglVertex2f( x2, y2 );
		pglColor4ub( colors[8], colors[9], colors[10], colors[11]);
		pglVertex2f( x3, y3 );
	pglEnd();

	pglShadeModel( GL_FLAT );
}

/*
=============
R_DrawRectangle
=============
*/
void R_DrawRectangle( float x, float y, float w, float h )
{
	pglBegin( GL_LINE_LOOP );
		pglVertex2f( x, y );
		pglVertex2f( x + w, y );
		pglVertex2f( x + w, y + h );
		pglVertex2f( x, y + h );
	pglEnd();
}

/*
=============
R_DrawFillGradientRectangle
=============
*/
void R_DrawFillGradientRectangle( float x, float y, float w, float h, byte colors[4*4] )
{
	pglShadeModel( GL_SMOOTH );

	pglBegin(GL_TRIANGLES);
			//pglColor4ub(colors[0], colors[1], colors[2], colors[3]);
			pglColor3ub(colors[0], colors[1], colors[2]);
			pglVertex2f( x, y );

	        //pglColor4ub(colors[8], colors[9], colors[10], colors[11]);
			pglColor3ub(colors[8], colors[9], colors[10]);
			pglVertex2f( x + w, y );

	        //pglColor4ub(colors[12], colors[13], colors[14], colors[15]);
			pglColor3ub(colors[12], colors[13], colors[14]);
	        pglVertex2f( x + w, y + h );

	        //pglColor4ub(colors[4], colors[5], colors[6], colors[7]);
	        pglColor3ub(colors[4], colors[5], colors[6]);
	        pglVertex2f( x, y + h );

	        //pglColor4ub(colors[12], colors[13], colors[14], colors[15]);
	        pglColor3ub(colors[12], colors[13], colors[14]);
	        pglVertex2f( x + w, y + h );

	        //pglColor4ub(colors[8], colors[9], colors[10], colors[11]);
	        pglColor3ub(colors[8], colors[9], colors[10]);
	        pglVertex2f( x, y );
	pglEnd();

	pglShadeModel( GL_FLAT );
}

/*
=============
R_DrawCircle
=============
*/

void R_DrawCircle(int x, int y, int w, int h)
{
	//float r = (float)(w/2);

	float r = (float)(w/2);

	/*
	pglBegin( GL_TRIANGLE_FAN );
		pglVertex2f(x, y);

		for( int i = 0; i < 360; i++ )
		{
			printf("%f %f\n", r*cos(M_PI * i / 180.0) + x, r*sin(M_PI * i / 180.0) + y);
			pglVertex2f(r*cos(M_PI * i / 180.0) + x, r*sin(M_PI * i / 180.0) + y);
		}

	pglEnd();
	*/

	pglBegin( GL_LINE_LOOP );
		//pglVertex2f(x - r, y);

		for( int i = 0; i < 180; i++ )
		{
			float theta = M_PI2 * i / 180.0;
			pglVertex2f(r*cos(theta) + (x + r), r*sin(theta) + (y+r));
		}

	pglEnd();
}

/*
=============
R_DrawFillCircle
=============
*/

void R_DrawFillCircle(int x, int y, int w, int h)
{
	float r = (float)(w/2);

	pglBegin( GL_TRIANGLE_FAN );
		//pglVertex2f(x, y);

		for( int i = 0; i < 180; i++ )
		{
			float theta = M_PI2 * i / 180.0;
			pglVertex2f(r*cos(theta) + (x + r), r*sin(theta) + (y+r));
		}

	pglEnd();
}

/*
=============
pfnDrawLine

=============
*/
void pfnDrawLine( int x, int y, int x2, int y2, int r, int g, int b, int a )
{
	r = bound( 0, r, 255 );
	g = bound( 0, g, 255 );
	b = bound( 0, b, 255 );
	a = bound( 0, a, 255 );

	pglColor4ub( r, g, b, a );
	GL_SetRenderMode( kRenderTransColor );
	R_DrawLine(x, y, x2, y2);
	pglColor4ub( 255, 255, 255, 255 );
}

/*
=============
pfnDrawTriangle

=============
*/
void pfnDrawTriangle( int x, int y, int x2, int y2, int x3, int y3, int r, int g, int b, int a )
{
	r = bound( 0, r, 255 );
	g = bound( 0, g, 255 );
	b = bound( 0, b, 255 );
	a = bound( 0, a, 255 );

	pglColor4ub( r, g, b, a );
	GL_SetRenderMode( kRenderTransColor );
	R_DrawTriangle(x, y, x2, y2, x3, y3);
	pglColor4ub( 255, 255, 255, 255 );
}

/*
=============
pfnDrawFillTriangle

=============
*/
void pfnDrawFillTriangle( int x, int y, int x2, int y2, int x3, int y3, int r, int g, int b, int a )
{
	r = bound( 0, r, 255 );
	g = bound( 0, g, 255 );
	b = bound( 0, b, 255 );
	a = bound( 0, a, 255 );

	pglColor4ub( r, g, b, a );
	GL_SetRenderMode( kRenderTransTexture );
	R_DrawFillTriangle(x, y, x2, y2, x3, y3, 0, 0, 1, 0, 0, 1, cls.fillImage);
	pglColor4ub( 255, 255, 255, 255 );
}

/*
=============
pfnDrawFillGradientTriangle

=============
*/
void pfnDrawFillGradientTriangle( int x, int y, int x2, int y2, int x3, int y3, byte colors[12] )
{

	GL_SetRenderMode( kRenderTransColor );
	R_DrawFillGadientTriangle(x, y, x2, y2, x3, y3, colors );
	pglColor4ub( 255, 255, 255, 255 );
}

/*
=============
pfnDrawRectangle

=============
*/
void pfnDrawRectangle( int x, int y, int width, int height, int r, int g, int b, int a )
{
	r = bound( 0, r, 255 );
	g = bound( 0, g, 255 );
	b = bound( 0, b, 255 );
	a = bound( 0, a, 255 );

	pglColor4ub( r, g, b, a );
	GL_SetRenderMode( kRenderTransColor );
	R_DrawRectangle( x, y, width, height );
	pglColor4ub( 255, 255, 255, 255 );
}

/*
=============
pfnDrawFillRectangle

=============
*/
void pfnDrawFillRectangle( int x, int y, int width, int height, int r, int g, int b, int a )
{
	r = bound( 0, r, 255 );
	g = bound( 0, g, 255 );
	b = bound( 0, b, 255 );
	a = bound( 0, a, 255 );
	pglColor4ub( r, g, b, a );
	GL_SetRenderMode( kRenderTransTexture );
	R_DrawStretchPic( x, y, width, height, 0, 0, 1, 1, cls.fillImage );
	pglColor4ub( 255, 255, 255, 255 );
}

/*
=============
pfnDrawFillGradientRectangle

=============
*/
void pfnDrawFillGradientRectangle( int x, int y, int width, int height, unsigned char color[4*4] )
{
	pglColor4ub( 255, 255, 255, 255 );
	GL_SetRenderMode( kRenderTransColor );
	R_DrawFillGradientRectangle( x, y, width, height, color );
	pglColor4ub( 255, 255, 255, 255 );
}

/*
=============
pfnDrawRectangle

=============
*/
void pfnDrawCircle( int x, int y, int width, int height, int r, int g, int b, int a )
{
	r = bound( 0, r, 255 );
	g = bound( 0, g, 255 );
	b = bound( 0, b, 255 );
	a = bound( 0, a, 255 );

	pglColor4ub( r, g, b, a );
	GL_SetRenderMode( kRenderTransColor );
	R_DrawCircle( x, y, width, height );
	pglColor4ub( 255, 255, 255, 255 );
}

/*
=============
pfnDrawFillRectangle

=============
*/
void pfnDrawFillCircle( int x, int y, int width, int height, int r, int g, int b, int a )
{
	r = bound( 0, r, 255 );
	g = bound( 0, g, 255 );
	b = bound( 0, b, 255 );
	a = bound( 0, a, 255 );
	pglColor4ub( r, g, b, a );
	GL_SetRenderMode( kRenderTransTexture );
	R_DrawFillCircle( x, y, width, height );
	pglColor4ub( 255, 255, 255, 255 );
}

#endif
