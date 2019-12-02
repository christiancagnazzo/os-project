#include "my_lib.h"

int main(){
	int i, sem_id_zero, sem_id_mutex, mat_id, conf_id, sem_id_matrice, x,y,ms_gp, pos, ms_mg;
	char * matrice;
	struct shared_set  * set;
	int * fork_value;
	struct msg_m_g master_giocatore;

	setvbuf(stdout, NULL, _IONBF, 0); /* NO BUFFER */
	configure_settings();
	
	/* CONFIGURAZIONE E GENERAZIONE SCACCHIERA */
	conf_id = shmget(KEY_2, sizeof(int)*10, IPC_CREAT | 0600);
	set = shmat(conf_id, NULL, 0);
	mat_id = shmget(KEY_1, sizeof(char)*(set->SO_BASE)*(set->SO_ALTEZZA), IPC_CREAT | 0666);
	matrice = shmat(mat_id, NULL, 0);

	/* INSERIMENTO DATI MATRICE*/
	for (pos = 0; pos < set->SO_BASE*set->SO_ALTEZZA; pos++)
		matrice[pos] = 0;

	/* CREO SEMAFORI PER SCACCHIERA */
	sem_id_matrice = semget(KEY_3,(set->SO_ALTEZZA*set->SO_BASE), IPC_CREAT | 0666);
	for (i = 0; i < (set->SO_ALTEZZA*set->SO_BASE); i++)
		sem_set_val(sem_id_matrice,i,1);

	/* SEMAFORI MUTUA ESCLUSIONE */
	sem_id_mutex = semget(KEY_5,2, IPC_CREAT | 0666);
	sem_set_val(sem_id_mutex,0,1); /* giocatore a turno piazza pedine */
	sem_set_val(sem_id_mutex,1,1); /* pedine si posizionano una alla volta */

	
	/* GENERAZIONE GIOCATORI E CODA PER COMUNICAZIONE */
	fork_value = malloc(sizeof(int)*set->SO_NUM_G);
	for (i = 0; i < set->SO_NUM_G; i++){
		switch (fork_value[i] = fork()){
			case -1:
				fprintf(stderr, "Errore nella creazione dei giocatori\n");
				exit(EXIT_FAILURE);

			case 0:
				if (execve("./giocatore",NULL,NULL) == -1){
					fprintf(stderr, "Execve error\n");
					exit(EXIT_FAILURE);
			}
		}
	}
	
	/* SEMAFORO PER ATTENDERE CHE I GIOCATORI PIAZZINO LE PEDINE */
	sem_id_zero = semget(KEY_0, 2, IPC_CREAT | 0666);
	sem_set_val(sem_id_zero, 0, set->SO_NUM_G);
	aspetta_zero(sem_id_zero, 0); /* ATTENDE FINCHE' NON VALE 0 */
	
	/* STAMPO GIOCATORI */
	ms_mg = msgget(KEY_6, IPC_CREAT | 0666);
	for (i = 0; i < set->SO_NUM_G; i++){
		msgrcv(ms_mg, &master_giocatore,((sizeof(int)*2)+sizeof(char)), fork_value[i], 0);
		printf("GIOCATORE: %c, MOSSE TOTALI: %d, PUNTEGGIO: %d\n",master_giocatore.giocatore,master_giocatore.mosse_residue,master_giocatore.punteggio);
	}
	
	matrice[23] = -23;
	/* STAMPO SCACCHIERA */
	pos = 0;
	for (x = 0; x < set->SO_ALTEZZA; x++){
		for (y = 0; y < set->SO_BASE; y++){
			if (semctl(sem_id_matrice,pos, GETVAL) == 0)
				printf("|%c ", matrice[pos++]); /* pedina */ 
			else
				if (matrice[pos] < 0) 	
					printf("|%d" , -matrice[pos++]); /* bandierina con relativo punteggio */
				else
					printf("|%c  ", matrice[pos++]);	 /* casella vuota */
		}
		printf("|\n");
	}
	

	/* ELIMINO SEMAFORI E MEMORIE CONDIVISE*/
	shmctl(mat_id, IPC_RMID, NULL); 
	shmctl(conf_id, IPC_RMID, NULL);
	shmdt(matrice);
	shmdt(set);
	semctl(sem_id_zero,0,IPC_RMID); /* 0 è ignorato*/
	semctl(sem_id_matrice,0,IPC_RMID);
	semctl(sem_id_mutex,0,IPC_RMID);
	ms_gp = msgget(KEY_4, 0666);
	msgctl(ms_gp,IPC_RMID,NULL);
	msgctl(ms_mg,IPC_RMID,NULL);
}


