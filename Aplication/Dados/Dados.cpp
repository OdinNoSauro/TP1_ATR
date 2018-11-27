#include "stdafx.h"
#define WIN32_LEAN_AND_MEAN 
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>				//_beginthreadex() e _endthreadex()

typedef unsigned (WINAPI *CAST_FUNCTION)(LPVOID);	//Casting para terceiro e sexto parâmetros da função
													//_beginthreadex
typedef unsigned *CAST_LPDWORD;

DWORD WINAPI ThreadCLP();		    //Thread representando leitura de CLP
DWORD WINAPI ThreadPCP();		    //Thread representando leitura de CLP
DWORD WINAPI ThreadCapture();			//Thread representando carro
DWORD WINAPI ThreadProd();		    //Thread representando leitura de CLP

int main() {
	DWORD dwIdCLP, dwIdPCP, dwIdCapture, dwIdTeclado, dwIdProd, dwIdDados;
	HANDLE hCLP = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)ThreadCLP, NULL, 0, (CAST_LPDWORD)&dwIdCLP);
	HANDLE hPCP = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)ThreadPCP, NULL, 0, (CAST_LPDWORD)&dwIdPCP);
	HANDLE hCapture = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)ThreadCapture, NULL, 0, (CAST_LPDWORD)&dwIdCapture);
	HANDLE hProd = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)ThreadProd, NULL, 0, (CAST_LPDWORD)&dwIdProd);
	return 0;
}

DWORD WINAPI ThreadCLP() {
	return NULL;
}

DWORD WINAPI ThreadPCP() {
	return NULL;
}

DWORD WINAPI ThreadCapture() {
	return NULL;
}

DWORD WINAPI ThreadProd() {
	return NULL;
}