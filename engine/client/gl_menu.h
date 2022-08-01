/*
 * gl_menu.h
 *
 *  Created on: 31 июл. 2022 г.
 *      Author: mr0maks
 */

#ifndef ENGINE_CLIENT_GL_MENU_H_
#define ENGINE_CLIENT_GL_MENU_H_

void pfnDrawLine( int x, int y, int x2, int y2, int r, int g, int b, int a );
void pfnDrawTriangle( int x, int y, int x2, int y2, int x3, int y3, int r, int g, int b, int a );
void pfnDrawFillTriangle( int x, int y, int x2, int y2, int x3, int y3, int r, int g, int b, int a );
void pfnDrawFillGradientTriangle( int x, int y, int x2, int y2, int x3, int y3, byte colors[12] );
void pfnDrawRectangle( int x, int y, int width, int height, int r, int g, int b, int a );
void pfnDrawFillRectangle( int x, int y, int width, int height, int r, int g, int b, int a );
void pfnDrawFillGradientRectangle( int x, int y, int width, int height, unsigned char color[4*4] );
void pfnDrawCircle( int x, int y, int width, int height, int r, int g, int b, int a );
void pfnDrawFillCircle( int x, int y, int width, int height, int r, int g, int b, int a );

#endif /* ENGINE_CLIENT_GL_MENU_H_ */
