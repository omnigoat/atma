module;

#  define WIN32_LEAN_AND_MEAN
#  include <Windows.h>
#  include <xmmintrin.h>
#  undef min
#  undef max

#include <Psapi.h>
#include <cstdint>
#include <cstdio>

#include <atma/assert.hpp>

export module atma.bench;

using NTSTATUS = long;

import atma.types;



export namespace atma::bench
{
	template <int Line, typename Expr>
	inline __forceinline void no_optimize(Expr&& expr)
	{
		static char const volatile* volatile _ = &reinterpret_cast<char const volatile&>(expr);
	}
}



export typedef enum _KPROFILE_SOURCE
{
    ProfileTime                     = 0x00,
    ProfileTotalIssues              = 0x02,
    ProfileBranchInstructions       = 0x06,
    ProfileCacheMisses              = 0x0A,
    ProfileBranchMispredictions     = 0x0B,
    ProfileTotalCycles              = 0x13,
    ProfileUnhaltedCoreCycles       = 0x19,
    ProfileInstructionRetired       = 0x1A,
    ProfileUnhaltedReferenceCycles  = 0x1B,
    ProfileLLCReference             = 0x1C,
    ProfileLLCMisses                = 0x1D,
    ProfileBranchInstructionRetired = 0x1E,
    ProfileBranchMispredictsRetired = 0x1F,
} KPROFILE_SOURCE, *PKPROFILE_SOURCE;


typedef NTSYSAPI NTSTATUS NTAPI F_NtCreateProfile(
    OUT PHANDLE        ProfileHandle,
    IN HANDLE          Process OPTIONAL,
    IN PVOID           ImageBase,
    IN ULONG           ImageSize,
    IN ULONG           BucketSize,
    IN PVOID           Buffer,
    IN ULONG           BufferSize,
    IN KPROFILE_SOURCE ProfileSource,
    IN KAFFINITY       Affinity
);

typedef NTSYSAPI NTSTATUS NTAPI F_NtStartProfile(
    IN HANDLE           ProfileHandle
);

typedef NTSYSAPI NTSTATUS NTAPI F_NtStopProfile(
    IN HANDLE           ProfileHandle
);

using ZwCreateProfileEx_fnptr_type = NTSYSAPI NTSTATUS (NTAPI*)(
    OUT PHANDLE        ProfileHandle,
    IN HANDLE          Process OPTIONAL,
    IN PVOID           ImageBase,
    IN ULONG           ImageSize,
    IN ULONG           BucketSize,
    IN PVOID           Buffer,
    IN ULONG           BufferSize,
    IN KPROFILE_SOURCE ProfileSource,
    IN USHORT          GroupCount,
	IN PGROUP_AFFINITY GroupAffinity);

using ZwStartProfile_fnptr_type = NTSYSAPI NTSTATUS (NTAPI*)(
	IN HANDLE ProfileHandle);

using ZwStopProfile_fnptr_type = NTSYSAPI NTSTATUS (NTAPI*)(
	IN HANDLE ProfileHandle);


HMODULE GetCurrentModule()
{
    HMODULE module = NULL;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR) GetCurrentModule, &module);
    return module;
}

#define DO_NOT_OPTIMIZE(var) 
#define SUM_COUNT (100 * 1000 * 1000)

// expected count of cycles for loop when testing IPC
#define CYCLE_COUNT (1000 * 1000 * 1000)

static uint8_t arr[SUM_COUNT];

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

export struct WinNtModuleContext
{
	WinNtModuleContext()
		: process_{GetCurrentProcess()}
		, module_{GetCurrentModule()}
	{
		HMODULE nt = LoadLibraryA("ntdll.dll");

		create_profile = (ZwCreateProfileEx_fnptr_type)GetProcAddress(nt, "ZwCreateProfileEx");
		start_profile = (ZwStartProfile_fnptr_type)GetProcAddress(nt, "ZwStartProfile");
		stop_profile = (ZwStopProfile_fnptr_type)GetProcAddress(nt, "ZwStopProfile");

		MODULEINFO module_info;
		if (BOOL module_info_success = GetModuleInformation(process_, module_, &module_info, sizeof(module_info)))
		{
			image_base_ = module_info.lpBaseOfDll;
			image_size_ = module_info.SizeOfImage;
		}
		else
		{
			// assert properly here
			printf("GetModuleInformation() failed!\n");
		}
	}

	HANDLE process() const { return process_; }
	HMODULE module() const { return module_; }

	void* image_base() const { return image_base_; }
	DWORD image_size() const { return image_size_; }

	ZwCreateProfileEx_fnptr_type create_profile;
	ZwStartProfile_fnptr_type start_profile;
	ZwStopProfile_fnptr_type stop_profile;

private:
	HANDLE process_{};
	HMODULE module_{};

	void* image_base_{};
	DWORD image_size_{};
};

export struct WinNtProfileSession
{
	WinNtProfileSession(WinNtModuleContext& ctx)
		: ctx_{ctx}
	{
		if (!ctx_.image_base())
		{
			// no valid address to instrument
			return;
		}

		// find largest power of two that fits image_size
		for (auto i = ctx_.image_size(); i > 4; i >>= 1)
			++pow2base_;
			
		create_profile(total_cycles_handle_, total_cycles_, ProfileTotalCycles);
		create_profile(branches_handle_, branches_, ProfileBranchInstructions);
		create_profile(branches_mispredicted_handle_, branches_mispredicted_, ProfileBranchMispredictions);
		create_profile(cache_misses_handle_, cache_misses_, ProfileCacheMisses);
	}

private:
	void create_profile(HANDLE& handle, ULONG& bucket, KPROFILE_SOURCE source)
	{
		[[maybe_unused]] NTSTATUS create_status = ctx_.create_profile(&handle, 
			ctx_.process(), ctx_.image_base(), ctx_.image_size(), 
			pow2base_, &bucket, sizeof(ULONG), 
			source,
			0, nullptr);

		ATMA_ASSERT(create_status == 0);
	}

private:
	WinNtModuleContext& ctx_;
	ULONG pow2base_{2};

	// handles
	HANDLE total_cycles_handle_{};
	HANDLE branches_handle_{};
	HANDLE branches_mispredicted_handle_{};
	HANDLE cache_misses_handle_{};

	// buckets
	ULONG total_cycles_{};
	ULONG branches_{};
	ULONG branches_mispredicted_{};
	ULONG cache_misses_{};
};

#if 0
ATMA_BENCHMARK_SUITE("hash-maps")
	.add_axis()
{
}
#endif

void test()
{
    HMODULE nt = LoadLibraryA("ntdll.dll");

    F_NtCreateProfile* NtCreateProfile = (F_NtCreateProfile*) GetProcAddress(nt, "NtCreateProfile");
    F_NtStartProfile* NtStartProfile = (F_NtStartProfile*) GetProcAddress(nt, "NtStartProfile");
    F_NtStopProfile* NtStopProfile = (F_NtStopProfile*) GetProcAddress(nt, "NtStopProfile");

	//[[maybe_unused]] auto blah = GetProcAddress(nt, "ZwCreateProfileEx");;

	


    HANDLE process = GetCurrentProcess();
    HMODULE module = GetCurrentModule();
    printf("%p %p\n", process, module);

    MODULEINFO module_info;
    BOOL module_info_success = GetModuleInformation(process, module, &module_info, sizeof(module_info));
    if (!module_info_success)
    {
        printf("GetModuleInformation() failed!\n");
        return;
    }

    void* image_base = module_info.lpBaseOfDll;
    auto image_size = module_info.SizeOfImage;

    printf("%p\n", image_base);
    printf("%d\n", (int) image_size);

    HANDLE profile, profile2;

    ULONG buffer[1] = {};
	ULONG buffer2[1] = {};


	// let's see how many profiles we can create at once
	{
		size_t i = 0;
		for ( ; i != 512; ++i)
		{
			NTSTATUS create_status = NtCreateProfile(&profile, process, image_base, image_size, 30, buffer, 4, ProfileLLCMisses, ULONG_PTR(-1));
			if (create_status != 0)
				break;
		}
		printf("concurrent profiles available: %llu\n", i);
	}

    NTSTATUS create_status = NtCreateProfile(&profile, process, image_base, image_size, 30, buffer, 4, ProfileLLCMisses, ULONG_PTR(-1));
	NTSTATUS create_status2 = NtCreateProfile(&profile2, process, image_base, image_size, 30, buffer2, 4, ProfileBranchMispredictions, ULONG_PTR(-1));
    printf("NtCreateProfile: %x\n", create_status);
	printf("NtCreateProfile: %x\n", create_status2);

    NtStartProfile(profile);
	NtStartProfile(profile2);
    {
        int* matrix = (int*) malloc(sizeof(int) * 16 * 1024 * 16 * 1024);
        for (int i = 0; i < 10000; i++)
            for (int j = 0; j < 10000; j++)
                matrix[i * 16 * 1024 + j] *= i * j;
        free(matrix);
    }
    NtStopProfile(profile);
	NtStopProfile(profile2);
    printf("Cache misses 1: %d\n", buffer[0]);
	printf("Branch mispredictions 1: %d\n", buffer2[0]);
    buffer[0] = 0;
	buffer2[0] = 0;


    NtStartProfile(profile);
	NtStartProfile(profile2);
    {
        int* matrix = (int*) malloc(sizeof(int) * 16 * 1024 * 16 * 1024);
        for (int i = 0; i < 10000; i++)
            for (int j = 0; j < 10000; j++)
                matrix[j * 16 * 1024 + i] *= i * j;
        free(matrix);
    }
    NtStopProfile(profile);
	NtStopProfile(profile2);
    printf("Cache misses 2: %d\n", buffer[0]);
	printf("Branch mispredictions 2: %d\n", buffer2[0]);
    buffer[0] = 0;
	buffer2[0] = 0;


	RandomizeArray();
	buffer2[0] = 0;
	NtStartProfile(profile2);
	{
		SumArray(nullptr);
	}
	NtStopProfile(profile2);
	printf("array - Branch mispredictions 1: %d\n", buffer2[0]);

	SortArray();
	buffer2[0] = 0;
	NtStartProfile(profile2);
	{
		SumArray(nullptr);
	}
	NtStopProfile(profile2);
	printf("array - Branch mispredictions 2: %d\n", buffer2[0]);
}
