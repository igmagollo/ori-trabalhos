#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FROM_BEGIN(block) ((block).inicio) // Retorna o endereço inicial do bloco na RAM
#define FROM_CURRENT(block) ((block).ponteiro_atual) // Retorna o endereço atual de manipulãção do bloco na RAM
#define FROM_END(block) ((void*)((unsigned long int)(block).inicio + 511)) // Retorna o endereço da ultima posição do bloco na RAM

#define ERRO -1
#define SUCCESS 1
#define TRUE 1
#define FALSE 0

typedef struct{
	char ra[201];
	char nome[201];
	char curso[201];
	char ano[6];
}registro;

typedef struct{
	void *inicio; // Aponta sempre para o inicio do blocos
	void *ponteiro_atual; // Utilizado para manipulação
	unsigned short count_bytes_validos; // Utilizado para contar os bits validos 
}Bloco;


// Aloca memoria para o bloco na RAM
// Usa void seu animal
const short cria_bloco(Bloco *bloco)
{
	bloco->inicio = malloc(512);
	if ( !bloco->inicio )
		return ERRO;
	bloco->ponteiro_atual = (*bloco).inicio;
	bloco->count_bytes_validos = 0;
	return SUCCESS;
}

// Função para buscar uma posição de memoria no bloco relativo a uma das referencias abaixo:
// Utiliza como referência posição inicial do bloco com FROM_BEGIN(bloco)
// Utiliza como referência posição atual de um ponteiro auxiliar com FROM_CURRENT(p)
// Utiliza como referência posição final do bloco com FROM_END(bloco)
// Semelhante à função fseek
void seek_in_block(Bloco *bloco, int number_of_bytes, void* from)
{
	char* pointer = (char*)from;
	pointer = &pointer[number_of_bytes];
	bloco->ponteiro_atual = (void*)pointer;
}

// Semelhante a fwrite, porem atua diretamente no bloco na RAM
void write_in_block(const void *ptr, size_t size, size_t nmemb, Bloco *bloco)
{
	memcpy(bloco->ponteiro_atual, ptr, size*nmemb);
	seek_in_block(bloco, size*nmemb, FROM_CURRENT(*bloco));
}

// Semelhante a fread, oporem atua diretamente no bloco na RAM
void read_from_block(void *ptr, size_t size, size_t nmemb, Bloco *bloco)
{
	memcpy(ptr, bloco->ponteiro_atual, size*nmemb);
	seek_in_block(bloco, size*nmemb, FROM_CURRENT(*bloco));	
}

// Escreve os bytes validos na forma little endian
void write_bytes_validos_in_block(Bloco *bloco)
{
	memcpy(FROM_END(*bloco),&((char*)&(bloco->count_bytes_validos))[1],1);
	memcpy((void*)((unsigned long int)FROM_END(*bloco)-1),&((char*)&(bloco->count_bytes_validos))[0],1);
}

// Le os bytes validos na forma little endian
void read_bytes_validos_in_block(Bloco *bloco)
{
	memcpy(&((char*)&(bloco->count_bytes_validos))[1],FROM_END(*bloco),1);
	memcpy(&((char*)&(bloco->count_bytes_validos))[0],(void*)((unsigned long int)FROM_END(*bloco)-1),1);	
}

// Verifica se o byte apontado pelo ponteiro atual é valido
const short is_byte_valido(const Bloco bloco)
{	
	if ( ((unsigned long int)bloco.ponteiro_atual) - bloco.count_bytes_validos >= (unsigned long int)FROM_BEGIN(bloco) )
		return FALSE;
	return TRUE;
}

void escreve_bloco_no_arquivo(FILE *arquivo, const Bloco *bloco)
{
	fwrite(bloco->inicio,512,1,arquivo);
}

const short le_bloco_do_arquivo(FILE *arquivo, Bloco *bloco)
{	
	// Falha ao ler ou fim do arquivo
	if( fread(bloco->inicio,512,1,arquivo) != 1 )
	{
		// Le novamente o primeiro bloco e retorna erro
		rewind(arquivo);
		fread(bloco->inicio,512,1,arquivo);
		bloco->ponteiro_atual = bloco->inicio;
		read_bytes_validos_in_block(bloco);
		return ERRO; 
	}
	bloco->ponteiro_atual = bloco->inicio;
	read_bytes_validos_in_block(bloco);
	return SUCCESS;
}

const unsigned short get_tamanho_registro(const registro reg)
{
	return (strlen(reg.ra) + strlen(reg.nome) + strlen(reg.curso) + strlen(reg.ano) + 5); // São 4 separadores e 1 terminador de registro
}

// Ponteiro atual deve estar no local correto para chamar esta função
const short escreve_registro_no_bloco(const registro reg, Bloco* block)
{
	char separador = 13;
	char terminador = 10;
	unsigned short length = get_tamanho_registro(reg);

	// Se o registro não couber no bloco
	if ( (length + (*block).ponteiro_atual - 1) > FROM_END(*block) - 2 )
		return ERRO;

	// Escreve Registro no bloco 
	write_in_block(reg.ra,sizeof(char),strlen(reg.ra),block);
	write_in_block(&separador,sizeof(char),1,block);
	write_in_block(reg.nome,sizeof(char),strlen(reg.nome),block);
	write_in_block(&separador,sizeof(char),1,block);
	write_in_block(reg.curso,sizeof(char),strlen(reg.curso),block);
	write_in_block(&separador,sizeof(char),1,block);
	write_in_block(reg.ano,sizeof(char),strlen(reg.ano),block);
	write_in_block(&separador,sizeof(char),1,block);
	write_in_block(&terminador,sizeof(char),1,block);

	block->count_bytes_validos += length;
	write_bytes_validos_in_block(block);

	return SUCCESS;
}

const short le_registro_do_bloco(registro *reg, Bloco *block)
{
	if ( !is_byte_valido(*block) )
		return ERRO;

	char aux = 0;
	int i = 0;
	while ( aux != 13 )
	{
		read_from_block(&aux, sizeof(char), 1, block);
		if ( aux != 13 )
			reg->ra[i] = aux;
		else
			reg->ra[i] = '\0';
		i++;
	}
	i = 0;
	aux = 0;
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
	i = 0;
	aux = 0;
	while ( aux != 13 )
	{
		read_from_block(&aux, sizeof(char), 1, block);
		if ( aux != 13 )
			reg->ano[i] = aux;
		else
			reg->ano[i] = '\0';
		i++;
	}
	seek_in_block(block, 1, FROM_CURRENT(*block));
	return SUCCESS;
}

// Avança para o proximo registro sem lê-lo
const short proximo_registro(Bloco *block)
{
	// Byte atual já é invalido
	if ( !is_byte_valido(*block) )
		return ERRO;

	// Caso contrario, o byte atual é valido, portanto percorra até o proximo registro e verifica se é válido
	char aux;
	while ( aux != 10 ) read_from_block(&aux, sizeof(char), 1, block);

	if ( !is_byte_valido(*block) )
		return ERRO;

	return SUCCESS;
}

// Movo ponteiro do bloco até a posição do registro, ou move até o inicio e retorna erro caso não encontre
const short busca_por_RA_no_bloco(Bloco *bloco, const unsigned int RA)
{
	char aux[201];
	memset(aux,'\0',1);
	char RA_string[201];
	sprintf(RA_string,"%d",RA);
	seek_in_block(bloco,0,FROM_BEGIN(*bloco));
	while ( is_byte_valido(*bloco) && strcmp(aux,RA_string) != 0 )
	{
		int i = 0;
		char verifica_separador = 0;
		while ( verifica_separador != 13 )
		{
			read_from_block(&verifica_separador, sizeof(char), 1, bloco);
			if ( verifica_separador != 13 )
				aux[i] = verifica_separador;
			else
				aux[i] = '\0';
			i++;
		}	
		seek_in_block(bloco,-(strlen(aux)+1),FROM_CURRENT(*bloco));
		if ( strcmp(aux,RA_string) != 0 )
		{
			if( proximo_registro(bloco) == ERRO )
			{
				seek_in_block(bloco,0,FROM_BEGIN(*bloco));
				return ERRO;
			}
		}
	}
	// Caso saia do loop, verifica se RA do registro de fato é o que procuramos
	if ( strcmp(aux,RA_string) != 0 )
	{
		seek_in_block(bloco,0,FROM_BEGIN(*bloco));
		return ERRO;
	}
	return SUCCESS;
}

const short busca_por_RA_no_arquivo(FILE* arquivo, Bloco *bloco, const unsigned int RA)
{
	int flag_achou = ERRO;
	rewind(arquivo);
	// Maravilhas da linguagem \0/
	// Le o bloco do arquivo, se der erro significa que chegou ao final do arquivo, a comparação preguiçosa faz
	// com que a segunda parte não seja executada
	// Se a leitura do bloco for um sucesso, entao busca no bloco, caso encontre, flag será atualizada e a condição
	// não será verificada novamente
	// Caso não encontre, o loop continua, até que o registro seja encontrado ou chegue o final do arquivo
	while(le_bloco_do_arquivo(arquivo,bloco) != ERRO && (flag_achou = busca_por_RA_no_bloco(bloco,RA)) == ERRO);
	if ( flag_achou == ERRO )
		return ERRO;
	return SUCCESS;
}


int main(void)
{
	//******************************************************/
	// Declaração das variaveis                
	// Registro auxiliar para busca e escrita de registros
	registro reg_atual;
	// Bloco atualmente na memoria RAM
	Bloco bloco_atual;
	char nome_arquivo_criado[201];
	int qtd_registros;
	int RA_busca;
	FILE *arquivo;

	// Cria um bloco vazio na RAM
	cria_bloco(&bloco_atual);

	scanf("%s",nome_arquivo_criado);
	strcat(nome_arquivo_criado,".dat");
	arquivo = fopen(nome_arquivo_criado,"w+b");
	if ( !arquivo )
	{
		return 0;
	}

	scanf("%d\n", &qtd_registros);

	while( qtd_registros > 0 )
	{
		fgets(reg_atual.ra,201,stdin);
		reg_atual.ra[strlen(reg_atual.ra)-1] = '\0'; // Remove \n da string
		fgets(reg_atual.nome,201,stdin);
		reg_atual.nome[strlen(reg_atual.nome)-1] = '\0'; // Remove \n da string
		fgets(reg_atual.curso,201,stdin);
		reg_atual.curso[strlen(reg_atual.curso)-1] = '\0'; // Remove \n da string
		fgets(reg_atual.ano,6,stdin);
		reg_atual.ano[strlen(reg_atual.ano)-1] = '\0'; // Remove \n da string
		qtd_registros--;

		/* Escreve o registro no bloco, caso não caiba, cria novo bloco */
		if ( escreve_registro_no_bloco(reg_atual, &bloco_atual) == ERRO )
		{	
			escreve_bloco_no_arquivo(arquivo, &bloco_atual);
			free(bloco_atual.inicio);
			cria_bloco(&bloco_atual);
			escreve_registro_no_bloco(reg_atual, &bloco_atual);
		}
	}
	escreve_bloco_no_arquivo(arquivo, &bloco_atual);
	scanf("%d", &RA_busca);
	while ( RA_busca != 0 )
	{
		if ( busca_por_RA_no_arquivo(arquivo,&bloco_atual,RA_busca) != ERRO )
		{
			le_registro_do_bloco(&reg_atual, &bloco_atual);
			printf("%s:%s:%s:%s\n", reg_atual.ra, reg_atual.nome, reg_atual.curso, reg_atual.ano);		
		}
		else printf("*\n");			
		scanf("%d", &RA_busca);
	}

	fclose(arquivo);
	free(bloco_atual.inicio);
	return 0;
}
