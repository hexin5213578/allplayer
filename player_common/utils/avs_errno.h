﻿#pragma once 

typedef enum {
	ERR_INVALID		=	-22,
	ERR_FAULT		=	-14,	
	ERR_ACCESS		=	-13,
	ERR_NOMEM		=	-12,
	ERR_AGAIN		=	-11,
	ERR_NOT_INIT	=	-7,
	ERR_IO			=	-5,
	ERR_FAIL		=	-1,
	ERR_SUCCESS		=	0,
	ERR_OPEN		=	1,
	ERR_STREAM_FIND	=	2,
	ERR_STREAM_OPEN	=	3,
}ACERROR;


typedef enum {
	SDL_ERROR_GETINFO			= -4,
	SDL_ERROR_CREATE_RENDERR	= -3,
	SDL_ERROR_CREATE_WIN		= -2,
	SDL_ERROR_HWND_INVALID		= -1,
	SDL_ERROR_SUCCESS			= 0,
}SDL_ERROR;

typedef enum {
	D3D_ERROR_CREATE_RENDERR		= -4,
	D3D_ERROR_SWAPCHAIN_GETBUFFER	= -3,
	D3D_ERROR_CREATE_DEVICE			= -2,
	D3D_ERROR_HWND_INVALID			= -1,
	D3D_ERROR_SUCCESS				= 0,
}D3D_ERROR;
