#include <stdio.h>
#include <stdlib.h>
#include <windows.h> 
#include <omp.h>

#define SIZE_N 5
#define SIZE_STRING 2048
#define DEBUG
// Компиляция: gcc lab3.c -o lab3 -fopenmp -std=c99

void omp_info();
void cpy_matrix(double dst[][SIZE_N], double src[][SIZE_N]);
void print_matrix(double m[][SIZE_N]);

void file_processing(char* filename);

void test_matrix()
{
    double m_src[SIZE_N][SIZE_N] = {1,2,3,4,5,
                                    6,7,8,9,10,
                                    11,12,13,14,15,
                                    16,17,18,19,20,
                                    21,22,23,24,25};
                                    
    double m_dst[SIZE_N][SIZE_N] = {0};
    
    cpy_matrix(m_dst, m_src);
    print_matrix(m_src);
    print_matrix(m_dst);
}

void test_file_processing()
{
    file_processing("mycode.txt");
}

int main(int argc, char** argv)
{
    //test_matrix();
    
    test_file_processing();
    
    return 0;
}

void cpy_matrix(double dst[][SIZE_N], double src[][SIZE_N])
{
    double buf = 0.0;
    int i = 0, j = 0;
    
    HANDLE  sendDataEvent = NULL, recieveDataEvent = NULL;
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = FALSE;
    
    sendDataEvent = CreateEvent(&sa, TRUE, FALSE, "sendDataEvent");
    recieveDataEvent = CreateEvent(&sa, TRUE, FALSE, "recieveDataEvent");
    
    SetEvent(recieveDataEvent);
    
    #pragma omp parallel num_threads(2) firstprivate(i,j) shared(buf, dst, src)
    {
        if(omp_get_thread_num() % 2 == 0)
        {
            // Producer
            for(i = 0; i < SIZE_N; ++i)
            {   
                for(j = 0; j < SIZE_N; ++j)
                {
                    WaitForSingleObject(recieveDataEvent, INFINITE);
                    ResetEvent(recieveDataEvent);
                    
                    #pragma omp critical(matrix) // critical section not use
                    {
                        buf = src[i][j];
                        
                        #ifdef DEBUG
                            printf("Thread # %d\n", omp_get_thread_num());
                            printf("Producer buf=%f\n", buf);
                        #endif
                    }
                    SetEvent(sendDataEvent);
                }
            }
        }
        else
        {
            // Consumer
            for(i = 0; i < SIZE_N; ++i)
            {
                for(j = 0; j < SIZE_N; ++j)
                {
                    WaitForSingleObject(sendDataEvent, INFINITE);
                    ResetEvent(sendDataEvent);
                    
                    #pragma omp critical(matrix)
                    {
                        dst[i][j] = buf;
                        #ifdef DEBUG
                            printf("Thread # %d\n", omp_get_thread_num());
                            printf("Consumer buf=%f\n", buf);
                        #endif
                    }
                    //Sleep(10);
                    SetEvent(recieveDataEvent);
                    
                }
            }
        }
    }
}

void file_processing(char* filename)
{
    FILE* file;
    char line[SIZE_STRING];
    int is_eof = 0;
    
    if((file = fopen(filename, "r")) == NULL) 
    {
        printf("Can not open file for read: %s", filename);
        return;
    }
    
    HANDLE sendDataEvent = NULL, recieveDataEvent = NULL;
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = FALSE;
    
    sendDataEvent = CreateEvent(&sa, TRUE, FALSE, "sendDataEvent");
    recieveDataEvent = CreateEvent(&sa, TRUE, FALSE, "recieveDataEvent");
    
    SetEvent(recieveDataEvent);
    
    #pragma omp parallel num_threads(2) shared(is_eof, line)
    {
        if(omp_get_thread_num() % 2 == 0)
        {
            while(!is_eof)
            {
                WaitForSingleObject(recieveDataEvent, INFINITE);
                ResetEvent(recieveDataEvent);
                #ifdef DEBUG
                    printf("\nThread #%d try read line\n", omp_get_thread_num());
                #endif
                if(fgets(line, SIZE_STRING, file) == NULL)
                    is_eof = 1;                
                SetEvent(sendDataEvent);
            }
        }
        else
        {
            while(!is_eof)
            {
                WaitForSingleObject(sendDataEvent, INFINITE);
                ResetEvent(sendDataEvent);
                if(is_eof) break;
                #ifdef DEBUG
                    printf("\nThread #%d handle line\n", omp_get_thread_num());
                #endif
                printf("%s", line);                
                SetEvent(recieveDataEvent);
            }
        }
        
    }

    fclose(file);
}

void print_matrix(double m[][SIZE_N])
{
    for(int i = 0; i < SIZE_N; ++i)
    {
        for(int j = 0; j < SIZE_N; ++j)
        {
            printf("%.1f\t", m[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

void omp_info()
{
    char* pEnv = getenv("OMP_NUM_THREADS");
    if (pEnv != NULL)
        printf("OMP_NUM_THREADS: %s\n", pEnv);
    pEnv = getenv("OMP_NUM_THREADS");
    if (pEnv != NULL)
        printf("Max threads in env: %s\n", pEnv);

    printf("Max threads = %d\n", omp_get_max_threads()); 
}