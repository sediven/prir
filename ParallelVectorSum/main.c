#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFER_SIZE 80

static key_t key1, key2;
static int id1, id2;

static int child_id;
static int children_number;
static int vector_length;

double sum( double *vector, int start, int end ) {

    int i;
    double sum = 0.0;
    for( i = start; i < end; i++ ) {
        sum += vector[i];
    }

    return sum;
}

void on_usr1( int signal ) {

    printf( "Dziecko %d:\t Otrzymałem USR1. Zaczynam liczenie...\n", getpid( ) );

    double *vector_shared, *partial_sums;

    if( ( id1 = shmget( key1, 0, 0666 ) ) == -1 ) {
        fprintf( stderr, "shmget error\n" );
        return;
    }

    if( ( id2 = shmget( key2, 0, 0666 ) ) == -1 ) {
        fprintf( stderr, "shmget error\n" );
        return;
    }

    if( ( vector_shared = shmat( id1, NULL, 0 ) ) == (void*) - 1 ) {
        fprintf( stderr, "shmat error\n" );
        return;
    }

    if( ( partial_sums = shmat( id2, NULL, 0 ) ) == (void*) - 1 ) {
        fprintf( stderr, "shmat error\n" );
        return;
    }

    printf( "Dziecko %d:\t Pamęć utworzona i podłączona.\n", getpid( ) );

    int elements_per_child = vector_length / children_number;
    int reszta = vector_length % children_number;

    if( child_id < reszta ) {
        elements_per_child += 1;
    }

    int start = child_id * elements_per_child;
    int end = start + elements_per_child;

    partial_sums[child_id] = sum( vector_shared, start, end );

    printf( "Dziecko %d:\tWynik cząstkowy obliczony, odłączam pamięć.\n", getpid( ) );

    shmdt( partial_sums );
    shmdt( vector_shared );
}

int main( int argc, char **argv ) {

    if( argc < 3 ) {
        printf( "Użycie: SuperParallelAdditionCalculator <plikwejsciowy> <N>\n"
                "gdzie:\n"
                "- <plikwejsciowy> \t--> plik z wektorem wejściowym\n"
                "- <N> \t--> liczba dzieci nielegalnie zatrudnionych do pracy\n" );
        return EXIT_FAILURE;
    }

    printf( "Identyfikator rodzica: %d\n", getpid( ) );

    char* input_file = argv[1];
    children_number = atoi( argv[2] );
    
    pid_t pids[children_number];
    key1 = 6525;
    key2 = 6535;

    sigset_t mask;
    struct sigaction usr1;

    FILE* f = fopen( input_file, "r" );
    char buffer[BUFFER_SIZE + 1];
    fgets( buffer, BUFFER_SIZE, f );
    vector_length = atoi( buffer );

    for( int i = 0; i < children_number; i++ ) {

        pid_t pid = fork( );

        switch( pid ) {
            case -1:
                fprintf( stderr, "Blad w fork\n" );
                return EXIT_FAILURE;
            case 0:
                child_id = i;

                sigemptyset( &mask );
                usr1.sa_handler = ( &on_usr1 );
                usr1.sa_mask = mask;
                usr1.sa_flags = SA_SIGINFO;
                sigaction( SIGUSR1, &usr1, NULL );

                pause( );
                return EXIT_SUCCESS;
            default:
                pids[i] = pid;
                break;
        }
    }

    double *vector = malloc( sizeof (double ) * vector_length );
    printf( "Liczba elementów do zsumowania: %d\n", vector_length );

    for( int i = 0; i < vector_length; i++ ) {
        fgets( buffer, BUFFER_SIZE, f );
        vector[i] = atof( buffer );
    }

    fclose( f );

    printf( "Wektor wczytany.\n" );

    double *vector_shared, *partial_sums;

    // Utworzenie pamięci na wektor
    if( ( id1 = shmget( key1, vector_length * sizeof (double ), 0666 | IPC_CREAT ) ) == -1 ) {
        fprintf( stderr, "shmget error\n" );
        return EXIT_FAILURE;
    }

    // Utworzenie pamięci na wyniki
    if( ( id2 = shmget( key2, children_number * sizeof (double ), 0666 | IPC_CREAT ) ) == -1 ) {
        fprintf( stderr, "shmget error\n" );
        return EXIT_FAILURE;
    }

    //Przyłączenie pamięci
    if( ( vector_shared = shmat( id1, NULL, 0 ) ) == (void*) - 1 ) {
        fprintf( stderr, "shmat error\n" );
        return EXIT_FAILURE;
    }
    if( ( partial_sums = shmat( id2, NULL, 0 ) ) == (void*) - 1 ) {
        fprintf( stderr, "shmat error\n" );
        return EXIT_FAILURE;
    }

    memcpy( vector_shared, vector, vector_length * sizeof (double ) );
    free( vector );
    sleep( 5 );

    for( int i = 0; i < children_number; i++ ) {
        kill( pids[i], SIGUSR1 );
    }

    int progress = 0;
    int id;

    while( ( id = wait( 0 ) ) != -1 ) {
        progress++;
        printf( "Otrzymałem wynik cząstkowy od dziecka: %d.\n"
                "\t\t\tPostęp: %d/%d\n", id, progress, children_number );
    }

    double final_sum = 0.0;

    for( int i = 0; i < children_number; i++ ) {
        final_sum += partial_sums[i];
    }

    printf( "Suma wszystkich elementów wektora: %f\n", final_sum );

    // Odłączenie pamięci
    shmdt( partial_sums );
    shmdt( vector_shared );
    // Usunięcie pamięci
    shmctl( id1, IPC_RMID, 0 );
    shmctl( id2, IPC_RMID, 0 );

    printf( "Dziękujemy za korzystanie i zapraszamy ponownie.\n"
            "\t\t\t Miłego dnia!\n" );

    return EXIT_SUCCESS;
}
