/*
 *     MakePPF3.c (Linux Version)
 *     written by Icarus/Paradox
 *
 *     Big Endian support by Hu Kares.
 *
 *     Creates PPF3.0 Patches.
 *     Feel free to use this source in and for your own
 *     programms.
 *
 *     To compile enter:
 *     gcc -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE makeppf3_linux.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#if defined(__APPLE__) || defined (MACOSX)

//////////////////////////////////////////////////////////////////////
// fseeko is already 64 bit for Darwin/MacOS X!
// fseeko64 undefined for Darwin/MacOS X!

#define	fseeko64		fseeko

//////////////////////////////////////////////////////////////////////
// ftello is already 64 bit for Darwin/MacOS X!
// ftello64 undefined for Darwin/MacOS X!

#define	ftello64		ftello

//////////////////////////////////////////////////////////////////////
// fopen is already 64 bit for Darwin/MacOS X!
// fopen64 undefined for Darwin/MacOS X!

#define	fopen64			fopen

//////////////////////////////////////////////////////////////////////
// "off_t" is already 64 bit for Darwin/MacOS X!
// "__off64_t" undefined for Darwin/MacOS X!

typedef	off_t			__off64_t;

#endif /* __APPLE__ || MACOSX */

//////////////////////////////////////////////////////////////////////
// Macros for little to big Endian conversion.

#ifdef __BIG_ENDIAN__

#define Endian16_Swap(value)	(value = (((((unsigned short) value) << 8) & 0xFF00)  | \
                                          ((((unsigned short) value) >> 8) & 0x00FF)))

#define Endian32_Swap(value)    (value = (((((unsigned long) value) << 24) & 0xFF000000)  | \
                                          ((((unsigned long) value) <<  8) & 0x00FF0000)  | \
                                          ((((unsigned long) value) >>  8) & 0x0000FF00)  | \
                                          ((((unsigned long) value) >> 24) & 0x000000FF)))

#define Endian64_Swap(value)	(value = (((((unsigned long long) value) << 56) & 0xFF00000000000000ULL)  | \
                                          ((((unsigned long long) value) << 40) & 0x00FF000000000000ULL)  | \
                                          ((((unsigned long long) value) << 24) & 0x0000FF0000000000ULL)  | \
                                          ((((unsigned long long) value) <<  8) & 0x000000FF00000000ULL)  | \
                                          ((((unsigned long long) value) >>  8) & 0x00000000FF000000ULL)  | \
                                          ((((unsigned long long) value) >> 24) & 0x0000000000FF0000ULL)  | \
                                          ((((unsigned long long) value) >> 40) & 0x000000000000FF00ULL)  | \
                                          ((((unsigned long long) value) >> 56) & 0x00000000000000FFULL)))

#else

#define	Endian16_Swap(value)
#define	Endian32_Swap(value)
#define	Endian64_Swap(value)

#endif /* __BIG_ENDIAN__ */

//////////////////////////////////////////////////////////////////////
// Used global variables.
FILE *ppf, *bin, *mod, *fileid;


void *a, *b;
unsigned char *x, *y;
typedef struct Arg
{
	int  undo;
	int  blockcheck;
	int  imagetype;
	int  desc;
	int  fileid;
	char *description;
	char *fileidname;
	char *origname;
	char *modname;
	char *ppfname;
}Argumentblock;
Argumentblock Arg;

//////////////////////////////////////////////////////////////////////
// Used prototypes.
int		OpenFilesForCreate(void);
void		CloseAllFiles(void);
void		CheckSwitches(int argc, char **argv);
void		PPFCreatePatch(void);
int		PPFCreateHeader(void);
int		PPFGetChanges(void);
int		WriteChanges(int amount, __off64_t chunk);
int		PPFAddFileId(void);
int		CheckIfPPF3(void);
void 		PPFShowPatchInfo(void);
int		CheckIfFileId(void);
//////////////////////////////////////////////////////////////////////
// Main routine.
int main(int argc, char **argv){
	printf("MakePPF v3.0 by =Icarus/Paradox= %s\n", __DATE__);
#ifdef __BIG_ENDIAN__
        printf("Big Endian support by =Hu Kares=\n\n");			// <Hu Kares> sum credz
#endif /* __BIG_ENDIAN__ */         
	if(argc==1){
		printf("Usage: PPF <command> [-<sw> [-<sw>...]] <original bin> <modified bin> <ppf>\n");
		printf("<Commands>\n");
		printf("  c : create PPF3.0 patch            a : add file_id.diz\n");
		printf("  s : show patchinfomation\n");
		printf("<Switches>\n");
		printf(" -u        : include undo data (default=off)\n");
		printf(" -x        : disable patchvalidation (default=off)\n");
		printf(" -i [0/1]  : imagetype, 0 = BIN, 1 = GI (default=bin)\n");
		printf(" -d \"text\" : use \"text\" as description\n");
		printf(" -f \"file\" : add \"file\" as file_id.diz\n");
                printf("\nExamples: PPF c -u -i 1 -d \"my elite patch\" game.bin patch.bin output.ppf\n");
		printf("          PPF a patch.ppf myfileid.txt\n");
		return(0);
	}

	//////////////////////////////////////////////////////////////////////
	// Setting defaults.
	Arg.undo=0;
	Arg.blockcheck=1;
	Arg.imagetype=0;
	Arg.fileid=0;
	Arg.desc=0;

	switch(*argv[1]){
	case 'c':			if(argc<5){
							printf("Error: need more input for command '%s'\n",argv[1]);
							break;
						}
						CheckSwitches(argc, argv);
						if(OpenFilesForCreate()){
							PPFCreatePatch();
						} else {
							break;
						}
						CloseAllFiles();
						break;
		case 's':		
						if(argc<3){
							printf("Error: need more input for command '%s'\n",argv[1]);
							break;
						}
						ppf=fopen(argv[2], "rb");
						if(!ppf){
							printf("Error: cannot open file '%s' ",argv[2]);
							break;
						}
						if(!CheckIfPPF3()){
							printf("Error: file '%s' is no PPF3.0 patch\n", argv[2]);
							fclose(ppf);
							break;
						}

						PPFShowPatchInfo();
						fclose(ppf);
						break;
		case 'a':		
						if(argc<4){
							printf("Error: need more input for command '%s'\n",argv[1]);
							break;
						}
						ppf=fopen(argv[2], "rb+");
						if(!ppf){
							printf("Error: cannot open file '%s' ",argv[2]);
							break;
						}

						fileid=fopen(argv[3], "rb");
						if(!fileid){
							printf("Error: cannot open file '%s' ",argv[3]);
							CloseAllFiles();
							break;
						}
						
						if(!CheckIfPPF3()){
							printf("Error: file '%s' is no PPF3.0 patch\n", argv[2]);
							CloseAllFiles();
							break;
						}

						if(!CheckIfFileId()){
							PPFAddFileId();
						} else {
							printf("Error: patch already contains a file_id.diz\n");
						}

						CloseAllFiles();
						break;

		default :		printf("Error: unknown command '%s'\n",argv[1]); break;
	}

	printf("Done.\n");
	return(0);
}

//////////////////////////////////////////////////////////////////////
// Show various patch information. (PPF3.0 Only)
void PPFShowPatchInfo(void){
	unsigned char x, desc[50], id[3073];
	unsigned short y;

	printf("Showing patchinfo... \n");
	printf("Version     : PPF3.0\n");
	printf("Enc.Method  : 2\n");
	fseeko64(ppf,56,SEEK_SET);
	fread(&x, 1, 1, ppf);
	printf("Imagetype   : ");
	if(!x){
		printf("BIN\n");
	} else {
		printf("GI\n");
	}

	fseeko64(ppf,57,SEEK_SET);
	fread(&x, 1, 1, ppf);
	printf("Validation  : ");
	if(!x){
		printf("Disabled\n");
	} else {
		printf("Enabled\n");
	}

	fseeko64(ppf,58,SEEK_SET);
	fread(&x, 1, 1, ppf);
	printf("Undo Data   : ");
	if(!x){
		printf("Not available\n");
	} else {
		printf("Available\n");
	}
	
	fseeko64(ppf,6,SEEK_SET);
	fread(&desc, 1, 50, ppf);
	desc[50]=0;
	printf("Description : %s\n",desc);

	printf("File.id_diz : ");
	if(!CheckIfFileId()){
		printf("Not available\n");
	} else {
		printf("Available\n");
		fseeko64(ppf,-2,SEEK_END);
		fread(&y, 1, 2, ppf);
                Endian16_Swap (y);		// <Hu Kares> little to big endian
                if (y > 3072) {			// <Hu Kares> to be secure: avoid segmentation fault!
                    y = 3072;
                }
		fseeko64(ppf,-(y+18),SEEK_END);
		fread(&id, 1, y, ppf);
		id[y]=0;
		printf("%s\n",id);
	}	
}

//////////////////////////////////////////////////////////////////////
// Check if a file_id.diz is available.
// Return: 0 - No file_id.diz
// Return: 1 - Yes.
int CheckIfFileId(){
	unsigned int chkmagic;

	fseeko64(ppf,-6,SEEK_END);
	fread(&chkmagic, 1, 4, ppf);
        Endian32_Swap (chkmagic);		// <Hu Kares> little to big endian
	if(chkmagic=='ZID.'){ return(1); }
	
	return(0);
}

//////////////////////////////////////////////////////////////////////
// Check if a file is a PPF3.0 Patch.
// Return: 0 - No PPF3.0
// Return: 1 - PPF3.0
int CheckIfPPF3(){
	unsigned int chkmagic;

	fseeko64(ppf,0,SEEK_SET);
	fread(&chkmagic, 1, 4, ppf);
        Endian32_Swap (chkmagic);		// <Hu Kares> little to big endian
	if(chkmagic=='3FPP'){ return(1); }
	
	return(0);
}

//////////////////////////////////////////////////////////////////////
// This routine adds a file_id.diz to a PPF3.0 patch.
// Return: 0 - Okay
// Return: 1 - Failed.
int PPFAddFileId(void){
	unsigned short fileidlength=0;
	unsigned char fileidstart[]="@BEGIN_FILE_ID.DIZ", fileidend[]="@END_FILE_ID.DIZ", buffer[3072];

	fseeko64(fileid,0,SEEK_END);
	fileidlength=(unsigned short)ftell(fileid);
	if(fileidlength>3072) fileidlength=3072;
	fseeko64(fileid,0,SEEK_SET);

	fread(&buffer, 1, fileidlength, fileid);
	fseeko64(ppf,0,SEEK_END);

	printf("Adding file_id.diz... "); fflush(stdout);

	if((fwrite(&fileidstart, 1, 18, ppf)) != 18) return(1);
	if((fwrite(&buffer, 1, fileidlength, ppf)) != fileidlength) return(1);
	if((fwrite(&fileidend, 1, 16, ppf)) != 16) return(1);
        Endian16_Swap (fileidlength);					// <Hu Kares> big to little endian
	if((fwrite(&fileidlength, 1, 2, ppf)) != 2) return(1);
	
	printf("done.\n");
	return(0);
}

//////////////////////////////////////////////////////////////////////
// Start to create the patch.
void PPFCreatePatch(void){

	if(PPFCreateHeader()){ printf("Error: headercreation failed\n"); return; }

	PPFGetChanges();
	if(Arg.fileid) PPFAddFileId();

	return;
}

//////////////////////////////////////////////////////////////////////
// Part of the PPF3.0 algorythm to find file-changes. Please note that
// 16 MegaBit is needed for the engine. Allocated by malloc();
// Return: 1 - Failed
// Return: 0 - Success
int PPFGetChanges(void){
	int read=0, eightmb=1048576, changes=0;
	unsigned long found=0;
	__off64_t chunk=0, filesize;
	float percent;

	//Allocate memory (8 Megabit = 1 Chunk)
	a=malloc(eightmb);
	x=(unsigned char*)(a);
	if(x==NULL){ printf("Error: insufficient memory available\n"); return(1); }

	//Allocate memory (8 Megabit = 1 Chunk)	
	b=malloc(eightmb);
	y=(unsigned char*)(b);
	if(y==NULL){ printf("Error: insufficient memory available\n"); free(x); return(1); }

	fseeko64(bin,0,SEEK_END);
	filesize=ftello64(bin);
        
        // <Hu Kares> just security:
        if(filesize == 0){ printf("Error: filesize of bin file is zero!\n"); free(x); free(y); return(1);}

	fseeko64(bin,0,SEEK_SET);
	fseeko64(mod,0,SEEK_SET);

	printf("Finding differences... \n");
	printf("Progress: "); fflush(stdout);
	do{
		read=fread(x, 1, eightmb, bin);
		if(read!=0){
			if(read==eightmb){
				fread(y, 1, eightmb, mod);
				changes=WriteChanges(eightmb, chunk);
			} else {
				fread(y, 1, read, mod);
				changes=WriteChanges(read, chunk);
			}			
		}
		chunk+=eightmb;
		found+=changes;

		percent=(float)chunk/filesize;
		printf("%6.2f %%\b\b\b\b\b\b\b\b",percent*100); fflush(stdout);

	} while (read!=0);

	printf("100.00 %% (%d entries found).\n", found);

	//Free memory.
	free(x); free(y);
	return(0);
}

//////////////////////////////////////////////////////////////////////
// This function actually scans the 8 Mbit blocks and writes down the
// patchdata.
// Return: Found differences.
int WriteChanges(int amount, __off64_t chunk){
	int found=0;
	__off64_t i=0, offset;

	for(i=0;i<amount;i++){
		if(x[i]!=y[i]){
			unsigned char k=0;
			offset=chunk+i;
                        Endian64_Swap (offset);			// <Hu Kares> big to little endian	
			fwrite(&offset, 1, 8, ppf );
			do{		
				k++; i++;
			} while (i<amount&&x[i]!=y[i]&&k!=0xff);
			fwrite(&k, 1, 1, ppf);
			fwrite(&y[i-k], 1, k, ppf);
			found++;
			if(Arg.undo){
				fwrite(&x[i-k], 1, k, ppf);	// Write undo data aswell
			}
			if(k==0xff) i--;
		}
	}
	return(found);
}


//////////////////////////////////////////////////////////////////////
// Create PPF3.0 Header.
// Return: 1 - Failed
// Return: 0 - Success
int PPFCreateHeader(void){
	unsigned char method=0x02, description[128], binblock[1024], dummy=0, i=0;
	unsigned char magic[]="PPF30";

	printf("Writing header... "); fflush(stdout);
	memset(description,0x20,50);

	if(Arg.desc){
		for(i=0;i<strlen(Arg.description);i++){
			description[i]=Arg.description[i];
		}
	}

	if((fwrite(&magic, 1, 5, ppf)) != 5 ) return(1);
	if((fwrite(&method, 1, 1, ppf)) != 1 ) return(1);
	if((fwrite(&description, 1, 50, ppf)) != 50 ) return(1);
	if((fwrite(&Arg.imagetype, 1, 1, ppf)) != 1 ) return(1);
	if((fwrite(&Arg.blockcheck, 1, 1, ppf)) != 1 ) return(1);
	if((fwrite(&Arg.undo, 1, 1, ppf)) != 1 ) return(1);
	if((fwrite(&dummy, 1, 1, ppf)) != 1 ) return(1);

	if(Arg.blockcheck){
		if(Arg.imagetype){
			fseeko64(bin,0x80A0,SEEK_SET);
		} else {
			fseeko64(bin,0x9320,SEEK_SET);
		}
		fread(&binblock, 1, 1024, bin);
		if((fwrite(&binblock, 1, 1024, ppf)) != 1024 ) return(1);
	}

	printf("done.\n");
	return(0);
}


//////////////////////////////////////////////////////////////////////
// Check all switches given in commandline and fill Arg structure.
// Return: 0 - Failed
// Return: 1 - Success
int OpenFilesForCreate(void){
	__off64_t fl1, fl2;

	bin=fopen64(Arg.origname, "rb");
	if(!bin){
		printf("Error: cannot open file '%s' ",Arg.origname);
		CloseAllFiles();
		return(0);
	}	
	mod=fopen64(Arg.modname,  "rb");
	if(!mod){
		printf("Error: cannot open file '%s' ",Arg.modname);
		CloseAllFiles();
		return(0);
	}

	fseeko64(bin,0,SEEK_END);
	fseeko64(mod,0,SEEK_END);
	fl1=ftello64(bin);
	fl2=ftello64(bin);
	//Check if files have same size.
	if(fl1 != fl2){
		printf("Error: bin files are different in size.");
		CloseAllFiles();
		return(0);
	}

	if(Arg.fileid){
		fileid=fopen64(Arg.fileidname, "rb");
		if(!fileid){
			printf("Error: cannot open file '%s' ",Arg.fileidname);
			CloseAllFiles();
			return(0);
		}
	}


	ppf=fopen64(Arg.ppfname, "wb+");
	if(!ppf){
		printf("Error: cannot create file '%s' ",Arg.ppfname);
		CloseAllFiles();
		return(0);
	}


	return(1);
}

//////////////////////////////////////////////////////////////////////
// Closing all files which are currently opened.
void CloseAllFiles(void){
	if(ppf) fclose(ppf);
	if(bin) fclose(bin);
	if(mod) fclose(mod);
	if(fileid) fclose(fileid);
}


//////////////////////////////////////////////////////////////////////
// Check all switches given in commandline and fill Arg structure.
void CheckSwitches(int argc, char **argv){
	int i;
	unsigned char *x;

	for(i=2;i<argc;i++){
		x=(unsigned char*)(argv[i]);
		if(x[0]=='-'){
		
			switch(x[1]){
				case 'u'	:	Arg.undo=1; break;
				case 'x'	:	Arg.blockcheck=0; break;
				case 'i'	:	if(*argv[i+1]=='0') Arg.imagetype=0;
								if(*argv[i+1]=='1') Arg.imagetype=1;
								i++;
								break;
				case 'd'	:	Arg.desc=1; Arg.description=argv[i+1];
								i++;
								break;
				case 'f'	:	Arg.fileid=1; Arg.fileidname=argv[i+1];
								i++;
								break;
				default		:	break;
			}

		}
	}
	Arg.ppfname=argv[argc-1];
	Arg.modname=argv[argc-2];
	Arg.origname=argv[argc-3];
}
