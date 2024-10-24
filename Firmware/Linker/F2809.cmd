/*
// TI File $Revision: /main/2 $
// Checkin $Date: August 2, 2006   16:57:02 $
//###########################################################################
//
// FILE:	F2809.cmd
//
// TITLE:	Linker Command File For F2809 Device
//
//###########################################################################
// $TI Release:$
// $Release Date:$
//###########################################################################
*/

/* ======================================================
// For Code Composer Studio V2.2 and later
// ---------------------------------------
// In addition to this memory linker command file, 
// add the header linker command file directly to the project. 
// The header linker command file is required to link the
// peripheral structures to the proper locations within 
// the memory map.
//
// The header linker files are found in <base>\DSP280x_Headers\cmd
//   
// For BIOS applications add:      DSP280x_Headers_BIOS.cmd
// For nonBIOS applications add:   DSP280x_Headers_nonBIOS.cmd    
========================================================= */

/* ======================================================
// For Code Composer Studio prior to V2.2
// --------------------------------------
// 1) Use one of the following -l statements to include the 
// header linker command file in the project. The header linker
// file is required to link the peripheral structures to the proper 
// locations within the memory map                                    */

/* Uncomment this line to include file only for non-BIOS applications */
/* -l DSP280x_Headers_nonBIOS.cmd */

/* Uncomment this line to include file only for BIOS applications */
/* -l DSP280x_Headers_BIOS.cmd */

/* 2) In your project add the path to <base>\DSP280x_headers\cmd to the
   library search path under project->build options, linker tab, 
   library search path (-i).
/*========================================================= */

/* Define the memory block start/length for the F2809  
   PAGE 0 will be used to organize program sections
   PAGE 1 will be used to organize data sections

   Notes: 
         Memory blocks on F2809 are uniform (ie same
         physical memory) in both PAGE 0 and PAGE 1.  
         That is the same memory region should not be
         defined for both PAGE 0 and PAGE 1.
         Doing so will result in corruption of program 
         and/or data. 
         
         L0/L1 and H0 memory blocks are mirrored - that is
         they can be accessed in high memory or low memory.
         For simplicity only one instance is used in this
         linker file. 
         
         Contiguous SARAM memory blocks or flash sectors can be
         be combined if required to create a larger memory block. 
*/

MEMORY
{
PAGE 0:    /* Program Memory */
           /* Memory (RAM/FLASH/OTP) blocks can be moved to PAGE1 for data allocation */

   RAML0L10     : origin = 0x008000, length = 0x001500     /* on-chip RAM block L0 & L1(0)*/
   OTP          : origin = 0x3D7800, length = 0x000400     /* on-chip OTP */
   FLASHH       : origin = 0x3D8000, length = 0x004000     /* on-chip FLASH */
   FLASHG       : origin = 0x3DC000, length = 0x004000     /* on-chip FLASH */
   FLASHF       : origin = 0x3E0000, length = 0x004000     /* on-chip FLASH */
   FLASHE       : origin = 0x3E4000, length = 0x004000     /* on-chip FLASH */
   FLASHD       : origin = 0x3E8000, length = 0x004000     /* on-chip FLASH */
   BEGIN        : origin = 0x3EC000, length = 0x000002     /* Part of FLASHC. Entry point for main firmware */
   FLASHC       : origin = 0x3EC002, length = 0x003ffe     /* on-chip FLASH */
   FLASHAB      : origin = 0x3F0000, length = 0x007F80     /* on-chip FLASH */
   CSM_RSVD     : origin = 0x3F7F80, length = 0x000076     /* Part of FLASHA.  Program with all 0x0000 when CSM is in use. */
   LOADER_BEGIN : origin = 0x3F7FF6, length = 0x000002     /* Part of FLASHA.  Used for "boot to Flash" bootloader mode. */
   CSM_PWL      : origin = 0x3F7FF8, length = 0x000008     /* Part of FLASHA.  CSM password locations in FLASHA */
   
   ROM          : origin = 0x3FF000, length = 0x000FC0     /* Boot ROM */
   RESET        : origin = 0x3FFFC0, length = 0x000002     /* part of boot ROM  */
   VECTORS      : origin = 0x3FFFC2, length = 0x00003E     /* part of boot ROM  */

PAGE 1 :   /* Data Memory */
           /* Memory (RAM/FLASH/OTP) blocks can be moved to PAGE0 for program allocation */
           /* Registers remain on PAGE1                                                  */

   RAMM0       : origin = 0x000000, length = 0x0003FF     /* on-chip RAM block M0 */
   RAMM0_FL    : origin = 0x0003FF, length = 0x000001     /* on-chip RAM block M0 for boot flag */
   RAMM1       : origin = 0x000480, length = 0x000380     /* on-chip RAM block M1 */
   BOOT_RSVD   : origin = 0x000400, length = 0x000080     /* Part of M1, BOOT rom will use this for stack */
   RAML11      : origin = 0x009500, length = 0x000B00     /* on-chip RAM block L1(1) */
   RAMH0       : origin = 0x3FA000, length = 0x002000     /* on-chip RAM block H0 */
}

/* Allocate sections to memory blocks.
   Note:
         codestart user defined section in DSP28_CodeStartBranch.asm used to redirect code 
                   execution when booting to flash
         ramfuncs  user defined section to store functions that will be copied from Flash into RAM
*/ 
 
SECTIONS
{
Flash28_API:
   {
        -lFlash2809_API_V100.lib(.econst)
        -lFlash2809_API_V100.lib(.text)
   }                   LOAD = FLASHD,
                       RUN = RAML0L10,
                       /* � ���������� Flash28_API_LoadStart ���-�� ���. ���.
                          �������  Flash28_API */
                       LOAD_START(_Flash28_API_LoadStart),
                       /* � ���������� Flash28_API_LoadEnd ���������� ���. ���.
                          �������  Flash28_API */
                       LOAD_END(_Flash28_API_LoadEnd),
                       /* � ���������� Flash28_API_RunStart ���������� ���.
                          ��� ������ API functions � RAM */
                       RUN_START(_Flash28_API_RunStart),
                       PAGE = 0

   /* Allocate program areas: */
   .cinit              : > FLASHC      PAGE = 0
   .pinit              : > FLASHC      PAGE = 0
   .text               : > FLASHC      PAGE = 0
   codestart           : > BEGIN       PAGE = 0
   ramfuncs            : LOAD = FLASHD,
                         RUN = RAML0L10,
                         LOAD_START(_RamfuncsLoadStart),
                         LOAD_END(_RamfuncsLoadEnd),
                         RUN_START(_RamfuncsRunStart),
                         PAGE = 0

   csmpasswds          : > CSM_PWL     PAGE = 0
   csm_rsvd            : > CSM_RSVD    PAGE = 0
   
   /* Allocate uninitalized data sections: */
   .stack              : > RAMM1       PAGE = 1
   .esysmem            : > RAMM1       PAGE = 1
   .ebss               : > RAML11      PAGE = 1

   /* Initalized sections go in Flash */
   /* For SDFlash to program these, they must be allocated to page 0 */
   .econst             : > FLASHD      PAGE = 0
   .switch             : > FLASHC      PAGE = 0

   /* Allocate IQ math areas: */
   IQmath              : LOAD = FLASHD,
                         RUN = RAML0L10,
                         LOAD_START(_IQmathLoadStart),
                         LOAD_END(_IQmathLoadEnd),
                         RUN_START(_IQmathRunStart),
                         PAGE = 0
                           
   IQmathTables        : > ROM, PAGE = 0, TYPE = NOLOAD

   IQmathTablesRam     : LOAD = FLASHD, PAGE = 0
                         RUN = RAMM0, PAGE = 1
                         LOAD_START(_IQmathTablesRamLoadStart),
                         LOAD_END(_IQmathTablesRamLoadEnd),
                         RUN_START(_IQmathTablesRamRunStart)

   /* Constants caching */
   ramconsts		   : LOAD = FLASHD, PAGE = 0
						 RUN = RAMM0, PAGE = 1
						 LOAD_START(_RamconstsLoadStart),
                         LOAD_END(_RamconstsLoadEnd),
						 RUN_START(_RamconstsRunStart)

   /* Allocate large memory blocks section: */
   data_mem	           : > RAMH0       PAGE = 1
   /* Allocate special memory blocks section: */
   data_mem2           : > RAMM0       PAGE = 1
   /* Allocate special memory boot-loader flag: */
   bl_flag             : > RAMM0_FL    PAGE = 1

   /* .reset is a standard section used by the compiler.  It contains the */ 
   /* the address of the start of _c_int00 for C Code.   /*
   /* When using the boot ROM this section and the CPU vector */
   /* table is not needed.  Thus the default type is set here to  */
   /* DSECT  */ 
   .reset              : > RESET,      PAGE = 0, TYPE = DSECT
   vectors             : > VECTORS     PAGE = 0, TYPE = DSECT
}

/*
//===========================================================================
// End of file.
//===========================================================================
*/

