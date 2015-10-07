

#ifdef __GNUC__
#if __GNUC__==2
{"__GNUC__", "2"},		/*XXXX*/
{"__GNUC_MINOR__","95"},
#else
{"__GNUC__", "3"},		/*XXXX*/
{"__GNUC_MINOR__","0"},
#endif
#endif


#ifdef __ELF__
{"__ELF__","1"},
#endif

#ifdef _MIPS_SIM
{"_MIPS_SIM","1"},
#endif
#ifdef _MIPS_ISA
#if  _MIPS_IS ==1
{"_MIPS_ISA","1"},
#else
{"_MIPS_ISA","2"},		/*XXX*/
#endif
#endif

#ifdef  _R3000
{"_R3000","1"},
#endif
#ifdef  __R3000
{"__R3000","1"},
#endif
#ifdef  __R3000__
{"__R3000__","1"},
#endif

#ifdef  _R5900
{"_R5900","1"},
#endif
#ifdef  __R5900
{"__R5900","1"},
#endif

#ifdef MIPSEL
{"MIPSEL","1"},
#endif
#ifdef __MIPSEL
{"__MIPSEL","1"},
#endif
#ifdef __MIPSEL__
{"__MIPSEL__","1"},
#endif

#ifdef MIPSEB
{"MIPSEB","1"},
#endif
#ifdef __MIPSEB
{"__MIPSEB","1"},
#endif
#ifdef __MIPSEB__
{"__MIPSEB__","1"},
#endif

#ifdef _MIPS_SZLONG
#if _MIPS_SZLONG==32
{"_MIPS_SZLONG","32"},
#else
{"_MIPS_SZLONG","64"},
#endif
#endif
#ifdef _MIPS_SZINT
#if _MIPS_SZINT==32
{"_MIPS_SZINT","32"},
#else
{"_MIPS_SZINT","64"},
#endif
#endif
#ifdef _MIPS_SZPTR
#if _MIPS_SZPTR==32
{"_MIPS_SZPTR","32"},
#else
{"_MIPS_SZPTR","64"},
#endif
#endif
#ifdef _MIPS_FPSET
#if _MIPS_FPSET==32
{"_MIPS_FPSET","32"},
#else
{"_MIPS_FPSET","64"},
#endif
#endif

#ifdef __PIC__
{"__PIC__","1"},
#endif
#ifdef __pic__
{"__pic__","1"},
#endif

#ifdef __linux
{"__linux","1"},
#endif
#ifdef __linux__
{"__linux__","1"},
#endif

