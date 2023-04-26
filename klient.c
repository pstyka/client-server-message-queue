#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
 
#define MAX 8184//max 8192 ale trzeba odjac 8 (long int)

typedef struct komunikat{
	long int odbiorca;
    long int nadawca;
	char wiadomosc[MAX];
} kom;
 
int id_kolejki;
int ilosc = 0;
int flaga = 0;
 
pthread_t  wyslij;
pthread_t  odbierz; 

void * wyslij_komunikat();
void * odbierz_komunikat();
void utworz_kolejke();
void przerwij();
 
int main()
{   
 
    utworz_kolejke();

    signal(SIGINT, przerwij); // ctrl c
 
    if(pthread_create(&wyslij, NULL, wyslij_komunikat, NULL) != 0)
    {
        perror("KLIENT: Nie moge utworzyc watku nadawcy!!\n");
        exit(EXIT_FAILURE);
    }
    if(pthread_create(&odbierz, NULL, odbierz_komunikat, NULL) != 0)
    {
        perror("KLIENT: Nie moge utworzyc watku odbiorcy!!\n");
        exit(EXIT_FAILURE);
    }
    if (pthread_join(wyslij, NULL) != 0)
    {
        perror("KLIENT: Blad w podlaczaniu watku nadawcy!");
        exit(EXIT_FAILURE);
    }
    if (pthread_join(odbierz, NULL) != 0)
    {
        perror("KLIENT: Blad w podlaczaniu watku odbiorcy!");
        exit(EXIT_FAILURE);
    }
    exit(0);
}
 
void utworz_kolejke()
{
    key_t klucz;
    klucz = ftok(".", 'k');
    if (klucz == -1)
    {
        perror("KLIENT: Blad w tworzeniu klucza ftok!\n");
        exit(EXIT_FAILURE);
    }
 
    id_kolejki=msgget(555, 0606);
    if(id_kolejki == -1)
    {
        if(errno == ENOENT)//brak kolejki, czyli serwer nie zostal wlaczony
        {
            printf("KLIENT: Serwer nie zostal wlaczony!\n");
            exit(EXIT_FAILURE);
        }
        perror("KLIENT: Blad dolaczania do kolejki komunikatow!\n");
        exit(EXIT_FAILURE);
    }
    printf("KLIENT: Pomyslnie dolaczono do kolejki komunikatow! %d\n",id_kolejki);
}
void * wyslij_komunikat()
{
    //sleep(1);
    struct msqid_ds kolejka;
    kom send;
    int amount, i;
    amount=0;
    char znak, text[MAX];
    char bufor[MAX];
 
    
   // serwer musi zawsze miec miejsce na swoja odpowiedz
    msgctl(id_kolejki, IPC_STAT, &kolejka);
    amount = kolejka.msg_qbytes-(MAX +sizeof(long));// przy max 8192 

    while(1)
    {
        int zm;
        flaga = 0;
        msgctl(id_kolejki, IPC_STAT, &kolejka);//
        //cbytes- aktualna ilosc b w kolejce, qbytes-maksymalna ilosc
        if(kolejka.msg_cbytes >= amount || amount<=0) // kolejka zapchana
        {
            printf("KLIENT: kolejka jest zapchana!\n");
            przerwij();
        }  
        
        //printf("bajty limit: %d\n",amount);
        printf("KLIENT: Ilosc wiadomosci w kolejce: %d\n",kolejka.msg_qnum);

        while(flaga==0)
        {
            i=0;
            printf("KLIENT: Podaj wiadomosc: ");

            while((znak=fgetc(stdin)) != '\n')
            {
                if (i < (MAX - 1))
                {
                    bufor[i] = znak;
                }
                i++;
            }
            //printf("i=%d\n",i);
            if (i < (MAX))
            {
                flaga = 1;
                bufor[i]='\0';
            }
            else
            {
                printf("Przekroczono maksymalna wielkosc wiadomosci\n");
            }

        }
        //printf("test=%s\n",send.wiadomosc);
        strcpy(send.wiadomosc,bufor);
        send.odbiorca = 1; 
        send.nadawca = getpid();

           
        //wyslij wiadomosc do serwera
        zm=msgsnd(id_kolejki, &send, 8192/*strlen(send.wiadomosc)+sizeof(long int)+1*/, 0);
        if(zm == -1)
        {
            if(errno == EIDRM) // kolejka zostala usunieta
            {
                printf("KLIENT: Serwer przestal odpowiadac!\n");
                exit(EXIT_FAILURE);
            }
            if(errno == EINVAL) // id kolejki jest nieprawidlowe lub rozmiar < 0
            {
                printf("KLIENT: Serwer przestal odpowiadac!\n");
                exit(EXIT_FAILURE);
            }
            perror("KLIENT: Blad wysylania wiadomosci do serwera!\n");
            exit(EXIT_FAILURE);
        }

        ilosc++;
        printf("KLIENT: Wiadomosc zostala wyslana do serwera!\n");
        sleep(1);
        
    }
}
 
void * odbierz_komunikat()
{
    while(1)
    {
        int zm;
        kom receive;
        if(ilosc > 0)
        {   //msgrcv(id,)
            zm = msgrcv(id_kolejki, &receive, MAX + sizeof(long int), getpid(), 0);
            if(zm == -1)
            {
                if(errno == EIDRM) //EIDRM - identifier removed, kolejka usunieta
                {
                    printf("KLIENT: Serwer przestal odpowiadac!\n");
                    exit(EXIT_FAILURE);
                }
                else if(errno == EINVAL) //EINVAL - Invalid argument, id kolejki jest nieprawidlowe lub rozmiar < 0
                {
                    printf("KLIENT: Serwer przestal odpowiadac!\n");
                    exit(EXIT_FAILURE);
                }
                perror("KLIENT: blad pobrania komunikatu z kolejki\n");
                exit(EXIT_FAILURE);
            }
            printf("\nKLIENT: Wiadomosc zwrotna: %s\n\n", receive.wiadomosc);
            ilosc--;
            sleep(1);
        }
    }
}
void przerwij()
{   //gdy ilosc wyslanych komunikatow nie jest rowna ilosci odebranych 
    if(ilosc==0)
    {
        printf("\n KLIENT: %d SIE ZAKONCZYL!.\n", getpid());
        exit(0);
    }
    else
    {
        printf("KLIENT: Nie moge zatrzymac klienta, nie otrzymano wszystkich wiadomosci od serwera!!\n");
        if(pthread_detach(wyslij)==-1) // odlacz watek wysylajacy
        {
            perror("KLIENT: blad usuwania watku!!\n");
            exit(EXIT_FAILURE);
        }
        while(ilosc != 0);
        exit(0);
 
    }
 
}