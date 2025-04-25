

#define DOCTEST_CONFIG_IMPLEMENT
#include <atma/benchmark.hpp>
#include <atma/unit_test.hpp>

#include <span>
#include <unordered_map>

import atma.benchmark;

// ::atma::bench::no_optimize<__LINE__>(x)
#define FORCE_MATERIALIZATION(x) 

template <typename T, size_t E>
static void sum(std::span<T, E> numbers)
{
	T result{0};
	
	for (auto& x : numbers)
	{
		if (x < 128)
		{
			result += x;
		}
	}

	FORCE_MATERIALIZATION(result);
}

// fill array with random values
template <typename T>
void fill_randomized(std::span<T> numbers)
{
	for (T random{1}; auto& x : numbers)
	{
		x = (uint8_t)(random >> 24);
		random = 0x01000193 * random + 0x811c9dc5;
	}
}



int main()
{
	//atma::bench::set_scenario_output(
	//	atma::bench::stdout_outputter_t{}
	//	//atma::bench::output_file_writer_t{"output.txt"}
	//);

	atma::bench::measure_all_to_stdout();
	//auto r = atma::bench::measure_all();

	//atma::bench::stdout_outputter_t s;
	//for (auto const& sr : r)
	//	s.output(sr);

	//auto r = atma::bench::quick_benchmark([]{
	//	int i{};
	//	i += 4;
	//});

	//atma::bench::measure_along<
	//  atma::bench::axis<"numbers", int, float>,
	//  atma::bench::axis<"chars", char8_t, char16_t>
	//>([]<typename N, typename C>{});
	


	//WinNtModuleContext ctx;
	//WinNtProfileSession profile_session(ctx);
	//
	//std::vector<int> numbers{100 * 1024 * 1024};
	//fill_randomized(std::span{numbers});
	//int numbers[4] = {1, 2, 3, 4};
	//sum(std::span{numbers});
}

#if 0
#if 0
#include <windows.h>
#include <evntrace.h>
#include <vector>
#include <stdio.h>

int
wmain()
{
    ULONG status = 0;

    try
    {
        ULONG guidCount;
        std::vector<TRACE_GUID_PROPERTIES> guidPropValues;
        std::vector<TRACE_GUID_PROPERTIES*> guidPropPointers;

        // First call is just to get the actual count, so allocate a small buffer.
        guidCount = 1;

        // May need to retry multiple times since new providers could be added
        // between calls.
        for (;;)
        {
            ULONG const allocated = guidCount;
            guidPropValues.resize(allocated);
            guidPropPointers.resize(allocated);

            // Initialize the pointers to point at the values.
            for (ULONG i = 0; i != allocated; i += 1)
            {
                guidPropPointers[i] = &guidPropValues[i];
            }

            guidCount = 0;
            status = EnumerateTraceGuids(guidPropPointers.data(), allocated, &guidCount);
            if (status != ERROR_MORE_DATA)
            {
                guidPropValues.resize(guidCount);
                break;
            }

        }

        if (status != ERROR_SUCCESS)
        {
            printf("EnumerateTraceGuids error: %u\n", status);
        }
        else
        {
			int i = 0;
            printf("GuidCount = %lu\n", guidCount);
            for (auto const& v : guidPropValues)
            {
				
                printf("[%4d] %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x - %hs\n",
					i++,
                    v.Guid.Data1, v.Guid.Data2, v.Guid.Data3,
                    v.Guid.Data4[0], v.Guid.Data4[1],
                    v.Guid.Data4[2], v.Guid.Data4[3], v.Guid.Data4[4],
                    v.Guid.Data4[5], v.Guid.Data4[6], v.Guid.Data4[7],
                    v.IsEnable ? "Enabled" : "Disabled");
            }
        }
    }
    catch (std::bad_alloc const&)
    {
        printf("Out of memory!\n");
        status = ERROR_OUTOFMEMORY;
    }

    return status;
}

#if 0
#include <atma/functor.hpp>
#include <atma/assert.hpp>
#include <array>
#include <memory>

int main(int argc, char** argv)
{
	return doctest::Context(argc, argv).run();
}


#endif


#else



#pragma once

#include <stdint.h>

//
// interface
//

enum
{
	MP_CycleCount,
	MP_Instructions,
	MP_BranchMisses,
	MP_BranchCount,
	MP_DataMisses,
	MP_DataAccess,
	MP_COUNT,
};

typedef struct {
	double ElapsedTime;
	uint64_t ContextSwitches;
	uint64_t Counters[MP_COUNT];
} MiniPerfResult;

typedef void MiniPerfFun(void* Arg);

// IMPORTANT NOTES
//
// == WINDOWS ==
//
// * Must run process as "administrator"
//
// * Check available counters with "wpr -pmcsources" command. If you see only "Timer" there
//   then that means Windows ETW does have PMU counters available. On AMD system you might
//   need to disble Virtualization in BIOS (be aware that prevents using WSL2)
//
// * ETW is setup to report PMU counters for every context switch. Code calculates delta
//   between measurements for target thread, and returns accumulated values.
//
// == LINUX ==
//
// * Must run process as root, for example, with "sudo"
//
// * Check available counters with "perf list" command. It should show multiple
//   [Hardware event] entries
//
// * Make sure you have NMI watchdog disbled, as that uses one PMU counter for itself.
//   To disable NMI watchdog, run: "echo 0 | sudo tee /proc/sys/kernel/nmi_watchdog"
//
// * perf uses only generic PMU counters for generic hardware events. It does not use fixed
//   ones. This means for Skylake+ only 4 events per core will be available. It should be
//   possible to use 3 fixed ones (cycles, instruction, refcycles) too, but setting them up
//   requires using arch specific register numbers which is not done here.
//
// == APPLE ==
//
// * Must run process as root, for example, with "sudo"
//

// runs function with argument and return measured PMU counter values
// execution of function is pinned to one CPU core

static MiniPerfResult MiniPerf(MiniPerfFun* Fun, void* Arg);

//
// implementation
//

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <evntrace.h>
#include <evntcons.h>

#include <intrin.h>
#define MP_ASSERT(Cond) do { if (!(Cond)) __debugbreak(); } while (0)

#pragma comment (lib, "advapi32")

typedef struct
{
	MiniPerfResult Result;
	DWORD ThreadId;
	DWORD CpuIndex;
	ULONG* CountersUsed;
	size_t CounterCount;

	MiniPerfFun* Fun;
	void* Arg;

	LARGE_INTEGER StartTime;
	LARGE_INTEGER EndTime;
} MiniPerfContext;

static const GUID MP_ThreadGuid = { 0x3d6fa8d1, 0xfe05, 0x11d0, { 0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c } };
static const GUID MP_PageFaultGuid = { 0x3d6fa8d3, 0xfe05, 0x11d0, { 0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c } };

// Skylake+ can have 4 generic counters + 3 fixed (cycles, instructions, refcycles)
static const LPCWSTR MP_IntelCounters[] =
{
	/* [MP_CycleCount]   = */ L"UnhaltedCoreCyclesFixed",
	/* [MP_Instructions] = */ L"InstructionsRetiredFixed",
	/* [MP_BranchMisses] = */ L"BranchMispredictions",
	/* [MP_BranchCount]  = */ L"BranchInstructions",
	// on Intel can use L3 cache counters
	/* [MP_DataMisses]   = */ L"LLCMisses",
	/* [MP_DataAccess]   = */ L"LLCReference",
};

// AMD Zen can have 6 generic counters
static const LPCWSTR MP_AmdCounters[] =
{
	/* [MP_CycleCount]   */ L"TotalCycles",
	/* [MP_Instructions] */ L"TotalIssues",
	/* [MP_BranchMisses] */ L"BranchMispredictions",
	/* [MP_BranchCount]  */ L"BranchInstructions",
	// on AMD can use L1 cache counters
	/* [MP_DataMisses]   */ L"DcacheMisses",
	/* [MP_DataAccess]   */ L"DcacheAccesses",
};

static const LPCWSTR MP_ArmCounters[] =
{
	/* [MP_CycleCount]   */ L"TotalCycles",
	/* [MP_Instructions] */ L"TotalIssues",
	/* [MP_BranchMisses] */ L"BranchMispredictions",
	/* [MP_BranchCount]  */ L"BranchInstructions",
	/* [MP_DataMisses]   */ L"DcacheMisses",
	/* [MP_DataAccess]   */ L"DcacheAccesses",
};

static void CALLBACK MiniPerf__Callback(EVENT_RECORD* Event)
{
	const GUID* Provider = &Event->EventHeader.ProviderId;
	UCHAR Opcode = Event->EventHeader.EventDescriptor.Opcode;
	UCHAR CpuIndex = (UCHAR)GetEventProcessorIndex(Event);
	MiniPerfContext* Context = (MiniPerfContext*)Event->UserContext;

	if (RtlEqualMemory(Provider, &MP_ThreadGuid, sizeof(MP_ThreadGuid)) && Opcode == 0x24 && CpuIndex == Context->CpuIndex)
	{
		MP_ASSERT(Event->UserDataLength >= 24);
		DWORD NewThreadId = *(DWORD*)((BYTE*)Event->UserData + 0);
		DWORD OldThreadId = *(DWORD*)((BYTE*)Event->UserData + 4);
		DWORD ThreadId = Context->ThreadId;

		for (size_t i = 0; i < Event->ExtendedDataCount; i++)
		{
			EVENT_HEADER_EXTENDED_DATA_ITEM* Item = Event->ExtendedData + i;
			if (Item->ExtType == EVENT_HEADER_EXT_TYPE_PMC_COUNTERS)
			{
				MP_ASSERT(Item->DataSize == sizeof(ULONG64) * Context->CounterCount);

				EVENT_EXTENDED_ITEM_PMC_COUNTERS* Pmc = (EVENT_EXTENDED_ITEM_PMC_COUNTERS*)Item->DataPtr;
				for (size_t c = 0; c < Item->DataSize / sizeof(ULONG64); c++)
				{
					size_t Counter = Context->CountersUsed[c];
					Context->Result.Counters[Counter] -= (NewThreadId == ThreadId) ? Pmc->Counter[c] : 0;
					Context->Result.Counters[Counter] += (OldThreadId == ThreadId) ? Pmc->Counter[c] : 0;
				}
			}
		}

		Context->Result.ContextSwitches += (OldThreadId == ThreadId);
	}
}

static DWORD CALLBACK MiniPerf__ProcessThread(LPVOID Arg)
{
	TRACEHANDLE Session = (TRACEHANDLE)Arg;
	ProcessTrace(&Session, 1, NULL, NULL);
	return 0;
}

static DWORD CALLBACK MiniPerf__FunThread(LPVOID Arg)
{
	MiniPerfContext* Context = (MiniPerfContext*)Arg;
	QueryPerformanceCounter(&Context->StartTime);
	Context->Fun(Context->Arg);
	QueryPerformanceCounter(&Context->EndTime);
	return 0;
}

//Returns the last Win32 error, in string format. Returns an empty string if there is no error.
std::string GetLastErrorAsString(DWORD errorMessageID = ::GetLastError())
{
    //Get the error message ID, if any.
    //DWORD errorMessageID = ::GetLastError();
    if(errorMessageID == 0) {
        return std::string(); //No error message has been recorded
    }
    
    LPSTR messageBuffer = nullptr;

    //Ask Win32 to give us the string version of that message ID.
    //The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
    
    //Copy the error message into a std::string.
    std::string message(messageBuffer, size);
    
    //Free the Win32's string's buffer.
    LocalFree(messageBuffer);
            
    return message;
}


MiniPerfResult MiniPerf(MiniPerfFun* Fun, void* Arg)
{
	ULONG Status;

	MiniPerfContext Context;
	ZeroMemory(&Context, sizeof(Context));

	// find PMU counters by looking up available names
	static ULONG CounterSources[MP_COUNT];
	static ULONG CountersUsed[MP_COUNT];
	static size_t CounterCount = 0;

	if (CounterCount == 0)
	{
#if defined(_M_AMD64)
		int CpuName[4];
		__cpuid(CpuName, 0);

		const LPCWSTR* CounterNames;
		if (CpuName[2] == 0x6c65746e) // GenuineI[ntel]
		{
			CounterNames = MP_IntelCounters;
		}
		else if (CpuName[2] == 0x444d4163) // Authenti[cAMD]
		{
			CounterNames = MP_AmdCounters;
		}
		else
		{
			MP_ASSERT(!"Unknown CPU");
			return Context.Result;
		}
#elif defined(_M_ARM64)
		const LPCWSTR* CounterNames = MP_ArmCounters;
#else
#	error Unknown architecture
#endif

		ULONG BufferSize;

		// how much memory needed to query PMU counter names
		Status = TraceQueryInformation(0, TraceProfileSourceListInfo, NULL, 0, &BufferSize);
		MP_ASSERT(Status == ERROR_BAD_LENGTH);

		BYTE* Buffer = (BYTE*)HeapAlloc(GetProcessHeap(), 0, BufferSize);
		MP_ASSERT(Buffer);

		// get PMU counter names
		Status = TraceQueryInformation(0, TraceProfileSourceListInfo, Buffer, BufferSize, &BufferSize);
		MP_ASSERT(Status == ERROR_SUCCESS);

		size_t Offset = 0;
		for (;;)
		{
			PROFILE_SOURCE_INFO* Info = (PROFILE_SOURCE_INFO*)(Buffer + Offset);

			for (size_t i = 0; i < MP_COUNT; i++)
			{
				if (lstrcmpW(Info->Description, CounterNames[i]) == 0)
				{
					CounterSources[CounterCount] = Info->Source;
					CountersUsed[CounterCount++] = (ULONG)i;
					break;
				}
			}

			if (Info->NextEntryOffset == 0)
			{
				break;
			}
			Offset += Info->NextEntryOffset;
		}

		HeapFree(GetProcessHeap(), 0, Buffer);
	}
	Context.CountersUsed = CountersUsed;
	Context.CounterCount = CounterCount;
	Context.Fun = Fun;
	Context.Arg = Arg;

	struct
	{
		EVENT_TRACE_PROPERTIES_V2 Properties;
		WCHAR Name[1024];
	} Trace;

	const WCHAR TraceName[] = L"MiniPerf";

	EVENT_TRACE_PROPERTIES_V2* Properties = &Trace.Properties;

	// stop existing trace in case it is already running
	ZeroMemory(&Trace, sizeof(Trace));
	Properties->Wnode.BufferSize = sizeof(Trace);
	Properties->LoggerNameOffset = sizeof(Trace.Properties);

	Status = ControlTraceW(0, TraceName, (EVENT_TRACE_PROPERTIES*)Properties, EVENT_TRACE_CONTROL_STOP);
	MP_ASSERT(Status == ERROR_SUCCESS || Status == ERROR_MORE_DATA || Status == ERROR_WMI_INSTANCE_NOT_FOUND);

	// start a new trace, capture context switches
	ZeroMemory(&Trace, sizeof(Trace));
	Properties->Wnode.BufferSize = sizeof(Trace);
	Properties->Wnode.ClientContext = 3;
	Properties->Wnode.Flags = WNODE_FLAG_TRACED_GUID | WNODE_FLAG_VERSIONED_PROPERTIES;
	Properties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE | EVENT_TRACE_SYSTEM_LOGGER_MODE;
	Properties->VersionNumber = 2;
	Properties->EnableFlags = EVENT_TRACE_FLAG_CSWITCH;
	Properties->LoggerNameOffset = sizeof(Trace.Properties);

	TRACEHANDLE TraceHandle;
	Status = StartTraceW(&TraceHandle, TraceName, (EVENT_TRACE_PROPERTIES*)Properties);
	if (Status != ERROR_SUCCESS)
	{
		// ERROR_ACCESS_DENIED -> need to run with admin privileges
		// ERROR_NO_SYSTEM_RESOURCES -> too many system traces already running

		// just run the function, which will measure time
		MiniPerf__FunThread(&Context);
	}
	else
	{
		NtCreatePRofile
		// enable PMU counters if there are any (otherwise only context switch count will be captured)
		if (CounterCount != 0)
		{
			Status = TraceSetInformation(TraceHandle, TracePmcCounterListInfo, CounterSources, (ULONG)(CounterCount * sizeof(CounterSources[0])));
			// if this triggers ERROR_BUSY = 0xaa, then I believe that that someone else is collecting PMU counters
			// in the system, and I'm not sure how or if at all you to forcefully stop/reconfigure it. Rebooting helps.
			MP_ASSERT(Status == ERROR_SUCCESS);

			auto blah = GetLastErrorAsString(Status);
			
			// collect PMU counters on context switch event
			CLASSIC_EVENT_ID EventId = { MP_ThreadGuid, 0x24 };
			Status = TraceSetInformation(TraceHandle, TracePmcEventListInfo, &EventId, sizeof(EventId));
			MP_ASSERT(Status == ERROR_SUCCESS);
		}

		EVENT_TRACE_LOGFILEW Log;
		ZeroMemory(&Log, sizeof(Log));
        Log.LoggerName = Trace.Name;
		Log.EventRecordCallback = &MiniPerf__Callback;
		Log.ProcessTraceMode = PROCESS_TRACE_MODE_EVENT_RECORD | PROCESS_TRACE_MODE_RAW_TIMESTAMP | PROCESS_TRACE_MODE_REAL_TIME;
		Log.Context = &Context;

		// open trace for processing incoming events
		TRACEHANDLE Session = OpenTraceW(&Log);
		MP_ASSERT(Session != INVALID_PROCESSTRACE_HANDLE);

		// start ETW processing thread
		HANDLE ProcessingThread = CreateThread(NULL, 0, &MiniPerf__ProcessThread, (LPVOID)Session, 0, NULL);
		MP_ASSERT(ProcessingThread);

		// execute target function
		// it will happen on thread so there is a context switch right at the start of execution to capture initial PMU counter values
		{
			// create suspended thread so we know ThreadId is fully available
			HANDLE FunThread = CreateThread(NULL, 0, &MiniPerf__FunThread, &Context, CREATE_SUSPENDED, &Context.ThreadId);
			MP_ASSERT(FunThread);

			// pin thread to one CPU core
			Context.CpuIndex = SetThreadIdealProcessor(FunThread, MAXIMUM_PROCESSORS);
			SetThreadAffinityMask(FunThread, 1ULL << Context.CpuIndex);

			// now allow thread to run, thus force context switch for target thread
			ResumeThread(FunThread);

			WaitForSingleObject(FunThread, INFINITE);
			CloseHandle(FunThread);
		}

		// stop producing new events
		Status = ControlTraceW(TraceHandle, NULL, (EVENT_TRACE_PROPERTIES*)Properties, EVENT_TRACE_CONTROL_STOP);
		MP_ASSERT(Status == ERROR_SUCCESS);

		// closes trace processing, this will make ETW to process all the pending events in buffers
		Status = CloseTrace(Session);
		MP_ASSERT(Status == ERROR_SUCCESS || Status == ERROR_CTX_CLOSE_PENDING);

		// wait until ETW processing thread finishes with callbacks
		WaitForSingleObject(ProcessingThread, INFINITE);
		CloseHandle(ProcessingThread);
	}

	LARGE_INTEGER Freq;
	QueryPerformanceFrequency(&Freq);
	Context.Result.ElapsedTime = (double)(Context.EndTime.QuadPart - Context.StartTime.QuadPart) / Freq.QuadPart;

	return Context.Result;
}

#elif defined(__linux__)

#include <time.h>
#if defined(__x86_64__)
#  include <cpuid.h>
#endif
#include <sched.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>
#include <assert.h>

MiniPerfResult MiniPerf(MiniPerfFun* Fun, void* Arg)
{
	MiniPerfResult Result = { 0 };

	int CounterCount = 0;
#if defined(__x86_64__)
	{
		int eax, ebx, ecx, edx;
		__cpuid(0, eax, ebx, ecx, edx);

		if (ecx == signature_INTEL_ecx)
		{
			__cpuid(0xa, eax, ebx, ecx, edx);
			CounterCount = (eax >> 8) & 0xff;
		}
		else if (ecx == signature_AMD_ecx)
		{
			__cpuid(0x80000001, eax, ebx, ecx, edx);
			CounterCount = ((eax >> 23) & 1) ? 6 : 0;
		}
		else
		{
			assert(!"Unknown CPU");
			return Result;
		}
	}
#else
	CounterCount = MP_COUNT; // TODO: is it possible to get this value on armv8 at runtime?
#endif

	static const uint32_t PerfConfig[MP_COUNT][2] =
	{
		[MP_CycleCount]   = { PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES },
		[MP_Instructions] = { PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS },
		[MP_BranchMisses] = { PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_MISSES },
		[MP_BranchCount]  = { PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_INSTRUCTIONS },
		[MP_DataMisses]   = { PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES },
		[MP_DataAccess]   = { PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_REFERENCES },
//		[MP_DataMisses]   = { PERF_TYPE_HW_CACHE, (PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)) },
//		[MP_DataAccess]   = { PERF_TYPE_HW_CACHE, (PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)) },
	};

	// capture up to MP_COUNT counters
	if (CounterCount > MP_COUNT)
	{
		CounterCount = MP_COUNT;
	}

	int PerfFile[MP_COUNT + 1] = { 0 };
	PerfFile[0] = -1;

	// query index of current CPU core
	int CpuIndex = sched_getcpu();

	// pin current thread to CPU core
	cpu_set_t CpuMask, CpuMaskOld;
	CPU_ZERO(&CpuMask);
	CPU_SET(CpuIndex, &CpuMask);
	sched_getaffinity(0, sizeof(CpuMaskOld), &CpuMaskOld);
	sched_setaffinity(0, sizeof(CpuMask), &CpuMask);

	int SetupFailed = 0;

	// perf syscall setup
	for (int i=0; i<CounterCount; i++)
	{
		struct perf_event_attr PerfAttr = { 0 };
		PerfAttr.type = PerfConfig[i][0];
		PerfAttr.size = sizeof(PerfAttr);
		PerfAttr.config = PerfConfig[i][1];
		PerfAttr.disabled = 1;
		PerfAttr.pinned = i == 0;
		PerfAttr.read_format = PERF_FORMAT_GROUP;

		PerfFile[i] = syscall(SYS_perf_event_open, &PerfAttr, 0, CpuIndex, PerfFile[0], 0);
		if (PerfFile[i] < 0)
		{
			// errno == EACCES - no permissions
			// errno == ENOENT - counter not available
			SetupFailed = 1;
			break;
		}
	}

	if (!SetupFailed)
	{
		// also collect context switches
		struct perf_event_attr PerfAttr = { 0 };
		PerfAttr.type = PERF_TYPE_SOFTWARE;
		PerfAttr.size = sizeof(PerfAttr);
		PerfAttr.config = PERF_COUNT_SW_CONTEXT_SWITCHES;
		PerfAttr.disabled = 1;
		PerfAttr.read_format = PERF_FORMAT_GROUP;

		PerfFile[CounterCount] = syscall(SYS_perf_event_open, &PerfAttr, 0, CpuIndex, PerfFile[0], 0);
	}

	struct timespec TimeStart;
	struct timespec TimeEnd;

	if (!SetupFailed)
	{
		ioctl(PerfFile[0], PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);
	}

	clock_gettime(CLOCK_MONOTONIC_RAW, &TimeStart);
	Fun(Arg);
	clock_gettime(CLOCK_MONOTONIC_RAW, &TimeEnd);

	if (!SetupFailed)
	{
		ioctl(PerfFile[0], PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);
	}

	// restore CPU affinity
	sched_setaffinity(0, sizeof(CpuMaskOld), &CpuMaskOld);

	// read counter values
	uint64_t Values[1+MP_COUNT+1];

	// if this condition fails, then most likely you have not disabled NMI watchdog
	// which means perf is not able to setup all the PMU counters - during setup
	// the value pinned=1 means to read counter values only if all of them are available
	if (-1 != read(PerfFile[0], Values, sizeof(Values)))
	{
		for (int i=0; i<CounterCount; i++)
		{
			Result.Counters[i] = Values[1+i];
		}
		Result.ContextSwitches = Values[1+CounterCount];
	}

	// done with perf
	for (int i=0; i<MP_COUNT+1; i++)
	{
		if (PerfFile[i] > 0)
		{
			close(PerfFile[i]);
		}
	}

	Result.ElapsedTime = (TimeEnd.tv_sec - TimeStart.tv_sec) + 1e-9 * (TimeEnd.tv_nsec - TimeStart.tv_nsec);

	return Result;
}

#elif defined(__APPLE__)

#include <stdint.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <assert.h>
#include <pthread.h>
#include <time.h>
#include <sys/resource.h>

// adapted from https://gist.github.com/ibireme/173517c208c7dc333ba962c1f0d67d12

typedef struct kpep_db kpep_db;
typedef struct kpep_event kpep_event;
typedef struct kpep_config kpep_config;
typedef uint64_t kpc_config_t;

#define KPC_MAX_COUNTERS			32
#define KPC_CLASS_CONFIGURABLE		(1)
#define KPC_CLASS_CONFIGURABLE_MASK	(1U << KPC_CLASS_CONFIGURABLE)

#define KPERF_FUNCS(X) 																							\
	X(int,		kpc_force_all_ctrs_set,		int value)															\
	X(int,		kpc_set_config,				uint32_t classes, kpc_config_t* config)								\
	X(int,		kpc_set_counting,			uint32_t classes)													\
	X(int,		kpc_set_thread_counting,	uint32_t classes)													\
	X(int,		kpc_get_thread_counters,	uint32_t tid, uint32_t buf_count, void*	buf)						\

#define KPERFDATA_FUNCS(X) 																						\
	X(int,		kpep_db_create,				const char *name, kpep_db** db)										\
	X(int,		kpep_db_events_count,		kpep_db* db, size_t* count)											\
	X(int,		kpep_db_events,				kpep_db* db, kpep_event** buf, size_t buf_size)						\
    X(int,		kpep_db_event,				kpep_db* db, const char* name, kpep_event** ev)						\
	X(int,		kpep_event_name,			kpep_event* ev, const char** name)									\
	X(int,		kpep_event_description,		kpep_event* ev, const char** desc)									\
    X(void,		kpep_db_free,				kpep_db* db)														\
	X(int,		kpep_config_create,			kpep_db* db, kpep_config** config)									\
	X(int,		kpep_config_force_counters,	kpep_config* cfg)													\
	X(int,		kpep_config_add_event,		kpep_config* cfg, kpep_event** ev, uint32_t flag, uint32_t* err)	\
	X(int,		kpep_config_kpc_classes,	kpep_config* cfg, uint32_t* classes)								\
	X(int,		kpep_config_kpc_count,		kpep_config* cfg, size_t* count)									\
	X(int,		kpep_config_kpc_map,		kpep_config* cfg, void* buf, size_t buf_size)						\
	X(int,		kpep_config_kpc,			kpep_config* cfg, kpc_config_t* buf, size_t buf_size)				\
	X(void,		kpep_config_free,			kpep_config *cfg)

#define X(ret, name, ...) static ret (*name)(__VA_ARGS__);
KPERF_FUNCS(X)
KPERFDATA_FUNCS(X)
#undef X

MiniPerfResult MiniPerf(MiniPerfFun* Fun, void* Arg)
{
	MiniPerfResult Result = { 0 };

	static uint32_t CounterClasses;
	static size_t CounterRegCount;
	static size_t CounterMap[KPC_MAX_COUNTERS];
	static kpc_config_t CounterRegs[KPC_MAX_COUNTERS];
    int ret;

	static int Init;
	if (!Init)
	{
		void* KPerf = dlopen("/System/Library/PrivateFrameworks/kperf.framework/kperf", RTLD_LAZY);
		assert(KPerf);

#define X(ret, name, ...) name = dlsym(KPerf, #name); assert(name);
		KPERF_FUNCS(X)
#undef X

		void* KPerfData = dlopen("/System/Library/PrivateFrameworks/kperfdata.framework/kperfdata", RTLD_LAZY);
		assert(KPerfData);

#define X(ret, name, ...) name = dlsym(KPerfData, #name); assert(name);
		KPERFDATA_FUNCS(X)
#undef X

		kpep_db* KpepDb;
		kpep_config* KpepConfig;

		ret = kpep_db_create(NULL, &KpepDb);			assert(!ret && "kpep_db_create failed");
		ret = kpep_config_create(KpepDb, &KpepConfig);	assert(!ret && "kpep_config_create failed");
		ret = kpep_config_force_counters(KpepConfig);	assert(!ret && "kpep_config_force_counters failed");

#if 0	// dump all available events
		size_t Count;
        kpep_db_events_count(KpepDb, &Count);
        kpep_event** Events = (kpep_event**)calloc(sizeof(*Events), Count);
        kpep_db_events(KpepDb, Events, Count * sizeof(*Events));
        for (size_t i=0; i<Count; i++)
        {
        	const char* Name;
        	const char* Desc;
        	kpep_event_name(Events[i], &Name);
        	kpep_event_description(Events[i], &Desc);
        	printf("%-35s %s\n", Name, Desc);
        }
	    free(Events);
#endif

		static const char* EventNames[][3] =
		{
			{ "FIXED_CYCLES",				"CPU_CLK_UNHALTED.THREAD",		0								}, // cycles
			{ "FIXED_INSTRUCTIONS",			"INST_RETIRED.ANY",				0								}, // instructions
			{ "BRANCH_MISPRED_NONSPEC",		"BRANCH_MISPREDICT",			"BR_MISP_RETIRED.ALL_BRANCHES"	}, // branch-misses
			{ "INST_BRANCH",				"BR_INST_RETIRED.ALL_BRANCHES",	0								}, // branch-count
		};

		for (size_t e=0; e<sizeof(EventNames)/sizeof(EventNames[0]); e++)
		{
			for (size_t n=0; n<sizeof(EventNames[0])/sizeof(EventNames[0][0]); n++)
			{
				kpep_event* Event;
				if (EventNames[e][n] && kpep_db_event(KpepDb, EventNames[e][n], &Event) == 0)
				{
					const int UserSpaceOnly = 1;
					ret = kpep_config_add_event(KpepConfig, &Event, UserSpaceOnly, NULL);
					assert(!ret && "kpep_config_add_event failed");
					break;
				}
			}
		}

		ret = kpep_config_kpc_classes(KpepConfig, &CounterClasses);				assert(!ret && "kpep_config_kpc_classes failed");
		ret = kpep_config_kpc_count(KpepConfig, &CounterRegCount);				assert(!ret && "kpep_config_kpc_count failed");
		ret = kpep_config_kpc_map(KpepConfig, CounterMap, sizeof(CounterMap));	assert(!ret && "kpep_config_kpc_map failed");
		ret = kpep_config_kpc(KpepConfig, CounterRegs, sizeof(CounterRegs));	assert(!ret && "kpep_config_kpc failed");

		kpep_config_free(KpepConfig);
		kpep_db_free(KpepDb);

		Init = 1;
	}

	qos_class_t ThreadClass;
	int ThreadPriority;
	pthread_get_qos_class_np(pthread_self(), &ThreadClass, &ThreadPriority);

	const int UseHighPerfCores = 1;
	pthread_set_qos_class_self_np(UseHighPerfCores ? QOS_CLASS_USER_INTERACTIVE : QOS_CLASS_BACKGROUND, ThreadPriority);

	int CountersEnabled = kpc_force_all_ctrs_set(1);
	if (CountersEnabled == 0)
	{
		if ((CounterClasses & KPC_CLASS_CONFIGURABLE_MASK) && CounterRegCount)
		{
			ret = kpc_set_config(CounterClasses, CounterRegs);
			assert(!ret && "kpc_set_config failed");
		}
		ret = kpc_set_counting(CounterClasses);
		assert(!ret && "kpc_set_counting failed");

		ret = kpc_set_thread_counting(CounterClasses);
		assert(!ret && "kpc_set_thread_counting failed");
	}

	struct rusage UsageStart;
	getrusage(RUSAGE_SELF, &UsageStart);

	uint64_t CountersStart[KPC_MAX_COUNTERS] = { 0 };
	if (CountersEnabled == 0)
	{
		ret = kpc_get_thread_counters(0, KPC_MAX_COUNTERS, CountersStart);
		assert(!ret && "kpc_get_thread_counters failed");
	}

	struct timespec TimeStart;
	struct timespec TimeEnd;
	clock_gettime(CLOCK_MONOTONIC_RAW, &TimeStart);
	Fun(Arg);
	clock_gettime(CLOCK_MONOTONIC_RAW, &TimeEnd);

	uint64_t CountersEnd[KPC_MAX_COUNTERS] = { 0 };
	if (CountersEnabled == 0)
	{
		ret = kpc_get_thread_counters(0, KPC_MAX_COUNTERS, CountersEnd);
		assert(!ret && "kpc_get_thread_counters failed");
	}

	struct rusage UsageEnd;
	getrusage(RUSAGE_SELF, &UsageEnd);

	if (CountersEnabled == 0)
	{
		kpc_set_thread_counting(0);
		kpc_set_counting(0);
		kpc_force_all_ctrs_set(0);
	}

	pthread_set_qos_class_self_np(ThreadClass, ThreadPriority);

	for (size_t i=0; i<MP_COUNT; i++)
	{
		size_t Index = CounterMap[i];
		Result.Counters[i] = CountersEnd[Index] - CountersStart[Index];
	}
	Result.ElapsedTime = (TimeEnd.tv_sec - TimeStart.tv_sec) + 1e-9 * (TimeEnd.tv_nsec - TimeStart.tv_nsec);
	Result.ContextSwitches = UsageEnd.ru_nvcsw + UsageEnd.ru_nivcsw - UsageStart.ru_nvcsw - UsageStart.ru_nivcsw;
	return Result;
}

#endif



//
// On Windows this example code must be compiled with with clang-cl:
//   clang-cl.exe -O2 miniperf_example.c
//

#define _GNU_SOURCE // required for linux includes

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define DO_NOT_OPTIMIZE(var) 
// __asm__ __volatile__("" : "+r"(var))
// 
// array size to test branch prediction on
#define SUM_COUNT (100 * 1000 * 1000)

// expected count of cycles for loop when testing IPC
#define CYCLE_COUNT (1000 * 1000 * 1000)

static uint8_t arr[SUM_COUNT];

// sums all the values in array for values smaller than 128
//
// clang generates code that for each iteration does either
// 2 or 3 branches depending if comparison is true or false
// 
// 2 branches if comparison is false:
//   * 1 loop iteration condition
//   * 1 for arr[i] comparison and jump back to loop beginning)
//
// 3 branches if comparison is true:
//   * 1 loop iteration condition
//   * 1 for arr[i] comparison
//   * 1 for unconditional jump back to loop beginning
//
// this means for uniformly random values in array, it will do
// total of 2.5*N branches, out of which N will be unpredictable
//
// we expect CPU to take 50% of unpredictable branches correctly
// that means 0.5*N of branch misses, 0.5/2.5 = 1/5 = 20% misses
//
// if array is sorted, then the branch inside will be very predictable
// and mispredict will be close to 0% for the same 2.5*N of total branches

static void SumArray(void* Arg)
{
	uint64_t result = 0;
	DO_NOT_OPTIMIZE(result);

	for (size_t i = 0; i < SUM_COUNT; i++)
	{
		if (arr[i] < 128)
		{
			DO_NOT_OPTIMIZE(result); // force compiler to generate actual branch
			result += arr[i];
		}
	}

	DO_NOT_OPTIMIZE(result);
}

// these three operations can execute in same cycle on modern machine
#if defined(__x86_64__) || 1
#define OP1(var) var <<= 1
#define OP2(var) var <<= 1
#define OP3(var) var += var1 + var2
#elif defined(__aarch64__)
#define OP1(var) __asm__ __volatile__("add %w0,%w0,%w0": "+r"(var))
#define OP2(var) __asm__ __volatile__("add %w0,%w0,%w0": "+r"(var))
#define OP3(var) __asm__ __volatile__("nop"            : "+r"(var))
#endif

#define OPS1x1 OP1(var1)
#define OPS2x1 OP1(var1); OP2(var2)
#define OPS3x1 OP1(var1); OP2(var2); OP3(var3)
#define OPS1x5    OPS1x1;   OPS1x1;   OPS1x1;   OPS1x1;  OPS1x1
#define OPS2x5    OPS2x1;   OPS2x1;   OPS2x1;   OPS2x1;  OPS2x1
#define OPS3x5    OPS3x1;   OPS3x1;   OPS3x1;   OPS3x1;  OPS3x1
#define OPS1x25   OPS1x5;   OPS1x5;   OPS1x5;   OPS1x5;  OPS1x5
#define OPS2x25   OPS2x5;   OPS2x5;   OPS2x5;   OPS2x5;  OPS2x5
#define OPS3x25   OPS3x5;   OPS3x5;   OPS3x5;   OPS3x5;  OPS3x5
#define OPS1x125  OPS1x25;  OPS1x25;  OPS1x25;  OPS1x25; OPS1x25
#define OPS2x125  OPS2x25;  OPS2x25;  OPS2x25;  OPS2x25; OPS2x25
#define OPS3x125  OPS3x25;  OPS3x25;  OPS3x25;  OPS3x25; OPS3x25
#define OPS1x500  OPS1x125; OPS1x125; OPS1x125; OPS1x125
#define OPS2x500  OPS2x125; OPS2x125; OPS2x125; OPS2x125
#define OPS3x500  OPS3x125; OPS3x125; OPS3x125; OPS3x125

// perform a loop that executes in CYCLE_COUNT cycles (with small overhead for loop counter)
// loop is executed with expected IPC 1, 2 or 3
static void LoopFun(void* Arg)
{
	size_t ipc = (size_t)Arg;

	uint32_t var1 = 0;
	uint32_t var2 = 0;
	uint32_t var3 = 0;
	DO_NOT_OPTIMIZE(var1);
	DO_NOT_OPTIMIZE(var2);
	DO_NOT_OPTIMIZE(var3);

	size_t count = CYCLE_COUNT / 500;
	
	switch (ipc)
	{
	case 1: do { OPS1x500; } while (count--); break;
	case 2: do { OPS2x500; } while (count--); break;
	case 3: do { OPS3x500; } while (count--); break;
	}

	DO_NOT_OPTIMIZE(var1);
	DO_NOT_OPTIMIZE(var2);
	DO_NOT_OPTIMIZE(var3);
}

#define DATA_ITER_COUNT (1000 * 1000)
#define CACHE_SIZE (32*1024)
#define CACHE_LINE (64)

#define DO_DATA_ACCESS 0

#if DO_DATA_ACCESS
static void DataAccess(void* Arg)
{
	volatile uint8_t* Data = (uint8_t*)Arg;

	uint32_t Sum = 0;
	DO_NOT_OPTIMIZE(Sum);

	for (size_t i=0; i<DATA_ITER_COUNT; i++)
	{
		// 6*N data accesses that are in cache
		for (size_t j=0; j<6*CACHE_SIZE; j+=CACHE_LINE)
		{
			Sum += Data[0];
		}
		DO_NOT_OPTIMIZE(Sum);

		// 2*N data accessses that are not in cache
		for (size_t j=0; j<2*CACHE_SIZE; j+=CACHE_LINE)
		{
			Sum += Data[CACHE_SIZE+j];
		}
		DO_NOT_OPTIMIZE(Sum);
	}
	DO_NOT_OPTIMIZE(Sum);
}
#endif

// sorts array with counting sort
static void SortArray(void)
{
	size_t offset = 0;
	uint32_t counts[256] = { 0 };
	for (size_t i = 0; i < SUM_COUNT; i++)
	{
		counts[arr[i]]++;
	}
	for (size_t i = 0; i < 256; i++)
	{
		for (size_t c = 0; c < counts[i]; c++)
		{
			arr[offset++] = (uint8_t)i;
		}
	}
}

// fill array with random values
static void RandomizeArray(void)
{
	uint32_t random = 1;
	for (size_t i = 0; i < SUM_COUNT; i++)
	{
		arr[i] = (uint8_t)(random >> 24);
		random = 0x01000193 * random + 0x811c9dc5;
	}
}


int wmain();

int main()
{
	wmain();

	MiniPerfResult Perf;

#if 1
	RandomizeArray();

	Perf = MiniPerf(&SumArray, NULL);
	printf("[%6.3fs] sum on random array, branch misses = %5.2f%%, %2uM of %3uM total, context switches=%lld\n",
		Perf.ElapsedTime,
		Perf.Counters[MP_BranchMisses] * 100.0 / Perf.Counters[MP_BranchCount],
		(unsigned)(Perf.Counters[MP_BranchMisses] / 1000000),
		(unsigned)(Perf.Counters[MP_BranchCount] / 1000000),
		(long long)Perf.ContextSwitches);

	SortArray();

	Perf = MiniPerf(&SumArray, NULL);
	printf("[%6.3fs] sum on sorted array, branch misses = %5.2f%%, %2uK of %3uM total, context switches=%lld\n",
		Perf.ElapsedTime,
		Perf.Counters[MP_BranchMisses] * 100.0 / Perf.Counters[MP_BranchCount],
		(unsigned)(Perf.Counters[MP_BranchMisses] / 1000),
		(unsigned)(Perf.Counters[MP_BranchCount] / 1000000),
		(long long)Perf.ContextSwitches);
#endif
	
#if 1
	for (size_t i = 1; i <= 3; i++)
	{
		Perf = MiniPerf(&LoopFun, (void*)i);
		printf("[%6.3fs] loop with expected ipc=%zu, measured IPC = %.2f, %uM insn in %uM cycles, context switches=%lld\n",
			Perf.ElapsedTime,
			i,
			(double)Perf.Counters[MP_Instructions] / Perf.Counters[MP_CycleCount],
			(unsigned)(Perf.Counters[MP_Instructions] / 1000000),
			(unsigned)(Perf.Counters[MP_CycleCount] / 1000000),
			(long long)Perf.ContextSwitches);
	}

#endif

#if DO_DATA_ACCESS
	uint8_t* Data = malloc(8 * CACHE_SIZE);
	memset(Data, 0xff, 8 * CACHE_SIZE);

	// 2*N out of total 8*N accesses = 25% miss ratio
	{
		Perf = MiniPerf(&DataAccess, Data);
		printf("[%6.3fs] data -> %5.2f%%, miss = %uM, total = %uM, context switches=%lld\n",
			Perf.ElapsedTime,
			Perf.Counters[MP_DataMisses] * 100.0 / Perf.Counters[MP_DataAccess],
			(unsigned)(Perf.Counters[MP_DataMisses] / 1000000),
			(unsigned)(Perf.Counters[MP_DataAccess] / 1000000),
			(long long)Perf.ContextSwitches);
	}
#endif

}

#include <combaseapi.h>

const unsigned MAX_SESSION_NAME_LEN = 1024;
const unsigned MAX_LOGFILE_PATH_LEN = 1024;
const unsigned PropertiesSize =
    sizeof(EVENT_TRACE_PROPERTIES) +
    (MAX_SESSION_NAME_LEN * sizeof(WCHAR)) +
    (MAX_LOGFILE_PATH_LEN * sizeof(WCHAR));

int wmain()
{
    ULONG status;
    std::vector<EVENT_TRACE_PROPERTIES*> sessions; // Array of pointers to property structures
    std::vector<BYTE> buffer;                      // Buffer that contains all the property structures
    ULONG sessionCount;                            // Actual number of sessions started on the computer

    // The size of the session name and log file name used by the
    // controllers are not known, therefore create a properties structure that allows
    // for the maximum size of both.

    try
    {
        sessionCount = 64; // Start with room for 64 sessions.
        do
        {
            sessions.resize(sessionCount);
            buffer.resize(PropertiesSize * sessionCount);

            for (size_t i = 0; i != sessions.size(); i += 1)
            {
                sessions[i] = (EVENT_TRACE_PROPERTIES*)&buffer[i * PropertiesSize];
                sessions[i]->Wnode.BufferSize = PropertiesSize;
                sessions[i]->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
                sessions[i]->LogFileNameOffset = sizeof(EVENT_TRACE_PROPERTIES) + (MAX_SESSION_NAME_LEN * sizeof(WCHAR));
            }

            status = QueryAllTracesW(&sessions[0], sessionCount, &sessionCount);
        } while (status == ERROR_MORE_DATA);

        if (status != ERROR_SUCCESS)
        {
            printf("Error calling QueryAllTraces: %u\n", status);
        }
        else
        {
            printf("Actual session count: %u.\n\n", sessionCount);

            for (ULONG i = 0; i < sessionCount; i++)
            {
                WCHAR sessionGuid[50];
                (void)StringFromGUID2(sessions[i]->Wnode.Guid, sessionGuid, ARRAYSIZE(sessionGuid));

				printf(
                    "Session GUID: %ls\n"
                    "Session ID: %llu\n"
                    "  Session name: %ls\n"
                    "  Log file: %ls\n"
                    "  min buffers: %u\n"
                    "  max buffers: %u\n"
                    "  buffers: %u\n"
                    "  buffers written: %u\n"
                    "  buffers lost: %u\n"
                    "  events lost: %u\n"
					"  enable-flags: %u\n"
                    "\n",
					sessionGuid,
                    sessions[i]->Wnode.HistoricalContext,
                    (PCWSTR)((LPBYTE)sessions[i] + sessions[i]->LoggerNameOffset),
                    (PCWSTR)((LPBYTE)sessions[i] + sessions[i]->LogFileNameOffset),
                    sessions[i]->MinimumBuffers,
                    sessions[i]->MaximumBuffers,
                    sessions[i]->NumberOfBuffers,
                    sessions[i]->BuffersWritten,
                    sessions[i]->LogBuffersLost,
                    sessions[i]->EventsLost,
					sessions[i]->EnableFlags);

				//[[maybe_unused]] auto really = GetTraceLoggerHandle(&sessions[i]->Wnode);
				//auto Status = ControlTraceW(really, NULL, (EVENT_TRACE_PROPERTIES*)sessions[i], EVENT_TRACE_CONTROL_QUERY);
				//MP_ASSERT(Status == ERROR_SUCCESS);
            }
        }
    }
    catch (std::bad_alloc const&)
    {
        printf("Error allocating memory for properties.\n");
        status = ERROR_OUTOFMEMORY;
    }

    return status;
}


#endif

#else


#endif