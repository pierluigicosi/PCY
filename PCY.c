#include <stdio.h>
#include <time.h> 
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#define HASH_DIM 500000


/*********************************** STRUTTURE PER HASH TABLE ***********************************/

typedef struct {
    uint32_t item1;
	uint32_t item2;
    uint32_t count;
} triplet;

// Struttura per linked list node
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
void *my_malloc (int id, int bytes);


int main (int argc, char **argv)
{
	clock_t start=clock();
	
	if(argc!=3){
		printf("usage: ./PCY <file name> <threshold>");
		exit(0);}
	
	char *file=argv[1];
	int s=atoi(argv[2]); //support threshold

	FILE *fp;
	if((fp=fopen(file, "rt"))==NULL) {
		printf("Error in file opening");
		exit(1);
	}
	
   	/************************************* PASS 1 ***************************************/
   	
   	int *buckets;
   	int n;  //numero di items
   	char **item_to_int;
	int l;  //numero di frequent items
	int *frequent_items;
	int id=getpid();
	
	
   	create_candidate_itemset(fp,s,&n,&item_to_int,&l,&frequent_items,&buckets);
	printf("il numero di coppie con i frequent items è: %d", (l*(l-1))/2);

	rewind(fp);
   	
   	/*********************************** BETWEEN PASS ************************************/
   	
   	uint8_t *bitmap;
	bitmap=my_malloc(id,HASH_DIM*sizeof(uint8_t));
   	create_bitmap(s,&buckets,bitmap);
   	
   	free(buckets);
   	
   	
   /************************************* PASS 2 *****************************************/
	
	printf("\nle coppie frequenti con threshold %d sono:\n",s);
	count_frequentPairs(fp,s,n,item_to_int,l,frequent_items,bitmap);
	
   	
   	//free memory
   	free(item_to_int);
   	free(frequent_items);
   	free(bitmap);

	clock_t end = clock();
   	printf("tempo totale di esecuzione %f",(double)(end-start)/CLOCKS_PER_SEC);
}
	

void *my_malloc (int id, int bytes) 
{
   void *buffer;
    if ((buffer = malloc ((size_t) bytes)) == NULL) {
      printf ("Error: Malloc failed for process %d\n", id);
      fflush (stdout);
	  exit(1);
   }
   return buffer;
}


/*********************************** FUNZIONI HASH TABLE ***********************************/
hash_table_t* create_hash_table() {

	int id=getpid();
    hash_table_t* hash_table;

	hash_table=my_malloc(id,sizeof(hash_table_t));

    for (uint32_t i = 0; i < HASH_DIM; i++) {
        hash_table->buckets[i] = NULL;
    }
    return hash_table;
}

// Inserisco una tripletta e aggiorno il count
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
		//Creo nuovo nodo
		node_t* new_node=my_malloc(id,sizeof(node_t));
	
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
	
	int id = getpid();

	*buckets=my_malloc(id,HASH_DIM*sizeof(int));

   	*item_to_int=my_malloc(id,sizeof(char *));
	(*item_to_int)[0] = my_malloc(id,24 * sizeof(char));

	*frequent_items=my_malloc(id,sizeof(int));
	
	*c=-1;  //numero di item distinti
	char item[24];
	
	int *item_count=my_malloc(id,sizeof(int));

	char *buf = my_malloc(id,80000*sizeof(char));

	//Legge le righe dal file
    while (fgets(buf, 80000, fp) != NULL) {
        int numRead = 0;

		int *basket=my_malloc(id,sizeof(int));

		int length=0; //lunghezza basket

        //Legge le stringhe separate da spazio nella riga
        while (sscanf(buf + numRead, "%s", item) == 1) {

			//Aggiorno il count dei singoli elementi
			int flag=0;	
			for (int i=0;i<*c+1;i++){
				if (strcmp((*item_to_int)[i],item)==0){  //item già presente
				flag=1; item_count[i]++;
				break;}
			}
	
	
			if(flag!=1){  //item non presente
				(*c)++;
				if (*c!=0){
					if((*item_to_int=realloc(*item_to_int,(*c+1)*sizeof(char *)))==NULL)
						exit(1);
					(*item_to_int)[*c] = my_malloc(id,24 * sizeof(char));
					if((item_count=realloc(item_count,(*c+1) * sizeof(int)))==NULL)
						exit(1);
				}
				strcpy((*item_to_int)[*c],item);
				item_count[*c]=1;
			}

			//Aumenta la lunghezza e copia l'elemento nell'array del basket
			if(*c!=0){
				if ((basket=realloc(basket,(length+1)*sizeof(int)))==NULL)
					exit (1);
			}
				
				
			for (uint32_t k=0;k<*c+1;k++){
				if(strcmp((*item_to_int)[k],item)==0){
					basket[length]=k;
					break;
				}
			}	
				
			length++;


            //Aggiorna l'indice di lettura per la prossima stringa
            numRead += strlen(item) + 1;
        }

		//Per ogni coppia aggiorna il count
		for (uint32_t i=0;i<length;i++){
			for (uint32_t j=i+1;j<length;j++){
				int hash_value = hash(basket[i],basket[j]);
			
				if ((*buckets)[hash_value]<s)
					(*buckets)[hash_value]=(*buckets)[hash_value]+1;
			
			}
		}

		free(basket);
    }

	//Conteggio dei frequent items
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
		return abs(((a+b)*(a+b)+3*b+a)/2) % HASH_DIM;
	else
		return abs(((a+b)*(a+b)+3*a+b)/2) % HASH_DIM;

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


void count_frequentPairs(FILE *fp,int s, int length,char **item_to_int,int length2,int *frequent_items, uint8_t *bitmap){

	int id = getpid();

	hash_table_t* hash_table = create_hash_table();
	
	char item[24];
	
	int t=0; //numero di candidate pairs
	int f=0;  //numero di frequent pairs

	char *buf=my_malloc(id,80000*sizeof(char));

	//Legge le righe dal file
    while (fgets(buf, 80000, fp) != NULL) {
        int numRead = 0;
		int *basket=my_malloc(id,sizeof(int));
		int c=-1; //lunghezza basket

        //Legge le stringhe separate da spazio nella riga
        while (sscanf(buf + numRead, "%s", item) == 1) {
			c++;
			
			if(c!=0){
				if((basket=realloc(basket,(c+1)*sizeof(int)))==NULL)
					exit(1);
			}
				
			for (uint32_t k=0;k<length+1;k++){
				if(strcmp(item_to_int[k],item)==0){
					basket[c]=k;
					break;
				}
			}

			numRead += strlen(item) + 1;
		}

		//Trova le coppie frequenti
		for (uint32_t i=0;i<=c;i++){
			for (uint32_t j=i+1;j<=c;j++){
				int hash_value=hash(basket[i],basket[j]);
				if(bitmap[hash_value]==1){  //se mappa in 1
					for(uint32_t i1=0;i1<=length2;i1++){
						if(basket[i]==frequent_items[i1]){  //se il primo elemento è frequente
							for(uint32_t i2=0;i2<=length2;i2++){
								if(basket[j]==frequent_items[i2]){  //se il secondo elemento è frequente
								
									int count=insert_retrieve(hash_table,basket[i],basket[j],hash_value);
								
									if(count==1){ //se la coppia è candidata frequente
										t++;
									}

									if(count==s){ //se la coppia è frequente
										f++;
										//printf("%s %s\n",item_to_int[basket[i]],item_to_int[basket[j]]);
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
	printf("il numero delle coppie candidate frequenti è: %d \n",t);
	printf("il numero delle coppie frequenti è: %d \n",f);
		

	//free memory
	destroy_hash_table(hash_table);
	free(buf);
	fclose(fp);
}