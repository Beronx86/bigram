/*
 * bigram.c

 *
 *  Created on: Jan 31, 2015
 *      Author: v-penlu
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAX_WORD_LENGTH 100

typedef double real;

typedef struct {
	char *biword;
	real count;
} BIWORD;

typedef struct cooccur_rec {
    int word1;
    int word2;
    real val;
} CREC;


long long hash_size, num_lines, num_words = 0, vocab_max_size = 2500;
BIWORD **bigram_table;
char *vocab;
char cooccur_file[100], vocab_file[100];
real total_biword_len = 0;

void ReadVocab() {
	long long count, a;
	char word[MAX_WORD_LENGTH], format[20];
	FILE *fin;
	fin = fopen(vocab_file, "r");
	vocab = (char *)calloc(vocab_max_size * MAX_WORD_LENGTH, sizeof(char));
	if (fin == NULL) {printf("Unable to opne vocab file %s.\n", vocab_file); return;}
	sprintf(format, "%%%ds %%lld\n", MAX_WORD_LENGTH);
	while (fscanf(fin, format, word, &count) != EOF) {
		strcpy(&vocab[num_words * MAX_WORD_LENGTH], word);
		num_words++;
		if (num_words + 2 >= vocab_max_size) {
			vocab_max_size += 1000;
			vocab = (char *)realloc(vocab, vocab_max_size * MAX_WORD_LENGTH * sizeof(char));
		}
	}
	fclose(fin);
}

char *ConcatenateWord(char* word1, char* word2) {
	char *biword;
	// biword = (char *)calloc(2 * MAX_WORD_LENGTH, sizeof(char));
	biword = (char *)calloc(strlen(word1) + strlen(word2) + 2, sizeof(char));
	strcpy(biword, word1);
	strcat(biword, "#");
	strcat(biword, word2);
	return biword;
}

long long GetWordHash(char *word) {
	unsigned long long a, hash = 0;
	for (a = 0; a < strlen(word); a++) hash = hash * 257 + word[a];
	hash = hash % hash_size;
	return hash;
}

void SaveCREC(CREC *cr) {
	char *biword, *word1, *word2;
	unsigned long long hash;
	// word1 = vocab[cr->word1 - 1LL];
	// word2 = vocab[cr->word2 - 1LL];
	word1 = &vocab[(cr->word1 - 1LL) * MAX_WORD_LENGTH];
	word2 = &vocab[(cr->word2 - 1LL) * MAX_WORD_LENGTH];
	biword = ConcatenateWord(word1, word2);
	hash = GetWordHash(biword);
	while (bigram_table[hash] != NULL) hash = (hash + 1) % hash_size;
	bigram_table[hash] = (BIWORD *)malloc(sizeof(BIWORD));
	// bigram_table[hash]->biword = (char *)calloc(strlen(biword) + 1, sizeof(char));
	// strcpy(bigram_table[hash]->biword, biword);
	bigram_table[hash]->biword = biword;
	bigram_table[hash]->count = cr->val;

	total_biword_len += strlen(biword) + 1.;
	// free(biword);
}

real GetCount(char *biword) {
	unsigned long long hash = GetWordHash(biword);
	while (1) {
		if (bigram_table[hash] == NULL) return 0.0;
		if (!strcmp(biword, bigram_table[hash]->biword)) return bigram_table[hash]->count;
		hash = (hash + 1) % hash_size;
	}
	return 0.0;
}

void ConstructBigramTable() {
	FILE *fin;
	CREC cr;
	char *word1, *word2;
	long long a, file_size, n = 0;
	fin = fopen(cooccur_file, "rb");
	if (fin == NULL) {fprintf(stderr, "Unable to open file %s.\n", cooccur_file); return;}
	fseeko(fin, 0, SEEK_END);
	file_size = ftello(fin);
	num_lines = file_size / sizeof(CREC);

	hash_size = (long long)num_lines / 0.7;
	bigram_table = (BIWORD **)malloc(hash_size * sizeof(BIWORD *));
	for (a = 0; a < hash_size; a++) bigram_table[a] = (BIWORD *)NULL;

	fprintf(stderr, "reading bigram file %s into memory\n", cooccur_file);
	fseeko(fin, 0, SEEK_SET);
	while (1) {
		n++;
		if (n % 1000000 == 0) fprintf(stderr, "%.2lf%% read into the memory\n", (n / (real)num_lines * 100));
		fread(&cr, sizeof(CREC), 1, fin);
		if (feof(fin)) break;
		// word1 = &vocab[(cr.word1 - 1LL) * MAX_WORD_LENGTH];
		// word2 = &vocab[(cr.word2 - 1LL) * MAX_WORD_LENGTH];
		// fprintf(stderr, "Saving %s#%s\n", word1, word2);
		SaveCREC(&cr);
	}
	fclose(fin);
}

int main() {
	FILE *fin;
	CREC cr;
	char *biword, *word1, *word2;
	real count;
	long long n = 0;
	strcpy(cooccur_file, "cooccurrence.shuf.bin");
	strcpy(vocab_file, "vocab.txt");


	ReadVocab();
	ConstructBigramTable();
	fin = fopen(cooccur_file, "rb");
	fprintf(stderr, "Checking bigram table in memory.\n");
	while (1) {
		n++;
		if (n % 1000000 == 0) fprintf(stderr, "%.2lf%% checked\n", (n / (real)num_lines * 100));
		fread(&cr, sizeof(CREC), 1, fin);
		if(feof(fin))break;
		// word1 = vocab[cr.word1 - 1LL];
		// word2 = vocab[cr.word2 - 1LL];

		word1 = &vocab[(cr.word1 - 1LL) * MAX_WORD_LENGTH];
		word2 = &vocab[(cr.word2 - 1LL) * MAX_WORD_LENGTH];
		biword = ConcatenateWord(word1, word2);
		count = GetCount(biword);
		if (cr.val != count) {
			fprintf(stderr, "%s#%s CR val: lf%, Bigram Table val: %ls", word1, word2, cr.val, count);
		}
		assert(cr.val == count);
		free(biword);
	}
	fclose(fin);

	fprintf(stderr, "vocab memory size: %.2f mb\n", vocab_max_size * MAX_WORD_LENGTH * sizeof(char) / 1024. / 1024.);
	fprintf(stderr, "Hash table ptr size: %.2f mb\n", hash_size * sizeof(BIWORD *) / 1024. / 1024.);
	fprintf(stderr, "Hash element size: %.2f mb\n", num_lines * sizeof(BIWORD) / 1024. / 1024.);
	fprintf(stderr, "Total biword length %.0f, Biword size: %.2f mb\n", total_biword_len, total_biword_len * sizeof(char) / 1024. / 1024.);

	return 0;
}
