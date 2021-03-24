//###############################
//# Driver Voltage Configuration
//###############################

// Core Options
#define CoreVOffset   -60 /* mV */
#define CoreVStatic   0   /* mV */
#define CoreMulti     0
#define CoreFixed     0

// Cache Options
#define CacheVOffset  0
#define CacheVStatic  0
#define CacheMulti    0
#define CacheFixed    0

// IMON Options
#define ImonSlope 0x001ULL
#define ImonOffset 0x80ULL



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


// toolbox for MSR OC Mailbox (experimental)
#define        OC_MB_COMMAND_EXEC       0x8000000000000000
#define        OC_MB_GET_CPU_CAPS       0x0000000100000000
#define        OC_MB_GET_TURBO_RATIOS   0x0000000200000000
#define        OC_MB_GET_FVIDS_RATIOS   0x0000001000000000
#define        OC_MB_SET_FVIDS_RATIOS   0x0000001100000000
#define        OC_MB_GET_SVID_PARAMS    0x0000001200000000
#define        OC_MB_SET_SVID_PARAMS    0x0000001300000000
#define        OC_MB_GET_FIVR_PARAMS    0x0000001400000000
#define        OC_MB_SET_FIVR_PARAMS    0x0000001500000000

// OC Mailbox Domains
#define        OC_MB_DOMAIN_IACORE      0x0000000000000000    // IA Core domain
#define        OC_MB_DOMAIN_GFX         0x0000010000000000
#define        OC_MB_DOMAIN_CLR         0x0000020000000000    // CLR (CBo/LLC/Ring) a.k.a. Cache/Uncore domain
#define        OC_MB_DOMAIN_SA          0x0000030000000000    // System Agent (SA) domain
#define        OC_MB_DOMAIN_AIO         0x0000040000000000
#define        OC_MB_DOMAIN_DIO         0x0000050000000000


// Model Specific Registers
#define        MSR_IA32_BIOS_SIGN_ID      0x08B
#define        MSR_PLATFORM_INFO          0x0CE
#define        MSR_OC_MAILBOX             0x150
#define        MSR_FLEX_RATIO             0x194
#define        MSR_TURBO_RATIO_LIMIT      0x1AD
#define        MSR_TURBO_RATIO_LIMIT1     0x1AE
#define        MSR_TURBO_RATIO_LIMIT2     0x1AF
#define        MSR_VR_CURRENT_CONFIG      0x601
#define        MSR_VR_MISC_CONFIG         0x603
#define        MSR_PKG_POWER_SKU_UNIT     0x606
#define        MSR_PKG_POWER_LIMIT        0x610  
#define        MSR_PKG_POWER_SKU          0x614
#define        MSR_DDR_RAPL_LIMIT         0x618
#define        MSR_UNCORE_RATIO_LIMIT     0x620
#define        MSR_VR_MISC_CONFIG2        0x636
#define        MSR_PP0_POWER_LIMIT        0x638
#define        MSR_PP1_POWER_LIMIT        0x640

// BARs
#define DEFAULT_MCHBAR		0xfed10000	/* 16 KB */
#define MCHBAR32(x) *((volatile UINT32 *)(DEFAULT_MCHBAR + x))
#define MCHBAR32_OR(x, or) MCHBAR32(x) = (MCHBAR32(x) | (or))
/* Some power MSRs are also represented in MCHBAR */
#define MCH_PKG_POWER_LIMIT_LO	0x59a0
#define MCH_PKG_POWER_LIMIT_HI	0x59a4

#define MCH_PL_LO 0xfed159a0
#define MCH_PL_HI 0xfed159aa

#define MCH_PL_VAL(X) *((volatile UINT32 *)(X))

// Some Constants
#define        CPUID_SIGNATURE                    0x306F2               // target CPUID, set 0xFFFFFFFF to bypass checking
#define        CPUID_VERSION_INFO                 0x1
#define        MSR_FLEX_RATIO_OC_LOCK_BIT         (1<<20)               // bit 20, set to lock MSR 0x194
#define        MSR_TURBO_RATIO_SEMAPHORE_BIT      0x8000000000000000    // set to execute changes writen to MSR 0x1AD, 0x1AE, 0x1AF
#define        OC_MB_FIVR_DYN_SVID_CONTROL_DIS    0x80000000			      // bit 31
#define        PLATFORM_INFO_SET_TDP              (1ULL << 29)
#define        PKG_POWER_LIMIT_MASK               (0x7fffULL)
#define        PKG_POWER_LIMIT_EN                 (1ULL << 15)
#define        PKG_POWER_LIMIT_TIME_SHIFT         17
#define        PKG_POWER_LIMIT_TIME_MASK          (0x7fULL)

// Some macros for OC MB voltage calculation
#define AdjVOffset(V) ((unsigned)((V << 15)/1000) & 0x000000000000ffe0)
#define AdjVStatic(V) ((unsigned)((V << 18)/1000) & 0x00000000000fff00)

#define CoreVoltage    (((AdjVOffset(CoreVOffset) << 16) | AdjVStatic(CoreVStatic)) | CoreMulti | (CoreFixed << 20))
#define CacheVoltage   (((AdjVOffset(CacheVOffset) << 16) | AdjVStatic(CacheVStatic)) | CacheMulti | (CacheFixed << 20))



// Shortcut to execute a OC mailbox command using return value RET
#define OC_MB_CMD(CMD, DOMAIN, RET) \
  wrmsr(MSR_OC_MAILBOX, OC_MB_COMMAND_EXEC | (CMD) | (DOMAIN));\
  RET = rdmsr(MSR_OC_MAILBOX);

// Check error rdmsr(OC_MB) result
#define OC_MB_ERROR(X) ((X >> 0x20) & 0xFF)

// CPU capabilities
// read from OC Mailbox
typedef struct {
  UINT64 core;
  UINT64 cache;
} cpu_caps_t;


// Forward declarations
void set_power_limits();


EFI_STATUS efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SysTab) {
  EFI_STATUS Status;
  EFI_LOADED_IMAGE *LoadedImage = NULL;

  InitializeLib(ImageHandle, SysTab);

  cpu_caps_t caps = {0};

  Print(L"V3 Turbo Loaded\r\n");

  wrmsr(MSR_IA32_BIOS_SIGN_ID, 0);

  // Check CPU
  UINT32 cpuid[4] = {0};
  __cpuid(CPUID_VERSION_INFO, cpuid[0], cpuid[1], cpuid[2], cpuid[3]);

  if ( cpuid[0] != CPUID_SIGNATURE ){
    Print(L"FAIL: Cpuid mismatch, found 0x%X\r\n", cpuid);
    goto exit;
  }

  // Check MicroCode
  UINT64 rsp = rdmsr(MSR_IA32_BIOS_SIGN_ID);
  if ( rsp >> 0x20 ){
    Print(L"FAIL: Micro code present...\r\n");
    goto exit;
  }

  // check OC Lock Bit not set
  rsp = rdmsr(MSR_FLEX_RATIO);
  if ( rsp & MSR_FLEX_RATIO_OC_LOCK_BIT ){
    Print(L"FAIL: OC locked...\r\n");
    goto exit;
  }

  // Get Core OC ratio and caps
  OC_MB_CMD(OC_MB_GET_CPU_CAPS, OC_MB_DOMAIN_IACORE, caps.core)
  if ( OC_MB_ERROR(caps.core) ){
    Print(L"FAIL: Core OC caps read failed\r\n");
  }

  // Get Cache OC ratio and caps
  OC_MB_CMD(OC_MB_GET_CPU_CAPS, OC_MB_DOMAIN_CLR, caps.cache)
  if ( OC_MB_ERROR(caps.cache) ){
    Print(L"FAIL: Cache OC caps read failed\r\n");
  }

  // Set Turbo Ratios
  {
    UINT64 val = caps.core & 0xff;
    val |= (val << 8) | (val << 16) | (val << 24);
    val |= (val << 32);
    wrmsr(MSR_TURBO_RATIO_LIMIT, val);
    wrmsr(MSR_TURBO_RATIO_LIMIT1, val);
    wrmsr(MSR_TURBO_RATIO_LIMIT2, val | MSR_TURBO_RATIO_SEMAPHORE_BIT);
  }

  // Set Cache Min/Max Ratios
  {
    UINT16 limit = caps.cache & 0x00ff;
    UINT64 val = rdmsr(MSR_UNCORE_RATIO_LIMIT);
    val &= 0xffffffffffff0000;
    val |= limit | (limit << 8);
    wrmsr(MSR_UNCORE_RATIO_LIMIT, val);
  }
/*
  // Set dunno
  {
    UINT64 val = caps.core & 0xff;
    val |= (val << 8) | (val << 16) | (val << 24);
    val |= (0x03ULL << 32);
    OC_MB_CMD(val, OC_MB_DOMAIN_IACORE, rsp)
    if ( OC_MB_ERROR(rsp) ){
      Print(L"FAIL: Could not set dunno\r\n");
    }
  }
*/
  // Set Core voltage
  {
    UINT64 val = CoreVoltage
      | (caps.core & 0xff)
      | OC_MB_SET_FVIDS_RATIOS;
    OC_MB_CMD(val, OC_MB_DOMAIN_IACORE, rsp)
    if ( OC_MB_ERROR(rsp) ){
      Print(L"FAIL: Could not set core voltage\r\n");
    }
  }

  // Set Cache voltage
  {
    UINT64 val = CacheVoltage
      | (caps.cache & 0xff)
      | OC_MB_SET_FVIDS_RATIOS;
    OC_MB_CMD(val, OC_MB_DOMAIN_CLR, rsp)
    if ( OC_MB_ERROR(rsp) ){
      Print(L"FAIL: Could not set cache voltage\r\n");
    }
  }

  // PowerCut
  {
    UINT64 val = OC_MB_SET_SVID_PARAMS 
      | OC_MB_FIVR_DYN_SVID_CONTROL_DIS;
    OC_MB_CMD(val, OC_MB_DOMAIN_IACORE, rsp)
    if ( OC_MB_ERROR(rsp) ){
      Print(L"FAIL: Could not apply PowerCut\r\n");
    }
  }

  // IOUT_SLOPE / OFFSET
  {
    UINT64 val = rdmsr(MSR_VR_MISC_CONFIG);
    val &= 0xfffc0000ffffffffULL;
    val |= (ImonSlope << 40) | (ImonOffset << 32);
    wrmsr(MSR_VR_MISC_CONFIG, val);
  }

  // IMAX
  {
    UINT64 val = rdmsr(MSR_VR_CURRENT_CONFIG);
    val |= 0x0000000000001fff;
    wrmsr(MSR_VR_CURRENT_CONFIG, val);
  }

  set_power_limits();

  // Lock MSR_FLEX_RATIO using the FLEX_RATIO_LOCK bit
  // This is needed to lock down our settings, otherwise
  // they would be corrected by the microcode update in the OS
  {
    UINT64 val = rdmsr(MSR_FLEX_RATIO);
    val |= MSR_FLEX_RATIO_OC_LOCK_BIT;
    wrmsr(MSR_FLEX_RATIO, val);
  }

exit:
  return EFI_SUCCESS;
}


/*
 * Configure processor power limits if possible
 * This must be done AFTER set of BIOS_RESET_CPL
 */
void set_power_limits()
{
	UINT64 msr = rdmsr(MSR_PLATFORM_INFO);
	UINT64 limit;
	unsigned int power_unit;
	unsigned int tdp, min_power, max_power, max_time;

	if (!(msr & PLATFORM_INFO_SET_TDP))
		return;

	/* Get units */
	msr = rdmsr(MSR_PKG_POWER_SKU_UNIT);
	power_unit = 2 << ((msr & 0xf) - 1);

	/* Get power defaults for this SKU */
	msr = rdmsr(MSR_PKG_POWER_SKU);
	tdp = msr & 0x7fff;
	min_power = (msr >> 16) & 0x7fff;
	max_power = (msr >> 32)  & 0x7fff;
	max_time = (msr >> 48) & 0x7f;

	Print(L"CPU TDP: %u Watts\r\n", tdp / power_unit);

	/* Set long term power limit to TDP */
	limit = 0;
	limit |= PKG_POWER_LIMIT_MASK;
	limit |= PKG_POWER_LIMIT_EN;
	limit |= PKG_POWER_LIMIT_TIME_MASK <<	PKG_POWER_LIMIT_TIME_SHIFT;

	/* Set short term power limit to 1.25 * TDP */
	limit |= (PKG_POWER_LIMIT_MASK << 32);
	limit |= (PKG_POWER_LIMIT_EN << 32);
	/* Power limit 2 time is only programmable on server SKU */

	wrmsr(MSR_PKG_POWER_LIMIT, limit);

	/* Set power limit values in MCHBAR as well */
	MCH_PL_VAL(MCH_PL_LO) = limit & 0x0ffffffff;
	MCH_PL_VAL(MCH_PL_HI) = (limit >> 32) & 0x0ffffffff;

}
