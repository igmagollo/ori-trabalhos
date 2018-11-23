/**
*	Autor: Gabriel Olivato 		743537
*	Autor: Igor Raphael Magollo 743550
*	Descrição: Trabalho 2 da disciplina de Organização e Recuperação de Informações - ORI
*
*	Neste trabalho foi optado por utilizar alocação dinâmica para maximizar o aproveitamento da memória
*	uma vez que em uma aplicação real onde os arquivos que serão intercalados são muito maiores que a
*	memória primária, o aproveitamento da memória deve ser maximizado para completar a intercalação em
* 	melhor tempo.
*/
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<math.h>

/**
*	Definição de quantidade de files que devem ser intercalados no file base
*/
#ifndef QTDFILES
#define QTDFILES 10
#endif

/**
*	Definição do tamanho máximo do buffer, em quantidade de inteiros.
*/
#ifndef BUFFER
#define BUFFER 5
#endif

/**
*	Macro para pegar indice complementar
*/
#define invert_index(i) (BUFFER - i)

#define ERRO 0
#define SUCESSO 1

#ifdef VERBOSE
#define dprintf(...) printf(__VA_ARGS__)
#else
#define dprintf(...) // printf(VA_ARGS)
#endif

/**
*	Definição de funções
*/

/**
*	Recebe como parâmetro um inteiro e retorna sua quantidade de digitos
*/
unsigned int intlen(int number)
{
	int i = 0;
	if ( number == 0 )
		return 1;
	while ( number != 0 )
	{
		number = (int)number/10;
		i++;
	}
	return i;
}

/**
*	Recebe como parametro a string target onde será escrito o parâmetro nome base do arquivo,
*	concatenado com um inteiro nth
*/
void get_file_name(char* target, char* base_name, int nth)
{
	int i;
	int n_digits = intlen(nth);
	int name_len = strlen(base_name) + n_digits + 1;
	char concat_number[n_digits+1];
	sprintf(concat_number,"%d",nth);
	strcpy(target,base_name);
	strcat(target,concat_number);
}

/**
*	reabastace o buffer especificado do arquivo especificado
*/
int resupply_file_buffer(FILE* arquivo, int* buffer)
{
	return fread(buffer, sizeof(int), BUFFER, arquivo);
}

/**
*	retorna o indice do arquivo que contem o menor elemento
*/
int get_menor(int** buffers, int* buffers_lengths, int nfiles)
{
	int i, indice_menor = -1, menor = -1;
	for ( i = 0; i < nfiles; i++ )
	{
		if ( buffers_lengths[i] != -1 )
		{
			if ( menor != -1)
			{
				if ( buffers[i][invert_index(buffers_lengths[i])] < menor )
				{
					indice_menor = i;
					menor = buffers[i][invert_index(buffers_lengths[i])];
				}
			}
			else
			{
				indice_menor = i;
				menor = buffers[i][invert_index(buffers_lengths[i])];
			}
		}
	}
	return indice_menor;
}

/**
*
*/
void intercala_files(FILE** arquivos, int nfiles, char* base_name)
{
	int** buffers;
	int buffers_lengths[nfiles];
	int i, index;
	FILE* base_file = fopen(base_name, "wb");
	if ( !base_file )
	{
		dprintf("! Falha ao criar arquivo base\n");
		exit(EXIT_FAILURE);
	}

	buffers = (int**)malloc(nfiles * sizeof(int*));
	for ( i = 0; i < nfiles; i++ )
	{
		buffers[i] = (int*)malloc(BUFFER * sizeof(int));
	}

	for ( i = 0; i < nfiles; i++ )
	{
		// Reabastecimento inicial
		buffers_lengths[i] = resupply_file_buffer(arquivos[i], buffers[i]);
		if ( buffers_lengths[i] == 0 )
			buffers_lengths[i] = -1;
	}

	index = get_menor(buffers, buffers_lengths, nfiles);
	while ( index != -1 )
	{
		dprintf("\t- Indice do arquivo com menor elemento atual: %d\n", index);
		dprintf("\t- Escrevendo %d no arquivo %s\n", 
			buffers[index][invert_index(buffers_lengths[index])], base_name);
		fwrite(&buffers[index][invert_index(buffers_lengths[index])], sizeof(int), 1, base_file);
		buffers_lengths[index]--;
		if ( buffers_lengths[index] == 0 )
		{
			buffers_lengths[index] = resupply_file_buffer(arquivos[index], buffers[index]);
			if ( !buffers_lengths[index] )
				buffers_lengths[index] = -1;
		}
		index = get_menor(buffers, buffers_lengths, nfiles);
	}

	for ( i = 0; i < nfiles; i++ )
	{
		free(buffers[i]);
	}
	free(buffers);
	fclose(base_file);
}

int main(void)
{
	char base_name[50];
	char* nome_arquivo;
	FILE** arquivos;
	int i, nfiles;
	dprintf("- Informe o nome base\n- ");
	scanf("%s", base_name);
	dprintf("- Encontrando arquivos a serem intercalados\n");

	i = 0;
	/**
	*	Loop que encontra todos os arquivos existentes para serem intercalados
	*	Enquanto a quantidade máxima de files ainda não foi alcançada e o ultimo arquivo procurado era
	*	válido, então continua buscando os próximos arquivos.
	*/
	while ( i < QTDFILES && (i == 0 ? 1 : arquivos[i-1]) )
	{
		// Allocando tamanho do nome (sómente o necessário)
		nome_arquivo = (char*)malloc((strlen(base_name) + intlen(i) + 1) * sizeof(char));
		if ( !nome_arquivo )
		{
			dprintf("! Falha ao alocar memoria para o nome do arquivo\n");
			exit(EXIT_FAILURE);
		}
		// Atribuindo o nome do i-ésimo arquivo na variável nome_arquivo
		get_file_name(nome_arquivo, base_name, i);
		
		dprintf("\t- Procurando pelo arquivo %s\n", nome_arquivo);

		if ( i == 0)
			// Primeira alocação
			arquivos = (FILE**)malloc(sizeof(FILE*));
		else
			// Realocando quantidade de files
			arquivos = (FILE**)realloc(arquivos, (i+1) * sizeof(FILE*));

		if ( !arquivos )
		{
			dprintf("! Falha ao alocar/realocar memoria para quantidade de files\n");
			exit(EXIT_FAILURE);
		}

		arquivos[i] = fopen(nome_arquivo,"rb");

		if( arquivos[i] )
			dprintf("\t- Arquivo %s encontrado\n", nome_arquivo);
		else
			dprintf("\t- Arquivo %s não encontrado\n", nome_arquivo);

		// Desaloca nome do arquivo.
		free(nome_arquivo);
		i++;
	}
	nfiles = i - 1;
	dprintf("- %d arquivos encontrados\n", nfiles);

	dprintf("- Iniciando intercalação em %s\n", base_name);
	intercala_files(arquivos, nfiles, base_name);
	dprintf("- Intercalação completa\n");

	dprintf("- Fechando arquivos e desalocando memória\n");
	for ( i = 0; i < nfiles; i++ )
		// Fecha os arquivos abertos
		fclose(arquivos[i]);
	// Desaloca os arquivos
	free(arquivos);
	return 0;
}