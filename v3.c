#include <efi.h> 
#include <efilib.h>

#include <cpuid.h>


static inline UINT64 rdmsr(UINT32 msr)
{
	uint32_t low, high;
	asm volatile (
		"rdmsr"
		: "=a"(low), "=d"(high)
		: "c"(msr)
	);
	return ((UINT64)high << 32) | low;
}


static inline void wrmsr(UINT32 msr, UINT64 value)
{
	uint32_t low = value & 0xFFFFFFFF;
	uint32_t high = value >> 32;
	asm volatile (
		"wrmsr"
		:
		: "c"(msr), "a"(low), "d"(high)
	);
}


// SVID fixed VCCIN voltages
#define				_DYNAMIC_SVID				0x0				// FIVR-controlled SVID
#define				_DEFAULT_SVID				0x0000074D			// no change to default VCCIN
#define				_VCCIN_1pt600				0x00000667			// 1.600 V
#define				_VCCIN_1pt625				0x00000680			// 1.625 V
#define				_VCCIN_1pt650				0x0000069A			// 1.650 V
#define				_VCCIN_1pt675				0x000006B4			// 1.675 V
#define				_VCCIN_1pt700				0x000006CD			// 1.700 V
#define				_VCCIN_1pt725				0x000006E7			// 1.725 V
#define				_VCCIN_1pt750				0x00000700			// 1.750 V
#define				_VCCIN_1pt775				0x0000071A			// 1.775 V
#define				_VCCIN_1pt800				0x00000734			// 1.800 V
#define				_VCCIN_1pt825				0x0000074D			// 1.825 V (default)
#define				_VCCIN_1pt850				0x00000767			// 1.850 V
#define				_VCCIN_1pt875				0x00000780			// 1.875 V
#define				_VCCIN_1pt900				0x0000079A			// 1.900 V
#define				_VCCIN_1pt925				0x000007B4			// 1.925 V
#define				_VCCIN_1pt950				0x000007CD			// 1.950 V
#define				_VCCIN_1pt975				0x000007E7			// 1.975 V
#define				_VCCIN_2pt000				0x00000800			// 2.000 V
#define				_VCCIN_2pt025				0x0000081A			// 2.025 V
#define				_VCCIN_2pt050				0x00000834			// 2.050 V
#define				_VCCIN_2pt075				0x0000084D			// 2.075 V
#define				_VCCIN_2pt100				0x00000867			// 2.100 V

// Adaptive negative dynamic voltage offsets for all domains
#define				_DEFAULT_FVID				0x0				// no change to default VID (Vcore)
#define				_FVID_MINUS_10_MV			0xFEC00000			// -10 mV  (-0.010 V)
#define				_FVID_MINUS_20_MV			0xFD800000			// -20 mV  (-0.020 V)
#define				_FVID_MINUS_25_MV			0xFCD00000			// -25 mV  (-0.025 V)
#define				_FVID_MINUS_30_MV			0xFC200000			// -30 mV  (-0.030 V)
#define				_FVID_MINUS_40_MV			0xFAE00000			// -40 mV  (-0.040 V)
#define				_FVID_MINUS_50_MV			0xF9A00000			// -50 mV  (-0.050 V)
#define				_FVID_MINUS_60_MV			0xF8600000			// -60 mV  (-0.060 V)
#define				_FVID_MINUS_65_MV			0xF7A00000			// -65 mV  (-0.065 V)
#define				_FVID_MINUS_67_MV			0xF7600000			// -67 mV  (-0.067 V)
#define				_FVID_MINUS_70_MV			0xF7000000			// -70 mV  (-0.070 V)
#define				_FVID_MINUS_75_MV			0xF6800000			// -75 mV  (-0.075 V)
#define				_FVID_MINUS_80_MV			0xF5C00000			// -80 mV  (-0.080 V)
#define				_FVID_MINUS_85_MV			0xF5200000			// -85 mV  (-0.085 V)
#define				_FVID_MINUS_90_MV			0xF4800000			// -90 mV  (-0.090 V)
#define				_FVID_MINUS_95_MV			0xF3E00000			// -95 mV  (-0.095 V)
#define				_FVID_MINUS_100_MV			0xF3400000			// -100 mV (-0.100 V)
#define				_FVID_MINUS_110_MV			0xF1E00000			// -110 mV (-0.110 V)
#define				_FVID_MINUS_120_MV			0xF0A00000			// -120 mV (-0.120 V)
#define				_FVID_MINUS_130_MV			0xEF600000			// -130 mV (-0.130 V)
#define				_FVID_MINUS_140_MV			0xEE200000			// -140 mV (-0.140 V)
#define				_FVID_MINUS_150_MV			0xECC00000			// -150 mV (-0.150 V)

// toolbox for MSR OC Mailbox (experimental)
#define				OC_MB_COMMAND_EXEC			0x8000000000000000
#define				OC_MB_GET_CPU_CAPS			0x0000000100000000
#define				OC_MB_GET_TURBO_RATIOS			0x0000000200000000
#define				OC_MB_GET_FVIDS_RATIOS			0x0000001000000000
#define				OC_MB_SET_FVIDS_RATIOS			0x0000001100000000
#define				OC_MB_GET_SVID_PARAMS			0x0000001200000000
#define				OC_MB_SET_SVID_PARAMS			0x0000001300000000
#define				OC_MB_GET_FIVR_PARAMS			0x0000001400000000
#define				OC_MB_SET_FIVR_PARAMS			0x0000001500000000

#define				OC_MB_DOMAIN_IACORE			0x0000000000000000		// IA Core domain
#define				OC_MB_DOMAIN_GFX			0x0000010000000000
#define				OC_MB_DOMAIN_CLR			0x0000020000000000		// CLR (CBo/LLC/Ring) a.k.a. Cache/Uncore domain
#define				OC_MB_DOMAIN_SA				0x0000030000000000		// System Agent (SA) domain
#define				OC_MB_DOMAIN_AIO			0x0000040000000000
#define				OC_MB_DOMAIN_DIO			0x0000050000000000

#define				OC_MB_FIVR_FAULTS_OVRD_EN		0x1				// bit 0
#define				OC_MB_FIVR_EFF_MODE_OVRD_EN		0x2				// bit 1
#define				OC_MB_FIVR_DYN_SVID_CONTROL_DIS		0x80000000			// bit 31

#define				OC_MB_SUCCESS				0x0
#define				OC_MB_REBOOT_REQUIRED			0x7

// Model Specific Registers
#define				MSR_IA32_BIOS_SIGN_ID			0x08B
#define				MSR_PLATFORM_INFO			0x0CE
#define				MSR_OC_MAILBOX				0x150
#define				MSR_FLEX_RATIO				0x194
#define				MSR_TURBO_RATIO_LIMIT			0x1AD
#define				MSR_TURBO_RATIO_LIMIT1 			0x1AE
#define				MSR_TURBO_RATIO_LIMIT2 			0x1AF
#define				MSR_TURBO_POWER_LIMIT			0x610	
#define				MSR_UNCORE_RATIO_LIMIT			0x620

// constants
#define			  CPUID_SIGNATURE		0x306F2				// target CPUID, set 0xFFFFFFFF to bypass checking
#define				AP_EXEC_TIMEOUT				1000000				// 1 second
#define				CPUID_VERSION_INFO			0x1
#define				CPUID_BRAND_STRING_BASE			0x80000002
#define				CPUID_BRAND_STRING_LEN			0x30
#define				MSR_FLEX_RATIO_OC_LOCK_BIT		(1<<20)			// bit 20, set to lock MSR 0x194
#define				MSR_TURBO_RATIO_SEMAPHORE_BIT		0x8000000000000000		// set to execute changes writen to MSR 0x1AD, 0x1AE, 0x1AF
#define				MSR_BIOS_NO_UCODE_PATCH			0x0
#define				UNLIMITED_POWER				0x00FFFFFF00FFFFFF




#define       OC_MB_CMD(CMD, DOMAIN, R) \
  wrmsr(MSR_OC_MAILBOX, OC_MB_COMMAND_EXEC | CMD | DOMAIN);\
  R = rdmsr(MSR_OC_MAILBOX);

#define OC_MB_ERROR(X) ((X >> 0x20) & 0xFF)

typedef struct {
  UINT64 core;
  UINT64 cache;
} cpu_caps_t;


// build options
#define CoreVOffset -70 /* mV */
#define CoreVStatic 980 /* mV */
#define CoreMulti   0          
#define CoreFixed   1          

#define CoreAdjVOffset (((CoreVOffset << 15)/1000) & 0x0ffe0)
#define CoreAdjVStatic (((CoreVStatic << 18)/1000) & 0xfff00)
#define CoreVoltage    (((CoreAdjVOffset << 16) | CoreAdjVStatic) | CoreMulti | (CoreFixed << 20))
//-------------------------------------------------------------------------------------------------------

#define CacheVOffset  -50
#define CacheVStatic  0
#define CacheMulti    0
#define CacheFixed    0

#define CacheAdjVOffset (((CacheVOffset << 15)/1000) & 0x0ffe0)
#define CacheAdjVStatic (((CacheVStatic << 18)/1000) & 0x0fff00)
#define CacheVoltage    (((CacheAdjVOffset << 16) | CacheAdjVStatic) | CacheMulti | (CacheFixed << 20))


EFI_STATUS efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SysTab) {
  EFI_STATUS Status;
  EFI_LOADED_IMAGE *LoadedImage = NULL;

  InitializeLib(ImageHandle, SysTab);

  cpu_caps_t caps = {0};

  Print(L"V3 Turbo Loaded\r\n");

  wrmsr(MSR_IA32_BIOS_SIGN_ID, 0);

  // Check CPU
  UINT32 cpuid[4] = {0};
  __cpuid(1, cpuid[0], cpuid[1], cpuid[2], cpuid[3]);

  if ( cpuid[0] != CPUID_SIGNATURE ){
    Print(L"FAIL: Cpuid mismatch, found 0x%X\n", cpuid);
    goto exit;
  }

  // Check MicroCode
  UINT64 rsp = rdmsr(MSR_IA32_BIOS_SIGN_ID);
  if ( rsp >> 0x20 ){
    Print(L"FAIL: Micro code present...\n");
    goto exit;
  }

	// check OC Lock Bit not set
	rsp = rdmsr(MSR_FLEX_RATIO);
  if ( rsp & MSR_FLEX_RATIO_OC_LOCK_BIT ){
    Print(L"FAIL: OC locked...\n");
    goto exit;
  }

  // Get Core OC ratio and caps
  OC_MB_CMD(OC_MB_GET_CPU_CAPS, OC_MB_DOMAIN_IACORE, caps.core)
  if ( OC_MB_ERROR(caps.core) ){
    Print(L"FAIL: Core OC caps read failed");
  }

  // Get Cache OC ratio and caps
  OC_MB_CMD(OC_MB_GET_CPU_CAPS, OC_MB_DOMAIN_CLR, caps.cache)
  if ( OC_MB_ERROR(caps.cache) ){
    Print(L"FAIL: Cache OC caps read failed");
  }

  // Set Core voltage
  {
    UINT64 val = CoreVoltage & ~0xffULL;
    val |= caps.core & 0xff;
    OC_MB_CMD(OC_MB_SET_FVIDS_RATIOS, OC_MB_DOMAIN_IACORE, rsp);
    if ( rsp ){
      Print(L"FAIL: Could not set core voltage");
    }
  }

  // Set Cache voltage
  {
    UINT64 val = CacheVoltage & ~0xffULL;
    val |= caps.cache & 0xff;
    OC_MB_CMD(OC_MB_SET_FVIDS_RATIOS, OC_MB_DOMAIN_IACORE, rsp);
    if ( rsp ){
      Print(L"FAIL: Could not set core voltage");
    }
  }

  // Set Turbo Ratios
  {
    UINT64 val = caps.core & 0xff;
    val |= (val << 8) | (val << 16) | (val << 24);
    val |= (val << 32);
    wrmsr(MSR_TURBO_RATIO_LIMIT, val);
    wrmsr(MSR_TURBO_RATIO_LIMIT1, val);
    wrmsr(MSR_TURBO_RATIO_LIMIT2, val | MSR_TURBO_RATIO_SEMAPHORE_BIT);
    if ( rsp ){
      Print(L"FAIL: Could not set core voltage");
    }
  }

  // Set Cache Min/Max Ratios
  {
    UINT16 limit = caps.cache & 0x00ff;
    UINT64 val = rdmsr(MSR_UNCORE_RATIO_LIMIT);
    val |= limit | (limit << 8);
    wrmsr(MSR_UNCORE_RATIO_LIMIT, val);
  }

  // Lock MSR_FLEX_RATIO using the FLEX_RATIO_LOCK bit
  {
    UINT64 val = rdmsr(MSR_FLEX_RATIO);
    val |= (1<<20);
    wrmsr(MSR_FLEX_RATIO, val);
  }

exit:
  return EFI_SUCCESS;
}



/*

main:
 
 mov [Handle], rcx         ; ImageHandle
 mov [SystemTable], rdx    ; pointer to SystemTable

 lea rdx, [_Start]
 mov rcx, [SystemTable]
 mov rcx, [rcx + EFI_SYSTEM_TABLE.ConOut]
 sub rsp, 4*8
 call [rcx + SIMPLE_TEXT_OUTPUT_INTERFACE.OutputString]
 add rsp, 4*8

 mov     eax,0
 mov     edx,0
 mov     ecx,8bh
 wrmsr
 mov     ecx,0
 inc     eax
 cpuid
 cmp     eax,306f2h
 jne     WrongCPU
 mov     ecx,8bh
 rdmsr
 cmp     edx,0
 jne     MicroCodePresent
 mov     ecx,194h
 rdmsr
 bt      eax,20
 jc      OC_Locked

 mov     ecx,150h                ; Get Core OC ratio and capabilities
 mov     edx,80000001h
 mov     eax,0
 wrmsr
 rdmsr
 cmp     dl,0
 jne     MailBoxError
 mov     [TopCore],al

 mov     edx,80000201h           ; Get Cache OC ratio and capabilities
 mov     eax,0
 wrmsr
 rdmsr
 cmp     dl,0
 jne     MailBoxError
 mov     [TopCache],al

 mov     eax,CoreVoltage     ; Set Core voltage
 mov     al,[TopCore]
 mov     edx,80000011h              
 wrmsr
 rdmsr
 cmp     dl,0
 jne     MailBoxError

 mov     eax,CacheVoltage     ; Set Cache voltage
 mov     al,[TopCache]
 mov     edx,80000211h
 wrmsr
 rdmsr
 cmp     dl,0
 jne     MailBoxError

 mov     al,[TopCore]            ; Set turbo ratio's
 mov     ah,al
 mov     dx,ax
 shl     eax,16
 mov     ax,dx
 mov     edx,eax
 mov     ecx,1adh
 wrmsr
 inc     ecx
 wrmsr
 inc     ecx
 or      edx,80000000h
 wrmsr

 mov     ecx,620h                ; Set Cache Min/Max Ratios
 rdmsr
 mov     al,[TopCache]
 mov     ah,al
 wrmsr

 mov     ecx,194h
 rdmsr
 bts     eax,20
 wrmsr

 lea rdx, [_Success]
 jmp Text_Exit

WrongCPU:
 lea rdx, [_WrongCPU]
 jmp Text_Exit

MicroCodePresent:
 lea rdx, [_MicroCodePresent]
 jmp Text_Exit

OC_Locked:
 lea rdx, [_OC_Locked]
 jmp Text_Exit

MailBoxError:
 lea rdx, [_MailBoxError]
 jmp Text_Exit

Text_Exit:
 mov rcx, [SystemTable]
 mov rcx, [rcx + EFI_SYSTEM_TABLE.ConOut]
 sub rsp, 4*8
 call [rcx + SIMPLE_TEXT_OUTPUT_INTERFACE.OutputString]
 add rsp, 4*8
 mov rax, EFI_SUCCESS
 retn

section '.data' data readable writeable

TopCore    db ?
TopCache   db ?

Handle      dq ?
SystemTable dq ?

_Start              du 13,10,'Xeon v3 All-Core Turbo Boost EFI Driver by C_Payne v0.1 (Single CPU)',13,10,0
_WrongCPU             du 'Failure - Wrong CPU.',13,10,0
_MicroCodePresent     du 'Failure - Microcode present.',13,10,0
_OC_Locked            du 'Failure - Overclocking Locked.',13,10,0
_MailBoxError         du 'Failure - Mailbox Error.',13,10,0
_Success              du 'Success.',13,10,0
*/
