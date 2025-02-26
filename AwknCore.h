// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the AWKNCORE_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// AWKNCORE_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef AWKNCORE_EXPORTS
#define AWKNCORE_API __declspec(dllexport)
#else
#define AWKNCORE_API __declspec(dllimport)
#endif

// This class is exported from the dll
class AWKNCORE_API CAwknCore {
public:
	CAwknCore(void);
	// TODO: add your methods here.
};

extern AWKNCORE_API int nAwknCore;

AWKNCORE_API int fnAwknCore(void);
