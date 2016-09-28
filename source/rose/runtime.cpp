#include <rose/runtime.hpp>

#include <rose/console.hpp>

#include <atma/threading.hpp>


using namespace rose;
using rose::runtime_t;

static void CALLBACK rose::FileIOCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
{
	auto& info = *reinterpret_cast<rose::runtime_t::dir_watch_t*>(lpOverlapped);

	if (FILE_NOTIFY_INFORMATION* fni = (FILE_NOTIFY_INFORMATION*)info.buf)
	{
		while (fni)
		{
			// do something?
			if (fni->NextEntryOffset == 0)
				break;

			fni = (FILE_NOTIFY_INFORMATION*)((byte*)fni + fni->NextEntryOffset);
		}
	}
	
	ReadDirectoryChangesW(
		info.handle,
		info.buf, info.bufsize,
		FALSE, info.notify,
		nullptr, &info.overlapped,
		&FileIOCompletionRoutine);
}




runtime_t::runtime_t()
	: filewatch_engine_{atma::thread::engine_t::defer_start_t{}}
{
}

runtime_t::~runtime_t()
{
}

auto runtime_t::initialize_console() -> void
{
}

auto runtime_t::get_console() -> console_t&
{
	if (!console_)
	{
		console_.reset(new console_t);
	}

	return *console_;
}

auto runtime_t::initialize_watching() -> void
{
	filewatch_engine_.start();
	filewatch_engine_.signal([]{ atma::this_thread::set_debug_name("filewatching"); });

	filewatch_engine_.signal_evergreen([&]
	{
		auto status = WaitForMultipleObjectsEx((DWORD)dir_handles_.size(), dir_handles_.data(), FALSE, 1000, TRUE);
		if (status == WAIT_TIMEOUT)
			return;

		//for (auto i = 0; i != dir_handles_.size(); ++i)
		//{
		//	if (status == WAIT_OBJECT_0 + i)
		//	{
		//		auto& info = dir_infos_[i];
		//		std::cout << "change in " << info.path.string() << std::endl;
		//
		//		//FindNextChangeNotification(dir_handles_[i]);
		//		
		//
		//		break;
		//	}
		//}
	});
}

auto runtime_t::register_directory_watch(
	path_t const& path,
	bool recursive,
	file_change_mask_t changes,
	file_change_callback_t const& callback) -> void
{
	if (!filewatch_engine_.is_running())
		initialize_watching();

	// WIN32 file-notify flags
	int notify = 0;
	if ((changes & file_change_t::created) || (changes & file_change_t::renamed) || (changes & file_change_t::deleted))
		notify |= FILE_NOTIFY_CHANGE_FILE_NAME;
	if (changes & file_change_t::attributed)
		notify |= FILE_NOTIFY_CHANGE_ATTRIBUTES;
	if (changes & file_change_t::changed)
		notify |= FILE_NOTIFY_CHANGE_LAST_WRITE;
	if (changes & file_change_t::securitied)
		notify |= FILE_NOTIFY_CHANGE_SECURITY;


	filewatch_engine_.signal([&, path, recursive, notify, callback]
	{
		auto candidate = dir_watchers_.find(path);
		if (candidate != dir_watchers_.end())
			return;

		// WIN32 strings :(
		int char16s = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.c_str(), -1, nullptr, 0);
		wchar_t* wpath = (wchar_t*)alloca(sizeof(wchar_t) * char16s);
		int r = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.c_str(), -1, wpath, char16s);
		if (r == 0)
			return;

		auto& info = dir_infos_.emplace_back();
		info.path = path;
		info.notify = notify;
		HANDLE dir = CreateFile(wpath,
			FILE_LIST_DIRECTORY, FILE_SHARE_READ,
			NULL, OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
			NULL);

		if (dir == INVALID_HANDLE_VALUE)
			return;

		info.handle = dir;

		DWORD bytes = 0;
		BOOL success = ReadDirectoryChangesW(dir,
			info.buf, info.bufsize,
			FALSE, notify,
			&bytes, &info.overlapped,
			&FileIOCompletionRoutine);

		if (success)
		{
			dir_handles_.push_back(dir);
		}
	});

	
}


