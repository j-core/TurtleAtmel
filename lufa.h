
//	Crap to make the LUFA library work

#ifndef LUFA_H
#define LUFA_H

	#define USB_CAN_BE_DEVICE
	#define USB_SERIES_2_AVR
	#define ATTR_NO_RETURN               __attribute__ ((noreturn))
	#define ATTR_WARN_UNUSED_RESULT      __attribute__ ((warn_unused_result))
	#define ATTR_NON_NULL_PTR_ARG(...)   __attribute__ ((nonnull (__VA_ARGS__)))
	#define ATTR_NAKED                   __attribute__ ((naked))
	#define ATTR_NO_INLINE               __attribute__ ((noinline))
	#define ATTR_ALWAYS_INLINE           __attribute__ ((always_inline))
	#define ATTR_PURE                    __attribute__ ((pure))
	#define ATTR_CONST                   __attribute__ ((const))
	#define ATTR_DEPRECATED              __attribute__ ((deprecated))
	#define ATTR_WEAK                    __attribute__ ((weak))
	#define ATTR_NO_INIT                     __attribute__ ((section (".noinit")))
	#define ATTR_INIT_SECTION(SectionIndex)  __attribute__ ((used, naked, section (".init" #SectionIndex )))
	#define ATTR_ALIAS(Func)                 __attribute__ ((alias( #Func )))
	#define ATTR_PACKED                      __attribute__ ((packed))
	#define ATTR_ALIGNED(Bytes)              __attribute__ ((aligned(Bytes)))
	#define GCC_FORCE_POINTER_ACCESS(StructPtr)   __asm__ __volatile__("" : "=b" (StructPtr) : "0" (StructPtr))
	#define GCC_MEMORY_BARRIER()                  __asm__ __volatile__("" ::: "memory");
	#define GCC_IS_COMPILE_CONST(x)               __builtin_constant_p(x)
	#define USE_STATIC_OPTIONS               (USB_DEVICE_OPT_FULLSPEED | USB_OPT_REG_ENABLED | USB_OPT_AUTO_PLL)
	#define USB_DEVICE_ONLY
	#define USE_FLASH_DESCRIPTORS
	#define FIXED_CONTROL_ENDPOINT_SIZE      8
	#define DEVICE_STATE_AS_GPIOR            0
	#define FIXED_NUM_CONFIGURATIONS         1
	#define INTERRUPT_CONTROL_ENDPOINT
#endif

