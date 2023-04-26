#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>


#define MAX 8184//max 8192 ale trzeba odjac 8 (long int)

int id_kolejki;
int flaga = 0;

void utworz_kolejke();
void wielkie_litery();
void wyslij_komunikat();
void odbierz_komunikat();
void usun_kolejke();
void zamknij_serwer();

typedef struct komunikat{
	long int odbiorca;
    long int nadawca;
	char wiadomosc[MAX];
} kom;

int main()
{   
    int i,mes=0;

    kom message;
    utworz_kolejke();

    signal(SIGINT, zamknij_serwer);
    while(1)
    {
        printf("SERWER: Czekam na komunikat...\n");
        //POBIERZ KOMUNIKAT OD KLIENTA
        mes = msgrcv(id_kolejki, &message, MAX + sizeof(long int), 1, 0);
        if(mes == -1)
        {   
            if(errno == E2BIG)//rozmiary wiadomosci sie roznia
            {
                printf("SERWER: maksymalna wielkosc wiadomosci w serwerze rozni sie od wartosci podanej w kliencie.\n");
                flaga = 1;
                zamknij_serwer();
                
            }
            perror("SERWER: blad pobrania komunikatu z kolejki\n");
            flaga = 1;
            zamknij_serwer();
            
        }
        printf("SERWER: Otrzymano wiadomosc od %ld o tresci: %s\n", message.nadawca, message.wiadomosc);
        sleep(15);
        //ZAMIEN NADAWCE I ODBIORCE
        message.odbiorca = message.nadawca;
        message.nadawca = 1;

        //POWIEKSZ LITERY
        for(i=0; i<strlen(message.wiadomosc); i++)
        {
            message.wiadomosc[i] = toupper(message.wiadomosc[i]);
        }
        printf("SERWER: Powiekszam litery: %s\nOdsylam wiadomosc.\n", message.wiadomosc);
        //WYSLIJ KOMUNIKAT DO KLIENTA
        //ostatni parametr: flaga reagujaca na przepelnienie kolejki IPC_NOWAIT lub 0.
        mes = msgsnd(id_kolejki, &message, /*strlen(message.wiadomosc) + sizeof(long int)+1*/8192, IPC_NOWAIT); 
        if(mes == -1)
        {
            perror("SERWER: blad dodania komunikatu do kolejki\n");
            flaga = 1;
            zamknij_serwer();
            
        }
        printf("SERWER: Wiadomosc zostala wyslana do klienta: %ld\n", message.odbiorca);
        sleep(1);
    }
    
}

void utworz_kolejke()
{
    key_t klucz;
    klucz = ftok(".", 'k');
    if (klucz == -1)
    {
        perror("SERWER: Blad w tworzeniu klucza ftok!\n");
        zamknij_serwer();
        
    }

    id_kolejki=msgget(555, IPC_CREAT | 0606);
    if(id_kolejki == -1)
    {
        perror("SERWER: Blad tworzenia kolejki komunikatow!\n");
        zamknij_serwer();
        
    }
    else
    {
        printf("SERWER: Pomyslnie utworzono kolejke komunikatow! %d\n", id_kolejki);
    }
}
void usun_kolejke()
{
    int koniec;
    koniec = msgctl(id_kolejki, IPC_RMID, 0);
    if(koniec==-1)
    {
        printf("SERWER: Blad usuwania kolejki komunikatow!");
        exit(EXIT_FAILURE);
    }
    else
    printf("SERWER: Kolejka komunikatow zostala usunieta: %d\n", id_kolejki);

    if(flaga == 0)
    {
        exit(0);
    }
    else
    {
        exit(EXIT_FAILURE);
    }
}
void zamknij_serwer()
{  
    printf("\nSERWER: SERWER ZAKONCZYL SWOJE DZIALANIE!!!\n");
    usun_kolejke();
}