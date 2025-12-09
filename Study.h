#ifndef STUDY_H
#define STUDY_H
#include <pthread.h>
#include <cstdio> 
#include <semaphore.h> 
#include <stdexcept>   
using namespace std;
class Study {
private:
    int studentNumNeeded;
    int tutorExists;
    int studentNumCurr;
    pthread_t Tutor;
    bool session;
    sem_t roomBlock;
    sem_t tutorBlock;
    pthread_mutex_t mutex;

public:
    Study(int sno, int tutor){
        if(sno<=0){
           throw std::invalid_argument("An error occurred.");
        }
        if(tutor!=0 && tutor!=1){
           throw std::invalid_argument("An error occurred.");
        }
        studentNumNeeded=sno;
        tutorExists=tutor;
        if(tutorExists==1){
            studentNumNeeded+=1;
        }
        session=false;
        studentNumCurr=0;
        sem_init(&roomBlock, 0, studentNumNeeded);
        sem_init(&tutorBlock, 0, 1);
        pthread_mutex_init(&mutex, NULL);
    }
    ~Study(){
        sem_destroy(&roomBlock);
        sem_destroy(&tutorBlock);
        pthread_mutex_destroy(&mutex);
    };
    void arrive(){
        printf("Thread ID: %ld | Status: Arrived at the IC.\n", pthread_self());
        sem_wait(&roomBlock);

        pthread_mutex_lock(&mutex);
        studentNumCurr++;
        

        if(tutorExists==0 && studentNumCurr==studentNumNeeded){
            printf("Thread ID: %ld | Status: There are enough students, the study session is starting.\n", pthread_self());
            session=true;
            
        }else if(tutorExists==1 && studentNumCurr==studentNumNeeded){
            Tutor = pthread_self();
            sem_wait(&tutorBlock);
            printf("Thread ID: %ld | Status: There are enough students, the study session is starting.\n", pthread_self());
            session=true;
        }else{
            printf("Thread ID: %ld | Status: Only %d students inside, studying individually.\n", pthread_self(), studentNumCurr);
        }
        pthread_mutex_unlock(&mutex);
        

    };
    void start();
    void leave(){
       
        pthread_mutex_lock(&mutex);
        if (session==false){
            studentNumCurr--;
            pthread_mutex_unlock(&mutex);
            printf("Thread ID: %ld | Status: No group study formed while I was waiting, I am leaving.\n", pthread_self());
            sem_post(&roomBlock);
            return;
        }
        pthread_mutex_unlock(&mutex);
        if(tutorExists==1){
            if(pthread_equal(pthread_self(), Tutor)){
                printf("Thread ID: %ld | Status: Session tutor speaking, the session is over.\n", pthread_self());
                sem_post(&tutorBlock);
            }else{
                sem_wait(&tutorBlock);
                printf("Thread ID: %ld | Status: I am a student and I am leaving.\n", pthread_self());
                sem_post(&tutorBlock);
            }
        }else{
            printf("Thread ID: %ld | Status: I am a student and I am leaving.\n", pthread_self());
        }
        pthread_mutex_lock(&mutex);
        studentNumCurr--;
        bool lastStudent = (studentNumCurr == 0);
       

        if(lastStudent){
            printf("Thread ID: %ld | Status: All students have left, the new students can come.\n", pthread_self());
            session=false;
            pthread_mutex_unlock(&mutex);
            for(int i=0; i<studentNumNeeded; i++){
                sem_post(&roomBlock);
            }
        }else{
             pthread_mutex_unlock(&mutex);
        }
    };
};
#endif 