#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FROM_BEGIN(block) ((block).inicio) // Retorna o endereço inicial do bloco na RAM
#define FROM_CURRENT(block) ((block).ponteiro_atual) // Retorna o endereço atual de manipulãção do bloco na RAM
#define FROM_END(block) ((void*)((unsigned long int)(block).inicio + 511)) // Retorna o endereço da ultima posição do bloco na RAM

#define DEBUG 0
#define ERRO -1

typedef struct{
	int ra;
	char nome[201];
	char curso[201];
	int ano;
}registro;

typedef struct{
	void *inicio; // Aponta sempre para o inicio do blocos
	void *ponteiro_atual; // Utilizado para manipulação
}Bloco;


// Aloca memoria para o bloco na RAM
// Usa void seu animal
const short int cria_bloco(Bloco *bloco)
{
	bloco->inicio = calloc(1,512);
	if ( !bloco->inicio )
		return ERRO;
	bloco->ponteiro_atual = (*bloco).inicio;
	return 1;
}

// Função para buscar uma posição de memoria no bloco relativo a uma das referencias abaixo:
// Utiliza como referência posição inicial do bloco com FROM_BEGIN(bloco)
// Utiliza como referência posição atual de um ponteiro auxiliar com FROM_CURRENT(p)
// Utiliza como referência posição final do bloco com FROM_END(bloco)
// Semelhante à função fseek
void seek_in_block(Bloco *bloco, size_t number_of_bytes, void* from)
{
	char* pointer = (char*)from;
	pointer = &pointer[number_of_bytes];
	bloco->ponteiro_atual = (void*)pointer;
}

void write_in_block(const void *ptr, size_t size, size_t nmemb, Bloco *bloco)
{
	memcpy(bloco->ponteiro_atual, ptr, size*nmemb);
	seek_in_block(bloco, size*nmemb, FROM_CURRENT(*bloco));
}

void read_from_block(void *ptr, size_t size, size_t nmemb, Bloco *bloco)
{
	memcpy(ptr, bloco->ponteiro_atual, size*nmemb);
	seek_in_block(bloco, size*nmemb, FROM_CURRENT(*bloco));	
}


const int get_tamanho_registro(const registro reg)
{
	return (sizeof(int) + strlen(reg.nome) + strlen(reg.curso) + sizeof(int) + 5); // São 4 separadores e 1 terminador de registro
}

// Ponteiro atual deve estar no local correto para chamar esta função
const short int escreve_registro_no_bloco(const registro reg, Bloco* block)
{
	char separador = 13;
	char terminador = 10; 
	// Se o registro não couber no bloco
	if ( (get_tamanho_registro(reg) + (*block).ponteiro_atual - 1) > FROM_END(*block) - 2 )
		return ERRO;

	write_in_block(&reg.ra,sizeof(int),1,block);
	write_in_block(&separador,sizeof(char),1,block);
	write_in_block(reg.nome,sizeof(char),strlen(reg.nome),block);
	write_in_block(&separador,sizeof(char),1,block);
	write_in_block(reg.curso,sizeof(char),strlen(reg.curso),block);
	write_in_block(&separador,sizeof(char),1,block);
	write_in_block(&reg.ano,sizeof(int),1,block);
	write_in_block(&separador,sizeof(char),1,block);
	write_in_block(&terminador,sizeof(char),1,block);

	return 0;
}

// Avança para o proximo registro sem lê-lo
const short int proximo_registro(Bloco *block)
{
	char aux;
	if (  )
		seek_in_block(block,sizeof(int),FROM_CURRENT(*block));
	seek_in_block(block, 1, FROM_CURRENT(*block));
	while ( aux != 13 ) read_from_block(&aux, sizeof(char), 1, block);
	aux = 0;
	while ( aux != 13 ) read_from_block(&aux, sizeof(char), 1, block);
	seek_in_block(block,sizeof(int)+2,FROM_CURRENT(*block));
}

// Movo ponteiro do bloco até a posição do registro, ou move até o inicio e retorna erro caso não encontre
const short int busca_por_RA(Bloco *bloco, const unsigned int RA)
{
	int aux = 0;
	seek_in_block(bloco,0,FROM_BEGIN(*bloco));
	while ( bloco->ponteiro_atual < FROM_END(*bloco) && aux != RA )
	{
		memcpy(&aux, bloco->ponteiro_atual, sizeof(int));
		if ( aux != RA )
			proximo_registro(bloco);
	}
	if ( aux != RA )
	{
		seek_in_block(bloco,0,FROM_BEGIN(*bloco));
		return ERRO;
	}
	return 1;
}

void le_registro_do_bloco(registro *reg, Bloco *block)
{
	char aux;
	int i = 0;
	read_from_block(&reg->ra, sizeof(int), 1, block);
	seek_in_block(block, 1, FROM_CURRENT(*block));
	while ( aux != 13 )
	{
		read_from_block(&aux, sizeof(char), 1, block);
		if ( aux != 13 )
			reg->nome[i] = aux;
		else
			reg->nome[i] = '\0';
		i++;
	}
	i = 0;
	aux = 0;
	while ( aux != 13 )
	{
		read_from_block(&aux, sizeof(char), 1, block);
		if ( aux != 13 )
			reg->curso[i] = aux;
		else
			reg->curso[i] = '\0';
		i++;
	}
	read_from_block(&reg->ano, sizeof(int), 1, block);
	seek_in_block(block, 2, FROM_CURRENT(*block));
}

int main(void)
{
	//******************************************************/
	// Declaração das variaveis                
	// Registro auxiliar para busca e escrita de registros
	registro reg_auxiliar;
	// Bloco atualmente na memoria RAM
	Bloco bloco_atual;
	char nome_arquivo_criado[201];
	int qtd_registros;
	FILE *arquivo;

	// Cria um bloco vazio na RAM
	cria_bloco(&bloco_atual);

	scanf("%s",nome_arquivo_criado);
	if ( DEBUG )
		printf("%s\n",nome_arquivo_criado);
	strcat(nome_arquivo_criado,".dat");
	arquivo = fopen(nome_arquivo_criado,"w+b");
	if ( !arquivo && DEBUG)
	{
		printf("Falha ao abrir arquivo.\n");
		return 0;
	}

	scanf("%d", &qtd_registros);
	if ( DEBUG )
		printf("%d\n", qtd_registros);

	while( qtd_registros > 0 )
	{
		scanf("%d\n",&reg_auxiliar.ra);
		fgets(reg_auxiliar.nome,201,stdin);
		reg_auxiliar.nome[strlen(reg_auxiliar.nome)-1] = '\0'; // Remove \n da string
		fgets(reg_auxiliar.curso,201,stdin);
		reg_auxiliar.curso[strlen(reg_auxiliar.curso)-1] = '\0'; // Remove \n da string
		scanf("%d",&reg_auxiliar.ano);
		if ( DEBUG )
			printf("%d|%s|%s|%d|#\n", reg_auxiliar.ra, reg_auxiliar.nome, reg_auxiliar.curso, reg_auxiliar.ano);
		qtd_registros--;

		if ( escreve_registro_no_bloco(reg_auxiliar, &bloco_atual) == ERRO )
		{	
			fwrite(bloco_atual.inicio,512,1,arquivo);
			free(bloco_atual.inicio);
			cria_bloco(&bloco_atual);
			escreve_registro_no_bloco(reg_auxiliar, &bloco_atual);
		}
	}
	fwrite(bloco_atual.inicio,512,1,arquivo);
	fseek(arquivo,0,SEEK_SET);
	free(bloco_atual.inicio);
	cria_bloco(&bloco_atual);
	fread(bloco_atual.inicio,512,1,arquivo);
	
	if ( busca_por_RA(&bloco_atual,743637) != ERRO)
	{
		le_registro_do_bloco(&reg_auxiliar, &bloco_atual);
		printf("%d|%s|%s|%d|#\n", reg_auxiliar.ra, reg_auxiliar.nome, reg_auxiliar.curso, reg_auxiliar.ano);
	}
	else
		printf("*\n");

	fclose(arquivo);
	free(bloco_atual.inicio);
	return 0;
}
