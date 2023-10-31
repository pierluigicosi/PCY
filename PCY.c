#include <stdio.h>
#include <time.h> 
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#define HASH_DIM 500000


/*************************** STRUTTURE PER HASH TABLE CON LINKED LIST ***************************/

typedef struct {
    uint32_t item1;
	uint32_t item2;
    uint32_t count;
} triplet;

typedef struct node {
    triplet triplet;
    struct node* next;
} node_t;

typedef struct {
    node_t* buckets[HASH_DIM];
} hash_table_t;

/************************************ DICHIARAZIONE FUNZIONI ***********************************/

int create_candidate_itemset(FILE *file, int s, int *c,char ***item_to_int,int *n, int **frequent_items, int **buckets);
int hash(int a,int b);
int create_bitmap(int s,int **buckets,uint8_t *bitmap);
void count_frequentPairs(FILE *file, int s,int length,char **item_to_int,int length2,int *frequent_items, uint8_t *bitmap);
hash_table_t* create_hash_table();
int insert_retrieve(hash_table_t* hash_table, uint32_t item1, uint32_t item2,int index);
void destroy_hash_table(hash_table_t* hash_table);
void *my_malloc (int bytes);


int main (int argc, char **argv)
{
	clock_t start=clock();
	
	if(argc!=3){
		printf("usage: ./PCY <file name> <threshold>");
		exit(0);}
	
	char *file=argv[1];	   /* File name */
	int s=atoi(argv[2]);   /* Support threshold */

	FILE *fp;
	if((fp=fopen(file, "rt"))==NULL) {
		printf("Error in file opening");
		exit(1);
	}
	
   	/************************************* PASSO 1 **************************************/
   	
   	int di;                /* Numero di distinct items */
   	char **item_to_int;	   /* Hash map item in interi */
	int *buckets;          /* Hash map per conteggio item */
	int fi;                /* Numero di frequent items */
	int *frequent_items;   /* Array contenente frequent items */
	
	
   	create_candidate_itemset(fp,s,&di,&item_to_int,&fi,&frequent_items,&buckets);
	printf("il numero di item distinti è : %d\n", di);
	printf("il numero di item frequenti è : %d\n", fi);
	printf("il numero di coppie composte da frequent items è : %d\n", (fi*(fi-1))/2);

	rewind(fp);
   	
   	/*********************************** PASSO INTERMEDIO ********************************/
   	
   	uint8_t *bitmap;
	bitmap=my_malloc(HASH_DIM*sizeof(uint8_t));
   	create_bitmap(s,&buckets,bitmap);
   	
   	free(buckets);
   	
   	
   /************************************* PASSO 2 *****************************************/
	
	printf("le coppie frequenti con threshold %d sono:\n",s);
	count_frequentPairs(fp,s,di,item_to_int,fi,frequent_items,bitmap);
	
   	
   	/* Libero la memoria */
   	free(item_to_int);
   	free(frequent_items);
   	free(bitmap);

	clock_t end = clock();
   	printf("tempo totale di esecuzione %f",(double)(end-start)/CLOCKS_PER_SEC);
}
	


/* Funzione per malloc personalizzata */
void *my_malloc (int bytes) 
{
   void *buffer;
    if ((buffer = malloc ((size_t) bytes)) == NULL) {
      fflush (stdout);
	  exit(1);
   }
   return buffer;
}


/*********************************** FUNZIONI HASH TABLE ***********************************/
hash_table_t* create_hash_table() {

    hash_table_t* hash_table;

	hash_table=my_malloc(sizeof(hash_table_t));

    for (uint32_t i = 0; i < HASH_DIM; i++) {
        hash_table->buckets[i] = NULL;
    }
    return hash_table;
}

/* Inserisco una tripletta e aggiorno il count */
int insert_retrieve(hash_table_t* hash_table, uint32_t item1, uint32_t item2,int index) {
	int flag=0;
	int id=getpid();

	node_t* current = hash_table->buckets[index];
    while (current != NULL) {
        if (current->triplet.item1 == item1 && current->triplet.item2 == item2 || 
			current->triplet.item1 == item2 && current->triplet.item2 == item1) {
            return current->triplet.count+=1;
			flag=1;
			break;
        }
        current = current->next;
    }

	if(flag==0){
		/* Se è una nuova coppia creo il nodo */
		node_t* new_node=my_malloc(sizeof(node_t));
	
		new_node->triplet.item1 = item1;
		new_node->triplet.item2 = item2;
		new_node->triplet.count = 1;
		new_node->next = hash_table->buckets[index];
		hash_table->buckets[index] = new_node;
		return 1;
	}
	return 0;
}

void destroy_hash_table(hash_table_t* hash_table) {
    for (uint32_t i = 0; i < HASH_DIM; i++) {
        node_t* current = hash_table->buckets[i];
        while (current != NULL) {
            node_t* next = current->next;
            free(current);
            current = next;
        }
    }
    free(hash_table);
}

/*********************************** FUNZIONI FREQUENT ITEMSET ***********************************/

int create_candidate_itemset(FILE *fp, int s,int *c,char ***item_to_int,int *n, int **frequent_items, int **buckets){
	
	char item[24];										   /* Singolo item letto */
	int *item_count=my_malloc(sizeof(int));			       /* Array per il conteggio di ogni item distinto */
	char *buf = my_malloc(8000*sizeof(char));              /* Buffer per contenere una riga */

	/* Allocazione strutture */
	*buckets=my_malloc(HASH_DIM*sizeof(int));           
   	*item_to_int=my_malloc(sizeof(char *));
	(*item_to_int)[0] = my_malloc(24 * sizeof(char));
	*frequent_items=my_malloc(sizeof(int));

	*c=0;  

	/* Legge le righe dal file */
    while (fgets(buf, 8000, fp) != NULL) {
        int numRead = 0;							  /* Indice lettura basket*/
		int *basket=my_malloc(sizeof(int));           /* Array basket */
		int length=0;                                 /* Lunghezza basket */

        /* Legge le stringhe separate da spazio nella riga */
        while (sscanf(buf + numRead, "%s", item) == 1) {

			/* Aggiorno il count dei singoli elementi */
			int flag=0;	
			for (int i=0;i<*c+1;i++){
				if (strcmp((*item_to_int)[i],item)==0){ 
				flag=1; item_count[i]++;
				break;}
			}
	
			/* Se l'item non è presente alloco lo spazio */
			if(flag!=1){  
				strcpy((*item_to_int)[*c],item);
				(*c)++;
				
				if((*item_to_int=realloc(*item_to_int,(*c+1)*sizeof(char *)))==NULL)
					exit(1);
				(*item_to_int)[*c] = my_malloc(24 * sizeof(char));
				if((item_count=realloc(item_count,(*c+1) * sizeof(int)))==NULL)
					exit(1);
						
				item_count[*c]=1;
			}

			/* Aumenta la lunghezza e copia l'elemento nell'array del basket */
			if ((basket=realloc(basket,(length+1)*sizeof(int)))==NULL)
				exit (1);
				
			for (uint32_t k=0;k<*c+1;k++){
				if(strcmp((*item_to_int)[k],item)==0){
					basket[length]=k;
					break;
				}
			}	
				
			length++;

            /* Aggiorna l'indice di lettura per la prossima stringa */
            numRead += strlen(item) + 1;
        }

		/* Per ogni coppia aggiorna il count */
		for (uint32_t i=0;i<length;i++){
			for (uint32_t j=i+1;j<length;j++){
				int hash_value = hash(basket[i],basket[j]);
			
				if ((*buckets)[hash_value]<s)
					(*buckets)[hash_value]=(*buckets)[hash_value]+1;
			
			}
		}

		free(basket);
    }

	/* Conteggio dei frequent items */
	(*n)=0;
	
	for (uint32_t i=0;i<(*c)+1;i++){
		if (item_count[i]>=s){
			(*frequent_items)[*n]=i;
			*n=*n+1;
			if ((*frequent_items=realloc(*frequent_items,(*n+1)*sizeof(int)))==NULL){
					printf("allocazione fallita");
					exit(1);
			}
		}
	}

	free(item_count);
	free(buf);
	return 0;
}


int hash(int a, int b){

	if (a<b)
		return ((a+b)*(a+b)+3*b+a)/2 % HASH_DIM;
	else
		return ((a+b)*(a+b)+3*a+b)/2 % HASH_DIM;

}


int create_bitmap(int s,int **buckets,uint8_t *bitmap){

	for (uint32_t i=0;i<HASH_DIM;i++){
		if((*buckets)[i]>=s)
			bitmap[i]=1;
		else
			bitmap[i]=0;
	}

	return 0;
}


void count_frequentPairs(FILE *fp,int s, int di,char **item_to_int,int fi,int *frequent_items, uint8_t *bitmap){

	char item[24];									  /* Singolo item letto */
	int nc=0;  										  /* Numero di candidate pairs */
	int nf=0;  				 					 	  /* Numero di frequent pairs */
	char *buf=my_malloc(8000*sizeof(char));       	  /* Buffer */

	hash_table_t* hash_table = create_hash_table();   /* Allocazione hash table per frequent pairs*/

	/* Legge le righe dal file */
    while (fgets(buf, 8000, fp) != NULL) {
        int numRead = 0;         				 /* Indice lettura */
		int *basket=my_malloc(sizeof(int));   /* Array contenente il basket */
		int lb=0;                                /* Lunghezza basket */

        /* Legge le stringhe separate da spazio nella riga */
        while (sscanf(buf + numRead, "%s", item) == 1) {
			lb++;
			
			if((basket=realloc(basket,(lb+1)*sizeof(int)))==NULL)
				exit(1);
				
			for (uint32_t k=0;k<=di;k++){
				if(strcmp(item_to_int[k],item)==0){
					basket[lb]=k;
					break;
				}
			}

			numRead += strlen(item) + 1;
		}

		/* Trova le coppie frequenti */
		for (uint32_t i=1;i<lb;i++){
			for (uint32_t j=i+1;j<=lb;j++){
				int hash_value=hash(basket[i],basket[j]);
				if(bitmap[hash_value]==1){  /* Se la coppia mappa in un bucket frequente */
					for(uint32_t i1=0;i1<=fi;i1++){
						if(basket[i]==frequent_items[i1]){  /* Se il primo elemento è frequente */
							for(uint32_t i2=0;i2<=fi;i2++){
								if(basket[j]==frequent_items[i2]){  /* Se il secondo elemento è frequente */
								
									int count=insert_retrieve(hash_table,basket[i],basket[j],hash_value);
								
									if(count==1){ /* La coppia è candidata frequente */
										nc++;
									}

									if(count==s){ /* La coppia è frequente */
										nf++;
										//printf("%s %s\n",item_to_int[basket[i]],item_to_int[basket[j]]); /* Stampa delle coppie frequenti */
									}
									break;
								}
							}
							break;
						}
					}
				}
			}
				
		}
		free(basket);
	}
	printf("il numero delle coppie candidate frequenti è: %d \n",nc);
	printf("il numero delle coppie frequenti è: %d \n",nf);
		

	/* Libero la memoria */
	destroy_hash_table(hash_table);
	free(buf);
	fclose(fp);
}