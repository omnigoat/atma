#pragma once
//======================================================================
#include <atma/string.hpp>
//======================================================================
namespace atma { namespace filesystem {


	struct file_t
	{
		file_t()
			: file_()
		{}

		file_t(char const* path) {
			file_ = fopen(path, "rb+");
		}

		~file_t() {
			if (file_)
				fclose(file_);
		}

		auto is_valid() const -> bool { return file_ != nullptr; }
		
		auto size() const -> size_t {
			auto n = ftell(file_);
			fseek(file_, 0, SEEK_END);
			auto r = ftell(file_);
			fseek(file_, n, SEEK_SET);
			return r;
		}

		auto read(void* dest, uint size) -> void {
			fread(dest, size, 1, file_);
		}

	private:
		FILE* file_;
	};

} }
