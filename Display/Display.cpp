#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>		// _getch

HANDLE hEscEvent;

int main() {
	system("chcp 1252"); // Comando para apresentar caracteres especiais no console
	printf("Processo display");
	hEscEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, "EscEvent");

	WaitForSingleObject(hEscEvent, INFINITE);
	printf("\nAcione uma tecla para terminar\n");
	_getch(); // Pare aqui, caso não esteja executando no ambiente MDS

	ExitProcess(EXIT_SUCCESS);
	return EXIT_SUCCESS;
}