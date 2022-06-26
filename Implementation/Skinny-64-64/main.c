#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <string.h>
#include "global.h"
#include "rprintf.h"
#include "lcd.h"


#define 	hexchar(X)   pgm_read_byte( HexChars+((X)&0x0f) )
#define D4 eS_PORTA4
#define D5 eS_PORTA5
#define D6 eS_PORTA6
#define D7 eS_PORTA7
#define RS eS_PORTA0
#define RW eS_PORTA1
#define EN eS_PORTA2

uint32_t hex2int(char *hex) {
	uint32_t val = 0;
	while (*hex) {
		// get current character then increment
		char byte = *hex++;
		// transform hex character to the 4bit equivalent number, using the ascii table indexes
		if (byte >= '0' && byte <= '9') byte = byte - '0';
		else if (byte >= 'a' && byte <='f') byte = byte - 'a' + 10;
		else if (byte >= 'A' && byte <='F') byte = byte - 'A' + 10;
		// shift 4 to make space for new digit, and add the 4 bits of the new digit
		val = (val << 4) | (byte & 0xF);
	}
	return val;
}

void subCells(unsigned char *IS);
void addConstants(unsigned char *IS, int round);
void addRoundTweakey(unsigned char *IS, unsigned char *TK1);//), unsigned char *TK2, unsigned char *TK3);
void shiftRows(unsigned char *IS);
void mixColumns(unsigned char *IS);
void skinny(unsigned char *c, const unsigned char *p, const unsigned char *k);

static const unsigned char check_k[] = {
	/* Known Answer Test A */
	//0xf, 0x5, 0x2, 0x6, 0x9, 0x8, 0x2, 0x6, 0xf, 0xc, 0x6, 0x8, 0x1, 0x2, 0x3, 0x8/* tweakey */ //f5269826fc681238
	0x8, 0x3,0x2, 0x1, 0x8, 0x6, 0xc, 0xf, 0x6, 0x2, 0x8, 0x9, 0x6, 0x2, 0x5, 0xf
};

static const unsigned char check_p[] = {
	/* Known Answer Test A */
	//0x0, 0x6, 0x0, 0x3, 0x4, 0xf, 0x9, 0x5, 0x7, 0x7, 0x2, 0x4, 0xd, 0x1, 0x9, 0xd// 0x9f, 0x44, 0xe9, 0x2b, 0x08, 0xf5, 0x50, 0xcb, /* plaintext */ 06034f957724d19d
	0xd, 0x9, 0x1, 0xd, 0x4, 0x2, 0x7, 0x7, 0x5, 0x9, 0xf, 0x4, 0x3, 0x0, 0x6, 0x0

};


static const unsigned char check_c[] = {
	
	0x7, 0xc, 0xa, 0x8, 0xb, 0x9, 0x2, 0x4, 0x2, 0xb, 0xf, 0xd, 0x9, 0x3, 0xb ,0xb// 0x1b, 0x38, 0xc6, 0x34, 0x6a, 0x10, 0xdc, 0xfa, /* ciphertext */ bb39dfb2429b8ac7

};


/* SKINNY Sbox */
static const int S4 [16] = {
	0xc,0x6, 0x9,0x0, 0x1,0xa, 0x2,0xb ,0x3,0x8 ,0x5,0xd ,0x4,0xe ,0x7,0xf};

/*Constants for AddConstants operation*/
static const unsigned char constants[32] = {
	0x01, 0x03, 0x07, 0x0f, 0x1f ,0x3e ,0x3d ,0x3b ,0x37 ,0x2f ,0x1e ,0x3c ,0x39 ,0x33 ,0x27 ,0x0e
   ,0x1d ,0x3a, 0x35, 0x2b, 0x16 ,0x2c ,0x18 ,0x30 ,0x21 ,0x02 ,0x05 ,0x0b ,0x17 ,0x2e ,0x1c ,0x38};
//0x01, 0x03, 0x07, 0x0f, 0x1f ,0x3e ,0x3d ,0x3b ,0x37 ,0x2f ,0x1e ,0x3c ,0x39 ,0x33 ,0x27 ,0x0e
//,0x1d ,0x3a, 0x35, 0x2b, 0x16 ,0x2c ,0x18 ,0x30 ,0x21 ,0x02 ,0x05 ,0x0b ,0x17 ,0x2e ,0x1c ,0x38

/*Binary matrix used in MixColumns operation*/
static const uint8_t M[4][4]= {
	{1, 0, 1, 1},
	{1, 0, 0, 0},
	{0, 1, 1, 0},
	{1, 0, 1, 0}};
		
		
	/*Permutation indeces for the key permutation in AddRoundTweakey*/
static const uint8_t key_perm [16]= {9, 15, 8, 13, 10, 14, 12, 11, 0, 1, 2, 3, 4, 5, 6, 7};
	
	/*Performs the SubCells operation*/
void subCells(unsigned char *IS) {

		//Goes through the array and swaps the value based on the Sbox
		for(int i = 0; i < 16; ++i){
			IS[i] = S4[IS[i]];
		}
}
	
/*Performs the AddConstants operation*/
void addConstants(unsigned char *IS, int round){

	//Get the correct values for constants c0 and c1
	unsigned char rc = constants[round];
	unsigned char c0 = rc << 4;
	c0 = c0 >> 4;
	unsigned char c1 = rc >> 4;

	//XOR the constants to the desired bytes
	IS[0] = IS[0] ^ c0;
	IS[4] = IS[4] ^ c1;
	IS[8] = IS[8] ^ 0x2;
}


/*Performs the AddRoundTweakey operation*/
void addRoundTweakey(unsigned char *IS, unsigned char *TK1){//, unsigned char *TK2, unsigned char *TK3){

	//XOR the keys to text to be ciphered
	for(int j = 0; j < 16; j++){
		IS[j] = IS[j]^TK1[j];
	}

	//Helper for storing temporary values
	unsigned char temp_key[16];
	memcpy(temp_key, TK1, sizeof(temp_key));

	//permutate TK1
	for(int i = 0; i < 16; ++i){
		TK1[i] = temp_key[key_perm[i]];
	}
}


/*Performs the ShiftRows operation*/
void shiftRows(unsigned char *IS){

	//Just swaps positions of the cells
	unsigned char temp2[16] = {IS[0], IS[1], IS[2], IS[3], IS[7], IS[4], IS[5], IS[6],
	IS[10], IS[11], IS[8], IS[9], IS[13], IS[14], IS[15], IS[12]};
	memcpy(IS, temp2, 16);
}

/*Performs the MixColumns operation*/
void mixColumns(unsigned char *IS){
	
	//Transform IS to matrix for easier matrix multiplication
	unsigned char IS_as_matrix[4][4] = {
		{IS[0], IS[1], IS[2], IS[3]},
		{IS[4], IS[5], IS[6], IS[7]},
		{IS[8], IS[9], IS[10], IS[11]},
		{IS[12], IS[13], IS[14], IS[15]}};

		//Matrix for storing the matrix multiplication answer
		unsigned char ans[4][4] = {0x00};

		//Loop structure for matrix multiplication
		for(int i = 0; i < 4; i++){
			for(int j = 0; j < 4; j++){
				for(int l = 0; l < 4; l++){
					if(M[j][l] == 1){
						ans[j][i] ^= IS_as_matrix[l][i];
					}
				}
			}
		}
		memcpy(IS, ans, 16);
	}
	




//* SKINNY-128-384 block cipher encryption.
//* Under 48-byte tweakey at k, encrypt 16-byte plaintext at p and store the 16-byte output at c.
//*/
void skinny(unsigned char *c, const unsigned char *p, const unsigned char *k) {

/*Initialize the IS array containing the plaintext and the 3 tweakeys*/
int round = 0;
unsigned char IS[16];
char *fp_data[16];
memcpy(IS, p, sizeof(IS));

unsigned char TK1[16];
memcpy(TK1, k, sizeof(TK1));

//Loop the cipher for 56 rounds
while(round < 32){
		//Subcells
		subCells(IS);

		//AddConstants
		addConstants(IS, round);

		//AddRoundKey
		addRoundTweakey(IS, TK1);

		//shift rows
		shiftRows(IS);

		//MixColumns
		mixColumns(IS);

		round++;
}

//Copy the final ciphertext to c
memcpy(c, IS, 16);


for(int i = 0; i < 16; i++){
	if (IS[i]==0)fp_data[i]="0";
	if (IS[i]==1)fp_data[i]="1";
	if (IS[i]==2)fp_data[i]="2";
	if (IS[i]==3)fp_data[i]="3";
	if (IS[i]==4)fp_data[i]="4";
	if (IS[i]==5)fp_data[i]="5";
	if (IS[i]==6)fp_data[i]="6";
	if (IS[i]==7)fp_data[i]="7";
	if (IS[i]==8)fp_data[i]="8";
	if (IS[i]==9)fp_data[i]="9";
	if (IS[i]==10)fp_data[i]="A";
	if (IS[i]==11)fp_data[i]="B";
	if (IS[i]==12)fp_data[i]="C";
	if (IS[i]==13)fp_data[i]="D";
	if (IS[i]==14)fp_data[i]="E";
	if (IS[i]==15)fp_data[i]="F";
}

lcdClear();
lcdGotoXY(0,0);
rprintf("Enc Skinny :");

for(int i = 0; i < 16; i++){
	lcdGotoXY(i, 1);
	uint16_t x = hex2int(fp_data[i]);
	rprintfu04(x);
}
sbi(PORTB,0);
_delay_ms(150);
cbi(PORTB,0);
_delay_ms(250);
lcdClear();
}




int main(int argc, char *argv[]) {
	lcdInit();
	rprintfInit(lcdDataWrite);
	sbi(DDRB,0);
	cbi(PORTB,0);
	const unsigned char *p = NULL;
	const unsigned char *k = NULL;
	unsigned char c[16];
	(void)argc;
	(void)argv;
	k = check_k;
	p = check_p;

	while (TRUE) {
		skinny(c, p, k);
	}
	return 0;
}